# Web Crawler for .nz Domains

A focused web crawler that traverses New Zealand top-level domains with robots.txt compliance and domain-based prioritization.

## Features

- **Robots.txt compliance** – Respects exclusion protocols
- **Domain prioritization** – New domains get highest priority; visited domains get lower priority
- **BFS traversal** – Systematic breadth-first exploration
- **NZ domain focus** – Only crawls `.nz` domains
- **Redirect handling** – Follows and logs redirects
- **Error resilience** – Handles HTTP errors and timeouts gracefully

## Quick Start

```bash
# Install dependencies
pip install -r requirements.txt

# Run crawler
python crawler.py