#ifndef PAGETABLEMANAGER_H
#define PAGETABLEMANAGER_H

#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>

class PageTableManager 
{
private:
    std::unordered_map<int, std::tuple<std::streampos, std::streamoff, int>> pageTable;
    float avgdl;
    int maxDocID;

    void computeAvgdl();

public:
    /**
     * @brief Loads the page table from a varbyte-encoded file.
     *
     * @param pageTableFile The file containing the varbyte-encoded page table.
     */
    PageTableManager(const std::string &pageTableFile);

    /**
     * @brief Retrieves the number of words for a given document ID.
     *
     * @param docID The document ID for which to retrieve the number of words.
     * @return The number of words in the document, or -1 if the document is not found.
     */
    int getNumWords(int docID);

    /**
     * @brief Retrieves the average document length.
     *
     * @return The average document length.
     */
    float getAvgdl();

    int getMaxDocID();

    /**
     * @brief Retrieves the paragraph text for a given document ID.
     *
     * @param docID The document ID for which to retrieve the paragraph text.
     * @param dataFile The file containing the document data.
     * @return The paragraph text of the document, or an empty string if the document is not found.
     */
    std::string getParagraph(int docID, const std::string &dataFile);

    int getSize();
};

#endif // PAGETABLEMANAGER_H
