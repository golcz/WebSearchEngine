#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#include <vector>
#include <utility>
#include <algorithm>

/**
 * @brief Fixed-size priority queue for top-K results.
 * 
 * Stores (docID, score) pairs and keeps only the top K by score.
 */
class TopKQueue 
{
private:
    std::vector<std::pair<int, float>> elements;
    int maxSize;
    
public:
    explicit TopKQueue(int maxSize);
    
    /**
     * @brief Add a (docID, score) pair to the queue.
     */
    void add(int docID, float score);
    
    /**
     * @brief Add a pair directly.
     */
    void add(const std::pair<int, float>& element);
    
    /**
     * @brief Sort by score descending and truncate to maxSize.
     */
    void finalize();
    
    /**
     * @brief Clear all elements.
     */
    void clear();
    
    /**
     * @brief Get all elements (sorted descending by score after finalize).
     */
    const std::vector<std::pair<int, float>>& getElements() const;
    
    /**
     * @brief Get the current size.
     */
    size_t size() const;
    
    /**
     * @brief Check if empty.
     */
    bool empty() const;
    
    /**
     * @brief For debugging.
     */
    void print() const;
};

#endif