#!/usr/bin/env python3
"""
Extract performance metrics from test output and generate CSV baseline file.
"""

import csv
import re
import sys
from datetime import datetime
import subprocess
import platform
import os

def get_system_info():
    """Get system information for baseline."""
    # Get CPU info
    if platform.system() == "Windows":
        cpu_info = subprocess.check_output("wmic cpu get name", shell=True).decode()
        cpu_model = cpu_info.split('\n')[1].strip()
    else:  # Linux/macOS
        cpu_info = subprocess.check_output("cat /proc/cpuinfo | grep 'model name' | uniq", shell=True).decode()
        cpu_model = cpu_info.split(':')[1].strip()
    
    # Get RAM info
    if platform.system() == "Windows":
        ram_info = subprocess.check_output("wmic computersystem get totalphysicalmemory", shell=True).decode()
        ram_bytes = int(ram_info.split('\n')[1].strip())
        ram_gb = f"{ram_bytes // (1024**3)}GB"
    else:  # Linux/macOS
        ram_info = subprocess.check_output("free -b", shell=True).decode()
        ram_bytes = int(ram_info.split('\n')[1].split()[1])
        ram_gb = f"{ram_bytes // (1024**3)}GB"
    
    return cpu_model, ram_gb

def get_git_commit_hash():
    """Get current git commit hash."""
    try:
        return subprocess.check_output("git rev-parse HEAD", shell=True).decode().strip()
    except:
        return "TBD"

def extract_metrics(input_file, output_file, build_config):
    """Extract metrics from test output and write to CSV."""
    metrics = {
        'Date': datetime.now().strftime('%Y-%m-%d'),
        'CommitHash': get_git_commit_hash(),
        'CPUModel': get_system_info()[0],
        'RAM': get_system_info()[1],
        'OS': f"{platform.system()} {platform.release()}",
        'BuildConfig': build_config
    }
    
    # Read test output
    with open(input_file, 'r') as f:
        content = f.read()
    
    # Extract metrics using regex patterns
    patterns = {
        'FileOpenTime': r'File open time: (\d+)ms',
        'FileSaveTime': r'File save time: (\d+)ms',
        'MemoryUsage': r'Memory usage: (\d+)MB',
        'TextInsertionTime': r'Text insertion time: (\d+)ms',
        'NavigationTime': r'Navigation time: (\d+)ms',
        'ScrollingTime': r'Scrolling time: (\d+)ms',
        'SearchReplaceTime': r'Search/Replace time: (\d+)ms'
    }
    
    for metric, pattern in patterns.items():
        match = re.search(pattern, content)
        metrics[metric] = match.group(1) if match else 'N/A'
    
    # Write to CSV
    fieldnames = ['Date', 'CommitHash', 'CPUModel', 'RAM', 'OS', 'BuildConfig',
                 'FileOpenTime', 'FileSaveTime', 'MemoryUsage', 'TextInsertionTime',
                 'NavigationTime', 'ScrollingTime', 'SearchReplaceTime']
    
    file_exists = os.path.isfile(output_file)
    
    with open(output_file, 'a', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        if not file_exists:
            writer.writeheader()
        writer.writerow(metrics)

def main():
    if len(sys.argv) != 3:
        print("Usage: python extract_performance_metrics.py <input_file> <build_config>")
        sys.exit(1)
    
    input_file = sys.argv[1]
    build_config = sys.argv[2]
    output_file = 'large_file_baselines.csv'
    
    extract_metrics(input_file, output_file, build_config)
    print(f"Metrics extracted and written to {output_file}")

if __name__ == "__main__":
    main() 