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

size_t getMemoryUsageKB() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return pmc.WorkingSetSize / 1024;
    }
    return 0;
#else
    struct rusage rusage_val; // Renamed to avoid conflict
    getrusage(RUSAGE_SELF, &rusage_val);
    return rusage_val.ru_maxrss;
#endif
}

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

void benchmarkFileLoading(const std::string& filename) {
    Editor editor;
    auto startTime = std::chrono::high_resolution_clock::now();
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return;
    }
    std::string line;
    while (std::getline(file, line)) {
        editor.getBuffer().addLine(line); // Use getBuffer() to add lines
    }
    file.close(); // Close the file
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    std::cout << "File loading benchmark:" << std::endl;
    std::cout << "  File size: " << editor.getBuffer().lineCount() << " lines" << std::endl;
    std::cout << "  Loading time: " << duration.count() << "ms" << std::endl;
}

void benchmarkCursorOperations(Editor& editor, size_t iterations) {
    auto startTime = std::chrono::high_resolution_clock::now();
    std::random_device rd;
    std::mt19937 gen(rd());
    if (editor.getBuffer().isEmpty() || editor.getBuffer().lineCount() == 0) {
        std::cout << "Cursor operations benchmark: Buffer is empty, cannot perform." << std::endl;
        return;
    }
    std::uniform_int_distribution<> lineDist(0, editor.getBuffer().lineCount() - 1);
    std::uniform_int_distribution<> colDist(0, 50); // Assuming lines can be up to 50 chars for random col

    for (size_t i = 0; i < iterations; ++i) {
        editor.setCursor(lineDist(gen), colDist(gen));
        editor.moveCursorDown();
        editor.moveCursorUp();
        editor.moveCursorLeft();
        editor.moveCursorRight();
        editor.moveCursorToLineStart();
        editor.moveCursorToLineEnd();
    }
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    std::cout << "Cursor operations benchmark:" << std::endl;
    std::cout << "  Iterations: " << iterations << std::endl;
    std::cout << "  Execution time: " << duration.count() << "ms" << std::endl;
    if (iterations > 0) {
      std::cout << "  Average time per iteration: " << static_cast<double>(duration.count()) / iterations << "ms" << std::endl;
    }
}

void benchmarkEditingOperations(Editor& editor, size_t iterations) {
    auto startTime = std::chrono::high_resolution_clock::now();
    std::random_device rd;
    std::mt19937 gen(rd());
    if (editor.getBuffer().isEmpty() || editor.getBuffer().lineCount() == 0) {
        editor.getBuffer().addLine("Initial line for editing benchmark.");
    }
    std::uniform_int_distribution<> opDist(0, 4);

    for (size_t i = 0; i < iterations; ++i) {
        std::uniform_int_distribution<> lineDist(0, editor.getBuffer().lineCount() - 1);
        size_t lineIdx = lineDist(gen);
        std::string currentLine = editor.getBuffer().getLine(lineIdx);
        std::uniform_int_distribution<> colDist(0, currentLine.length());
        editor.setCursor(lineIdx, colDist(gen));
        int operation = opDist(gen);
        switch (operation) {
            case 0: editor.typeText("Benchmark text"); break;
            case 1: if (!currentLine.empty()) editor.backspace(); break;
            case 2: editor.newLine(); break;
            case 3: editor.getBuffer().replaceLine(editor.getCursorLine(), "Replaced benchmark line"); break;
            case 4: if (editor.getBuffer().lineCount() > 1) editor.getBuffer().deleteLine(editor.getCursorLine()); break;
        }
    }
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    std::cout << "Editing operations benchmark:" << std::endl;
    std::cout << "  Iterations: " << iterations << std::endl;
    std::cout << "  Execution time: " << duration.count() << "ms" << std::endl;
    if (iterations > 0) {
        std::cout << "  Average time per operation: " << static_cast<double>(duration.count()) / iterations << "ms" << std::endl;
    }
}

void benchmarkUndoRedo(Editor& editor, size_t iterations) {
    if (iterations == 0) return;
    for (size_t i = 0; i < iterations; ++i) {
        editor.typeText("UndoRedo Line " + std::to_string(i));
        editor.newLine();
    }
    auto startTime = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < iterations * 2; ++i) editor.undo(); // Undo all new lines and texts
    for (size_t i = 0; i < iterations * 2; ++i) editor.redo(); // Redo all
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    std::cout << "Undo/Redo operations benchmark:" << std::endl;
    std::cout << "  Total operations: " << (iterations * 4) << std::endl;
    std::cout << "  Execution time: " << duration.count() << "ms" << std::endl;
    if (iterations > 0) {
        std::cout << "  Average time per operation: " << static_cast<double>(duration.count()) / (iterations * 4) << "ms" << std::endl;
    }
}

void benchmarkSearch(Editor& editor, const std::vector<std::string>& searchTerms) {
    if (editor.getBuffer().isEmpty() || editor.getBuffer().lineCount() == 0) {
        std::cout << "Search benchmark: Buffer is empty." << std::endl; return;
    }
    auto startTime = std::chrono::high_resolution_clock::now();
    size_t totalMatches = 0;
    for (const auto& term : searchTerms) {
        editor.setCursor(0,0); // Reset for each term
        while(editor.search(term, editor.getCursorLine(), editor.getCursorCol())) {
            totalMatches++;
            // Move cursor past match to find next: crude way, real searchNext would be better
            if (editor.getCursorLine() < editor.getBuffer().lineCount() -1 ) {
                editor.setCursor(editor.getCursorLine() + 1, 0);
            } else if (editor.getCursorCol() < editor.getBuffer().getLine(editor.getCursorLine()).length()) {
                 editor.setCursor(editor.getCursorLine(), editor.getCursorCol() + 1);
            } else {
                break; // No more place to search from
            }
        }
    }
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    std::cout << "Search operations benchmark:" << std::endl;
    std::cout << "  Search terms processed: " << searchTerms.size() << std::endl;
    std::cout << "  Total matches found: " << totalMatches << std::endl;
    std::cout << "  Execution time: " << duration.count() << "ms" << std::endl;
}

void benchmarkSyntaxHighlighting(Editor& editor) {
    editor.enableSyntaxHighlighting(true);
    editor.setFilename("benchmark.cpp"); // Ensure a highlighter is chosen
    if (!editor.getCurrentHighlighter()) {
        std::cout << "Syntax highlighting benchmark: No highlighter found for .cpp" << std::endl; return;
    }
    auto startTime = std::chrono::high_resolution_clock::now();
    editor.getHighlightingStyles(); // Force calculation
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    std::cout << "Syntax highlighting benchmark:" << std::endl;
    std::cout << "  Buffer lines: " << editor.getBuffer().lineCount() << std::endl;
    std::cout << "  Execution time: " << duration.count() << "ms" << std::endl;
    if (editor.getBuffer().lineCount() > 0) {
        std::cout << "  Average time per line: " << static_cast<double>(duration.count()) / editor.getBuffer().lineCount() << "ms" << std::endl;
    }
}

void benchmarkLongRunningStability(Editor& editor, size_t iterations) {
    std::ofstream memoryLog("memory_usage_log.csv");
    memoryLog << "Iteration,Operation,MemoryBefore(KB),MemoryAfter(KB),Delta(KB),Duration(ms)" << std::endl;
    size_t initialMemory = getMemoryUsageKB();
    auto totalStartTime = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < iterations; ++i) {
        size_t memBefore = getMemoryUsageKB();
        auto opStartTime = std::chrono::high_resolution_clock::now();
        std::string opName = "N/A";

        int opType = i % 5;
        if (opType == 0) { 
            opName = "TypeTextNewLine";
            editor.typeText("Iter " + std::to_string(i)); editor.newLine(); 
        }
        else if (opType == 1) { opName = "Backspace"; editor.backspace(); }
        else if (opType == 2) { opName = "UndoRedo"; editor.undo(); editor.redo(); }
        else if (opType == 3) { 
            opName = "Select";
            if (editor.getBuffer().lineCount() > 0) {
                editor.setCursor(i % editor.getBuffer().lineCount(), 0); 
                editor.setSelectionStart(); 
                editor.moveCursorRight(); 
                editor.setSelectionEnd();
            }
        }
        else if (opType == 4 && editor.isSyntaxHighlightingEnabled() && editor.getCurrentHighlighter()) {
            opName = "Highlight";
            editor.getHighlightingStyles();
        }

        auto opEndTime = std::chrono::high_resolution_clock::now();
        size_t memAfter = getMemoryUsageKB();
        auto opDuration = std::chrono::duration_cast<std::chrono::milliseconds>(opEndTime - opStartTime);
        memoryLog << i << "," << opName << "," << memBefore << "," << memAfter << "," 
                  << (static_cast<long>(memAfter) - static_cast<long>(memBefore)) << ","
                  << opDuration.count() << std::endl;
        if (i % 100 == 0) std::cout << "Stability iteration " << i << " done." << std::endl;
    }
    auto totalEndTime = std::chrono::high_resolution_clock::now();
    std::cout << "Long-running stability benchmark completed:" << std::endl;
    std::cout << "  Initial Memory: " << initialMemory << " KB" << std::endl;
    std::cout << "  Final Memory:   " << getMemoryUsageKB() << " KB" << std::endl;
    std::cout << "  Total Duration: " << std::chrono::duration_cast<std::chrono::seconds>(totalEndTime - totalStartTime).count() << "s" << std::endl;
    memoryLog.close();
}

void benchmarkFileLoading_withMemory(const std::string& filename) {
    Editor editor;
    size_t memoryBefore = getMemoryUsageKB();
    benchmarkFileLoading(filename); // Calls the original benchmarkFileLoading
    size_t memoryAfter = getMemoryUsageKB();
    std::cout << "File loading (with memory tracking):" << std::endl;
    printMemoryDelta(memoryBefore, memoryAfter);
}

void stressTestLargeEdits(Editor& editor, size_t operationCount) {
    size_t memoryBefore = getMemoryUsageKB();
    std::cout << "Large edits stress test (improved):" << std::endl;
    auto startTime = std::chrono::high_resolution_clock::now();
    std::random_device rd;
    std::mt19937 gen(rd());

    for (size_t i = 0; i < operationCount; ++i) {
        if (editor.getBuffer().isEmpty() || editor.getBuffer().lineCount() == 0) editor.getBuffer().addLine("Seed line");
        std::uniform_int_distribution<> lineDist(0, editor.getBuffer().lineCount() - 1);
        std::uniform_int_distribution<> opDist(0, 4);
        int operation = opDist(gen);
        size_t lineIdx = lineDist(gen);
        editor.setCursor(lineIdx, 0);

        switch (operation) {
            case 0: { // Insert large text
                std::string largeInsert;
                for(int k=0; k<10; ++k) largeInsert += "Stress test line insertion part " + std::to_string(k) + "\n";
                editor.typeText(largeInsert);
                break;
            }
            case 1: { // Delete multiple lines
                for(int k=0; k<5 && editor.getBuffer().lineCount() > 1; ++k) editor.getBuffer().deleteLine(editor.getCursorLine());
                break;
            }
            case 2: if(editor.canUndo()) editor.undo(); break;
            case 3: if(editor.canRedo()) editor.redo(); break;
            case 4: { // Replace line with large content
                std::string repText = "Replaced stress line with extra content " + std::to_string(i);
                editor.getBuffer().replaceLine(editor.getCursorLine(), repText);
                break;
            }
        }
         if (i % (operationCount/100 + 1) == 0) std::cout << "Stress test op " << i << " done." << std::endl;
    }
    auto endTime = std::chrono::high_resolution_clock::now();
    printMemoryDelta(memoryBefore, getMemoryUsageKB());
    std::cout << "Stress test duration: " << std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count() << "ms" << std::endl;
}


// --- Main Function (Commented out for Google Test integration) ---
/*
int main(int argc, char* argv[]) {
    std::cout << "Text Editor Performance Benchmarks" << std::endl;
    std::cout << "------------------------------------" << std::endl;

    Editor editor; // Create one editor instance for benchmarks that modify state
    std::string largeFileName = "large_generated_file.txt";
    size_t lineCountForFile = 10000; // 10k lines
    size_t avgLineLengthForFile = 80;
    size_t benchmarkIterations = 1000; // Iterations for cursor/editing/undo benchmarks
    size_t stabilityIterations = 5000; // Iterations for long-running stability
    size_t stressEditOps = 2000;       // Operations for stress test

    // Argument parsing (optional)
    if (argc > 1) {
        std::string arg1 = argv[1];
        if (arg1 == "--generate-only") {
            if (argc > 3) { // ./benchmark --generate-only <lines> <avg_len>
                 lineCountForFile = std::stoul(argv[2]);
                 avgLineLengthForFile = std::stoul(argv[3]);
            }
            generateLargeFile(largeFileName, lineCountForFile, avgLineLengthForFile);
            return 0;
        }
        // Add more specific benchmark flags if needed
        // e.g., --lines X, --iterations Y
    }

    // Generate the large file if it doesn't exist or needs regeneration for specific tests
    // For consistent benchmarks, you might want to ensure this file is the same each time.
    // std::ifstream checkFile(largeFileName);
    // if (!checkFile.good()) { // Or always regenerate for fresh data
        generateLargeFile(largeFileName, lineCountForFile, avgLineLengthForFile);
    // }
    // checkFile.close();


    std::cout << "\n--- File Loading (No Memory Tracking) ---" << std::endl;
    benchmarkFileLoading(largeFileName);

    std::cout << "\n--- File Loading (With Memory Tracking) ---" << std::endl;
    benchmarkFileLoading_withMemory(largeFileName);

    // Load the large file into the main editor instance for subsequent tests
    std::cout << "\nLoading editor with large file for other benchmarks..." << std::endl;
    Editor mainEditor; // Use a separate editor for tests that load specific files
    mainEditor.openFile(largeFileName);
    if (mainEditor.getBuffer().isEmpty()) {
        std::cerr << "Failed to load " << largeFileName << " into main editor for benchmarks. Exiting." << std::endl;
        return 1;
    }
    std::cout << "Editor loaded with " << mainEditor.getBuffer().lineCount() << " lines." << std::endl;

    std::cout << "\n--- Cursor Operations ---" << std::endl;
    benchmarkCursorOperations(mainEditor, benchmarkIterations);

    std::cout << "\n--- Editing Operations ---" << std::endl;
    // Note: Editing operations modify the buffer. Consider re-loading or using a copy for subsequent tests if needed.
    benchmarkEditingOperations(mainEditor, benchmarkIterations);

    std::cout << "\n--- Undo/Redo Operations ---" << std::endl;
    // For a clean undo/redo test, ideally start with a fresh editor or known state
    Editor undoEditor; // Use a fresh editor for undo/redo
    benchmarkUndoRedo(undoEditor, benchmarkIterations / 10); // Fewer iterations for undo/redo setup

    std::cout << "\n--- Search Operations ---" << std::endl;
    std::vector<std::string> searchTerms = {"Line", "text", "benchmark", "randomXYZnonexistent", editor.getBuffer().getLine(editor.getBuffer().lineCount()/2).substr(0,10)};
    benchmarkSearch(mainEditor, searchTerms);

    std::cout << "\n--- Syntax Highlighting ---" << std::endl;
    benchmarkSyntaxHighlighting(mainEditor);
    
    std::cout << "\n--- Stress Test Large Edits ---" << std::endl;
    // Create a fresh editor for stress test or reload a known state
    Editor stressEditor;
    stressEditor.openFile(largeFileName); // Start with large file
    stressTestLargeEdits(stressEditor, stressEditOps);

    std::cout << "\n--- Long-Running Stability ---" << std::endl;
    // This test modifies the editor state significantly. Run last or with a dedicated editor.
    Editor stabilityEditor;
    stabilityEditor.openFile(largeFileName);
    benchmarkLongRunningStability(stabilityEditor, stabilityIterations);

    std::cout << "\n------------------------------------" << std::endl;
    std::cout << "All benchmarks completed." << std::endl;
    return 0;
}
*/
