# Build Instructions

## Prerequisites

- Python 3.8+ (for crawler)
- C++17 compiler

## Crawler Setup

```bash
cd crawler
pip install beautifulsoup4 lxml
python crawler.py
Indexer Build
bash
cd indexer
```

## Indexer and Search Setup

# Compile core utilities
```bash
g++ -std=c++17 -c core/Utils.cpp -o Utils.o
g++ -std=c++17 -c core/LexiconManager.cpp -o LexiconManager.o
```

# Compile indexing tools
```bash
g++ -std=c++17 -o PageTableBuilder indexing/PageTableBuilder.cpp Utils.o LexiconManager.o
g++ -std=c++17 -o ChunkBuilder indexing/ChunkBuilder.cpp Utils.o LexiconManager.o
g++ -std=c++17 -o IndexMerger indexing/IndexMerger.cpp
g++ -std=c++17 -o EncodedIndexBuilder indexing/EncodedIndexBuilder.cpp Utils.o
```

## Search Engine Build

```bash
cd search
```

# Compile core
```bash
g++ -std=c++17 -c ../indexer/core/Utils.cpp -o Utils.o
g++ -std=c++17 -c ../indexer/core/LexiconManager.cpp -o LexiconManager.o
g++ -std=c++17 -c ../indexer/core/PageTable.cpp -o PageTable.o
g++ -std=c++17 -c ../indexer/core/PostingList.cpp -o PostingList.o
```

# Compile search components
```bash
g++ -std=c++17 -c PriorityQueue.cpp -o PriorityQueue.o
g++ -std=c++17 -c QueryProcessor.cpp -o QueryProcessor.o
```

# Link
```bash
g++ -std=c++17 -o QueryProcessor main.cpp \
    Utils.o LexiconManager.o PageTable.o PostingList.o \
    PriorityQueue.o QueryProcessor.o
```

#Quick Test
```bash
# Create a tiny test collection
echo -e "0\tThis is a test document.\n1\tAnother test here." > test.tsv
```

# Index it
```bash
./PageTableBuilder test.tsv
./ChunkBuilder test.tsv 100
./IndexMerger temp_indexes/ merged.txt 100 100
./EncodedIndexBuilder merged.txt encoded.bin lexicon.txt
```

# Search it
```bash
echo "c test" | ./QueryProcessor lexicon.txt word_id_map.txt page_table.bin encoded.bin test.tsv 5
```