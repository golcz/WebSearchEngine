#ifndef QUERY_PROCESSOR_H
#define QUERY_PROCESSOR_H

#include <string>
#include <vector>
#include <map>
#include <unordered_map>

class LexiconManager;
class PageTable;
class TopKQueue;
class PostingList;

/**
 * @brief Main query processing class.
 * 
 * Handles BM25 scoring with conjunctive and disjunctive query modes.
 */
class QueryProcessor 
{
private:
    // References to external data structures
    std::map<int, std::pair<long, long>>& lexicon;  // wordID -> (metadataStart, metadataEnd)
    LexiconManager& lexiconManager;
    PageTable& pageTable;
    TopKQueue& topKQueue;
    std::unordered_map<int, int>& wordDocFreq;      // wordID -> document frequency
    
    std::string encodedIndexFile;
    std::string dataFile;
    
    /**
     * @brief Creates PostingList objects for each query term.
     * @param terms Tokenized query terms
     * @return Vector of valid posting lists (empty if any term missing for conjunctive)
     */
    std::vector<PostingList> loadPostingLists(const std::vector<std::string>& terms);
    
    /**
     * @brief Sort posting lists by rarity (fewest documents first) for efficiency.
     */
    void sortByRarity(std::vector<PostingList>& lists);
    
public:
    QueryProcessor(std::map<int, std::pair<long, long>>& lexicon,
                   LexiconManager& lm,
                   PageTable& pt,
                   TopKQueue& pq,
                   std::unordered_map<int, int>& wordDocFreq,
                   const std::string& encodedIndexFile,
                   const std::string& dataFile);
    
    /**
     * @brief Run conjunctive query (AND semantics).
     * @param terms Tokenized query terms
     */
    void conjunctiveQuery(const std::vector<std::string>& terms);
    
    /**
     * @brief Run disjunctive query (OR semantics with BM25 ranking).
     * @param terms Tokenized query terms
     */
    void disjunctiveQuery(const std::vector<std::string>& terms);
    
    /**
     * @brief Process a query string with specified mode.
     * @param queryRaw Raw query string
     * @param mode 'c' for conjunctive, 'd' for disjunctive
     */
    void processQuery(const std::string& queryRaw, char mode);
    
    /**
     * @brief Print results to console.
     */
    void printResults() const;
    
    /**
     * @brief Write results to file in TREC format.
     * @param outputFile Path to output file
     * @param queryID Query identifier
     */
    void writeResultsToFile(const std::string& outputFile, int queryID) const;
};

#endif