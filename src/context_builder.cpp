#include "aiws/context_builder.hpp"

#include "aiws/text_processor.hpp"

namespace aiws {

std::vector<ContextItem> ContextBuilder::build(const std::vector<SearchResult>& ranked, std::size_t token_budget) const {

    std::vector<ContextItem> items;
    std::size_t remaining_tokens = token_budget;

    for (const SearchResult& result : ranked) {

        if (remaining_tokens == 0) {
            break;
        }

        std::vector<std::string> tokens =
            TextProcessor::terms(result.text);

        ContextItem item;

        item.chunk_id = result.chunk_id;
        item.document_id = result.document_id;
        item.chunk_sequence = result.chunk_sequence;
        item.score = result.score;

        if (tokens.size() <= remaining_tokens) {

            item.text = result.text;
            item.token_count = tokens.size();
            item.truncated = false;

            remaining_tokens -= tokens.size();
        }
        else {

            item.text =
                TextProcessor::join(tokens, 0, remaining_tokens);

            item.token_count = remaining_tokens;
            item.truncated = true;

            remaining_tokens = 0;
        }

        items.push_back(item);
    }

    return items;
}

}  // namespace aiws
