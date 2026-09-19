#include "aiws/processing_core.hpp"

#include "aiws/chunker.hpp"
#include "aiws/context_builder.hpp"
#include "aiws/corpus_index.hpp"
#include "aiws/retrieval_engine.hpp"
#include "aiws/text_processor.hpp"

#include <stdexcept>

namespace aiws {

struct ProcessingCore::Impl {
    Chunker chunker{
        ChunkingPolicy{
            ProcessingCore::kMaxChunkTokens,
            ProcessingCore::kChunkOverlap,
            ProcessingCore::kParagraphPreferenceWindow
        }
    };

    RetrievalEngine retrieval_engine;
    ContextBuilder context_builder;

    std::vector<Chunk> chunks;
    CorpusIndex index;
};

ProcessingCore::ProcessingCore() : impl_(std::make_unique<Impl>()) { }

ProcessingCore::~ProcessingCore() = default;

ProcessingCore::ProcessingCore(ProcessingCore&&) noexcept = default;

ProcessingCore& ProcessingCore::operator=(ProcessingCore&&) noexcept = default;

std::string ProcessingCore::normalize(const std::string& text) {
    return TextProcessor::normalize(text);
}

void ProcessingCore::rebuild(const Workspace& workspace) {
    const std::vector<Document>& documents = workspace.documents();

    std::vector<Chunk> new_chunks;

    for (std::size_t document_index = 0; document_index < documents.size(); ++document_index) {

        const Document& document = documents[document_index];

        // Check for duplicate document IDs
        for (std::size_t previous_index = 0; previous_index < document_index; ++previous_index) {

            if (document.id() == documents[previous_index].id()) {
                throw std::invalid_argument(
                    "duplicate document id in workspace"
                );
            }
        }

        std::vector<Chunk> document_chunks =
            impl_->chunker.chunk(
                document,
                document_index
            );

        for (const Chunk& chunk : document_chunks) {new_chunks.push_back(chunk);}
    }
    
    impl_->chunks = new_chunks;
    impl_->index.build(impl_->chunks);
}

const std::vector<Chunk>& ProcessingCore::chunks() const noexcept {
    return impl_->chunks;
}

std::size_t ProcessingCore::chunk_count() const noexcept {
    return impl_->chunks.size();
}

std::size_t ProcessingCore::document_frequency(const std::string& term) const {
    std::vector<std::string> terms = TextProcessor::terms(term);

    if (terms.empty()) {
        return 0;
    }

    if (terms.size() > 1) {
        throw std::invalid_argument(
            "term must normalize to a single token"
        );
    }

    return impl_->index.document_frequency(terms[0]);
}

std::size_t ProcessingCore::term_frequency(const std::string& term,
                                           const std::string& chunk_id) const {

    std::vector<std::string> terms = TextProcessor::terms(term);

    if (terms.empty()) {
        return 0;
    }

    if (terms.size() > 1) {
        throw std::invalid_argument(
            "term must normalize to a single token"
        );
    }

    return impl_->index.term_frequency(
        terms[0],
        chunk_id
    );
}

std::vector<SearchResult> ProcessingCore::search(const std::string& query, int i) const {
    return impl_->retrieval_engine.search(
        query,
        i,
        impl_->chunks,
        impl_->index
    );
}

std::vector<ContextItem> ProcessingCore::build_context(const std::string& query,
                                                       int i,
                                                       std::size_t token_budget) const {

    std::vector<SearchResult> results =
        search(query, i);

    return impl_->context_builder.build(
        results,
        token_budget
    );
                                                        
}

}  // namespace aiws
