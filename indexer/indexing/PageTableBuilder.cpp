#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <tuple>
#include "Utils.h"
#include "LexiconManager.h"

using namespace std;

/**
 * @class PageTableBuilder
 * @brief Constructs a page table and assigns Word IDs to each word in the document corpus.
 *        This class uses varbyte encoding for efficient storage.
 */
class PageTableBuilder 
{
private:
    unordered_map<int, tuple<streampos, streamoff, int>> pageTable;  // Maps docID -> (startPos, endPosDiff, wordCount)
    LexiconManager lexiconManager;  // Manages the word -> word ID assignments

    /**
     * @brief Processes a single line of text to build the page table entry and register words in the Word ID map.
     *
     * @param line The line containing document data.
     * @param startPos The starting position of the paragraph text in the file.
     * @param endPos The ending position of the paragraph text in the file.
     */
    void processLineForPageTable(const string &line, streampos startPos, streampos endPos) 
    {
        size_t tabPos = line.find('\t');
        if (tabPos == string::npos) 
        {
            cerr << "Error parsing line: " << line << endl;
            return;
        }

        // Extract docID and paragraph text
        string docIdStr = line.substr(0, tabPos);
        string paragraph = line.substr(tabPos + 1);
        int docID = stoi(docIdStr);

        // Calculate the start position of the actual paragraph text (after '\t')
        streampos textStartPos = startPos + static_cast<streamoff>(tabPos + 1);
        vector<string> words = Utils::tokenize(paragraph);  // Tokenize paragraph to get word list
        int wordCount = words.size();

        // Register each word in the Word ID map
        for (const string &word : words) 
        {
            lexiconManager.addWord(word);
        }

        // Store the paragraph start position, difference between end and textStartPos, and word count in the page table
        streamoff endDiff = endPos - textStartPos;
        pageTable[docID] = make_tuple(textStartPos, endDiff, wordCount);
    }

    /**
     * @brief Writes the page table to a binary file using varbyte encoding for each field.
     *
     * @param pageTableFilename The name of the file to write the page table to.
     */
    void writePageTable(const string &pageTableFilename) 
    {
        ofstream pageTableFile(pageTableFilename, ios::binary);
        if (!pageTableFile.is_open()) 
        {
            cerr << "Error opening file: " << pageTableFilename << endl;
            return;
        }

        for (const auto &entry : pageTable) 
        {
            int docID = entry.first;
            streampos startPos;
            streamoff endDiff;
            int wordCount;
            tie(startPos, endDiff, wordCount) = entry.second;

            // Encode and write docID, startPos, endDiff, and wordCount using varbyte encoding
            Utils::varbyteEncode(pageTableFile, docID);
            Utils::varbyteEncode(pageTableFile, static_cast<uint32_t>(startPos));
            Utils::varbyteEncode(pageTableFile, static_cast<uint32_t>(endDiff));
            Utils::varbyteEncode(pageTableFile, wordCount);
        }

        pageTableFile.close();
    }

public:
    /**
     * @brief Builds the page table and Word ID map by reading the input file and processing each line.
     *
     * @param filename The name of the input file.
     */
    void buildPageTable(const string &filename) 
    {
        ifstream inputFile(filename, ios::in | ios::binary);
        if (!inputFile.is_open()) {
            cerr << "Error opening file: " << filename << endl;
            return;
        }

        const size_t CHUNK_SIZE = 200 * 1024 * 1024;
        vector<char> buffer(CHUNK_SIZE);
        string line;

        streampos filePos = 0;
        while (inputFile) {
            inputFile.seekg(filePos);
            inputFile.read(buffer.data(), CHUNK_SIZE);

            // Find the last complete line in the buffer
            streamoff lastNewline = string(buffer.data(), CHUNK_SIZE).rfind('\n');
            if (lastNewline == string::npos) {
                cerr << "Error: No newline found in buffer!" << endl;
                break;
            }

            // Process each line up to the last complete line
            istringstream chunkStream(string(buffer.data(), lastNewline));
            while (getline(chunkStream, line)) {
                streamoff posOffset = static_cast<streamoff>(chunkStream.tellg()) - static_cast<streamoff>(line.length()) - 1;
                streampos startPos = static_cast<streampos>(filePos + posOffset);
                streampos endPos = static_cast<streampos>(filePos + static_cast<streamoff>(chunkStream.tellg()));

                processLineForPageTable(line, startPos, endPos);
            }

            filePos += lastNewline + 1;
        }

        inputFile.close();

        writePageTable("page_table_varbyte.bin");  // Write the page table in varbyte format
        lexiconManager.saveWordIdMap();  // Save the Word ID map with default filename
        cout << "Varbyte-encoded page table and Word ID map created successfully!" << endl;
    }
};

/*
int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <input_file>" << endl;
        return 1;
    }

    string inputFile = argv[1];

    PageTableBuilder pageTableBuilder;
    pageTableBuilder.buildPageTable(inputFile);

    return 0;
}
*/