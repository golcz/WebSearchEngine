"""
Web Crawler for New Zealand (.nz) Domains

A multi-threaded web crawler with robots.txt compliance, domain-based prioritization,
and BFS traversal. Built for educational purposes as part of a graduate coursework.

Author: Roman Vakhrushev
Course: NYU Tandon Web Search Engines 2024
"""

import random
import os
import sys
import urllib.request 
import urllib.robotparser
from urllib.parse import urljoin, urlparse
from queue import PriorityQueue 
from bs4 import BeautifulSoup
import time
import socket
from typing import Set, Dict, Tuple, Optional
from dataclasses import dataclass
from enum import IntEnum


# ============================================================================
# Configuration Constants
# ============================================================================

@dataclass
class CrawlerConfig:
    """Configuration parameters for the web crawler."""
    
    # Crawling limits
    max_urls: int = 5000
    seed_size: int = 20
    
    # Priority queue thresholds
    HIGH_PRIORITY: int = 0      # New domains
    MEDIUM_PRIORITY: int = 1    # Domains with 2-3 visits
    LOW_PRIORITY: int = 2       # Domains with 4-9 visits
    LOWEST_PRIORITY: int = 3    # Domains with 10+ visits
    
    # Domain visit thresholds for priority assignment
    MEDIUM_THRESHOLD_MIN: int = 2
    MEDIUM_THRESHOLD_MAX: int = 3
    LOW_THRESHOLD_MAX: int = 9
    
    # Random seed for reproducibility
    random_seed: int = 22
    
    # Timeout for socket connections (seconds)
    socket_timeout: int = 1
    
    # User agent string
    user_agent: str = 'NYU-CS-Crawler/1.3 (Educational Project)'


# ============================================================================
# Web Crawler Class
# ============================================================================

class NZWebCrawler:
    """
    A web crawler that traverses New Zealand (.nz) domains with BFS and
    domain-based prioritization. Respects robots.txt and handles redirects.
    """
    
    def __init__(self, config: CrawlerConfig = None):
        """Initialize crawler with optional configuration."""
        self.config = config or CrawlerConfig()
        
        # State tracking
        self.domain_visit_count: Dict[str, int] = {}
        self.visited_urls: Set[str] = set()
        self.bad_extensions: Set[str] = set()
        
        # Statistics
        self.total_bytes: int = 0
        self.http_errors: int = 0
        self.start_time: float = 0.0
        
        # URL queue (priority, (depth, url))
        self.url_queue: PriorityQueue = PriorityQueue()
        
        # Configure socket timeout
        socket.setdefaulttimeout(self.config.socket_timeout)
        
        # Set random seed for reproducibility
        random.seed(self.config.random_seed)
        
        # Configure URL opener with custom user agent
        self.opener = urllib.request.build_opener()
        self.opener.addheaders = [('User-agent', self.config.user_agent)]
        urllib.request.install_opener(self.opener)
    
    def _should_respect_robots(self, url: str) -> bool:
        """
        Check robots.txt for the given URL.
        
        Args:
            url: The URL to check against robots.txt
            
        Returns:
            True if crawling is allowed, False otherwise
        """
        parsed = urlparse(url)
        scheme = parsed.scheme
        domain = parsed.netloc
        
        try:
            robot = urllib.robotparser.RobotFileParser()
            robot.set_url(f"{scheme}://{domain}/robots.txt")
            robot.read()
            return robot.can_fetch("*", url)
        except Exception:
            # If robots.txt can't be read, assume allowed
            return True
    
    def _get_priority_for_domain(self, domain: str) -> int:
        """
        Calculate priority based on how many times domain has been visited.
        
        Priority scheme:
        - 0 (highest): New domains
        - 1: Domains visited 2-3 times
        - 2: Domains visited 4-9 times  
        - 3 (lowest): Domains visited 10+ times
        
        Args:
            domain: The domain to check
            
        Returns:
            Priority level (0 = highest, 3 = lowest)
        """
        if domain not in self.domain_visit_count:
            return self.config.HIGH_PRIORITY
        
        visit_count = self.domain_visit_count[domain]
        
        if self.config.MEDIUM_THRESHOLD_MIN <= visit_count <= self.config.MEDIUM_THRESHOLD_MAX:
            return self.config.MEDIUM_PRIORITY
        elif visit_count <= self.config.LOW_THRESHOLD_MAX:
            return self.config.LOW_PRIORITY
        else:
            return self.config.LOWEST_PRIORITY
    
    def _is_valid_nz_domain(self, url: str) -> bool:
        """
        Check if URL belongs to a New Zealand (.nz) top-level domain.
        
        Args:
            url: The URL to check
            
        Returns:
            True if domain ends with .nz, False otherwise
        """
        domain = urlparse(url).netloc
        _, suffix, _ = domain.partition('.nz')
        return suffix != ''
    
    def _normalize_url(self, url: str, base_url: Optional[str] = None) -> Optional[str]:
        """
        Normalize URL by removing fragments and resolving relative paths.
        
        Args:
            url: The URL to normalize
            base_url: Optional base URL for resolving relative paths
            
        Returns:
            Normalized URL or None if invalid
        """
        if not url:
            return None
        
        # Resolve relative URLs
        if base_url:
            url = urljoin(base_url, url)
        
        # Remove fragment identifiers
        url, _, _ = url.partition('#')
        
        return url
    
    def _extract_and_queue_links(self, soup: BeautifulSoup, parent_url: str, depth: int):
        """
        Extract all links from BeautifulSoup object and add to queue.
        
        Args:
            soup: BeautifulSoup parsed HTML
            parent_url: URL of the page being parsed
            depth: Current crawl depth
        """
        # Handle base tag for relative URL resolution
        base_tag = soup.find('base')
        base_href = base_tag.get('href') if base_tag else None
        
        if base_href:
            base_href = urljoin(parent_url, base_href)
        
        # Find all anchor tags
        for link in soup.find_all('a'):
            href = link.get('href')
            if not href:
                continue
            
            # Resolve URL
            if base_href:
                full_url = urljoin(base_href, href)
            else:
                full_url = urljoin(parent_url, href)
            
            # Normalize URL
            full_url = self._normalize_url(full_url)
            if not full_url:
                continue
            
            # Skip bad extensions and CGI scripts
            ext = os.path.splitext(full_url)[1].lower()
            if ext in self.bad_extensions or 'cgi' in full_url.lower():
                continue
            
            # Only crawl .nz domains
            if not self._is_valid_nz_domain(full_url):
                continue
            
            parsed = urlparse(full_url)
            domain = parsed.netloc
            
            # Skip if already visited
            if full_url in self.visited_urls:
                continue
            
            # Mark as visited and update domain count
            self.visited_urls.add(full_url)
            self.domain_visit_count[domain] = self.domain_visit_count.get(domain, 0) + 1
            
            # Calculate priority and add to queue
            priority = self._get_priority_for_domain(domain)
            self.url_queue.put((priority, (depth + 1, full_url)))
    
    def create_seed_file(self, seed_list_file: str) -> None:
        """
        Initialize crawler with random seeds from a domain list file.
        
        Args:
            seed_list_file: Path to file containing list of seed URLs
        """
        with open(seed_list_file, 'r') as f:
            all_seeds = f.read().splitlines()
        
        # Select random seeds
        selected_seeds = set()
        while len(selected_seeds) < self.config.seed_size:
            seed = random.choice(all_seeds)
            selected_seeds.add(seed)
        
        # Initialize queue and visited tracking
        for seed in selected_seeds:
            parsed = urlparse(seed)
            domain = parsed.netloc
            
            self.url_queue.put((self.config.HIGH_PRIORITY, (0, seed)))
            self.visited_urls.add(seed)
            self.domain_visit_count[domain] = 1
        
        # Write seeds to file for reproducibility
        with open("seed.txt", "w") as f:
            for seed in sorted(selected_seeds):
                print(seed, file=f)
    
    def crawl(self, seed_file: str, log_file: str = "log.txt") -> None:
        """
        Main crawling loop.
        
        Args:
            seed_file: Path to file containing seed URLs
            log_file: Path to output log file
        """
        self.start_time = time.time()
        
        # Initialize seeds
        self.create_seed_file(seed_file)
        
        # Open log file
        with open(log_file, "w") as log:
            url_id = 0
            
            while url_id < self.config.max_urls and not self.url_queue.empty():
                try:
                    # Get next URL from queue
                    priority, (depth, url) = self.url_queue.get()
                    
                    # Check robots.txt
                    if not self._should_respect_robots(url):
                        continue
                    
                    # Fetch the page
                    with urllib.request.urlopen(url) as response:
                        # Check content type
                        content_type = response.info().get_content_type()
                        if content_type != "text/html":
                            ext = os.path.splitext(url)[1]
                            self.bad_extensions.add(ext)
                            continue
                        
                        # Handle redirects
                        final_url = response.geturl()
                        if final_url != url:
                            final_parsed = urlparse(final_url)
                            final_domain = final_parsed.netloc
                            self.visited_urls.add(final_url)
                            self.domain_visit_count[final_domain] = self.domain_visit_count.get(final_domain, 0) + 1
                        
                        # Read and process HTML
                        html = response.read()
                        self.total_bytes += len(html)
                        
                        # Log successful request
                        print(f"{url_id} {depth} {url} {time.ctime(time.time())} {response.getcode()} {len(html)}", file=log)
                        
                        # Parse HTML and extract links
                        soup = BeautifulSoup(html, "lxml")
                        self._extract_and_queue_links(soup, url, depth)
                        
                except urllib.error.HTTPError as e:
                    # Log HTTP errors (404, 403, etc.)
                    print(f"{url_id} {depth} {url} {time.ctime(time.time())} {e.code} 0", file=log)
                    self.http_errors += 1
                    
                except Exception as e:
                    # Log other errors but continue crawling
                    # Don't increment url_id on failure to maintain count
                    continue
                
                url_id += 1
        
        # Write final statistics
        elapsed = time.time() - self.start_time
        with open(log_file, "a") as log:
            print(f"Number of files read: {self.config.max_urls}", file=log)
            print(f"Total time: {time.strftime('%H:%M:%S', time.gmtime(elapsed))}", file=log)
            print(f"Total size of files read (bytes): {self.total_bytes}", file=log)
            print(f"Total number of HTTP errors: {self.http_errors}", file=log)


# ============================================================================
# Main Entry Point
# ============================================================================

if __name__ == "__main__":
    # Configuration
    config = CrawlerConfig(max_urls=5000)
    
    # Run crawler
    crawler = NZWebCrawler(config)
    crawler.crawl("nz_domain_seeds_list.txt", "log.txt")