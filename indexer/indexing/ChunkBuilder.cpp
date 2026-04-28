#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <vector>
#include <sys/stat.h>
#include "Utils.h"
#include "LexiconManager.h"

#ifdef _WIN32
#include <direct.h>  // For _mkdir on Windows
#endif

using namespace std;

/**
 * @class InvertedIndexBuilder
 * @brief Builds an inverted index by processing `x` lines at a time, creating temporary index files
 *        with unique wordID entries, where each file has postings lists in sorted order.
 */
class InvertedIndexBuilder 
{
private:
    LexiconManager lexiconManager;  ///< Manages word-to-ID mapping

    /**
     * @brief Creates the directory for storing temporary index files if it doesn't exist.
     *        Ensures compatibility for Windows and POSIX systems.
     */
    void createTempIndexesDirectory() 
    {
        #ifdef _WIN32
                _mkdir("temp_indexes");
        #else
                mkdir("temp_indexes", 0777);
        #endif
    }

    /**
     * @brief Writes all postings from memory to the specified output file in sorted order.
     *
     * @param postings Map of wordID to document-frequency pairs for all postings.
     * @param outFile Output file stream to write the postings to.
     */
    void writePostingsToTempFile(const map<int, vector<pair<int, int>>> &postings, ofstream &outFile) {
        for (const auto &entry : postings) 
        {
            int wordID = entry.first;
            const auto &docFreqList = entry.second;

            stringstream ss;
            ss << wordID << ":";
            for (const auto &docFreq : docFreqList) 
            {
                ss << " " << docFreq.first << " " << docFreq.second;
            }
            outFile << ss.str() << "\n";
        }
    }

public:
    /**
     * @brief Processes the input file by reading `x` lines at a time, constructing postings lists,
     *        and writing each set of processed postings to a temporary file. The temporary index files
     *        are stored in sorted order by wordID with unique entries per file.
     *
     * @param filename The input data file containing document text.
     * @param linesPerChunk Number of lines to process for each temporary index file.
     */
    void buildInvertedIndex(const string &filename, size_t linesPerChunk) 
    {
        lexiconManager.loadWordIdMap();  // Initialize the word-to-ID mapping

        createTempIndexesDirectory();  // Create the directory for temporary index files

        ifstream inputFile(filename);
        if (!inputFile.is_open()) 
        {
            cerr << "Error opening file: " << filename << endl;
            return;
        }

        string line;
        int chunkCount = 0;  // To track temporary file count

        while (inputFile) 
        {
            map<int, vector<pair<int, int>>> postings;  // Accumulate postings per chunk

            // Read linesPerChunk lines at once
            for (size_t lineCount = 0; lineCount < linesPerChunk && getline(inputFile, line); ++lineCount) 
            {
                // Parse docID and paragraph text from the line
                size_t tabPos = line.find('\t');
                if (tabPos == string::npos) continue;

                string docIdStr = line.substr(0, tabPos);
                string paragraph = line.substr(tabPos + 1);
                int docID = stoi(docIdStr);

                // Tokenize the paragraph and count occurrences of each word
                vector<string> words = Utils::tokenize(paragraph);
                map<int, int> wordCount;
                for (const string &word : words) 
                {
                    int wordID = lexiconManager.getWordId(word);
                    if (wordID != -1) 
                    {
                        wordCount[wordID]++;
                    }
                }

                // Aggregate docID and frequency for each wordID
                for (const auto &entry : wordCount) 
                {
                    int wordID = entry.first;
                    int frequency = entry.second;
                    postings[wordID].emplace_back(docID, frequency);
                }
            }

            // If postings map is empty, no more data to process
            if (postings.empty()) break;

            // Create and open a temporary file for the current chunk
            string tempFile = "temp_indexes/temp_posting_" + to_string(chunkCount++) + ".txt";
            ofstream outFile(tempFile, ios::out | ios::trunc);
            if (!outFile.is_open()) 
            {
                cerr << "Error opening file for writing: " << tempFile << endl;
                return;
            }

            // Write all postings for the chunk to the temp file and close it
            writePostingsToTempFile(postings, outFile);
            outFile.close();
        }

        inputFile.close();
        cout << "Temporary inverted index files created and stored in `temp_indexes` folder." << endl;
    }
};

int main(int argc, char *argv[]) 
{
    if (argc < 3) 
    {
        cerr << "Usage: " << argv[0] << " <input_file> <lines_per_chunk>" << endl;
        return 1;
    }

    string inputFile = argv[1];
    size_t linesPerChunk = static_cast<size_t>(stoi(argv[2]));

    InvertedIndexBuilder indexBuilder;
    indexBuilder.buildInvertedIndex(inputFile, linesPerChunk);

    return 0;
}
