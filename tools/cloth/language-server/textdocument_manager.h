#pragma once

#include <memory>
#include <unordered_map>

#include <cloth/backend_config.h>
#include <lsp/types.h>

#include "cloth_document.h"
#include "util.h"

class TextdocumentManager
{
public:
    TextdocumentManager() = default;

    void open(lsp::TextDocumentItem&& doc, std::shared_ptr<cloth::BackendConfig> backend)
    {
        if (doc.languageId == "cloth") {
            documents.try_emplace(doc.uri, std::move(doc.text), doc.uri, backend);
        }
        else {
            debug << "[TextdocumentManager] Opened document " << doc.uri.toString()
                << " is not a cloth document. Ignoring it.\n";
        }
    }

    void update(const lsp::FileUri& uri, const lsp::TextDocumentContentChangeEvent_Text&)
    {
        debug << "[TextdocumentManager] Full text update for " << uri.toString()
              << " - not implemented!" << std::flush;
    }

    void update(const lsp::FileUri& uri,
                const lsp::TextDocumentContentChangeEvent_Range_Text& change)
    {
        debug << "[TextdocumentManager] Partial text update for range "
              << change.range << ": \"" << change.text << "\""
              << std::flush;

        auto& doc = documents.at(uri);
        doc.replace(change.range, change.text);

        // debug << "New document:\n";
        // for (auto [i, line] : doc.lines | std::views::enumerate) {
        //     debug << std::setw(4) << std::setfill(' ') << i << " | " << line;
        // }
        // debug << std::flush;
    }

    void close(const lsp::FileUri& documentUri) {
        documents.erase(documentUri);
    }

    auto getDocument(const lsp::FileUri& uri) -> const ClothDocument*
    {
        auto it = documents.find(uri);
        if (it != documents.end()) {
            return &it->second;
        }
        return nullptr;
    }

private:
    std::unordered_map<lsp::FileUri, ClothDocument> documents;
};
