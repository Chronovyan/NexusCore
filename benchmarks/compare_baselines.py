#!/usr/bin/env python3
"""
Compare performance baselines and generate comparison report.
"""

import csv
import sys
from datetime import datetime
import os

def load_baselines(csv_file):
    """Load baseline data from CSV file."""
    baselines = []
    with open(csv_file, 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            baselines.append(row)
    return baselines

def calculate_percentage_change(old, new):
    """Calculate percentage change between two values."""
    try:
        old_val = float(old)
        new_val = float(new)
        if old_val == 0:
            return "N/A"
        return ((new_val - old_val) / old_val) * 100
    except (ValueError, TypeError):
        return "N/A"

def compare_baselines(baselines):
    """Compare baselines and generate comparison report."""
    if len(baselines) < 2:
        print("Need at least 2 baselines for comparison")
        return
    
    # Sort by date
    baselines.sort(key=lambda x: x['Date'])
    
    # Get latest two baselines
    latest = baselines[-1]
    previous = baselines[-2]
    
    # Generate comparison
    comparison = {
        'Date': latest['Date'],
        'PreviousDate': previous['Date'],
        'CommitHash': latest['CommitHash'],
        'PreviousCommitHash': previous['CommitHash'],
        'BuildConfig': latest['BuildConfig'],
        'PreviousBuildConfig': previous['BuildConfig']
    }
    
    # Compare metrics
    metrics = ['FileOpenTime', 'FileSaveTime', 'MemoryUsage', 
              'TextInsertionTime', 'NavigationTime', 'ScrollingTime', 
              'SearchReplaceTime']
    
    for metric in metrics:
        comparison[f"{metric}_Change"] = calculate_percentage_change(
            previous[metric], latest[metric])
    
    return comparison

def generate_markdown_report(comparison):
    """Generate markdown report from comparison data."""
    report = f"""# Performance Comparison Report

## Overview
- **Date**: {comparison['Date']}
- **Previous Date**: {comparison['PreviousDate']}
- **Commit**: {comparison['CommitHash']}
- **Previous Commit**: {comparison['PreviousCommitHash']}
- **Build Config**: {comparison['BuildConfig']}
- **Previous Build Config**: {comparison['PreviousBuildConfig']}

## Performance Changes

| Metric | Change |
|--------|--------|
"""
    
    metrics = ['FileOpenTime', 'FileSaveTime', 'MemoryUsage', 
              'TextInsertionTime', 'NavigationTime', 'ScrollingTime', 
              'SearchReplaceTime']
    
    for metric in metrics:
        change = comparison[f"{metric}_Change"]
        if change != "N/A":
            change = f"{change:+.2f}%"
        report += f"| {metric} | {change} |\n"
    
    return report

def main():
    if len(sys.argv) != 2:
        print("Usage: python compare_baselines.py <baseline_file>")
        sys.exit(1)
    
    baseline_file = sys.argv[1]
    baselines = load_baselines(baseline_file)
    comparison = compare_baselines(baselines)
    
    if comparison:
        report = generate_markdown_report(comparison)
        output_file = 'docs/performance_comparisons.md'
        os.makedirs(os.path.dirname(output_file), exist_ok=True)
        
        with open(output_file, 'w') as f:
            f.write(report)
        print(f"Comparison report written to {output_file}")

if __name__ == "__main__":
    main() 