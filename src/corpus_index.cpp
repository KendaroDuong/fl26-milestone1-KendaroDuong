#include "aiws/corpus_index.hpp"

#include "aiws/text_processor.hpp"

namespace aiws {

CorpusIndex::CorpusIndex(const std::vector<Chunk>& chunks) {
    build(chunks);
}

void CorpusIndex::build(const std::vector<Chunk>& chunks) {

    postings_.clear();
    chunk_by_id_.clear();

    for (std::size_t chunk_index = 0;
         chunk_index < chunks.size();
         ++chunk_index) {

        const Chunk& chunk = chunks[chunk_index];

        chunk_by_id_[chunk.id] = chunk_index;

        std::vector<std::string> terms = TextProcessor::terms(chunk.text);

        std::unordered_map<std::string, std::size_t> term_counts;

        for (const std::string& term : terms) {
            term_counts[term]++;
        }

        for (const auto& term_count : term_counts) {
            Posting posting;

            posting.chunk_index = chunk_index;
            posting.frequency = term_count.second;

            postings_[term_count.first].push_back(posting);
        }
    }
}

std::size_t CorpusIndex::document_frequency(
    const std::string& normalized_term) const noexcept {

    auto term = postings_.find(normalized_term);

    if (term == postings_.end()) {
        return 0;
    }

    return term->second.size();
}

std::size_t CorpusIndex::term_frequency(
    const std::string& normalized_term,
    const std::string& chunk_id) const noexcept {

    auto chunk = chunk_by_id_.find(chunk_id);

    if (chunk == chunk_by_id_.end()) {
        return 0;
    }

    auto term = postings_.find(normalized_term);

    if (term == postings_.end()) {
        return 0;
    }

    std::size_t chunk_index = chunk->second;

    for (const Posting& posting : term->second) {

        if (posting.chunk_index == chunk_index) {
            return posting.frequency;
        }
    }

    return 0;
}

const std::vector<CorpusIndex::Posting>* CorpusIndex::postings(
    const std::string& normalized_term) const noexcept {

    auto term = postings_.find(normalized_term);

    if (term == postings_.end()) {
        return nullptr;
    }

    return &term->second;
}

const Chunk* CorpusIndex::find_chunk(
    const std::vector<Chunk>& chunks,
    const std::string& chunk_id) const noexcept {

    auto chunk = chunk_by_id_.find(chunk_id);

    if (chunk == chunk_by_id_.end()) {
        return nullptr;
    }

    std::size_t index = chunk->second;

    if (index >= chunks.size()) {
        return nullptr;
    }

    return &chunks[index];

}

std::size_t CorpusIndex::chunk_index(const std::string& chunk_id) const {

    return chunk_by_id_.at(chunk_id);
}

}  // namespace aiws
