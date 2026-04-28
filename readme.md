markdown
# Web Search Engine

A complete simple search engine with a web crawler, inverted index, and BM25 ranking.

Built as a part of my Master's in CS at NYU.

## Project Structure
├── crawler/ # Python web crawler (.nz domains)

├── indexer/ # C++ inverted index builder

|── search/ # C++ BM25 search engine

## Modules

### 1. Web Crawler

Focused crawler for New Zealand domains with robots.txt compliance.

```bash
cd crawler
pip install -r requirements.txt
python crawler.py
```
Output: log.txt (crawl log with URLs, status codes, timestamps)

### 2. Index Builder
Builds compressed inverted index using SPIMI and varbyte encoding.
Requires document collection (to index) in .tsv format.

```bash
cd indexer
make
./PageTableBuilder ../data/collection.tsv
./ChunkBuilder ../data/collection.tsv 1000000
./IndexMerger temp_indexes/ merged_index.txt
./EncodedIndexBuilder merged_index.txt encoded_index.bin lexicon.txt
```

### 3. Search Engine
BM25 search with conjunctive (c) and disjunctive (d) queries. Can be used in either interactive (online) or file-based mode.

```bash
cd search
make
./QueryProcessor lexicon.txt word_id_map.txt page_table.bin \
    encoded_index.bin ../data/collection.tsv 10
```

Author
Roman Vakhrushev | NYU Tandon MS CS | Web Search Engines | December 2024
