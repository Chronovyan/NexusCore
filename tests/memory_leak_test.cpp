#include "gtest/gtest.h"
#include "Editor.h"
#include <vector>
#include <string>
#include <chrono>
#include <numeric>
#include <fstream>

#ifndef _WIN32
#include <sys/resource.h> // For getrusage on Unix
#else
#include <windows.h> // For Windows memory usage APIs
#include <psapi.h>    // For GetProcessMemoryInfo
#endif

// Utility class for memory leak detection
class MemoryLeakTest : public ::testing::Test {
protected:
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
    
    // Structure to hold memory samples
    struct MemorySample {
        size_t iteration;
        size_t memoryUsageKB;
        size_t itemCount; // Number of items in buffer, undo stack, etc.
        
        MemorySample(size_t iter, size_t mem, size_t items) 
            : iteration(iter), memoryUsageKB(mem), itemCount(items) {}
    };
    
    // Collect memory samples during iterations
    std::vector<MemorySample> collectMemorySamples(
        std::function<void(size_t)> operation,
        std::function<size_t()> getItemCount,
        size_t iterations) {
        
        std::vector<MemorySample> samples;
        
        // Sample initial memory
        size_t initialMemory = getMemoryUsageKB();
        size_t initialItems = getItemCount();
        samples.emplace_back(0, initialMemory, initialItems);
        
        // Perform operations and collect samples
        for (size_t i = 1; i <= iterations; ++i) {
            operation(i);
            
            if (i % 10 == 0 || i == iterations) {
                size_t memory = getMemoryUsageKB();
                size_t items = getItemCount();
                samples.emplace_back(i, memory, items);
            }
        }
        
        // Add a final sample after a small delay to catch any delayed allocations
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        size_t finalMemory = getMemoryUsageKB();
        size_t finalItems = getItemCount();
        samples.emplace_back(iterations + 1, finalMemory, finalItems);
        
        return samples;
    }
    
    // Analyze memory samples for leaks
    bool detectLeaks(const std::vector<MemorySample>& samples, double thresholdRatio = 1.5) {
        if (samples.size() < 2) {
            return false;
        }
        
        // First phase: increased memory with constant item count
        bool stabilitySeen = false;
        size_t stableItems = 0;
        size_t stableMemoryBaseline = 0;
        
        for (size_t i = 1; i < samples.size(); ++i) {
            const auto& current = samples[i];
            const auto& previous = samples[i-1];
            
            // If item count hasn't changed but memory increased significantly
            if (current.itemCount == previous.itemCount && 
                current.memoryUsageKB > previous.memoryUsageKB * thresholdRatio) {
                return true; // Potential leak detected
            }
            
            // Look for stable sequence (non-changing item count)
            if (i > 1 && current.itemCount == previous.itemCount) {
                if (!stabilitySeen) {
                    stabilitySeen = true;
                    stableItems = current.itemCount;
                    stableMemoryBaseline = current.memoryUsageKB;
                } else if (stableItems == current.itemCount && 
                          current.memoryUsageKB > stableMemoryBaseline * thresholdRatio) {
                    return true; // Memory growth during stable item count
                }
            } else {
                stabilitySeen = false;
            }
        }
        
        // Second phase: memory doesn't decrease after items decrease
        const auto& first = samples.front();
        const auto& last = samples.back();
        
        if (last.itemCount < first.itemCount * 0.5 && 
            last.memoryUsageKB > first.memoryUsageKB * 0.9) {
            return true; // Items decreased but memory didn't follow
        }
        
        // Check if final memory is higher than initial despite returning to the initial state
        if (last.itemCount <= first.itemCount && 
            last.memoryUsageKB > first.memoryUsageKB * thresholdRatio) {
            return true; // Memory didn't return to initial level
        }
        
        return false; // No leaks detected
    }
    
    // Output memory samples to a CSV file for analysis
    void writeMemorySamplesToCSV(
        const std::vector<MemorySample>& samples, 
        const std::string& filename) {
        
        std::ofstream outFile(filename);
        if (!outFile) {
            std::cerr << "Failed to create file: " << filename << std::endl;
            return;
        }
        
        outFile << "Iteration,MemoryUsage(KB),ItemCount" << std::endl;
        
        for (const auto& sample : samples) {
            outFile << sample.iteration << "," 
                    << sample.memoryUsageKB << "," 
                    << sample.itemCount << std::endl;
        }
        
        outFile.close();
    }
};

// Test for buffer operations
TEST_F(MemoryLeakTest, BufferMemoryTest) {
    Editor editor;
    
    auto operation = [&](size_t i) {
        // Add a line
        editor.addLine("Line " + std::to_string(i));
        
        // After every 20 iterations, remove 10 lines
        if (i % 20 == 0 && editor.getBuffer().lineCount() > 10) {
            for (int j = 0; j < 10; j++) {
                editor.deleteLine(0);
            }
        }
    };
    
    auto getItemCount = [&]() {
        return editor.getBuffer().lineCount();
    };
    
    auto samples = collectMemorySamples(operation, getItemCount, 100);
    writeMemorySamplesToCSV(samples, "reports/buffer_memory_test.csv");
    
    EXPECT_FALSE(detectLeaks(samples));
}

// Test for undo/redo operations
TEST_F(MemoryLeakTest, UndoRedoMemoryTest) {
    Editor editor;
    size_t undoOperations = 0;
    
    auto operation = [&](size_t i) {
        if (i % 3 == 0 && editor.canUndo()) {
            // Undo operation
            editor.undo();
            undoOperations++;
        } else if (i % 7 == 0 && editor.canRedo()) {
            // Redo operation
            editor.redo();
            undoOperations--;
        } else {
            // Add content to create undo states
            editor.typeText("Text for undo test " + std::to_string(i));
            editor.newLine();
        }
    };
    
    auto getItemCount = [&]() {
        // Approximate representation of undo stack size
        return editor.getBuffer().lineCount() - undoOperations;
    };
    
    auto samples = collectMemorySamples(operation, getItemCount, 200);
    writeMemorySamplesToCSV(samples, "reports/undo_redo_memory_test.csv");
    
    EXPECT_FALSE(detectLeaks(samples));
}

// Test for selection and clipboard operations
TEST_F(MemoryLeakTest, ClipboardMemoryTest) {
    Editor editor;
    size_t clipboardOperations = 0;
    
    // Set up initial content
    for (int i = 0; i < 50; i++) {
        editor.addLine("Line " + std::to_string(i) + " for clipboard test");
    }
    
    auto operation = [&](size_t i) {
        // Set random cursor position
        size_t line = i % editor.getBuffer().lineCount();
        
        // Calculate end line for selection, ensuring it's within bounds
        size_t calculated_endLine = (std::min)(line + 3, editor.getBuffer().lineCount() > 0 ? editor.getBuffer().lineCount() - 1 : 0);
        
        editor.setCursor(line, 0);
        
        // Create selection spanning multiple lines
        editor.setSelectionStart();
        editor.setCursor(calculated_endLine, 5); // Use the correctly calculated endLine
        editor.setSelectionEnd();
        
        if (i % 3 == 0) {
            // Copy operation
            editor.copySelectedText();
        } else if (i % 3 == 1) {
            // Cut operation
            editor.cutSelectedText();
            clipboardOperations++;
        } else {
            // Paste operation
            editor.pasteText();
        }
    };
    
    auto getItemCount = [&]() {
        return editor.getBuffer().lineCount() + clipboardOperations;
    };
    
    auto samples = collectMemorySamples(operation, getItemCount, 100);
    writeMemorySamplesToCSV(samples, "reports/clipboard_memory_test.csv");
    
    EXPECT_FALSE(detectLeaks(samples));
}

// Test for multiple operations over a long period
TEST_F(MemoryLeakTest, LongTermMemoryTest) {
    Editor editor;
    
    // Initialize with significant content
    for (int i = 0; i < 20; i++) {
        editor.addLine("Initial line " + std::to_string(i));
    }
    
    auto operation = [&](size_t i) {
        int opType = i % 5;
        
        switch (opType) {
            case 0: // Add text
                editor.typeText("Added text " + std::to_string(i));
                editor.newLine();
                break;
                
            case 1: // Delete text
                if (editor.getBuffer().lineCount() > 1) {
                    size_t line = i % editor.getBuffer().lineCount();
                    editor.deleteLine(line);
                }
                break;
                
            case 2: // Undo/redo
                if (i % 2 == 0 && editor.canUndo()) {
                    editor.undo();
                } else if (editor.canRedo()) {
                    editor.redo();
                }
                break;
                
            case 3: // Selection and clipboard
                if (editor.getBuffer().lineCount() > 1) {
                    size_t line = i % editor.getBuffer().lineCount();
                    editor.setCursor(line, 0);
                    editor.setSelectionStart();
                    editor.moveCursorRight();
                    editor.moveCursorRight();
                    editor.setSelectionEnd();
                    
                    if (i % 2 == 0) {
                        editor.copySelectedText();
                    } else {
                        editor.cutSelectedText();
                    }
                }
                break;
                
            case 4: // Paste
                editor.pasteText();
                break;
        }
    };
    
    auto getItemCount = [&]() {
        return editor.getBuffer().lineCount();
    };
    
    // Run a longer test to catch gradual leaks
    auto samples = collectMemorySamples(operation, getItemCount, 500);
    writeMemorySamplesToCSV(samples, "reports/long_term_memory_test.csv");
    
    EXPECT_FALSE(detectLeaks(samples));
}

// Test for syntax highlighting related memory issues
TEST_F(MemoryLeakTest, SyntaxHighlightingMemoryTest) {
    Editor editor;
    
    // Set a C++ file extension to get the C++ highlighter
    editor.setFilename("test.cpp");
    editor.enableSyntaxHighlighting(true);
    
    // Initialize with C++-like content to trigger highlighting
    for (int i = 0; i < 30; i++) {
        std::string line;
        
        if (i % 5 == 0) {
            line = "// Comment line " + std::to_string(i);
        } else if (i % 5 == 1) {
            line = "int variable_" + std::to_string(i) + " = " + std::to_string(i * 10) + ";";
        } else if (i % 5 == 2) {
            line = "std::string text_" + std::to_string(i) + " = \"Sample text " + std::to_string(i) + "\";";
        } else if (i % 5 == 3) {
            line = "if (condition_" + std::to_string(i) + ") {";
        } else {
            line = "}";
        }
        
        editor.addLine(line);
    }
    
    auto operation = [&](size_t i) {
        // Move cursor, forcing highlight recalculation
        size_t line = i % editor.getBuffer().lineCount();
        editor.setCursor(line, 0);
        
        // Get highlighting styles to force calculation
        editor.getHighlightingStyles();
        
        if (i % 10 == 0) {
            // Add new highlighted content
            std::string line;
            if (i % 2 == 0) {
                line = "int new_var_" + std::to_string(i) + " = " + std::to_string(i) + ";";
            } else {
                line = "// New comment " + std::to_string(i);
            }
            editor.addLine(line);
        }
        
        // Invalidate highlighting cache every few operations
        if (i % 5 == 0) {
            editor.invalidateHighlightingCache();
        }
    };
    
    auto getItemCount = [&]() {
        return editor.getBuffer().lineCount();
    };
    
    auto samples = collectMemorySamples(operation, getItemCount, 200);
    writeMemorySamplesToCSV(samples, "reports/syntax_highlighting_memory_test.csv");
    
    EXPECT_FALSE(detectLeaks(samples));
} 