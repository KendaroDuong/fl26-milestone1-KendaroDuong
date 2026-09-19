#include "aiws/chunker.hpp"

#include "aiws/text_processor.hpp"

#include <stdexcept>
#include <string>

namespace aiws {

Chunker::Chunker(ChunkingPolicy policy) : policy_(policy) {
    if (policy_.max_tokens == 0 || policy_.overlap >= policy_.max_tokens ||
        policy_.paragraph_window > policy_.max_tokens) {
        throw std::invalid_argument("invalid chunking policy");
    }
}

std::vector<Chunk> Chunker::chunk(const Document& document, std::size_t document_order) const {
    std::vector<Chunk> chunks;
    std::vector<TokenInfo> tokens = TextProcessor::tokenize(document.text());

    if (tokens.empty()) {
        return chunks;
    }

    std::size_t chunk_start = 0;
    std::size_t chunk_sequence = 0;

    while (chunk_start < tokens.size()) {

        std::size_t chunk_end =
            chunk_start + policy_.max_tokens;

        // Prevent the chunk from going past the end of the tokens
        if (chunk_end > tokens.size()) {
            chunk_end = tokens.size();
        }

        // If not the last chunk check for paragraph break
        if (chunk_end < tokens.size()) {

            std::size_t paragraph_search_start =
                chunk_end - policy_.paragraph_window;

            // Search backwards for the closest paragraph break
            for (std::size_t token_index = chunk_end;
                 token_index >= paragraph_search_start;
                 --token_index) {

                if (token_index == 0) {
                    break;
                }

                if (tokens[token_index].paragraph !=
                    tokens[token_index - 1].paragraph) {

                    chunk_end = token_index;
                    break;
                }
            }
        }

        Chunk new_chunk;

        new_chunk.document_id = document.id();
        new_chunk.document_order = document_order;
        new_chunk.sequence = chunk_sequence;

        new_chunk.id = document.id() + "#" + std::to_string(chunk_sequence);

        new_chunk.text =
            TextProcessor::join(
                tokens,
                chunk_start,
                chunk_end
            );

        new_chunk.token_count = chunk_end - chunk_start;

        new_chunk.source_begin = tokens[chunk_start].begin;

        new_chunk.source_end = tokens[chunk_end - 1].end;

        chunks.push_back(new_chunk);

        ++chunk_sequence;

        // Stop if all tokens have been added
        if (chunk_end == tokens.size()) {
            break;
        }

        // Move forward while keeping the requested overlap
        chunk_start =
            chunk_end - policy_.overlap;
    }

    return chunks;
}

}  // namespace aiws
