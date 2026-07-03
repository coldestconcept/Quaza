#!/usr/bin/env python3
"""
prosperopatches.com scraper for PS5 backports
"""

import requests
import re
import json
import os
import sys
from pathlib import Path

BACKPORT_DIR = "/data/pkg_tool/backports"

def scrape_prospero(ppsaid):
    """Scrape backport info from prosperopatches.com"""
    url = f"https://prosperopatches.com/{ppsaid}"
    
    try:
        response = requests.get(url, timeout=30)
        response.raise_for_status()
        
        html = response.text
        
        # Extract info
        info = {
            'ppsaid': ppsaid,
            'title': extract_title(html),
            'required_fw': extract_firmware(html),
            'backport_target': extract_backport_target(html),
            'download_links': extract_downloads(html),
            'file_size': extract_size(html)
        }
        
        return info
        
    except Exception as e:
        print(f"Error scraping {ppsaid}: {e}")
        return None

def extract_title(html):
    """Extract game title from page"""
    match = re.search(r'<title>(.*?)</title>', html)
    return match.group(1) if match else "Unknown"

def extract_firmware(html):
    """Extract required firmware"""
    match = re.search(r'Requires FW ([\d\.]+)', html)
    return match.group(1) if match else "Unknown"

def extract_backport_target(html):
    """Extract backport target firmware"""
    match = re.search(r'Backported to ([\d\.]+)', html)
    return match.group(1) if match else None

def extract_downloads(html):
    """Extract download links"""
    links = []
    # Pattern for download links (varies by host)
    patterns = [
        r'href="(https://[^"]*\.pkg[^"]*)"',
        r'href="(https://drive\.google\.com[^"]*)"',
        r'href="(https://[^"]*mediafire[^"]*)"'
    ]
    for pattern in patterns:
        matches = re.findall(pattern, html)
        links.extend(matches)
    return list(set(links))

def extract_size(html):
    """Extract file size"""
    match = re.search(r'([\d\.]+)\s*(GB|MB)', html, re.IGNORECASE)
    return match.group(0) if match else "Unknown"

def download_backport(ppsaid, info):
    """Download and extract backport"""
    game_dir = Path(BACKPORT_DIR) / ppsaid
    game_dir.mkdir(parents=True, exist_ok=True)
    
    # Save info
    with open(game_dir / "info.json", 'w') as f:
        json.dump(info, f, indent=2)
    
    # Download chunks
    for i, link in enumerate(info['download_links']):
        print(f"Downloading chunk {i+1}/{len(info['download_links'])}...")
        # Implement download logic here
        # Handle Google Drive, MediaFire, etc.
        
    # Merge chunks if needed
    # Extract fakelib
    
    return game_dir

def main():
    if len(sys.argv) < 2:
        print("Usage: scraper.py <PPSA_ID>")
        sys.exit(1)
    
    ppsaid = sys.argv[1]
    
    print(f"Scraping prosperopatches.com for {ppsaid}...")
    info = scrape_prospero(ppsaid)
    
    if info:
        print(f"Found: {info['title']}")
        print(f"Required FW: {info['required_fw']}")
        print(f"Backport to: {info['backport_target']}")
        
        if info['backport_target']:
            print("Downloading backport...")
            download_backport(ppsaid, info)
            print(f"Backport saved to {BACKPORT_DIR}/{ppsaid}/")
        else:
            print("No backport available")
    else:
        print("Failed to scrape")

if __name__ == "__main__":
    main()
