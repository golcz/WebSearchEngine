#include "PostingList.h"
#include "Utils.h"
#include <iostream>

PostingList::PostingList(int wordID_, const std::string& encodedIndexFile,
    long metadataStart_, long metadataEnd_, int maxDocID_)
    : wordID(wordID_), metadataStart(metadataStart_), metadataEnd(metadataEnd_),
    maxDocID(maxDocID_), blockIndex(0), blockDecoded(false), position(0) 
{
    indexFile.open(encodedIndexFile, std::ios::binary);
    if (!indexFile.is_open()) 
    {
        std::cerr << "Error opening encoded index file." << std::endl;
        return;
    }
    
    loadMetadata();
}

PostingList::~PostingList() 
{
    if (indexFile.is_open()) 
    {
        indexFile.close();
    }
}

void PostingList::loadMetadata() {
    indexFile.seekg(metadataStart, std::ios::beg);
    
    while (indexFile.good() && indexFile.tellg() < metadataEnd) 
    {
        int lastDocID = Utils::varbyteDecode(indexFile);
        int docIDBlockSize = Utils::varbyteDecode(indexFile);
        int freqBlockSize = Utils::varbyteDecode(indexFile);
        
        if (lastDocID == -1 || docIDBlockSize == -1 || freqBlockSize == -1) break;
        
        lastDocIDs.push_back(lastDocID);
        docIDBlockSizes.push_back(docIDBlockSize);
        freqBlockSizes.push_back(freqBlockSize);
    }
}

void PostingList::decodeDocIDBlock() 
{
    docIDs.clear();
    
    int currentDocID = Utils::varbyteDecode(indexFile);
    docIDs.push_back(currentDocID);
    
    int bytesRead = Utils::varbyteEncode(currentDocID).size();
    while (bytesRead < docIDBlockSizes[blockIndex]) 
    {
        int offset = Utils::varbyteDecode(indexFile);
        if (offset == -1) break;
        currentDocID += offset;
        docIDs.push_back(currentDocID);
        bytesRead += Utils::varbyteEncode(offset).size();
    }
}

void PostingList::decodeFrequencyBlock() 
{
    frequencies.clear();
    
    int bytesRead = 0;
    while (bytesRead < freqBlockSizes[blockIndex]) 
    {
        int freq = Utils::varbyteDecode(indexFile);
        if (freq == -1) break;
        frequencies.push_back(freq);
        bytesRead += Utils::varbyteEncode(freq).size();
    }
}

int PostingList::nextGEQ(int targetDocID) 
{
    int bytesToIgnore = 0;
    
    // Skip blocks that end before target
    while (blockIndex < lastDocIDs.size() && lastDocIDs[blockIndex] < targetDocID) 
    {
        if (!blockDecoded) 
        {
            bytesToIgnore += docIDBlockSizes[blockIndex] + freqBlockSizes[blockIndex];
        }
        blockDecoded = false;
        blockIndex++;
    }
    
    if (blockIndex >= lastDocIDs.size()) 
    {
        return maxDocID + 1;  // No more documents
    }
    
    // Decode current block if not already
    if (!blockDecoded) 
    {
        indexFile.ignore(bytesToIgnore);
        decodeDocIDBlock();
        decodeFrequencyBlock();
        position = 0;
        blockDecoded = true;
    }
    
    // Advance within block
    while (position < (int)docIDs.size() && docIDs[position] < targetDocID) 
    {
        position++;
    }
    
    if (position >= (int)docIDs.size()) 
    {
        // Move to next block
        blockIndex++;
        blockDecoded = false;
        return nextGEQ(targetDocID);
    }
    
    return docIDs[position];
}

int PostingList::getCurrentFrequency() const 
{
    if (!blockDecoded || position >= (int)frequencies.size()) 
    {
        return 0;
    }
    return frequencies[position];
}

float PostingList::computeBM25Score(int N, int n, int D, float avgdl, 
    float k1, float b) const 
{
    int f = getCurrentFrequency();
    if (f == 0) return 0.0f;
    
    float idf = std::log((N - n + 0.5f) / (n + 0.5f) + 1.0f);
    float termFreqNorm = (f * (k1 + 1.0f)) / 
                         (f + k1 * (1.0f - b + b * D / avgdl));
    
    return idf * termFreqNorm;
}