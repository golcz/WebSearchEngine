#ifndef POSTING_LIST_H
#define POSTING_LIST_H

#include <string>
#include <vector>
#include <fstream>

/**
 * @brief Represents a compressed posting list for a single term.
 * 
 * Reads block-compressed postings from the encoded index file.
 * Supports nextGEQ (next document ID >= target) for query processing.
 */
class PostingList 
{
private:
    int wordID;
    std::ifstream indexFile;        // Binary file stream to encoded index
    long metadataStart, metadataEnd;
    int maxDocID;
    
    // Block metadata (per block of 128 postings)
    std::vector<int> lastDocIDs;      // Last doc ID in each block
    std::vector<int> docIDBlockSizes; // Size of docID block in bytes
    std::vector<int> freqBlockSizes;  // Size of frequency block in bytes
    
    // Current block state
    size_t blockIndex;
    bool blockDecoded;
    std::vector<int> docIDs;          // Decoded doc IDs in current block
    std::vector<int> frequencies;     // Decoded frequencies in current block
    int position;                     // Position within current block
    
    // Internal methods
    void decodeDocIDBlock();
    void decodeFrequencyBlock();
    void loadMetadata();

public:
    /**
     * @param wordID_ The term's word ID
     * @param encodedIndexFile Path to encoded index binary file
     * @param metadataStart_ Byte offset where this term's metadata begins
     * @param metadataEnd_ Byte offset where this term's metadata ends
     * @param maxDocID_ Total number of documents (for sentinel)
     */
    PostingList(int wordID_, const std::string& encodedIndexFile, 
                long metadataStart_, long metadataEnd_, int maxDocID_);
    
    ~PostingList();
    
    /**
     * @brief Returns the word ID for this posting list.
     */
    int getWordID() const { return wordID; }
    
    /**
     * @brief Finds the next document ID >= target.
     * @return Document ID, or maxDocID+1 if none found
     */
    int nextGEQ(int targetDocID);
    
    /**
     * @brief Gets the frequency of the current document (call after nextGEQ).
     */
    int getCurrentFrequency() const;
    
    /**
     * @brief Computes BM25 contribution for the current document.
     * @param N Total number of documents
     * @param n Document frequency of this term
     * @param D Length of current document
     * @param avgdl Average document length
     * @param k1 BM25 parameter (default 1.5)
     * @param b BM25 parameter (default 0.75)
     */
    float computeBM25Score(int N, int n, int D, float avgdl, 
                           float k1 = 1.5f, float b = 0.75f) const;
    
    /**
     * @brief Check if the posting list is valid (term exists).
     */
    bool isValid() const { return !lastDocIDs.empty(); }
};

#endif