#include "QueryProcessor.h"
#include "LexiconManager.h"
#include "PageTable.h"
#include "PriorityQueue.h"
#include "PostingList.h"
#include "Utils.h"

#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <limits>
#include <chrono>
#include <unordered_set>

// For timing
using namespace std::chrono;

QueryProcessor::QueryProcessor(std::map<int, std::pair<long, long>>& lexicon,
                               LexiconManager& lm,
                               PageTable& pt,
                               TopKQueue& pq,
                               std::unordered_map<int, int>& wordDocFreq,
                               const std::string& encodedIndexFile,
                               const std::string& dataFile)
    : lexicon(lexicon), lexiconManager(lm), pageTable(pt), topKQueue(pq),
      wordDocFreq(wordDocFreq), encodedIndexFile(encodedIndexFile), dataFile(dataFile) {}

bool QueryProcessor::termExists(const std::string& term) const 
{
    int wordID = lexiconManager.getWordId(term);
    return (wordID != -1 && lexicon.find(wordID) != lexicon.end());
}

std::vector<PostingList> QueryProcessor::loadPostingLists(const std::vector<std::string>& terms) 
{
    std::vector<PostingList> lists;
    
    for (const auto& term : terms) 
    {
        int wordID = lexiconManager.getWordId(term);
        if (wordID != -1 && lexicon.find(wordID) != lexicon.end()) 
        {
            auto [metadataStart, metadataEnd] = lexicon[wordID];
            lists.emplace_back(wordID, encodedIndexFile, metadataStart, metadataEnd,
                               pageTable.getMaxDocID() + 1);
        }
    }
    
    return lists;
}

void QueryProcessor::sortByRarity(std::vector<PostingList>& lists) 
{
    std::sort(lists.begin(), lists.end(),
              [this](const PostingList& a, const PostingList& b) 
              {
                  return wordDocFreq[a.getWordID()] < wordDocFreq[b.getWordID()];
              });
}

void QueryProcessor::conjunctiveQuery(const std::vector<std::string>& terms) 
{
    std::vector<PostingList> lists = loadPostingLists(terms);
    
    // For conjunctive, all terms must exist
    if (lists.size() != terms.size()) 
    {
        std::cout << "Warning: One or more terms not found in index. "
                  << "Found " << lists.size() << " of " << terms.size() << " terms." << std::endl;
        
        // Print missing terms
        for (const auto& term : terms) 
        {
            if (!termExists(term)) 
            {
                std::cout << "  Missing term: \"" << term << "\"" << std::endl;
            }
        }
        return;
    }
    
    // Sort by rarity for efficiency (least frequent term first)
    sortByRarity(lists);
    
    int docID = 0;
    int maxDocID = pageTable.getMaxDocID();
    int N = pageTable.getSize();
    
    topKQueue.clear();
    
    while (docID <= maxDocID) 
    {
        // Get next candidate from the rarest term
        docID = lists[0].nextGEQ(docID);
        if (docID > maxDocID) break;
        
        // Check if all other lists have the same docID
        bool allMatch = true;
        for (size_t i = 1; i < lists.size(); ++i) 
        {
            if (lists[i].nextGEQ(docID) != docID) 
            {
                allMatch = false;
                break;
            }
        }
        
        if (allMatch) 
        {
            float score = 0.0f;
            int docLen = pageTable.getNumWords(docID);
            
            for (const auto& list : lists) 
            {
                score += list.computeBM25Score(N, wordDocFreq[list.getWordID()],
                                               docLen, pageTable.getAvgdl(), k1, b);
            }
            topKQueue.add(docID, score);
        }
        docID++;
    }
    
    topKQueue.finalize();
}

void QueryProcessor::disjunctiveQuery(const std::vector<std::string>& terms) 
{
    std::vector<PostingList> lists = loadPostingLists(terms);
    
    if (lists.empty()) 
    {
        std::cout << "Warning: No terms found in index." << std::endl;
        return;
    }
    
    int maxDocID = pageTable.getMaxDocID();
    int N = pageTable.getSize();
    
    // Use a map to accumulate scores (could be optimized with array for dense docIDs)
    std::unordered_map<int, float> scores;
    
    topKQueue.clear();
    
    // Process each posting list
    for (auto& list : lists) 
    {
        int docID = 0;
        int n = wordDocFreq[list.getWordID()];
        
        while (docID <= maxDocID) 
        {
            docID = list.nextGEQ(docID);
            if (docID > maxDocID) break;
            
            int docLen = pageTable.getNumWords(docID);
            float score = list.computeBM25Score(N, n, docLen, pageTable.getAvgdl(), k1, b);
            
            scores[docID] += score;
            docID++;
        }
    }
    
    // Transfer to priority queue and sort
    for (const auto& [docID, score] : scores) 
    {
        topKQueue.add(docID, score);
    }
    
    topKQueue.finalize();
}

void QueryProcessor::processQuery(const std::string& queryRaw, char mode) 
{
    if (mode != 'c' && mode != 'd') 
    {
        std::cerr << "Invalid query mode. Use 'c' for conjunctive or 'd' for disjunctive." << std::endl;
        return;
    }
    
    // Tokenize the query
    std::vector<std::string> terms = Utils::tokenize(queryRaw);
    
    if (terms.empty()) 
    {
        std::cout << "Empty query." << std::endl;
        return;
    }
    
    // Execute the appropriate query
    if (mode == 'c') 
    {
        conjunctiveQuery(terms);
    }
    else 
    {
        disjunctiveQuery(terms);
    }
}

void QueryProcessor::printResults() const 
{
    const auto& results = topKQueue.getElements();
    
    if (results.empty()) 
    {
        std::cout << "No results found." << std::endl;
        return;
    }
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "Top " << results.size() << " Results:" << std::endl;
    std::cout << "========================================" << std::endl;
    
    for (const auto& [docID, score] : results) 
    {
        std::cout << "\nDoc ID: " << docID << " | Score: " << score << std::endl;
        std::cout << "----------------------------------------" << std::endl;
        
        std::string paragraph = pageTable.getParagraph(docID, dataFile);
        // Truncate long paragraphs for display
        if (paragraph.length() > 500) 
        {
            std::cout << paragraph.substr(0, 500) << "... [truncated]" << std::endl;
        }
        else 
        {
            std::cout << paragraph << std::endl;
        }
    }
    std::cout << "========================================" << std::endl;
}

void QueryProcessor::printResultsSummary() const 
{
    const auto& results = topKQueue.getElements();
    
    if (results.empty()) 
    {
        std::cout << "No results found." << std::endl;
        return;
    }
    
    std::cout << "\nTop " << results.size() << " Results (docID, score):" << std::endl;
    for (const auto& [docID, score] : results) 
    {
        std::cout << "  " << docID << "\t" << score << std::endl;
    }
}

void QueryProcessor::writeResultsToFile(const std::string& outputFile, int queryID) const 
{
    const size_t bufferSize = 200 * 1024 * 1024; // 200 MB buffer
    static std::stringstream buffer;  // Static to persist across calls (for batch mode)
    static std::ofstream file;
    
    // Open file on first call
    static bool fileOpened = false;
    if (!fileOpened) 
    {
        file.open(outputFile, std::ios_base::app);
        if (!file.is_open()) 
        {
            throw std::runtime_error("Failed to open output file: " + outputFile);
        }
        fileOpened = true;
    }
    
    int counter = 0;
    int maxResults = topKQueue.getMaxSize();
    
    for (const auto& [docID, score] : topKQueue.getElements()) 
    {
        if (counter >= maxResults) break;
        ++counter;
        
        buffer << queryID << "\t" << docID << "\t" << score << "\n";
        
        // Flush if buffer is full
        if (static_cast<size_t>(buffer.tellp()) >= bufferSize) 
        {
            file << buffer.rdbuf();
            buffer.str("");
            buffer.clear();
        }
    }
    
    // Flush remaining on destruction (handled by static, but we'll flush periodically)
    // For simplicity, we flush after each write in batch mode by checking size
    if (static_cast<size_t>(buffer.tellp()) > 0) 
    {
        file << buffer.rdbuf();
        buffer.str("");
        buffer.clear();
    }
}

void QueryProcessor::interactiveMode() 
{
    std::string input;
    char mode;
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "BM25 Search Engine Interactive Mode" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << "  c <query>  - Conjunctive query (AND)" << std::endl;
    std::cout << "  d <query>  - Disjunctive query (OR)" << std::endl;
    std::cout << "  q          - Quit" << std::endl;
    std::cout << "========================================" << std::endl;
    
    while (true) 
    {
        std::cout << "\n> ";
        std::getline(std::cin, input);
        
        if (input.empty()) continue;
        
        // Parse command
        mode = input[0];
        if (mode == 'q' || mode == 'Q') 
        {
            std::cout << "Goodbye!" << std::endl;
            break;
        }
        
        if (mode != 'c' && mode != 'd') 
        {
            std::cout << "Invalid command. Use 'c', 'd', or 'q'." << std::endl;
            continue;
        }
        
        // Extract query (skip first character and space)
        std::string query;
        if (input.length() > 2) 
        {
            query = input.substr(2);
        } 
        else 
        {
            std::cout << "Empty query." << std::endl;
            continue;
        }
        
        // Time the query
        auto start = high_resolution_clock::now();
        
        processQuery(query, mode);
        
        auto end = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(end - start);
        
        printResults();
        std::cout << "\nQuery time: " << duration.count() << " ms" << std::endl;
    }
}

void QueryProcessor::batchMode(const std::string& queryIDMapFile,
                               const std::string& evalFile,
                               const std::string& resultsFile,
                               char queryMode) 
{
    // Load query ID map
    std::unordered_map<int, std::string> queryIDMap;
    std::ifstream mapFile(queryIDMapFile);
    if (!mapFile.is_open()) 
    {
        throw std::runtime_error("Failed to open query ID map file: " + queryIDMapFile);
    }
    
    std::string line;
    while (std::getline(mapFile, line)) 
    {
        std::istringstream iss(line);
        std::string token;
        int queryID;
        std::string query;
        
        std::getline(iss, token, '\t');
        queryID = std::stoi(token);
        std::getline(iss, query, '\t');
        queryIDMap[queryID] = query;
    }
    mapFile.close();
    
    // Load eval file (query IDs to process)
    std::unordered_set<int> queriesToProcess;
    std::ifstream evalFileStream(evalFile);
    if (!evalFileStream.is_open()) 
    {
        throw std::runtime_error("Failed to open eval file: " + evalFile);
    }
    
    while (std::getline(evalFileStream, line)) 
    {
        std::istringstream iss(line);
        std::string token;
        std::getline(iss, token, '\t');
        queriesToProcess.insert(std::stoi(token));
    }
    evalFileStream.close();
    
    // Clear output file
    std::ofstream clearFile(resultsFile, std::ios::trunc);
    clearFile.close();
    
    // Process each query
    int processed = 0;
    auto totalStart = high_resolution_clock::now();
    
    for (int queryID : queriesToProcess) 
    {
        auto it = queryIDMap.find(queryID);
        if (it == queryIDMap.end()) 
        {
            std::cerr << "Warning: Query ID " << queryID << " not found in map file." << std::endl;
            continue;
        }
        
        processQuery(it->second, queryMode);
        writeResultsToFile(resultsFile, queryID);
        processed++;
        
        // Progress update every 100 queries
        if (processed % 100 == 0) 
        {
            auto now = high_resolution_clock::now();
            auto elapsed = duration_cast<seconds>(now - totalStart);
            std::cout << "Processed " << processed << " queries in " << elapsed.count() << " seconds" << std::endl;
        }
    }
    
    auto totalEnd = high_resolution_clock::now();
    auto totalDuration = duration_cast<seconds>(totalEnd - totalStart);
    
    std::cout << "\nBatch processing complete!" << std::endl;
    std::cout << "Processed " << processed << " queries in " << totalDuration.count() << " seconds" << std::endl;
    std::cout << "Results written to: " << resultsFile << std::endl;
}