#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <string>
#include "LexiconManager.h"

using namespace std;

/**
 * @brief Loads an existing Word ID map from "word_id_map.txt".
 *        This function initializes the Word ID map from a file if it exists.
 */
LexiconManager::LexiconManager(const string &filename) 
{
    ifstream inFile(filename);
    if (!inFile.is_open()) 
    {
        cerr << "Error opening word ID map file for reading" << endl;
        return;
    }

    string word;
    int id;
    while (inFile >> word >> id) 
    {
        wordIDMap[word] = id;
    }

    inFile.close();
}

/**
 * @brief Adds a word to the Word ID map with an initial ID of -1 if it does not exist.
 *
 * @param word The word to add to the map.
 */
void LexiconManager::addWord(const string &word) 
{
    if (wordIDMap.find(word) == wordIDMap.end()) 
    {
        wordIDMap[word] = -1;  // Initialize with -1 to indicate unassigned ID
    }
}

/**
 * @brief Retrieves the ID for a given word. Returns -1 if the word is not in the map.
 *
 * @param word The word to get an ID for.
 * @return The ID of the word, or -1 if the word is not found.
 */
int LexiconManager::getWordId(const string &word) const 
{
    auto it = wordIDMap.find(word);
    return (it != wordIDMap.end()) ? it->second : -1;
}

/**
 * @brief Saves the Word ID map to "word_id_map.txt" in alphabetical order.
 *        Assigns sequential IDs starting from 0 to each word during saving.
 */
void LexiconManager::saveWordIdMap() 
{
    ofstream outFile("word_id_map.txt", ios::out | ios::trunc);
    if (!outFile.is_open()) 
    {
        cerr << "Error opening word ID map file for writing" << endl;
        return;
    }

    // Collect and sort words alphabetically
    vector<string> words;
    for (const auto &entry : wordIDMap) {
        words.push_back(entry.first);
    }
    sort(words.begin(), words.end());

    // Assign sequential IDs and write to file
    int currentID = 0;
    for (const auto &word : words) {
        wordIDMap[word] = currentID;  // Update in-memory map
        outFile << word << " " << currentID++ << endl;
    }

    outFile.close();
}
