#!/usr/bin/env python3
import sys
import json
import urllib.request
import urllib.error
import re

def scrape_game_patches(title_id):
    """
    Scrapes prosperopatches.com for system update files and 
    delta structures matching the specific PS5 target game ID.
    """
    # Clean user parameter structure inputs
    title_id = title_id.strip().upper()
    if not re.match(r"^[A-Z]{4}\d{5}$", title_id):
        return {
            "status": "error",
            "message": f"Malformed Title ID format structure '{title_id}' (Expected example: CUSA00001 or PPSA00001)."
        }

    # Construct tracking portal destination endpoint reference
    target_url = f"https://prosperopatches.com{title_id}"
    
    headers = {
        'User-Agent': 'Mozilla/5.0 (PlayStation 5; console) QuazaPKG/1.0 Engine'
    }
    
    try:
        req = urllib.request.Request(target_url, headers=headers)
        with urllib.request.urlopen(req, timeout=10) as response:
            html_content = response.read().decode('utf-8')
            
    except urllib.error.HTTPError as e:
        if e.code == 404:
            return {"status": "error", "message": f"Title ID {title_id} not indexed on prosperopatches.com."}
        return {"status": "error", "message": f"HTTP request failed with status: {e.code}"}
    except urllib.error.URLError as e:
        return {"status": "error", "message": f"Network transport pipeline error: {e.reason}"}
    except Exception as e:
        return {"status": "error", "message": f"Unexpected runtime connection exception: {str(e)}"}

    # Initialize data tracking buckets
    available_patches = []
    
    # Extract package patch metadata blocks using native safe regex structures
    # Captures update links, package revision versions, and hardware system requirements
    pkg_blocks = re.findall(r'href=["\'](http[s]?://[^"\']+\.pkg)["\'].*?>.*?(\d+\.\d+).*?</td>', html_content, re.DOTALL | re.IGNORECASE)
    
    for url, version in pkg_blocks:
        # Deduplicate and extract base filename identifiers
        filename = url.split('/')[-1]
        
        # Infer delta patch dependency flags or backport eligibility parameters
        is_delta = "delta" in url.lower() or "patch" in url.lower()
        
        available_patches.append({
            "version": version,
            "filename": filename,
            "download_url": url,
            "is_delta_patch": is_delta,
            "backport_eligible": float(version) > 1.00
        })

    # Sort manifest data layers from newest patch iteration revisions downward
    available_patches.sort(key=lambda x: x['version'], reverse=True)

    if not available_patches:
        return {
            "status": "success",
            "title_id": title_id,
            "message": "Title directory found but no downloadable delta retail assets were indexed.",
            "patches": []
        }

    return {
        "status": "success",
        "title_id": title_id,
        "latest_version": available_patches[0]["version"],
        "patch_count": len(available_patches),
        "patches": available_patches
    }

if __name__ == "__main__":
    # Ensure system command boundary parameters exist before execution
    if len(sys.argv) < 2:
        error_payload = {
            "status": "error",
            "message": "Missing Target Argument Parameter. Usage: python3 scraper.py <TitleID>"
        }
        print(json.dumps(error_payload, indent=4))
        sys.exit(1)
        
    target_id = sys.argv[1]
    runtime_manifest = scrape_game_patches(target_id)
    
    # Hand off standard pure JSON output stream directly to standard system pipelines
    print(json.dumps(runtime_manifest, indent=4))
