#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <random>
#include <fstream>
#include "../src/Editor.h"
#include <map>

#ifndef _WIN32
#include <sys/resource.h> // For getrusage on Unix
#else
#include <windows.h> // For Windows memory usage APIs
#include <psapi.h>    // For GetProcessMemoryInfo
#endif

// Generate a large text file with given number of lines
void generateLargeFile(const std::string& filename, size_t lineCount, size_t avgLineLength) {
    std::ofstream outFile(filename);
    if (!outFile) {
        std::cerr << "Failed to create file: " << filename << std::endl;
        return;
    }
    
    // Random number generators
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> lineLenDist(avgLineLength / 2, avgLineLength * 3 / 2);
    std::uniform_int_distribution<> charDist(32, 126); // ASCII printable characters
    
    for (size_t i = 0; i < lineCount; ++i) {
        size_t lineLen = lineLenDist(gen);
        for (size_t j = 0; j < lineLen; ++j) {
            outFile << static_cast<char>(charDist(gen));
        }
        outFile << '\n';
    }
    
    outFile.close();
    std::cout << "Generated file " << filename << " with " << lineCount << " lines" << std::endl;
}

// Get current process memory usage in KB
size_t getMemoryUsageKB() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return pmc.WorkingSetSize / 1024;
    }
    return 0;
#else
    struct rusage rusage;
    getrusage(RUSAGE_SELF, &rusage);
    return rusage.ru_maxrss;
#endif
}

// Print memory usage delta in a formatted way
void printMemoryDelta(size_t before, size_t after) {
    long delta = static_cast<long>(after) - static_cast<long>(before);
    std::cout << "  Memory usage: " << before << " KB -> " << after << " KB";
    if (delta > 0) {
        std::cout << " (+" << delta << " KB)";
    } else if (delta < 0) {
        std::cout << " (" << delta << " KB)";
    } else {
        std::cout << " (no change)";
    }
    std::cout << std::endl;
}

// Benchmark file loading
void benchmarkFileLoading(const std::string& filename) {
    Editor editor;
    
    // Measure time to load file
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Load file line by line
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        editor.addLine(line);
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    std::cout << "File loading benchmark:" << std::endl;
    std::cout << "  File size: " << editor.getBuffer().lineCount() << " lines" << std::endl;
    std::cout << "  Loading time: " << duration.count() << "ms" << std::endl;
}

// Benchmark cursor movement operations
void benchmarkCursorOperations(Editor& editor, size_t iterations) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Random cursor movements
    for (size_t i = 0; i < iterations; ++i) {
        // Move down through file
        for (size_t j = 0; j < 100 && j < editor.getBuffer().lineCount(); ++j) {
            editor.moveCursorDown();
        }
        
        // Move right on current line
        for (size_t j = 0; j < 20; ++j) {
            editor.moveCursorRight();
        }
        
        // Move up
        for (size_t j = 0; j < 50 && j < editor.getBuffer().lineCount(); ++j) {
            editor.moveCursorUp();
        }
        
        // Move left
        for (size_t j = 0; j < 10; ++j) {
            editor.moveCursorLeft();
        }
        
        // Jump to start/end
        editor.moveCursorToLineStart();
        editor.moveCursorToLineEnd();
        
        // Random position
        size_t randomLine = i % editor.getBuffer().lineCount();
        editor.setCursor(randomLine, 0);
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    std::cout << "Cursor operations benchmark:" << std::endl;
    std::cout << "  Iterations: " << iterations << std::endl;
    std::cout << "  Execution time: " << duration.count() << "ms" << std::endl;
    std::cout << "  Average time per iteration: " << static_cast<double>(duration.count()) / iterations << "ms" << std::endl;
}

// Benchmark text editing operations
void benchmarkEditingOperations(Editor& editor, size_t iterations) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Get buffer size for random operations
    size_t lineCount = editor.getBuffer().lineCount();
    
    // Random number generators
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> lineDist(0, lineCount > 0 ? lineCount - 1 : 0);
    std::uniform_int_distribution<> opDist(0, 4); // 5 different operations
    
    for (size_t i = 0; i < iterations; ++i) {
        // Select a random line if possible
        if (lineCount > 0) {
            size_t line = lineDist(gen);
            editor.setCursor(line, 0);
        }
        
        // Perform a random operation
        int operation = opDist(gen);
        switch (operation) {
            case 0: // Insert text
                editor.typeText("Benchmark text insertion");
                break;
            case 1: // Delete text
                editor.moveCursorToLineEnd();
                for (int j = 0; j < 5; ++j) {
                    editor.backspace();
                }
                break;
            case 2: // New line
                editor.newLine();
                break;
            case 3: // Replace line
                editor.replaceLine(editor.getCursorLine(), "Replaced line during benchmark");
                break;
            case 4: // Delete line
                if (lineCount > 1) { // Ensure we don't delete the last line
                    editor.deleteLine(editor.getCursorLine());
                    lineCount--; // Update lineCount
                }
                break;
        }
        
        // Update lineCount after additions
        lineCount = editor.getBuffer().lineCount();
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    std::cout << "Editing operations benchmark:" << std::endl;
    std::cout << "  Iterations: " << iterations << std::endl;
    std::cout << "  Execution time: " << duration.count() << "ms" << std::endl;
    std::cout << "  Average time per operation: " << static_cast<double>(duration.count()) / iterations << "ms" << std::endl;
}

// Benchmark undo/redo operations
void benchmarkUndoRedo(Editor& editor, size_t iterations) {
    // First perform a series of operations
    std::cout << "Setting up undo/redo benchmark with " << iterations << " operations..." << std::endl;
    
    // Add some text operations to create undo history
    for (size_t i = 0; i < iterations; ++i) {
        editor.typeText("Text for undo benchmark " + std::to_string(i));
        editor.newLine();
    }
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Perform undo operations
    std::cout << "Performing " << iterations << " undo operations..." << std::endl;
    for (size_t i = 0; i < iterations; ++i) {
        editor.undo();
    }
    
    // Perform redo operations
    std::cout << "Performing " << iterations << " redo operations..." << std::endl;
    for (size_t i = 0; i < iterations; ++i) {
        editor.redo();
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    std::cout << "Undo/Redo operations benchmark:" << std::endl;
    std::cout << "  Total operations: " << (iterations * 2) << " (" << iterations << " undo + " << iterations << " redo)" << std::endl;
    std::cout << "  Execution time: " << duration.count() << "ms" << std::endl;
    std::cout << "  Average time per operation: " << static_cast<double>(duration.count()) / (iterations * 2) << "ms" << std::endl;
}

// Benchmark search operations
void benchmarkSearch(Editor& editor, const std::vector<std::string>& searchTerms) {
    std::cout << "Starting search benchmark..." << std::endl;
    
    // Check if the buffer is properly initialized
    if (editor.getBuffer().isEmpty()) {
        std::cout << "Error: Cannot perform search on empty buffer" << std::endl;
        return;
    }
    
    std::cout << "Buffer has " << editor.getBuffer().lineCount() << " lines for search" << std::endl;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    size_t totalSearches = 0;
    size_t matchesFound = 0;
    size_t failedSearches = 0;
    std::map<std::string, size_t> searchStats;
    
    try {
        for (const auto& term : searchTerms) {
            std::cout << "Searching for term: \"" << term << "\"" << std::endl;
            
            // Store original cursor position
            size_t originalLine = editor.getCursorLine();
            size_t originalCol = editor.getCursorCol();
            
            // Reset cursor to start of file for consistent benchmarking
            editor.setCursor(0, 0);
            
            // Initial search
            std::cout << "  Initial search..." << std::flush;
            bool found = false;
            
            try {
                auto termStartTime = std::chrono::high_resolution_clock::now();
                found = editor.search(term);
                auto termEndTime = std::chrono::high_resolution_clock::now();
                auto termDuration = std::chrono::duration_cast<std::chrono::milliseconds>(termEndTime - termStartTime);
                
                totalSearches++;
                searchStats[term] = termDuration.count();
                
                std::cout << (found ? " found match." : " no match found.") << 
                    " (" << termDuration.count() << "ms)" << std::endl;
            } 
            catch (const std::exception& e) {
                std::cout << " EXCEPTION: " << e.what() << std::endl;
                failedSearches++;
                continue;
            }
            catch (...) {
                std::cout << " UNKNOWN EXCEPTION" << std::endl;
                failedSearches++;
                continue;
            }
            
            if (found) {
                matchesFound++;
                
                // Search for additional occurrences
                std::cout << "  Searching for more occurrences..." << std::endl;
                int searchCount = 1;
                
                try {
                    while (searchCount < 100 && editor.searchNext()) {  // Limit to 100 occurrences to avoid infinite loops
                        totalSearches++;
                        matchesFound++;
                        searchCount++;
                        if (searchCount % 5 == 0) {
                            std::cout << "    Found " << searchCount << " matches so far..." << std::endl;
                        }
                    }
                } 
                catch (const std::exception& e) {
                    std::cout << "  EXCEPTION during searchNext: " << e.what() << std::endl;
                    failedSearches++;
                }
                catch (...) {
                    std::cout << "  UNKNOWN EXCEPTION during searchNext" << std::endl;
                    failedSearches++;
                }
            }
            
            std::cout << "  Found total " << matchesFound << " occurrences of \"" << term << "\"" << std::endl;
            matchesFound = 0; // Reset for next term
            
            // Restore original cursor position
            try {
                editor.setCursor(originalLine, originalCol);
            } catch (...) {
                // If restoring cursor fails, just set to start of file
                editor.setCursor(0, 0);
            }
        }
    }
    catch (const std::exception& e) {
        std::cout << "CRITICAL EXCEPTION in search benchmark: " << e.what() << std::endl;
    }
    catch (...) {
        std::cout << "UNKNOWN CRITICAL EXCEPTION in search benchmark" << std::endl;
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    std::cout << "Search operations benchmark summary:" << std::endl;
    std::cout << "  Search terms: " << searchTerms.size() << std::endl;
    std::cout << "  Total searches: " << totalSearches << std::endl;
    std::cout << "  Matches found: " << matchesFound << std::endl;
    std::cout << "  Failed searches: " << failedSearches << std::endl;
    std::cout << "  Execution time: " << duration.count() << "ms" << std::endl;
    if (totalSearches > 0) {
        std::cout << "  Average time per search: " << static_cast<double>(duration.count()) / totalSearches << "ms" << std::endl;
    }
    
    if (!searchStats.empty()) {
        std::cout << "  Per-term initial search times:" << std::endl;
        for (const auto& [term, time] : searchStats) {
            std::cout << "    \"" << term << "\": " << time << "ms" << std::endl;
        }
    }
}

// Benchmark syntax highlighting
void benchmarkSyntaxHighlighting(Editor& editor) {
    std::cout << "Starting syntax highlighting benchmark..." << std::endl;
    
    // Make sure syntax highlighting is enabled
    editor.enableSyntaxHighlighting(true);
    
    // Set a C++ file extension to get the C++ highlighter
    editor.setFilename("benchmark.cpp");
    
    // Check if we have a highlighter
    SyntaxHighlighter* highlighter = editor.getCurrentHighlighter();
    if (!highlighter) {
        std::cout << "Error: Failed to get syntax highlighter for C++ files." << std::endl;
        return;
    }
    std::cout << "Using " << highlighter->getLanguageName() << " syntax highlighter." << std::endl;
    
    std::cout << "Timing highlight calculation for " << editor.getBuffer().lineCount() << " lines..." << std::endl;
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Get highlighting styles - this forces the calculation of all styles
    auto styles = editor.getHighlightingStyles();
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    // Count total style elements for statistics
    size_t totalStyles = 0;
    for (const auto& lineStyles : styles) {
        totalStyles += lineStyles.size();
    }
    
    std::cout << "Syntax highlighting benchmark results:" << std::endl;
    std::cout << "  Buffer size: " << editor.getBuffer().lineCount() << " lines" << std::endl;
    std::cout << "  Total style elements: " << totalStyles << std::endl;
    std::cout << "  Execution time: " << duration.count() << "ms" << std::endl;
    std::cout << "  Average time per line: " << 
        (editor.getBuffer().lineCount() > 0 ? 
         static_cast<double>(duration.count()) / editor.getBuffer().lineCount() : 0) << 
        "ms" << std::endl;
}

// Benchmark long-running stability
void benchmarkLongRunningStability(Editor& editor, size_t iterations) {
    // Create a log file to track memory usage over time
    std::ofstream memoryLog("memory_usage_log.csv");
    memoryLog << "Iteration,Operation,MemoryBefore(KB),MemoryAfter(KB),Delta(KB),Duration(ms)" << std::endl;
    
    std::cout << "Long-running stability benchmark:" << std::endl;
    std::cout << "  Iterations: " << iterations << std::endl;
    
    size_t initialMemory = getMemoryUsageKB();
    std::cout << "  Initial memory usage: " << initialMemory << " KB" << std::endl;
    
    size_t before, after;
    std::chrono::milliseconds duration;
    auto startTime = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < iterations; ++i) {
        if (i % 100 == 0) {
            std::cout << "  Completed " << i << " iterations..." << std::endl;
        }
        
        // 1. Typing test
        before = getMemoryUsageKB();
        auto opStart = std::chrono::high_resolution_clock::now();
        editor.typeText("Line of text for iteration " + std::to_string(i));
        editor.newLine();
        auto opEnd = std::chrono::high_resolution_clock::now();
        after = getMemoryUsageKB();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(opEnd - opStart);
        memoryLog << i << ",TypeText," << before << "," << after << "," 
                  << (static_cast<long>(after) - static_cast<long>(before)) << ","
                  << duration.count() << std::endl;
        
        // 2. Delete operations
        before = getMemoryUsageKB();
        opStart = std::chrono::high_resolution_clock::now();
        for (int j = 0; j < 5; ++j) {
            editor.backspace();
        }
        opEnd = std::chrono::high_resolution_clock::now();
        after = getMemoryUsageKB();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(opEnd - opStart);
        memoryLog << i << ",Backspace," << before << "," << after << "," 
                  << (static_cast<long>(after) - static_cast<long>(before)) << ","
                  << duration.count() << std::endl;
        
        // 3. Undo/Redo test
        before = getMemoryUsageKB();
        opStart = std::chrono::high_resolution_clock::now();
        editor.undo();
        editor.undo();
        editor.redo();
        opEnd = std::chrono::high_resolution_clock::now();
        after = getMemoryUsageKB();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(opEnd - opStart);
        memoryLog << i << ",UndoRedo," << before << "," << after << "," 
                  << (static_cast<long>(after) - static_cast<long>(before)) << ","
                  << duration.count() << std::endl;
        
        // 4. Cursor movement and selection
        before = getMemoryUsageKB();
        opStart = std::chrono::high_resolution_clock::now();
        editor.setCursor(i % editor.getBuffer().lineCount(), 0);
        editor.setSelectionStart();
        editor.moveCursorRight();
        editor.moveCursorRight();
        editor.moveCursorRight();
        editor.setSelectionEnd();
        opEnd = std::chrono::high_resolution_clock::now();
        after = getMemoryUsageKB();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(opEnd - opStart);
        memoryLog << i << ",Selection," << before << "," << after << "," 
                  << (static_cast<long>(after) - static_cast<long>(before)) << ","
                  << duration.count() << std::endl;
        
        // 5. Syntax highlighting operation (if enabled)
        if (editor.isSyntaxHighlightingEnabled() && editor.getCurrentHighlighter() != nullptr) {
            before = getMemoryUsageKB();
            opStart = std::chrono::high_resolution_clock::now();
            editor.getHighlightingStyles(); // Force highlight calculation
            opEnd = std::chrono::high_resolution_clock::now();
            after = getMemoryUsageKB();
            duration = std::chrono::duration_cast<std::chrono::milliseconds>(opEnd - opStart);
            memoryLog << i << ",Highlighting," << before << "," << after << "," 
                      << (static_cast<long>(after) - static_cast<long>(before)) << ","
                      << duration.count() << std::endl;
        }
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto totalDuration = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime);
    
    size_t finalMemory = getMemoryUsageKB();
    std::cout << "  Final memory usage: " << finalMemory << " KB" << std::endl;
    printMemoryDelta(initialMemory, finalMemory);
    std::cout << "  Total run time: " << totalDuration.count() << " seconds" << std::endl;
    std::cout << "  Memory usage log written to memory_usage_log.csv" << std::endl;
    
    memoryLog.close();
}

// Modified functions to include memory usage tracking
void benchmarkFileLoading_withMemory(const std::string& filename) {
    Editor editor;
    
    // Measure memory before loading
    size_t memoryBefore = getMemoryUsageKB();
    std::cout << "  Memory before loading: " << memoryBefore << " KB" << std::endl;
    
    // Measure time to load file
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Load file line by line
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        editor.addLine(line);
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    // Measure memory after loading
    size_t memoryAfter = getMemoryUsageKB();
    
    std::cout << "File loading benchmark (with memory):" << std::endl;
    std::cout << "  File size: " << editor.getBuffer().lineCount() << " lines" << std::endl;
    std::cout << "  Loading time: " << duration.count() << "ms" << std::endl;
    printMemoryDelta(memoryBefore, memoryAfter);
}

// Stress test with multiple large edit operations
void stressTestLargeEdits(Editor& editor, size_t operationCount) {
    size_t memoryBefore = getMemoryUsageKB();
    std::cout << "Large edits stress test:" << std::endl;
    std::cout << "  Initial lines: " << editor.getBuffer().lineCount() << std::endl;
    std::cout << "  Operations: " << operationCount << std::endl;
    std::cout << "  Memory before: " << memoryBefore << " KB" << std::endl;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Random operations on a large scale
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> opDist(0, 4); // 5 operations
    
    for (size_t i = 0; i < operationCount; ++i) {
        if (i % 100 == 0) {
            std::cout << "  Operation " << i << "..." << std::endl;
        }
        
        int operation = opDist(gen);
        switch (operation) {
            case 0: {
                // Insert a large block of text
                std::string largeText;
                for (int j = 0; j < 100; ++j) {
                    largeText += "Large text insertion line " + std::to_string(j) + " of stress test.\n";
                }
                editor.typeText(largeText);
                break;
            }
            case 1: {
                // Delete multiple lines
                size_t lineCount = editor.getBuffer().lineCount();
                if (lineCount > 10) {
                    size_t lineToDelete = lineCount / 2; // Delete from middle
                    for (int j = 0; j < 10 && lineToDelete < lineCount; ++j, ++lineToDelete) {
                        editor.deleteLine(lineToDelete);
                        lineCount--;
                    }
                }
                break;
            }
            case 2: {
                // Multiple consecutive undo operations
                for (int j = 0; j < 5; ++j) {
                    if (editor.canUndo()) {
                        editor.undo();
                    }
                }
                break;
            }
            case 3: {
                // Multiple consecutive redo operations
                for (int j = 0; j < 5; ++j) {
                    if (editor.canRedo()) {
                        editor.redo();
                    }
                }
                break;
            }
            case 4: {
                // Replace lines with large content
                size_t lineCount = editor.getBuffer().lineCount();
                if (lineCount > 5) {
                    std::uniform_int_distribution<> lineDist(0, lineCount - 1);
                    for (int j = 0; j < 5; ++j) {
                        size_t lineIndex = lineDist(gen);
                        std::string replacementText = "Replacement text for line " + 
                                                     std::to_string(lineIndex) + 
                                                     " in stress test iteration " + 
                                                     std::to_string(i) + 
                                                     " with extra padding to make it long.";
                        editor.replaceLine(lineIndex, replacementText);
                    }
                }
                break;
            }
        }
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    size_t memoryAfter = getMemoryUsageKB();
    std::cout << "  Final lines: " << editor.getBuffer().lineCount() << std::endl;
    std::cout << "  Execution time: " << duration.count() << "ms" << std::endl;
    printMemoryDelta(memoryBefore, memoryAfter);
}

int main(int argc, char* argv[]) {
    const std::string testFilename = "benchmark_test_file.txt";
    size_t lineCount = 1000;  // Default size
    size_t avgLineLength = 80;
    size_t iterations = 100;  // Default iterations
    bool runSearchBenchmark = false;  // Disable search benchmark by default
    bool runSyntaxBenchmark = true;   // Enable syntax highlighting benchmark by default
    
    // Test control flags
    bool runCursorBenchmark = true;
    bool runEditingBenchmark = true;
    bool runUndoRedoBenchmark = true;
    bool customTestsSpecified = false;
    
    // Parse command line arguments if provided
    if (argc > 1) {
        lineCount = std::stoul(argv[1]);
    }
    if (argc > 2) {
        avgLineLength = std::stoul(argv[2]);
    }
    if (argc > 3) {
        iterations = std::stoul(argv[3]);
    }
    if (argc > 4) {
        // 4th argument controls if search benchmark runs
        std::string arg4(argv[4]);
        runSearchBenchmark = (arg4 == "search");
    }
    if (argc > 5) {
        // 5th argument controls syntax benchmark
        std::string arg5(argv[5]);
        if (arg5 == "0" || arg5 == "false" || arg5 == "off") {
            runSyntaxBenchmark = false;
        }
    }
    
    // Check for custom test selections (6th argument onwards)
    if (argc > 6) {
        // If custom tests are specified, disable all by default
        runCursorBenchmark = false;
        runEditingBenchmark = false;
        runUndoRedoBenchmark = false;
        customTestsSpecified = true;
        
        // Parse custom test selections
        for (int i = 6; i < argc; i++) {
            std::string arg(argv[i]);
            if (arg == "cursor") {
                runCursorBenchmark = true;
            } else if (arg == "edit") {
                runEditingBenchmark = true;
            } else if (arg == "undoredo") {
                runUndoRedoBenchmark = true;
            } else if (arg == "all") {
                runCursorBenchmark = true;
                runEditingBenchmark = true;
                runUndoRedoBenchmark = true;
                break;
            }
        }
    }
    
    std::cout << "=== Text Editor Performance Benchmark ===" << std::endl;
    std::cout << "Configuration:" << std::endl;
    std::cout << "  Line count: " << lineCount << std::endl;
    std::cout << "  Average line length: " << avgLineLength << std::endl;
    std::cout << "  Iterations for operations: " << iterations << std::endl;
    std::cout << "  Run search benchmark: " << (runSearchBenchmark ? "yes" : "no") << std::endl;
    std::cout << "  Run syntax benchmark: " << (runSyntaxBenchmark ? "yes" : "no") << std::endl;
    
    if (customTestsSpecified) {
        std::cout << "  Custom test selection:" << std::endl;
        std::cout << "    Cursor operations: " << (runCursorBenchmark ? "yes" : "no") << std::endl;
        std::cout << "    Editing operations: " << (runEditingBenchmark ? "yes" : "no") << std::endl;
        std::cout << "    Undo/Redo operations: " << (runUndoRedoBenchmark ? "yes" : "no") << std::endl;
    }
    
    std::cout << std::endl;
    
    try {
        // Generate test file
        generateLargeFile(testFilename, lineCount, avgLineLength);
        
        // Benchmark file loading
        benchmarkFileLoading(testFilename);
        
        // Load file for other benchmarks
        Editor editor;
        std::ifstream file(testFilename);
        if (!file) {
            std::cerr << "Failed to open file: " << testFilename << std::endl;
            return 1;
        }
        
        std::string line;
        while (std::getline(file, line)) {
            editor.addLine(line);
        }
        file.close();
        
        std::cout << "\nRunning benchmarks..." << std::endl;
        
        // Run cursor operations benchmark
        if (runCursorBenchmark) {
            try {
                std::cout << "\n=== CURSOR OPERATIONS BENCHMARK ===" << std::endl;
                benchmarkCursorOperations(editor, iterations);
            } catch (const std::exception& e) {
                std::cerr << "Exception in cursor benchmark: " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "Unknown exception in cursor benchmark" << std::endl;
            }
        } else {
            std::cout << "\nSkipping cursor operations benchmark" << std::endl;
        }
        
        // Run editing operations benchmark
        if (runEditingBenchmark) {
            try {
                std::cout << "\n=== EDITING OPERATIONS BENCHMARK ===" << std::endl;
                
                // Create a copy for editing benchmarks to avoid affecting other tests
                Editor editingEditor;
                std::ifstream editFile(testFilename);
                std::string editLine;
                while (std::getline(editFile, editLine)) {
                    editingEditor.addLine(editLine);
                }
                editFile.close();
                
                benchmarkEditingOperations(editingEditor, iterations / 10); // Fewer iterations for editing
            } catch (const std::exception& e) {
                std::cerr << "Exception in editing benchmark: " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "Unknown exception in editing benchmark" << std::endl;
            }
        } else {
            std::cout << "\nSkipping editing operations benchmark" << std::endl;
        }
        
        // Run undo/redo operations benchmark
        if (runUndoRedoBenchmark) {
            try {
                std::cout << "\n=== UNDO/REDO OPERATIONS BENCHMARK ===" << std::endl;
                
                // Fresh editor for undo/redo benchmark
                Editor undoRedoEditor;
                benchmarkUndoRedo(undoRedoEditor, iterations / 10); // Fewer iterations for undo/redo
            } catch (const std::exception& e) {
                std::cerr << "Exception in undo/redo benchmark: " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "Unknown exception in undo/redo benchmark" << std::endl;
            }
        } else {
            std::cout << "\nSkipping undo/redo operations benchmark" << std::endl;
        }
        
        // Run search benchmark
        if (runSearchBenchmark) {
            std::cout << "\n=== SEARCH OPERATIONS BENCHMARK ===" << std::endl;
            std::vector<std::string> searchTerms = {
                "benchmark", "text", "The", "performance", "editor", "random"
            };
            benchmarkSearch(editor, searchTerms);
        } else {
            std::cout << "\nSkipping search benchmark (disabled)" << std::endl;
        }
        
        // Run syntax highlighting benchmark
        if (runSyntaxBenchmark) {
            try {
                std::cout << "\n=== SYNTAX HIGHLIGHTING BENCHMARK ===" << std::endl;
                benchmarkSyntaxHighlighting(editor);
            } catch (const std::exception& e) {
                std::cerr << "Exception in syntax highlighting benchmark: " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "Unknown exception in syntax highlighting benchmark" << std::endl;
            }
        } else {
            std::cout << "\nSkipping syntax highlighting benchmark (disabled)" << std::endl;
        }
        
        // Run long-running stability benchmark
        benchmarkLongRunningStability(editor, iterations);
        
        // Run stress test with large edits
        stressTestLargeEdits(editor, iterations);
        
        std::cout << "\nBenchmarks complete!" << std::endl;
        
        // Clean up test file
        std::remove(testFilename.c_str());
        
    } catch (const std::exception& e) {
        std::cerr << "Critical exception in main benchmark routine: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown critical exception in main benchmark routine" << std::endl;
        return 1;
    }
    
    return 0;
} 