#include "aiws/text_processor.hpp"

namespace aiws {

namespace {

bool is_term_char(char c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9');
}

char make_lower(char c) {
    if (c >= 'A' && c <= 'Z') {
        return static_cast<char>(c - 'A' + 'a');
    }
    return c;
}

}  // namespace

std::vector<TokenInfo> TextProcessor::tokenize(const std::string& text) {

    std::vector<TokenInfo> tokens;

    std::size_t paragraph = 0;
    std::size_t i = 0;
    bool saw_newline = false;

    while (i < text.size()) {

        // Skip chars until char
        while (i < text.size() && !is_term_char(text[i])) {
            if (text[i] == '\n') {
                if (saw_newline) {
                    ++paragraph;
                }
                saw_newline = true;
            } 
            else if (text[i] != ' ' && text[i] != '\t' && text[i] != '\r') {
                saw_newline = false;
            }
            ++i;
        }

        if (i >= text.size()) {
            break;
        }

        std::size_t begin = i;
        std::string token;

        // Read the word
        while (i < text.size() && is_term_char(text[i])) {
            token += make_lower(text[i]);
            ++i;
        }

        std::size_t end = i;

        tokens.push_back(TokenInfo{token, begin, end, paragraph});

        saw_newline = false;
    }

    return tokens;
}

std::vector<std::string> TextProcessor::terms(const std::string& text) {
    std::vector<std::string> result;
    std::vector<TokenInfo> tokens = tokenize(text);

    for (std::size_t i = 0; i < tokens.size(); ++i) {
        result.push_back(tokens[i].token);
    }

return result;
}

std::string TextProcessor::normalize(const std::string& text) {
    std::vector<std::string> words = terms(text);
    return join(words, 0, words.size());
}

std::string TextProcessor::join(const std::vector<TokenInfo>& tokens,
                                std::size_t begin,
                                std::size_t end) {

    std::string result;

    for (std::size_t i = begin; i < end && i < tokens.size(); ++i) {
        if (i > begin) {
            result += " ";
        }
        result += tokens[i].token;
    }

    return result;
}

std::string TextProcessor::join(const std::vector<std::string>& tokens,
                                std::size_t begin,
                                std::size_t end) {

    std::string result;

    for (std::size_t i = begin; i < end && i < tokens.size(); ++i) {
        if (i > begin) {
            result += " ";
        }
        result += tokens[i];
    }

    return result;
}

}  // namespace aiws
