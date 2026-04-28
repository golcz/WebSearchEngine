#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <tuple>
#include <queue>
#include <dirent.h>  // For directory access on systems without <filesystem>
#include <sys/stat.h>

using namespace std;

/**
 * @struct IndexEntry
 * @brief Represents a single entry in the inverted index with word ID, document ID, frequency, and file index.
 *        The comparison operator optimizes sorting by wordID and maintains order by fileIndex in case of ties.
 */
struct IndexEntry 
{
    int wordID, docID, frequency, fileIndex;

    // Comparison operator for min-heap to prioritize lower wordID, then by fileIndex
    bool operator<(const IndexEntry &other) const 
    {
        if (wordID == other.wordID) {
            return fileIndex > other.fileIndex;  // Prioritize entries from earlier files for equal wordIDs
        }
        return wordID > other.wordID;  // Min-heap based on wordID in ascending order
    }
};

/**
 * @brief Parses a line from a temporary index file and converts it into an IndexEntry structure.
 *
 * @param line The line to parse, formatted as "wordID: docID frequency ...".
 * @param fileIndex The index of the file from which the entry is read.
 * @return A vector of IndexEntry objects containing wordID, docID, frequency, and fileIndex.
 */
vector<IndexEntry> parseLine(const string &line, int fileIndex) 
{
    istringstream iss(line);
    int wordID;
    char colon;
    iss >> wordID >> colon;

    vector<IndexEntry> entries;
    int docID, frequency;
    while (iss >> docID >> frequency) 
    {
        entries.push_back({wordID, docID, frequency, fileIndex});
    }

    return entries;
}

/**
 * @brief Retrieves all file names in the specified directory, filtering out "." and "..".
 *
 * @param directoryPath The path of the directory to search.
 * @return A vector of file paths within the directory.
 */
vector<string> getFilesInDirectory(const string &directoryPath) 
{
    vector<string> tempFiles;
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(directoryPath.c_str())) != NULL) 
    {
        while ((ent = readdir(dir)) != NULL) 
        {
            string fileName = ent->d_name;
            if (fileName != "." && fileName != "..") 
            {
                tempFiles.push_back(directoryPath + "/" + fileName);
            }
        }
        closedir(dir);
    } 
    else 
    {
        cerr << "Error opening directory: " << directoryPath << endl;
    }
    return tempFiles;
}

/**
 * @brief Merges sorted temporary index files into a single sorted output file.
 *        Consolidates docID and frequency pairs for each wordID across files.
 *
 * @param directoryPath The path of the directory containing temporary index files.
 * @param outputFile The path of the output file where the merged index will be written.
 * @param readBufferSize The number of lines to read from each file at once.
 * @param writeBufferSize The size of the write buffer for the output file.
 */
void mergeSortedIndexFiles(const string &directoryPath, const string &outputFile, size_t readBufferSize, size_t writeBufferSize) 
{
    vector<string> tempFiles = getFilesInDirectory(directoryPath);
    if (tempFiles.empty()) 
    {
        cerr << "No files found in directory: " << directoryPath << endl;
        return;
    }

    vector<ifstream> tempFileStreams(tempFiles.size());
    for (size_t i = 0; i < tempFiles.size(); ++i) 
    {
        tempFileStreams[i].open(tempFiles[i]);
        if (!tempFileStreams[i].is_open()) 
        {
            cerr << "Error opening temporary file: " << tempFiles[i] << endl;
            return;
        }
    }

    priority_queue<IndexEntry> minHeap;
    vector<string> outputBuffer;
    outputBuffer.reserve(writeBufferSize);

    vector<vector<IndexEntry>> readBuffers(tempFiles.size(), vector<IndexEntry>());
    int currentWordID = -1;
    stringstream currentLine;

    // Fill initial entries for each temp file into the heap
    for (size_t i = 0; i < tempFiles.size(); ++i) 
    {
        string line;
        while (getline(tempFileStreams[i], line) && readBuffers[i].size() < readBufferSize) 
        {
            auto entries = parseLine(line, static_cast<int>(i));
            readBuffers[i].insert(readBuffers[i].end(), entries.begin(), entries.end());
        }

        if (!readBuffers[i].empty()) 
        {
            minHeap.push(readBuffers[i].front());
            readBuffers[i].erase(readBuffers[i].begin());
        }
    }

    ofstream outFile(outputFile);
    if (!outFile.is_open()) 
    {
        cerr << "Error opening output file: " << outputFile << endl;
        return;
    }

    // Process the min-heap and write the merged entries to the output file
    while (!minHeap.empty()) 
    {
        IndexEntry smallestEntry = minHeap.top();
        minHeap.pop();

        if (smallestEntry.wordID != currentWordID) 
        {
            // Flush currentLine to outputBuffer
            if (!currentLine.str().empty()) {
                outputBuffer.push_back(currentLine.str());
                if (outputBuffer.size() >= writeBufferSize) 
                {
                    // Write the output buffer to file
                    for (const auto &line : outputBuffer) 
                    {
                        outFile << line << "\n";
                    }
                    outputBuffer.clear();
                }
            }

            currentWordID = smallestEntry.wordID;
            currentLine.str("");
            currentLine << currentWordID << ":";
        }

        // Add docID and frequency to the current wordID entry
        currentLine << " " << smallestEntry.docID << " " << smallestEntry.frequency;

        int fileIndex = smallestEntry.fileIndex;
        string line;
        if (readBuffers[fileIndex].empty() && getline(tempFileStreams[fileIndex], line)) 
        {
            auto entries = parseLine(line, fileIndex);
            readBuffers[fileIndex].insert(readBuffers[fileIndex].end(), entries.begin(), entries.end());
        }

        if (!readBuffers[fileIndex].empty()) 
        {
            minHeap.push(readBuffers[fileIndex].front());
            readBuffers[fileIndex].erase(readBuffers[fileIndex].begin());
        }
    }

    // Final flush of currentLine and outputBuffer to file
    if (!currentLine.str().empty()) 
    {
        outputBuffer.push_back(currentLine.str());
    }

    for (const auto &line : outputBuffer) 
    {
        outFile << line << "\n";
    }
    outFile.close();

    for (auto &tempFileStream : tempFileStreams) 
    {
        if (tempFileStream.is_open()) 
        {
            tempFileStream.close();
        }
    }

    cout << "Merged sorted index files into " << outputFile << endl;
}

int main(int argc, char *argv[]) 
{
    if (argc < 5) 
    {
        cerr << "Usage: " << argv[0] << " <directory_path> <output_file> <read_buffer_size> <write_buffer_size>" << endl;
        return 1;
    }

    string directoryPath = argv[1];
    string outputFile = argv[2];
    size_t readBufferSize = static_cast<size_t>(stoi(argv[3]));
    size_t writeBufferSize = static_cast<size_t>(stoi(argv[4]));

    mergeSortedIndexFiles(directoryPath, outputFile, readBufferSize, writeBufferSize);

    return 0;
}
