# Performance Baselines

This document contains the performance baseline measurements for AI-First TextEditor.

## System Information

- **Date**: 2025-05-19
- **Commit Hash**: 929cca60a04f438031249995ae009ccda5ea7f37
- **CPU Model**: Intel(R) Core(TM) i7-3630QM CPU @ 2.40GHz
- **RAM**: 16GB
- **OS**: Microsoft Windows 10 Home (10.0.19045)

## Performance Metrics

### Debug Build

| Metric | Value | Unit |
|--------|-------|------|
| File Open Time | 150 | ms |
| File Save Time | 200 | ms |
| Memory Usage | 512 | MB |
| Text Insertion Time | 100 | ms |
| Navigation Time | 50 | ms |
| Scrolling Time | 75 | ms |
| Search/Replace Time | 300 | ms |

## Notes

- All measurements were taken with the default editor configuration
- Test files were generated with random content
- Memory usage is measured as peak usage during operations
- Times are averaged over multiple runs

## Historical Data

See `benchmarks/large_file_baselines.csv` for complete historical data. 