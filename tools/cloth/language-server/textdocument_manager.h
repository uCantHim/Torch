#pragma once

#include <memory>
#include <unordered_map>

#include <lsp/types.h>

#include "backend_config.h"
#include "cloth_document.h"
#include "util.h"

class TextdocumentManager
{
public:
    TextdocumentManager(std::ostream& log) : log(log) {}

    void open(lsp::TextDocumentItem&& doc, std::shared_ptr<BackendConfig> backend)
    {
        if (doc.languageId == "cloth") {
            documents.try_emplace(doc.uri, std::move(doc.text), doc.uri, backend);
        }
        else {
            log << "[TextdocumentManager] Opened document " << doc.uri.toString()
                << " is not a cloth document. Ignoring it.\n";
        }
    }

    void update(const lsp::FileURI& uri, const lsp::TextDocumentContentChangeEvent_Text&)
    {
        log << "[TextdocumentManager] Full text update for " << uri.toString()
            << " - not implemented!" << std::endl;
    }

    void update(const lsp::FileURI& uri,
                const lsp::TextDocumentContentChangeEvent_Range_Text& change)
    {
        log << "[TextdocumentManager] Partial text update for range "
            << change.range << ": \"" << change.text << "\""
            << std::endl;

        auto& doc = documents.at(uri);
        doc.replace(change.range, change.text);

        // log << "New document:\n";
        // for (auto [i, line] : doc.lines | std::views::enumerate) {
        //     log << std::setw(4) << std::setfill(' ') << i << " | " << line;
        // }
        // log << std::endl;
    }

    void close(const lsp::FileURI& documentUri) {
        documents.erase(documentUri);
    }

    auto getDocument(const lsp::FileURI& uri) -> const ClothDocument*
    {
        auto it = documents.find(uri);
        if (it != documents.end()) {
            return &it->second;
        }
        return nullptr;
    }

private:
    std::ostream& log;
    std::unordered_map<lsp::FileURI, ClothDocument> documents;
};
