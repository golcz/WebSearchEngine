#include "Utils.h"
#include "PageTableManager.h"
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <tuple>
#include <vector>

PageTableManager::PageTableManager(const std::string &pageTableFile) 
{
    std::ifstream file(pageTableFile, std::ios::binary);
    if (!file.is_open()) 
    {
        std::cerr << "Error opening page table file: " << pageTableFile << std::endl;
        return;
    }

    maxDocID = 0;

    // Read the varbyte-encoded entries
    while (file) {
        // Decode docID
        uint32_t docID = Utils::varbyteDecode(file);

        // Decode start position, end difference, and number of words
        uint32_t startPos = Utils::varbyteDecode(file);
        uint32_t endDiff = Utils::varbyteDecode(file);
        int numWords = Utils::varbyteDecode(file);

        // Store the values in the page table
        pageTable[docID] = std::make_tuple(static_cast<std::streampos>(startPos),
                                           static_cast<std::streamoff>(endDiff),
                                           numWords);
        if(docID > maxDocID)
        {
            maxDocID = docID;
        }
    }
    file.close();
    PageTableManager::computeAvgdl();
}

int PageTableManager::getNumWords(int docID)
{
    if (pageTable.find(docID)!= pageTable.end()) 
    {
        return std::get<2>(pageTable[docID]);  // Return number of words
    }
    return -1;  // Document not found
}

float PageTableManager::getAvgdl()
{
    return avgdl;
}

int PageTableManager::getSize()
{
    return pageTable.size();
}

int PageTableManager::getMaxDocID()
{
    return maxDocID;
}

std::string PageTableManager::getParagraph(int docID, const std::string &dataFile)
{
    if (pageTable.find(docID) == pageTable.end()) {
        return "";  // Document not found
    }

    // Retrieve the start position and end difference
    std::streampos startPos = std::get<0>(pageTable[docID]);
    std::streamoff endDiff = std::get<1>(pageTable[docID]);

    std::ifstream file(dataFile, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error opening data file: " << dataFile << std::endl;
        return "";
    }

    // Seek to the start position and read the paragraph
    file.seekg(startPos);
    std::vector<char> buffer(static_cast<size_t>(endDiff));
    file.read(buffer.data(), endDiff);

    file.close();
    return std::string(buffer.begin(), buffer.end());
}

void PageTableManager::computeAvgdl()
{
    int numlen = 0;
    for (auto& it: pageTable) 
    {
        numlen+= std::get<2>(it.second);
    }
    avgdl = float(numlen)/pageTable.size();
}