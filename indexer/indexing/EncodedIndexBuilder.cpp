#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <tuple>
#include <cstdint>

using namespace std;

/**
 * @brief VarByte encoding function for a single integer.
 */
vector<uint8_t> varbyteEncode(int num) 
{
    vector<uint8_t> bytes;
    while (true) 
    {
        uint8_t byte = num % 128;
        num /= 128;
        if (num == 0) 
        {
            byte |= 128; // Set continuation bit for the last byte
            bytes.push_back(byte);
            break;
        } 
        else 
        {
            bytes.push_back(byte);
        }
    }
    return bytes;
}

/**
 * @brief Helper function to write encoded integers to a buffer.
 */
void writeEncodedInteger(vector<uint8_t> &buffer, int value) 
{
    vector<uint8_t> encoded = varbyteEncode(value);
    buffer.insert(buffer.end(), encoded.begin(), encoded.end());
}

/**
 * @brief Helper function to write encoded integers directly to an output file.
 */
void writeEncodedInteger(ofstream &outFile, int value) 
{
    vector<uint8_t> encoded = varbyteEncode(value);
    outFile.write(reinterpret_cast<const char*>(encoded.data()), encoded.size());
}

/**
 * @brief Encodes and writes the index file with metadata and docID/frequency blocks.
 */
void encodeIndex(const string &finalIndexFile, const string &encodedIndexFile, const string &lexiconFile) 
{
    ifstream inputFile(finalIndexFile);
    ofstream outputFile(encodedIndexFile, ios::binary);
    ofstream lexiconOutput(lexiconFile);

    if (!inputFile.is_open() || !outputFile.is_open() || !lexiconOutput.is_open()) 
    {
        cerr << "Error opening input/output files." << endl;
        return;
    }

    map<int, tuple<long, long, int>> lexiconMap; // Map of wordID to metadata start/end positions and docID count
    string line;
    long filePosition = 0; // Track file position for metadata

    while (getline(inputFile, line)) 
    {
        istringstream iss(line);
        int wordID;
        char colon;
        iss >> wordID >> colon;

        vector<int> docIDs, frequencies;
        int docID, frequency;
        while (iss >> docID >> frequency) 
        {
            docIDs.push_back(docID);
            frequencies.push_back(frequency);
        }

        int docIDCount = docIDs.size();
        vector<tuple<int, int, int>> metadataEntries; // To store metadata for each block
        vector<uint8_t> docFreqBuffer;                // Buffer to store docID and frequency blocks temporarily
        long metadataStart = filePosition;

        // Process blocks of 128 postings
        for (size_t i = 0; i < docIDs.size(); i += 128) 
        {
            size_t blockSize = min(size_t(128), docIDs.size() - i);

            // Collect metadata for the block
            int lastDocID = docIDs[i + blockSize - 1];
            long docIDBlockStart = docFreqBuffer.size();

            // Encode docID block: first docID followed by offsets
            writeEncodedInteger(docFreqBuffer, docIDs[i]);
            for (size_t j = 1; j < blockSize; ++j) 
            {
                int offset = docIDs[i + j] - docIDs[i + j - 1];
                writeEncodedInteger(docFreqBuffer, offset);
            }
            long docIDBlockEnd = docFreqBuffer.size();

            // Encode frequency block
            long freqBlockStart = docFreqBuffer.size();
            for (size_t j = 0; j < blockSize; ++j) 
            {
                writeEncodedInteger(docFreqBuffer, frequencies[i + j]);
            }
            long freqBlockEnd = docFreqBuffer.size();

            // Store metadata entry (lastDocID, docID block length, frequency block length)
            metadataEntries.push_back(make_tuple(lastDocID, docIDBlockEnd - docIDBlockStart, freqBlockEnd - freqBlockStart));
        }

        // Write the metadata block first for this wordID
        for (const auto &entry : metadataEntries) 
        {
            int lastDocID = get<0>(entry);
            int docIDBlockSize = get<1>(entry);
            int freqBlockSize = get<2>(entry);

            writeEncodedInteger(outputFile, lastDocID);
            writeEncodedInteger(outputFile, docIDBlockSize);
            writeEncodedInteger(outputFile, freqBlockSize);

            filePosition += varbyteEncode(lastDocID).size() +
                            varbyteEncode(docIDBlockSize).size() +
                            varbyteEncode(freqBlockSize).size();
        }

        // Set metadataEnd here, after writing metadata but before the docFreqBuffer
        long metadataEnd = filePosition;

        // Store lexicon entry for this wordID with metadata start, end positions, and docID count
        lexiconMap[wordID] = make_tuple(metadataStart, metadataEnd, docIDCount);

        // Now write the docID and frequency blocks from the buffer
        outputFile.write(reinterpret_cast<const char*>(docFreqBuffer.data()), docFreqBuffer.size());
        filePosition += docFreqBuffer.size();
    }

    // Write lexicon data
    for (const auto &entry : lexiconMap) 
    {
        int wordID = entry.first;
        long metadataStart = get<0>(entry.second);
        long metadataEnd = get<1>(entry.second);
        int docIDCount = get<2>(entry.second);
        lexiconOutput << wordID << " " << metadataStart << " " << metadataEnd << " " << docIDCount << "\n";
    }

    inputFile.close();
    outputFile.close();
    lexiconOutput.close();
}

int main(int argc, char *argv[]) 
{
    if (argc < 4) 
    {
        cerr << "Usage: " << argv[0] << " <final_index_file> <encoded_index_file> <lexicon_file>" << endl;
        return 1;
    }

    string finalIndexFile = argv[1];
    string encodedIndexFile = argv[2];
    string lexiconFile = argv[3];

    encodeIndex(finalIndexFile, encodedIndexFile, lexiconFile);

    cout << "Encoded index and lexicon files generated successfully." << endl;
    return 0;
}
