#pragma once

#include <cmath>
#include <string>

#include <lsp/types.h>
#include <rapidfuzz/fuzz.hpp>
#include <shader_tools/ShaderDocument.h>
#include <trc/material/FragmentShader.h>
#include <trc/material/TorchMaterialSettings.h>

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
            auto content = getVariableDocumentation(var->name);

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

    auto findOccurrences(const shader_edit::Variable& var) const
        -> std::vector<lsp::Range>
    {
        const auto shaderDoc = shader_edit::parseShader(lines);

        std::vector<lsp::Range> res;
        for (const auto& [_, loc] : shaderDoc.variablesByName.at(var.name))
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

    auto findVariableAt(const lsp::Position& pos) const -> std::optional<shader_edit::Variable>
    {
        return findVariableAt(pos, shader_edit::parseShader(lines));
    }

    static auto findVariableAt(const lsp::Position& pos, const shader_edit::ParseResult& doc)
        -> std::optional<shader_edit::Variable>
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

    // TODO: Store ParseResult with a dirty flag
    std::vector<std::string> lines;
};
