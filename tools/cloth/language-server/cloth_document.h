#pragma once

#include <cmath>
#include <string>

#include <lsp/types.h>
#include <rapidfuzz/fuzz.hpp>
#include <trc/material/FragmentShader.h>
#include <trc/material/TorchMaterialSettings.h>
#include <trc_util/algorithm/VectorTransform.h>

#include <cloth/builtins.h>
#include <cloth/cloth.h>
#include <cloth/parser.h>

#include "backend_config.h"
#include "util.h"

class ClothDocument
{
public:
    explicit ClothDocument(std::string text,
                           lsp::FileURI _uri,
                           std::shared_ptr<BackendConfig> _backend)
        :
        uri(std::move(_uri)),
        backend(_backend),
        lines(toLines(text))
    {
        cloth::BuiltinProvider provider;
        builtins = std::ranges::to<std::vector>(provider.getAllDefinitions());
        std::ranges::sort(builtins, [](auto& a, auto& b){ return a.fullId < b.fullId; });
    }

    void replace(const lsp::Range& range, const std::string& text)
    {
        const auto lineBegin = range.start.line;
        const auto lineEnd = range.end.line;
        const auto charBegin = range.start.character;
        const auto charEnd = range.end.character;
        assert(lines.size() > lineEnd);  // It seems that lineEnd is always a valid line index

        auto prefix = lines[lineBegin].substr(0, charBegin);
        auto change = toLines(prefix + text);

        lines[lineEnd].erase(0, charEnd);
        if (!change.empty())
        {
            lines[lineEnd].insert(0, change.back());
            change.pop_back();
        }

        lines.erase(lines.begin() + lineBegin, lines.begin() + lineEnd);
        lines.insert(lines.begin() + lineBegin, change.begin(), change.end());
    }

    auto makeDiagnostics() const -> std::vector<lsp::Diagnostic>
    {
        std::vector<lsp::Diagnostic> res;
        for (const auto& err : _getCompileErrors())
        {
            const auto loc = err.location;
            res.emplace_back(lsp::Diagnostic{
                .range{ .start{ loc.line, uint(loc.firstChar) }, .end{ loc.line, uint(loc.endChar) } },
                .message=err.message,
                .severity=lsp::DiagnosticSeverity::Error,
                .source="Cloth Language Server",
            });
        }

        return res;
    }

    auto makeCompletionSuggestions(const lsp::Position& pos) const
        -> std::vector<lsp::CompletionItem>
    {
        const auto word = findWordAt(pos);
        std::vector<std::pair<double, const cloth::Builtin*>> scores(builtins.size());
        for (const auto& [i, builtin] : std::views::enumerate(builtins)) {
            scores[i] = { rapidfuzz::fuzz::ratio(word, builtin.fullId), &builtin };
        }
        std::ranges::sort(scores, [](auto& a, auto& b){ return a.first < b.first; });

        std::vector<lsp::CompletionItem> res;
        for (const auto& [_, builtin] : scores)
        {
            auto& item = res.emplace_back();
            item.label = builtin->fullId;
            item.labelDetails = lsp::CompletionItemLabelDetails{
                .detail=makeArgumentListAnnotation(*builtin),
            };
            item.detail = getVariableDocumentation(builtin->fullId);
            item.kind = lsp::CompletionItemKind::Variable;
        }

        return res;
    }

    auto getHoverInformation(const lsp::Position& pos) const -> std::optional<lsp::Hover>
    {
        if (auto var = findVariableAt(pos))
        {
            const auto loc = var->location;
            auto content = getVariableDocumentation(var->id.id);

            return lsp::Hover{
                .contents = lsp::MarkupContent{
                    .kind = lsp::MarkupKind::Markdown,
                    .value = std::move(content),
                },
                .range = lsp::Range{
                    .start{ loc.line, static_cast<uint>(loc.firstChar) },
                    .end{ loc.line, static_cast<uint>(loc.endChar) },
                },
            };
        }

        return std::nullopt;
    }

    auto findOccurrences(const cloth::parser::Variable& var) const
        -> std::vector<lsp::Range>
    {
        const auto shaderDoc = _getParsedDocument();

        std::vector<lsp::Range> res;
        for (const auto& loc : shaderDoc.allReferences.at(var.fullDeclText))
        {
            res.emplace_back(lsp::Range{
                .start{ loc.line, static_cast<uint>(loc.firstChar), },
                .end{ loc.line, static_cast<uint>(loc.endChar), },
            });
        }
        return res;
    }

    auto findOccurrencesAt(const lsp::Position& pos) const -> std::optional<std::vector<lsp::Range>>
    {
        if (auto var = findVariableAt(pos)) {
            return findOccurrences(*var);
        }

        return std::nullopt;
    }

    auto findWordAt(const lsp::Position& pos) const -> std::string
    {
        auto isClothWordChar = [](char c) {
            return std::isalnum(c) || c == ':' || c == '$';
        };

        const auto& line = lines.at(pos.line);

        int charPos = static_cast<int>(pos.character) - 1;
        while (charPos >= 0 && isClothWordChar(line[charPos])) {
            --charPos;
        }

        return line.substr(charPos + 1, pos.character - (charPos + 1));
    }

    auto findVariableAt(const lsp::Position& pos) const -> std::optional<cloth::parser::Variable>
    {
        return findVariableAt(pos, _getParsedDocument());
    }

    static auto findVariableAt(const lsp::Position& pos, const cloth::parser::Result& doc)
        -> std::optional<cloth::parser::Variable>
    {
        for (const auto& var : doc.variablesInOrderOfOccurrence)
        {
            for (const auto& loc : doc.allReferences.at(var.fullDeclText))
            {
                if (loc.line < pos.line) continue;
                if (loc.line > pos.line) break;

                if (loc.firstChar <= pos.character && loc.endChar > pos.character) {
                    return var;
                }
            }
        }

        return std::nullopt;
    }

    static auto getVariableDocumentation(const std::string& varName) -> std::string
    {
        std::stringstream ss;
        ss << "# " << varName << "\n"
           << "\n";

        if (varName.starts_with("out:")) {
            ss << "References the \""
               << trc::util::splitString(varName, ':').at(1) << "\" output parameter.\n";
        }
        else {
            ss << "References the \"" << varName << "\" built-in.\n";
        }

        return ss.str();
    }

    static auto makeArgumentListAnnotation(const cloth::Builtin& builtin) -> std::string
    {
        if (builtin.args.empty()) {
            return {};
        }

        auto makeArgNames = [&] -> std::generator<std::string> {
            for (const auto& argType : builtin.args)
            {
                switch (argType)
                {
                case cloth::Builtin::ArgType::eValue:
                    co_yield "value";
                    break;
                case cloth::Builtin::ArgType::eResourcePath:
                    co_yield "path";
                    break;
                }
            }
        };
        std::stringstream ss;
        ss << "[";
        ss << std::ranges::to<std::string>(std::views::join_with(makeArgNames(), ','));
        ss << "]";
        return ss.str();
    }

    auto _getParsedDocument() const -> cloth::parser::Result
    {
        // TODO: Store ParseResult with a dirty flag. Implement a cache here.
        auto res = cloth::parser::parseDocument(lines);
        if (res) {
            return *res;
        }
        else {
            return res.error().partialResult;
        }
    }

    auto _getCompileErrors() const -> std::vector<cloth::parser::Error>
    {
        // Merge parse errors and compile errors together - show as much as possible
        std::vector<cloth::parser::Error> errors;

        // Parse document
        auto parsed = cloth::parser::parseDocument(lines);
        if (!parsed) {
            errors = std::move(parsed.error().errors);
        }

        // Compile document to shader module
        auto caps = backend->makeCapabilityConfig();
        auto outputs = backend->makeOutputConfig();
        auto compileResult = cloth::compileShader(
            parsed ? *parsed : parsed.error().partialResult,
            caps,
            *outputs
        );

        if (!compileResult) {
            trc::util::merge(errors, compileResult.error().errors);
        }
        return errors;
    }

    lsp::FileURI uri;
    std::shared_ptr<BackendConfig> backend;
    std::vector<cloth::Builtin> builtins;

    std::vector<std::string> lines;
};
