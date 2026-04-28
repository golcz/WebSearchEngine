#include "PriorityQueue.h"
#include <iostream>

TopKQueue::TopKQueue(int maxSize) : maxSize(maxSize) 
{
    elements.reserve(maxSize);
}

void TopKQueue::add(int docID, float score) 
{
    elements.emplace_back(docID, score);
}

void TopKQueue::add(const std::pair<int, float>& element) 
{
    elements.push_back(element);
}

void TopKQueue::finalize() 
{
    std::sort(elements.begin(), elements.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    if ((int)elements.size() > maxSize) 
    {
        elements.resize(maxSize);
    }
}

void TopKQueue::clear() 
{
    elements.clear();
}

const std::vector<std::pair<int, float>>& TopKQueue::getElements() const 
{
    return elements;
}

size_t TopKQueue::size() const 
{
    return elements.size();
}

bool TopKQueue::empty() const 
{
    return elements.empty();
}

void TopKQueue::print() const 
{
    for (const auto& elem : elements) 
    {
        std::cout << "(" << elem.first << ", " << elem.second << ") ";
    }
    std::cout << std::endl;
}