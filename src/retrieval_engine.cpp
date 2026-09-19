#include "aiws/retrieval_engine.hpp"
#include "aiws/text_processor.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace aiws {

double RetrievalEngine::canonical_score(double value) {
    double scale = 1e12;
    return std::round(value * scale) / scale;
}

std::vector<SearchResult> RetrievalEngine::search(const std::string& query,
                                                  int i,
                                                  const std::vector<Chunk>& chunks,
                                                  const CorpusIndex& index) const {
    if (i < 0) {
        throw std::invalid_argument("i must not be negative");
    }

    std::vector<SearchResult> results;

    if (i == 0) {
        return results;
    }

    std::vector<std::string> query_terms =
        TextProcessor::terms(query);

    std::vector<std::string> unique_terms;

    for (const std::string& term : query_terms) {
        bool already_exists = false;

        for (const std::string& unique_term : unique_terms) {
            if (term == unique_term) {
                already_exists = true;
                break;
            }
        }

        if (!already_exists) {
            unique_terms.push_back(term);
        }
    }

    if (unique_terms.empty()) {
        return results;
    }

    struct Candidate {
        std::size_t chunk_index;
        double score;
        std::size_t matched_terms;
    };

    std::vector<double> scores(chunks.size(), 0.0);
    std::vector<std::size_t> matches(chunks.size(), 0);

    for (const std::string& term : unique_terms) {

        std::size_t document_frequency = index.document_frequency(term);

        if (document_frequency == 0) {
            continue;
        }

        const std::vector<CorpusIndex::Posting>* postings =
            index.postings(term);

        if (postings == nullptr) {
            continue;
        }

        double idf =
            std::log(
                static_cast<double>(chunks.size() + 1) /
                static_cast<double>(document_frequency + 1)
            ) + 1.0;

        for (const CorpusIndex::Posting& posting : *postings) {

            double tf =1.0 + std::log(static_cast<double>(posting.frequency));

            scores[posting.chunk_index] += tf * idf;
            matches[posting.chunk_index]++;
        }
    }

    std::vector<Candidate> candidates;

    for (std::size_t chunk_index = 0;
         chunk_index < chunks.size();
         ++chunk_index) {

        if (matches[chunk_index] == 0) {
            continue;
        }

        double coverage =
            1.0 +
            0.10 *
            static_cast<double>(matches[chunk_index]) /
            static_cast<double>(unique_terms.size());

        double final_score = canonical_score(scores[chunk_index] * coverage);

        Candidate candidate;

        candidate.chunk_index = chunk_index;
        candidate.score = final_score;
        candidate.matched_terms = matches[chunk_index];

        candidates.push_back(candidate);
    }

    std::sort(candidates.begin(),candidates.end(), [&chunks](const Candidate& first, 
            const Candidate& second) {

            if (first.score != second.score) {
                return first.score > second.score;
            }

            const Chunk& first_chunk = chunks[first.chunk_index];

            const Chunk& second_chunk = chunks[second.chunk_index];

            if (first_chunk.document_order != second_chunk.document_order) {

                return first_chunk.document_order < second_chunk.document_order;
            }

            return first_chunk.sequence < second_chunk.sequence;
        }
    );

    std::size_t limit = candidates.size();

    if (static_cast<std::size_t>(i) < limit) {
        limit = static_cast<std::size_t>(i);
    }

    for (std::size_t result_index = 0;
         result_index < limit;
         ++result_index) {

        const Candidate& candidate = candidates[result_index];

        const Chunk& chunk = chunks[candidate.chunk_index];

        SearchResult result;

        result.chunk_id = chunk.id;
        result.document_id = chunk.document_id;
        result.chunk_sequence = chunk.sequence;
        result.text = chunk.text;
        result.score = candidate.score;
        result.matched_terms = candidate.matched_terms;

        results.push_back(result);
    }

    return results;

}

}  // namespace aiws

