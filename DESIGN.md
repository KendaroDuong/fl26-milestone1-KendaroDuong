# M1 DESIGN.md

Replace this template with your own concise engineering explanation.

## 1. System structure
Describe the major responsibilities in your M1 subsystem and how they interact.

The processing core is set up so that each class has its own main job. The overall process starts with the workspace where the documents are processed into tokens and then split into chunks. After the chunks are made the corpus index stores information about the terms inside those chunks. When a query is entered, the query is normalized, matched against the index, and the best chunks are ranked. Those ranked chunks are then used to build the final context while staying within the token budget.

The TextProcessor handles the text rules used throughout the project. It changes letters to lowercase, keeps letters and numbers, separates words when punctuation appears, and keeps track of paragraph changes. The Chunker uses the processed tokens to split a document into chunks. It tries to keep chunks under the maximum token size while also looking for paragraph breaks near the end of a chunk. It also adds overlap between chunks so that some information is shared between them.

The CorpusIndex stores where terms appear and how many times they appear in each chunk. Instead of storing another full copy of each chunk it instead stores the position of the chunk and its term frequency. The RetrievalEngine uses the index to find matching chunks and gives each chunk a score based on how well it matches the query. The results are then sorted so the highest scoring chunks are returned first. The ContextBuilder takes those ranked results and adds them to the context until the token limit is reached. If the last chunk is too large it is shortened to fit.

The ProcessingCore connects all of these parts together. During the rebuild process it goes through the documents in the workspace, creates the chunks, and then rebuilds the corpus index. During search, it sends the query, chunks, and index to the retrieval engine. When build_context is called, it first gets the ranked search results and then sends those results to the context builder.

## 2. Design decisions
Explain the principal data structures, interfaces, and ownership decisions in your implementation and why you selected them.

One design choice was using Impl inside ProcessingCore. This allows the internal parts of the processing core to stay inside the cpp file instead of exposing everything in the header. The ProcessingCore still controls the chunks, index, chunker, retrieval engine, and context builder, but the user of the class does not need to know how all of those parts are stored internally.

Another design choice was having the CorpusIndex store chunk positions instead of storing full copies of every chunk. Each posting stores the chunk index and the number of times the term appears in that chunk. This makes more sense because the actual chunk data is already stored inside ProcessingCore. Keeping only the position in the index avoids storing the same chunk text in multiple places.

The Chunk and TokenInfo structures also store information that is needed later in the program. For example, TokenInfo stores the paragraph number and source positions, while Chunk stores the document order, sequence number, and source range. Keeping this information directly with the token or chunk makes it easier for the chunker and retrieval engine to use it without needing another data structure.

Term validation is handled inside ProcessingCore. The corpus index expects a term that is already normalized into one token. If the input becomes no tokens after normalization, the processing core returns 0. If it becomes more than one token it throws an invalid argument error. This keeps the corpus index simpler because it only has to deal with valid terms.

## 3. Correctness and consistency
Identify the important invariants or failure cases your design must preserve and explain how your design addresses them.

The rebuild process is designed so that bad input does not leave the processing core in a partly updated state. The new chunks are created first, and duplicate document IDs are checked before the stored data is replaced. If a duplicate ID is found, an exception is thrown before the old corpus is changed. This means the previous valid data is still available even if a later rebuild fails.

Rebuilding the same workspace also does not keep adding the same information again. The stored chunk list is replaced by the new chunks, and the corpus index clears its old data before rebuilding. Because of this, rebuilding the same workspace twice should still result in the same number of chunks and the same term frequencies.

The ranking is also kept consistent. Search results are first ordered by their score. If two chunks have the same score, the document order is used. If they are also from the same document order, the chunk sequence is used. This makes the search results stay in the same order instead of depending on how a map happens to store its values.

The chunker also has specific behavior for paragraph boundaries. It looks for a paragraph break near the maximum chunk size and uses the latest valid break inside the allowed window. If there is no valid paragraph break, it cuts the chunk at the maximum token size. The context builder also makes sure the final context never goes over the requested token budget. If a result does not fully fit, only the part that fits is included.

## 4. Testing strategy
Explain what your tests cover and which risks or boundaries you considered most important.

The tests are written to check the individual parts of the system instead of only testing everything through ProcessingCore. This makes it easier to tell where a problem is coming from. The TextProcessor tests check normalization, punctuation, paragraph detection, and joining tokens. The Chunker tests check the maximum chunk size, overlap, paragraph breaks, empty documents, and source positions in the original text.

The CorpusIndex tests check document frequency, term frequency, missing terms, missing chunk IDs, and rebuilding the index. The retrieval tests check that matching chunks are returned, that the result limit works, that negative values cause an error, and that ties are ordered correctly by document order and chunk sequence. These tests help make sure that the ranking does not depend on the order the chunks were originally stored.

The processing core tests check rebuild behavior, duplicate document IDs, repeated rebuilds, and term validation. This helps make sure that a failed rebuild does not remove the previous valid data and that running rebuild more than once does not duplicate the corpus. The context tests check a zero token budget, an exact fit, a truncated chunk, and multiple chunks sharing one token budget.

There are also tests for some boundary cases that could easily cause errors. For example, paragraph breaks are tested at the edge of the preferred chunking window, just outside the window, and with multiple possible paragraph breaks. The source position test also makes sure that source_begin and source_end point to the correct part of the original document even when there is whitespace before the first word.

## 5. Alternatives considered
Discuss at least two plausible design alternatives and why you did not choose them.

One other option was to store full copies of every Chunk inside the CorpusIndex. This would make the index more independent, but it would also mean that the same chunk information would be stored in both ProcessingCore and CorpusIndex. I decided that storing the chunk position made more sense because there only needs to be one main copy of the chunk data.

Another option was to rebuild the stored corpus directly while going through each document. This could use slightly less temporary memory, but it would cause problems if an error happened halfway through the rebuild. For example, if a duplicate document ID was found after some chunks had already been added, the program would have to undo those changes. Building the new data first and replacing the stored data only after everything succeeds makes the rebuild process easier to manage and keeps the old data safe if something goes wrong.