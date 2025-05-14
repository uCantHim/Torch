#pragma once

#include <cmath>
#include <string>

#include <lsp/types.h>
#include <rapidfuzz/fuzz.hpp>
#include <trc/material/FragmentShader.h>
#include <trc/material/TorchMaterialSettings.h>

#include <cloth/parser.h>

#include "util.h"

class ClothDocument
{
public:
    explicit ClothDocument(std::string text)
        : lines(toLines(text))
    {
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
        auto errs = _getParseErrors();
        if (errs)
        {
            for (const auto& err : errs->errors)
            {
                const auto loc = err.location;
                res.emplace_back(lsp::Diagnostic{
                    .range{ .start{ loc.line, uint(loc.firstChar) }, .end{ loc.line, uint(loc.endChar) } },
                    .message=err.message,
                    .severity=lsp::DiagnosticSeverity::Error,
                    .source="Cloth Language Server",
                });
            }
        }

        return res;
    }

    auto makeCompletionSuggestions(const lsp::Position& pos) const
        -> std::vector<lsp::CompletionItem>
    {
        static const std::vector<trc::shader::Capability> allCaps{
            trc::MaterialCapability::kCameraWorldPos,
            trc::MaterialCapability::kTangentToWorldSpaceMatrix,
            trc::MaterialCapability::kVertexWorldPos,
            trc::MaterialCapability::kVertexNormal,
            trc::MaterialCapability::kVertexUV,
            trc::MaterialCapability::kTextureSample,
            trc::MaterialCapability::kTime,
            trc::MaterialCapability::kTimeDelta,
        };

        const auto word = findWordAt(pos);
        std::vector<std::pair<double, std::string_view>> scores(allCaps.size());
        for (const auto& [i, cap] : std::views::enumerate(allCaps)) {
            scores[i] = { rapidfuzz::fuzz::ratio(word, cap.getName()), cap.getName() };
        }
        std::ranges::sort(scores, [](auto& a, auto& b){ return a.first < b.first; });

        std::vector<lsp::CompletionItem> res;
        for (const auto& [_, capName] : scores)
        {
            auto& item = res.emplace_back();
            item.label = capName;
            item.detail = "Capability";
            item.kind = lsp::CompletionItemKind::Variable;
        }

        return res;
    }

    auto getHoverInformation(const lsp::Position& pos) const -> std::optional<lsp::Hover>
    {
        if (auto var = findVariableAt(pos))
        {
            const auto loc = var->location;
            auto content = getVariableDocumentation(var->fullId.id);

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
        for (const auto& var : shaderDoc.variablesByName.at(var.fullId))
        {
            const auto loc = var.location;
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
            if (var.location.line < pos.line) continue;
            if (var.location.line > pos.line) break;

            if (var.location.firstChar <= pos.character && var.location.endChar > pos.character) {
                return var;
            }
        }

        return std::nullopt;
    }

    static auto getVariableDocumentation(const std::string& varName) -> std::string
    {
        std::stringstream ss;
        ss << "# " << varName << "\n"
           << "\n";

        if (varName.starts_with("cap:")) {
            ss << "References the \""
               << trc::util::splitString(varName, ':').at(1) << "\" capability.\n";
        }
        else if (varName.starts_with("out:")) {
            ss << "References the \""
               << trc::util::splitString(varName, ':').at(1) << "\" output parameter.\n";
        }

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

    auto _getParseErrors() const -> std::optional<cloth::parser::IncompleteResult>
    {
        auto res = cloth::parser::parseDocument(lines);
        if (res) {
            return std::nullopt;
        }
        else {
            return res.error();
        }
    }

    std::vector<std::string> lines;
    //cloth::parser::Result parseResult;
};
