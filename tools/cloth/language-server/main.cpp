#include <iostream>
#include <fstream>

#include <lsp/connection.h>
#include <lsp/io/standardio.h>
#include <lsp/messages.h>
#include <lsp/messagehandler.h>

#include "textdocument_manager.h"

int main()
{
    std::ofstream log{ "cloth-lsp.log", std::ios_base::app };

    lsp::Connection con{ lsp::io::standardInput(), lsp::io::standardOutput() };
    lsp::MessageHandler msgHandler{ con };
    log << "Cloth language server started." << std::endl;

    TextdocumentManager documentManager{ log };

    msgHandler.add<lsp::requests::Initialize>(
        [](const lsp::MessageId& /*id*/, lsp::requests::Initialize::Params&& /*params*/)
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
        [&](const lsp::MessageId& /*id*/, lsp::requests::TextDocument_Completion::Params&& params)
        {
            log << "Request for completion on document " << params.textDocument.uri.toString()
                << std::endl;

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
        [&](const lsp::MessageId& /*id*/, lsp::requests::TextDocument_Hover::Params&& params)
            -> lsp::requests::TextDocument_Hover::Result
        {
            log << "Request for hover on document " << params.textDocument.uri.toString()
                << std::endl;

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
        [&](const lsp::MessageId& /*id*/, lsp::requests::TextDocument_DocumentHighlight::Params&& params)
            -> lsp::requests::TextDocument_DocumentHighlight::Result
        {
            log << "Request for document highlight on document " << params.textDocument.uri.toString()
                << std::endl;

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
            log << "Opened text document \"" << params.textDocument.uri.toString() << "\""
                << " [language type: " << params.textDocument.languageId << "]." << std::endl;
            documentManager.open(std::move(params.textDocument));
        }
    );
    msgHandler.add<lsp::notifications::TextDocument_DidChange>(
        [&](lsp::notifications::TextDocument_DidChange::Params&& params)
        {
            log << "Text document \"" << params.textDocument.uri.toString() << "\""
                << " has changed." << std::endl;

            const auto& uri = params.textDocument.uri;
            for (const auto& change : params.contentChanges) {
                std::visit([&](const auto& c){ documentManager.update(uri, c); }, change);
            }

            // Send new diagnostics to the client
            if (auto doc = documentManager.getDocument(uri))
            {
                auto diagnostics = doc->makeDiagnostics();
                msgHandler.sendNotification<lsp::notifications::TextDocument_PublishDiagnostics>(
                    lsp::notifications::TextDocument_PublishDiagnostics::Params{
                        .uri=uri,
                        .diagnostics=std::move(diagnostics),
                        .version=params.textDocument.version,
                    }
                );
            }
        }
    );
    msgHandler.add<lsp::notifications::TextDocument_DidClose>(
        [&](lsp::notifications::TextDocument_DidClose::Params&& params)
        {
            log << "Closed text document \"" << params.textDocument.uri.toString() << "\"." << std::endl;
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
    log << "Cloth language server stopped." << std::endl;

    return 0;
}
