#include <iostream>

#include <cloth/torch_impl.h>
#include <lsp/connection.h>
#include <lsp/io/standardio.h>
#include <lsp/messages.h>
#include <lsp/messagehandler.h>
#include <trc/material/FragmentShader.h>
#include <trc/material/TorchMaterialSettings.h>

#include "textdocument_manager.h"

void sendDiagnostics(lsp::MessageHandler& msgHandler, const ClothDocument& doc)
{
    auto diagnostics = doc.makeDiagnostics();
    msgHandler.sendNotification<lsp::notifications::TextDocument_PublishDiagnostics>(
        lsp::notifications::TextDocument_PublishDiagnostics::Params{
            .uri=doc.uri,
            .diagnostics=std::move(diagnostics),
            .version=std::nullopt,
        }
    );
}

int main()
{
    lsp::Connection con{ lsp::io::standardIO() };
    lsp::MessageHandler msgHandler{ con };
    debug << "Cloth language server started." << std::flush;

    TextdocumentManager documentManager;
    auto engineBackend = std::make_shared<cloth::TorchImpl>();

    msgHandler.add<lsp::requests::Initialize>(
        [](lsp::requests::Initialize::Params&& /*params*/)
        {
            lsp::requests::Initialize::Result res{
                .capabilities{
                    .textDocumentSync = lsp::TextDocumentSyncOptions{
                        .openClose=true,
                        .change=lsp::TextDocumentSyncKind::Incremental,
                    },
                    .completionProvider = lsp::CompletionOptions{
                        .triggerCharacters = std::vector<std::string>{ "$" },
                        .resolveProvider = true,
                        .completionItem = lsp::CompletionOptionsCompletionItem{
                            .labelDetailsSupport = true,
                        },
                    },
                    .hoverProvider = lsp::HoverOptions{
                        .workDoneProgress = false,
                    },
                    .documentHighlightProvider = lsp::DocumentHighlightOptions{
                        .workDoneProgress = false,
                    },

                    // These signal the server's capability for "pull model" diagnostics:
                    // .diagnosticProvider{}
                },
                .serverInfo = lsp::InitializeResultServerInfo{
                    .name = "Cloth language server",
                    .version = "0.0.1",
                },
            };
            return res;
        }
    );

    msgHandler.add<lsp::requests::TextDocument_Completion>(
        [&](lsp::requests::TextDocument_Completion::Params&& params)
        {
            debug << "Request for completion on document " << params.textDocument.uri.toString()
                  << std::flush;

            auto doc = documentManager.getDocument(params.textDocument.uri);
            if (!doc) {
                return lsp::requests::TextDocument_Completion::Result{};
            }

            return lsp::requests::TextDocument_Completion::Result{
                doc->makeCompletionSuggestions(params.position)
            };
        }
    );
    msgHandler.add<lsp::requests::TextDocument_Hover>(
        [&](lsp::requests::TextDocument_Hover::Params&& params)
            -> lsp::requests::TextDocument_Hover::Result
        {
            debug << "Request for hover on document " << params.textDocument.uri.toString()
                  << std::flush;

            auto doc = documentManager.getDocument(params.textDocument.uri);
            if (!doc) {
                return {};
            }

            if (auto hover = doc->getHoverInformation(params.position)) {
                return hover.value();
            }
            return {};
        }
    );
    msgHandler.add<lsp::requests::TextDocument_DocumentHighlight>(
        [&](lsp::requests::TextDocument_DocumentHighlight::Params&& params)
            -> lsp::requests::TextDocument_DocumentHighlight::Result
        {
            debug << "Request for document highlight on document " << params.textDocument.uri.toString()
                  << std::flush;

            auto doc = documentManager.getDocument(params.textDocument.uri);
            if (!doc) {
                return {};
            }

            if (auto locs = doc->findOccurrencesAt(params.position))
            {
                return locs.value()
                    | std::views::transform([](const lsp::Range& r) {
                        return lsp::DocumentHighlight{
                            .range = r,
                            .kind = lsp::DocumentHighlightKind::Text,
                        };
                    })
                    | std::ranges::to<std::vector>();
            }
            return {};
        }
    );

    msgHandler.add<lsp::notifications::TextDocument_DidOpen>(
        [&](lsp::notifications::TextDocument_DidOpen::Params&& params)
        {
            debug << "Opened text document \"" << params.textDocument.uri.toString() << "\""
                  << " [language type: " << params.textDocument.languageId << "]." << std::flush;
            documentManager.open(std::move(params.textDocument), engineBackend);

            // Send initial diagnostics to the client
            if (auto doc = documentManager.getDocument(params.textDocument.uri)) {
                sendDiagnostics(msgHandler, *doc);
            }
        }
    );
    msgHandler.add<lsp::notifications::TextDocument_DidChange>(
        [&](lsp::notifications::TextDocument_DidChange::Params&& params)
        {
            debug << "Text document \"" << params.textDocument.uri.toString() << "\""
                  << " has changed." << std::flush;

            const auto& uri = params.textDocument.uri;
            for (const auto& change : params.contentChanges) {
                std::visit([&](const auto& c){ documentManager.update(uri, c); }, change);
            }

            // Send new diagnostics to the client
            if (auto doc = documentManager.getDocument(params.textDocument.uri)) {
                sendDiagnostics(msgHandler, *doc);
            }
        }
    );
    msgHandler.add<lsp::notifications::TextDocument_DidClose>(
        [&](lsp::notifications::TextDocument_DidClose::Params&& params)
        {
            debug << "Closed text document \"" << params.textDocument.uri.toString() << "\"." << std::flush;
            documentManager.close(params.textDocument.uri);
        }
    );

    bool isRunning{ true };
    msgHandler.add<lsp::notifications::Exit>([&]() {
        isRunning = false;
    });

    while (isRunning) {
        msgHandler.processIncomingMessages();
    }
    debug << "Cloth language server stopped." << std::flush;

    return 0;
}
