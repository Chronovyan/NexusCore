#!/usr/bin/env python3
"""
Generate performance trend visualizations from baseline data.
"""

import csv
import sys
import matplotlib.pyplot as plt
import pandas as pd
from datetime import datetime
import os

def load_baselines(csv_file):
    """Load baseline data from CSV file."""
    return pd.read_csv(csv_file)

def plot_metric_trend(df, metric, output_file):
    """Plot trend for a specific metric."""
    plt.figure(figsize=(12, 6))
    
    # Convert date strings to datetime objects
    dates = pd.to_datetime(df['Date'])
    
    # Plot the metric
    plt.plot(dates, df[metric], marker='o', linestyle='-', linewidth=2)
    
    # Customize the plot
    plt.title(f'{metric} Trend Over Time')
    plt.xlabel('Date')
    plt.ylabel(metric)
    plt.grid(True, linestyle='--', alpha=0.7)
    
    # Rotate x-axis labels for better readability
    plt.xticks(rotation=45)
    
    # Adjust layout to prevent label cutoff
    plt.tight_layout()
    
    # Save the plot
    plt.savefig(output_file)
    plt.close()

def generate_trend_plots(baseline_file):
    """Generate trend plots for all metrics."""
    # Load data
    df = load_baselines(baseline_file)
    
    # Create output directory if it doesn't exist
    os.makedirs('benchmarks', exist_ok=True)
    
    # Metrics to plot
    metrics = ['FileOpenTime', 'FileSaveTime', 'MemoryUsage', 
              'TextInsertionTime', 'NavigationTime', 'ScrollingTime', 
              'SearchReplaceTime']
    
    # Generate plots for each metric
    for metric in metrics:
        output_file = f'benchmarks/performance_trends_{metric}.png'
        plot_metric_trend(df, metric, output_file)
        print(f"Generated trend plot for {metric}")

def main():
    if len(sys.argv) != 2:
        print("Usage: python visualize_trends.py <baseline_file>")
        sys.exit(1)
    
    baseline_file = sys.argv[1]
    generate_trend_plots(baseline_file)

if __name__ == "__main__":
    main() 