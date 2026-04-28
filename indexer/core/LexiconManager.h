// LexiconManager.h
#pragma once
#include <string>
#include <unordered_map>

class LexiconManager 
{
private:
    std::unordered_map<std::string, int> wordIDMap;  // Map of words to Word IDs

public:
    /**
     * @brief Loads the Word ID map from the default file "word_id_map.txt".
     */
    LexiconManager(const std::string &filename);

    /**
     * @brief Adds a word to the map if not already present, with an initial ID of -1.
     *
     * @param word The word to add.
     */
    void addWord(const std::string &word);

    /**
     * @brief Gets the Word ID for a given word.
     *
     * @param word The word to get an ID for.
     * @return int The Word ID for the word, or -1 if the word does not exist.
     */
    int getWordId(const std::string &word) const;

    /**
     * @brief Saves the Word ID map to "word_id_map.txt".
     *        Assigns sorted IDs based on alphabetical order during saving.
     */
    void saveWordIdMap();
};
