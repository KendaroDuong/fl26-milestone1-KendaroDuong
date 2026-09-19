#include "aiws/processing_core.hpp"
#include "aiws/text_processor.hpp"
#include "aiws/chunker.hpp"
#include "aiws/corpus_index.hpp"
#include "aiws/retrieval_engine.hpp"
#include "aiws/context_builder.hpp"
#include "aiws/document.hpp"
#include "aiws/workspace.hpp"

#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace aiws;

namespace {

std::string make_words(int first, int amount) {
    std::string text;

    for (int word = 0; word < amount; word++) {
        if (!text.empty()) {
            text += " ";
        }

        text += "w" + std::to_string(first + word);
    }

    return text;
}

}


// ---------------------------------------------------------
// Context tests
// ---------------------------------------------------------

void test_context() {
    Workspace workspace;

    workspace.add_document(
        Document{"doc1", "Test", "alpha beta gamma delta epsilon"}
    );

    ProcessingCore core;
    core.rebuild(workspace);

    std::vector<ContextItem> empty =
        core.build_context("alpha", 10, 0);

    assert(empty.empty());

    std::vector<ContextItem> full =
        core.build_context("alpha", 10, 5);

    assert(full.size() == 1);
    assert(full[0].token_count == 5);
    assert(full[0].truncated == false);

    std::vector<ContextItem> small =
        core.build_context("alpha", 10, 3);

    assert(small.size() == 1);
    assert(small[0].token_count == 3);
    assert(small[0].truncated == true);
}


// ---------------------------------------------------------
// RetrievalEngine tests
// ---------------------------------------------------------

void test_retrieval() {
    Chunk first;
    first.id = "one#0";
    first.document_id = "one";
    first.document_order = 0;
    first.sequence = 0;
    first.text = "alpha beta";

    Chunk second;
    second.id = "two#0";
    second.document_id = "two";
    second.document_order = 1;
    second.sequence = 0;
    second.text = "alpha beta";

    std::vector<Chunk> chunks;
    chunks.push_back(first);
    chunks.push_back(second);

    CorpusIndex index(chunks);
    RetrievalEngine engine;

    std::vector<SearchResult> results =
        engine.search("alpha beta", 10, chunks, index);

    assert(results.size() == 2);
    assert(results[0].document_id == "one");
    assert(results[1].document_id == "two");

    std::vector<SearchResult> none =
        engine.search("alpha", 0, chunks, index);

    assert(none.empty());

    std::vector<SearchResult> partial =
        engine.search("alpha missingword", 10, chunks, index);

    assert(partial.size() == 2);

    bool caught_error = false;

    try {
        engine.search("alpha", -1, chunks, index);
    }
    catch (const std::invalid_argument&) {
        caught_error = true;
    }

    assert(caught_error);
}


// ---------------------------------------------------------
// CorpusIndex tests
// ---------------------------------------------------------

void test_index() {
    Chunk first;
    first.id = "a#0";
    first.document_id = "a";
    first.document_order = 0;
    first.sequence = 0;
    first.text = "alpha beta beta";
    first.token_count = 3;

    Chunk second;
    second.id = "b#0";
    second.document_id = "b";
    second.document_order = 1;
    second.sequence = 0;
    second.text = "beta gamma";
    second.token_count = 2;

    std::vector<Chunk> chunks;
    chunks.push_back(first);
    chunks.push_back(second);

    CorpusIndex index(chunks);

    assert(index.document_frequency("beta") == 2);
    assert(index.document_frequency("alpha") == 1);
    assert(index.document_frequency("nothing") == 0);

    assert(index.term_frequency("beta", "a#0") == 2);
    assert(index.term_frequency("beta", "fake#0") == 0);

    std::vector<Chunk> new_chunks;
    new_chunks.push_back(first);

    index.build(new_chunks);

    assert(index.document_frequency("gamma") == 0);
    assert(index.document_frequency("alpha") == 1);

    assert(index.find_chunk(new_chunks, "a#0") != nullptr);
    assert(index.find_chunk(new_chunks, "bad#0") == nullptr);

    assert(index.chunk_index("a#0") == 0);
}


// ---------------------------------------------------------
// TextProcessor tests
// ---------------------------------------------------------

void test_text() {
    assert(TextProcessor::normalize("Hello, WORLD! 2026") == "hello world 2026");

    assert(TextProcessor::normalize("R2-D2") == "r2 d2");

    assert(TextProcessor::normalize("").empty());

    assert(TextProcessor::normalize("...!!!").empty());

    std::vector<TokenInfo> paragraph_tokens = TextProcessor::tokenize("alpha beta\n\ngamma delta");

    assert(paragraph_tokens.size() == 4);

    assert(paragraph_tokens[0].paragraph == 0);
    assert(paragraph_tokens[1].paragraph == 0);
    assert(paragraph_tokens[2].paragraph == 1);
    assert(paragraph_tokens[3].paragraph == 1);

    std::vector<TokenInfo> windows_newline =
        TextProcessor::tokenize("alpha\r\n\r\nbeta");

    assert(windows_newline.size() == 2);
    assert(windows_newline[0].paragraph == 0);
    assert(windows_newline[1].paragraph == 1);

    std::vector<TokenInfo> no_paragraph =
        TextProcessor::tokenize("alpha\n-\nbeta");

    assert(no_paragraph.size() == 2);
    assert(no_paragraph[0].paragraph == 0);
    assert(no_paragraph[1].paragraph == 0);

    std::vector<std::string> words =
        TextProcessor::terms("one two three");

    assert(TextProcessor::join(words, 1, 3) == "two three");

    std::vector<TokenInfo> word_info = TextProcessor::tokenize("one two three");

    assert(TextProcessor::join(word_info, 0, 2) == "one two");
}


// ---------------------------------------------------------
// Chunker tests
// ---------------------------------------------------------

void test_chunker() {
    Chunker chunker;

    Document long_document{
        "long",
        "Long",
        make_words(0, 130)
    };

    std::vector<Chunk> long_chunks =
        chunker.chunk(long_document, 0);

    assert(long_chunks.size() == 2);
    assert(long_chunks[0].token_count == 120);
    assert(long_chunks[1].token_count == 30);

    Document small_document{
        "small",
        "Small",
        make_words(0, 50)
    };

    std::vector<Chunk> small_chunks =
        chunker.chunk(small_document, 0);

    assert(small_chunks.size() == 1);
    assert(small_chunks[0].token_count == 50);

    Document blank_document{
        "blank",
        "Blank",
        "   \n\n   "
    };

    std::vector<Chunk> blank_chunks =
        chunker.chunk(blank_document, 0);

    assert(blank_chunks.empty());

    std::string paragraph_text =
        make_words(0, 100) +
        "\n\n" +
        make_words(100, 30);

    Document paragraph_document{
        "paragraph",
        "Paragraph",
        paragraph_text
    };

    std::vector<Chunk> paragraph_chunks =
        chunker.chunk(paragraph_document, 0);

    assert(paragraph_chunks.size() == 2);
    assert(paragraph_chunks[0].token_count == 100);
    assert(paragraph_chunks[1].token_count == 50);

    assert(paragraph_chunks[0].id == "paragraph#0");
    assert(paragraph_chunks[1].id == "paragraph#1");
}


// ---------------------------------------------------------
// ProcessingCore rebuild tests
// ---------------------------------------------------------

void test_rebuild() {
    ProcessingCore core;

    Workspace first_workspace;

    first_workspace.add_document(
        Document{"d1", "One", "alpha beta"}
    );

    first_workspace.add_document(
        Document{"d2", "Two", "gamma delta"}
    );

    core.rebuild(first_workspace);

    std::size_t original_count =
        core.chunk_count();

    assert(original_count == 2);

    Workspace duplicate_workspace;

    duplicate_workspace.add_document(
        Document{"same", "One", "alpha"}
    );

    duplicate_workspace.add_document(
        Document{"same", "Two", "beta"}
    );

    bool caught_duplicate = false;

    try {
        core.rebuild(duplicate_workspace);
    }
    catch (const std::invalid_argument&) {
        caught_duplicate = true;
    }

    assert(caught_duplicate);

    assert(
        core.chunk_count() == original_count
    );
}


// ---------------------------------------------------------
// Full system test
// ---------------------------------------------------------

void test_full_system() {
    Workspace workspace;

    workspace.add_document(
        Document{
            "A",
            "First",
            "retrieval systems use an index to rank chunks"
        }
    );

    workspace.add_document(
        Document{
            "B",
            "Second",
            "an index supports fast retrieval of ranked chunks"
        }
    );

    workspace.add_document(
        Document{
            "C",
            "Third",
            "this document talks about something else"
        }
    );

    ProcessingCore core;
    core.rebuild(workspace);

    std::vector<SearchResult> results =
        core.search("retrieval index chunks", 10);

    assert(results.size() == 2);

    bool found_a = false;
    bool found_b = false;
    bool found_c = false;

    for (const SearchResult& result : results) {
        if (result.document_id == "A") {
            found_a = true;
        }

        if (result.document_id == "B") {
            found_b = true;
        }

        if (result.document_id == "C") {
            found_c = true;
        }
    }

    assert(found_a);
    assert(found_b);
    assert(!found_c);

    std::vector<ContextItem> context =
        core.build_context(
            "retrieval index chunks",
            10,
            100
        );

    std::size_t used_tokens = 0;

    for (const ContextItem& item : context) {
        used_tokens += item.token_count;
    }

    assert(used_tokens <= 100);
}

// ---------------------------------------------------------
// ProcessingCore term validation
// ---------------------------------------------------------

void test_term_validation() {
    Workspace workspace;
    workspace.add_document(Document{"d1", "One", "alpha beta"});

    ProcessingCore core;
    core.rebuild(workspace);

    assert(core.document_frequency("...") == 0);
    assert(core.term_frequency("...", "d1#0") == 0);

    bool document_error = false;

    try {
        core.document_frequency("alpha beta");
    }
    catch (const std::invalid_argument&) {
        document_error = true;
    }

    assert(document_error);

    bool term_error = false;

    try {
        core.term_frequency("alpha beta", "d1#0");
    }
    catch (const std::invalid_argument&) {
        term_error = true;
    }

    assert(term_error);

    assert(core.document_frequency("ALPHA!") == 1);
    assert(core.term_frequency("ALPHA!", "d1#0") == 1);
}


// ---------------------------------------------------------
// Chunk source position test
// ---------------------------------------------------------

void test_chunk_source_positions() {
    Document document{"source", "Source", "   alpha beta"};
    Chunker chunker;

    std::vector<Chunk> chunks = chunker.chunk(document, 0);

    assert(chunks.size() == 1);
    assert(chunks[0].source_begin == 3);
    assert(chunks[0].source_end == 13);

    std::string original_text =
        document.text().substr(
            chunks[0].source_begin,
            chunks[0].source_end - chunks[0].source_begin
        );

    assert(original_text == "alpha beta");
}


// ---------------------------------------------------------
// ProcessingCore repeated rebuild test
// ---------------------------------------------------------

void test_repeated_rebuild() {
    Workspace workspace;
    workspace.add_document(Document{"d1", "One", "alpha beta"});
    workspace.add_document(Document{"d2", "Two", "gamma delta"});

    ProcessingCore core;

    core.rebuild(workspace);
    core.rebuild(workspace);

    assert(core.chunk_count() == 2);
    assert(core.document_frequency("alpha") == 1);
    assert(core.term_frequency("alpha", "d1#0") == 1);
}


// ---------------------------------------------------------
// Chunker latest paragraph test
// ---------------------------------------------------------

void test_chunker_latest_paragraph() {
    std::string text =
        make_words(0, 105) + "\n\n" +
        make_words(105, 5) + "\n\n" +
        make_words(110, 20);

    Document document{"multi", "Multi", text};
    Chunker chunker;

    std::vector<Chunk> chunks = chunker.chunk(document, 0);

    assert(chunks.size() == 2);
    assert(chunks[0].token_count == 110);
    assert(chunks[1].token_count == 40);
}


// ---------------------------------------------------------
// Chunker early paragraph test
// ---------------------------------------------------------

void test_chunker_early_paragraph() {
    std::string text =
        make_words(0, 99) + "\n\n" +
        make_words(99, 31);

    Document document{"early", "Early", text};
    Chunker chunker;

    std::vector<Chunk> chunks = chunker.chunk(document, 0);

    assert(chunks.size() == 2);
    assert(chunks[0].token_count == 120);
    assert(chunks[1].token_count == 30);
}


// ---------------------------------------------------------
// RetrievalEngine sequence order test
// ---------------------------------------------------------

void test_retrieval_sequence_order() {
    Chunk first;
    first.id = "doc#1";
    first.document_id = "doc";
    first.document_order = 0;
    first.sequence = 1;
    first.text = "alpha beta";

    Chunk second;
    second.id = "doc#0";
    second.document_id = "doc";
    second.document_order = 0;
    second.sequence = 0;
    second.text = "alpha beta";

    std::vector<Chunk> chunks;
    chunks.push_back(first);
    chunks.push_back(second);

    CorpusIndex index(chunks);
    RetrievalEngine engine;

    std::vector<SearchResult> results =
        engine.search("alpha beta", 10, chunks, index);

    assert(results.size() == 2);
    assert(results[0].chunk_sequence == 0);
    assert(results[1].chunk_sequence == 1);
}


// ---------------------------------------------------------
// ContextBuilder multiple item test
// ---------------------------------------------------------

void test_context_multiple_items() {
    SearchResult first;
    first.chunk_id = "c1";
    first.document_id = "d1";
    first.chunk_sequence = 0;
    first.text = "alpha beta gamma";
    first.score = 3.0;

    SearchResult second;
    second.chunk_id = "c2";
    second.document_id = "d2";
    second.chunk_sequence = 0;
    second.text = "delta epsilon zeta eta";
    second.score = 2.0;

    SearchResult third;
    third.chunk_id = "c3";
    third.document_id = "d3";
    third.chunk_sequence = 0;
    third.text = "theta iota kappa";
    third.score = 1.0;

    std::vector<SearchResult> results;
    results.push_back(first);
    results.push_back(second);
    results.push_back(third);

    ContextBuilder builder;

    std::vector<ContextItem> context =
        builder.build(results, 9);

    assert(context.size() == 3);

    assert(context[0].chunk_id == "c1");
    assert(context[0].token_count == 3);
    assert(context[0].truncated == false);

    assert(context[1].chunk_id == "c2");
    assert(context[1].token_count == 4);
    assert(context[1].truncated == false);

    assert(context[2].chunk_id == "c3");
    assert(context[2].token_count == 2);
    assert(context[2].truncated == true);

    std::vector<ContextItem> exact_context =
        builder.build(results, 7);

    assert(exact_context.size() == 2);
    assert(exact_context[0].truncated == false);
    assert(exact_context[1].truncated == false);
}

// ---------------------------------------------------------
// Main
// ---------------------------------------------------------

int main() {
    test_context();
    test_context_multiple_items();

    test_retrieval();
    test_retrieval_sequence_order();

    test_text();

    test_rebuild();
    test_repeated_rebuild();
    test_term_validation();

    test_index();

    test_chunker();
    test_chunk_source_positions();
    test_chunker_latest_paragraph();
    test_chunker_early_paragraph();

    test_full_system();

    std::cout << "All tests passed.\n";

    return 0;
}