#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "SyntaxHighlighter.h"
#include "SyntaxHighlightingManager.h"
#include "TextBuffer.h"
#include "Editor.h"
#include <random>
#include <string>
#include <fstream>
#include <vector>
#include <algorithm>
#include <memory>
#include <functional>
#include <chrono>
#include <filesystem>

namespace fs = std::filesystem;

// Fuzzer base class to handle common fuzzing operations
class Fuzzer {
protected:
    std::mt19937 rng;
    
public:
    Fuzzer(unsigned int seed = std::random_device{}()) : rng(seed) {}
    
    // Generate a random string of given length
    std::string randomString(size_t length) {
        static const char charset[] = 
            "abcdefghijklmnopqrstuvwxyz"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "0123456789"
            "!@#$%^&*()_+-=[]{}|;:,.<>?/\\\"'";
        
        std::string result;
        result.reserve(length);
        
        std::uniform_int_distribution<size_t> dist(0, sizeof(charset) - 2);
        for (size_t i = 0; i < length; ++i) {
            result += charset[dist(rng)];
        }
        
        return result;
    }
    
    // Generate a random C++-like content
    std::string randomCppContent(size_t numLines) {
        std::vector<std::string> cppElements = {
            "int ", "float ", "double ", "char ", "void ", "auto ", "const ", "static ",
            "class ", "struct ", "enum ", "namespace ", "template ", "typename ",
            "return ", "if ", "else ", "for ", "while ", "do ", "switch ", "case ", "break ", "continue ",
            "#include ", "#define ", "#ifdef ", "#ifndef ", "#endif ", "#pragma ",
            "\"string literal\" ", "\'c\' ", "0x123 ", "123 ", "123.456f ",
            "/* block comment */ ", "// line comment\n",
            "{ ", "} ", "( ", ") ", "[ ", "] ", "; ", ", ", ":: ", "-> ", ".",
            "+ ", "- ", "* ", "/ ", "% ", "= ", "== ", "!= ", "> ", "< ", ">= ", "<= ", "&& ", "|| ", "! "
        };
        
        std::string content;
        std::uniform_int_distribution<size_t> elementDist(0, cppElements.size() - 1);
        std::uniform_int_distribution<size_t> lineElementsDist(1, 15);  // Elements per line
        
        for (size_t i = 0; i < numLines; ++i) {
            size_t elements = lineElementsDist(rng);
            for (size_t j = 0; j < elements; ++j) {
                content += cppElements[elementDist(rng)];
            }
            content += "\n";
        }
        
        return content;
    }
    
    // Generate random malformed C++ content (intentionally broken)
    std::string randomMalformedCppContent(size_t numLines) {
        std::string content = randomCppContent(numLines);
        
        // Introduce syntax errors (e.g., unmatched quotes, brackets, incomplete comments)
        std::vector<std::string> errors = {
            "\"unclosed string literal\n",
            "/* unclosed comment\n",
            "} extra closing brace */",
            "{ unclosed brace",
            "unclosed parenthesis (",
            "#error intentional error\n",
            "#define MACRO(x"
        };
        
        std::uniform_int_distribution<size_t> errorDist(0, errors.size() - 1);
        std::uniform_int_distribution<size_t> lineInsertDist(0, numLines > 0 ? numLines - 1 : 0);
        
        // Insert 1-3 errors
        std::uniform_int_distribution<size_t> numErrorsDist(1, 3);
        size_t numErrors = numErrorsDist(rng);
        
        for (size_t i = 0; i < numErrors && numLines > 0; ++i) {
            size_t insertPos = lineInsertDist(rng);
            size_t lineStart = 0;
            for (size_t j = 0; j < insertPos && lineStart != std::string::npos; ++j) {
                lineStart = content.find('\n', lineStart);
                if (lineStart != std::string::npos) lineStart++;
            }
            
            if (lineStart != std::string::npos) {
                size_t lineEnd = content.find('\n', lineStart);
                if (lineEnd == std::string::npos) lineEnd = content.length();
                
                std::string error = errors[errorDist(rng)];
                content.replace(lineStart, lineEnd - lineStart, error);
            }
        }
        
        return content;
    }
    
    // Write content to a temporary file and return the filename
    std::string createTempFile(const std::string& content, const std::string& extension = ".cpp") {
        // Create a temporary file with random name in system temp directory
        std::string filename = fs::temp_directory_path().string() + "/fuzz_test_" + 
                               std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) +
                               extension;
        
        std::ofstream file(filename, std::ios::binary);
        if (file) {
            file << content;
            file.close();
            return filename;
        }
        
        throw std::runtime_error("Failed to create temporary file: " + filename);
    }
    
    void cleanupTempFile(const std::string& filename) {
        try {
            fs::remove(filename);
        } catch (const std::exception& e) {
            std::cerr << "Warning: Failed to remove temporary file: " << filename << std::endl;
            std::cerr << "  Error: " << e.what() << std::endl;
        }
    }
};

// Specialized fuzzer for syntax highlighting
class SyntaxHighlightingFuzzer : public Fuzzer {
public:
    SyntaxHighlightingFuzzer(unsigned int seed = std::random_device{}()) : Fuzzer(seed) {}
    
    // Fuzz test the CppHighlighter with random content
    void fuzzCppHighlighter(int iterations) {
        CppHighlighter highlighter;
        
        for (int i = 0; i < iterations; ++i) {
            // Generate random C++-like content
            std::uniform_int_distribution<size_t> lengthDist(1, 1000);
            std::string testContent = randomCppContent(lengthDist(rng) % 50 + 1);
            
            // Split the content into lines
            std::vector<std::string> lines;
            size_t pos = 0;
            size_t found;
            while ((found = testContent.find('\n', pos)) != std::string::npos) {
                lines.push_back(testContent.substr(pos, found - pos));
                pos = found + 1;
            }
            if (pos < testContent.length()) {
                lines.push_back(testContent.substr(pos));
            }
            
            // Try to highlight each line
            for (size_t j = 0; j < lines.size(); ++j) {
                try {
                    auto styles = highlighter.highlightLine(lines[j], j);
                    
                    // Basic validation - ensure styles are within line bounds
                    for (const auto& style : *styles) {
                        EXPECT_LE(style.endCol, lines[j].length())
                            << "Style end column exceeds line length";
                        EXPECT_LE(style.startCol, style.endCol)
                            << "Style start column exceeds end column";
                    }
                } catch (const std::exception& e) {
                    FAIL() << "Exception thrown during highlighting: " << e.what()
                           << "\nLine content: " << lines[j];
                } catch (...) {
                    FAIL() << "Unknown exception thrown during highlighting"
                           << "\nLine content: " << lines[j];
                }
            }
            
            // Try to highlight the entire buffer
            TextBuffer buffer;
            for (const auto& line : lines) {
                buffer.addLine(line);
            }
            
            try {
                auto styles = highlighter.highlightBuffer(buffer);
                EXPECT_EQ(styles.size(), buffer.lineCount())
                    << "Highlighter returned incorrect number of lines";
            } catch (const std::exception& e) {
                FAIL() << "Exception thrown during buffer highlighting: " << e.what();
            } catch (...) {
                FAIL() << "Unknown exception thrown during buffer highlighting";
            }
        }
    }
    
    // Fuzz test the SyntaxHighlightingManager
    void fuzzSyntaxHighlightingManager(int iterations) {
        SyntaxHighlightingManager manager;
        TextBuffer buffer;
        
        // Register built-in highlighters (they should be auto-registered by registry)
        
        for (int i = 0; i < iterations; ++i) {
            // Generate random content
            std::uniform_int_distribution<size_t> lineCountDist(1, 100);
            size_t lineCount = lineCountDist(rng);
            
            // Clear the buffer and add new lines
            buffer = TextBuffer(); // Reset the buffer
            for (size_t j = 0; j < lineCount; ++j) {
                std::uniform_int_distribution<size_t> lineLengthDist(0, 200);
                buffer.addLine(randomString(lineLengthDist(rng)));
            }
            
            // Set the buffer for the manager
            manager.setBuffer(&buffer);
            
            // Enable/disable randomly
            std::bernoulli_distribution enableDist(0.7); // 70% chance to enable
            manager.setEnabled(enableDist(rng));
            
            try {
                // Invalidate random line ranges
                std::uniform_int_distribution<size_t> invalidateAllDist(0, 5);
                if (invalidateAllDist(rng) == 0) {
                    // 1 in 6 chance to invalidate all
                    manager.invalidateAllLines();
                } else {
                    // Invalidate random subset of lines
                    std::uniform_int_distribution<size_t> lineIndexDist(0, lineCount ? lineCount - 1 : 0);
                    size_t lineToInvalidate = lineCount ? lineIndexDist(rng) : 0;
                    manager.invalidateLine(lineToInvalidate);
                }
                
                // Get highlighting styles for random line ranges
                if (lineCount > 0) {
                    std::uniform_int_distribution<size_t> startLineDist(0, lineCount - 1);
                    size_t startLine = startLineDist(rng);
                    std::uniform_int_distribution<size_t> endLineDist(startLine, lineCount - 1);
                    size_t endLine = endLineDist(rng);
                    
                    auto styles = manager.getHighlightingStyles(startLine, endLine);
                    
                    // Basic validation
                    EXPECT_EQ(styles.size(), endLine - startLine + 1)
                        << "Manager returned incorrect number of style lines";
                }
            } catch (const std::exception& e) {
                FAIL() << "Exception thrown during manager operations: " << e.what();
            } catch (...) {
                FAIL() << "Unknown exception thrown during manager operations";
            }
        }
    }
};

// Specialized fuzzer for file I/O operations
class FileIOFuzzer : public Fuzzer {
public:
    FileIOFuzzer(unsigned int seed = std::random_device{}()) : Fuzzer(seed) {}
    
    // Fuzz test the Editor's file loading functionality
    void fuzzFileLoading(int iterations) {
        // Create an editor instance
        Editor editor;
        
        for (int i = 0; i < iterations; ++i) {
            std::string filename;
            try {
                // Generate different types of content
                std::uniform_int_distribution<int> contentTypeDist(0, 3);
                int contentType = contentTypeDist(rng);
                
                switch (contentType) {
                    case 0: { // Valid C++ content
                        std::uniform_int_distribution<size_t> lineDist(1, 200);
                        std::string content = randomCppContent(lineDist(rng));
                        filename = createTempFile(content, ".cpp");
                        break;
                    }
                    case 1: { // Malformed C++ content
                        std::uniform_int_distribution<size_t> lineDist(1, 200);
                        std::string content = randomMalformedCppContent(lineDist(rng));
                        filename = createTempFile(content, ".cpp");
                        break;
                    }
                    case 2: { // Random binary-like data
                        std::uniform_int_distribution<size_t> sizeDist(1, 10000);
                        std::string content = randomString(sizeDist(rng));
                        filename = createTempFile(content, ".bin");
                        break;
                    }
                    case 3: { // Empty file
                        filename = createTempFile("", ".txt");
                        break;
                    }
                }
                
                // Try to open the file in the editor
                try {
                    editor.openFile(filename);
                    
                    // If we got here, file loading succeeded
                    // Perform some basic verification
                    EXPECT_FALSE(editor.getBuffer().isEmpty() && fs::file_size(filename) > 0)
                        << "Buffer is empty but file has content";
                        
                    // Test some basic editor operations on the loaded file
                    if (!editor.getBuffer().isEmpty()) {
                        // Place cursor at random position
                        std::uniform_int_distribution<size_t> lineDist(0, editor.getBuffer().lineCount() - 1);
                        size_t line = lineDist(rng);
                        std::uniform_int_distribution<size_t> colDist(0, editor.getBuffer().getLine(line).length());
                        size_t col = colDist(rng);
                        
                        editor.setCursor(line, col);
                        
                        // Try random editor operation
                        std::uniform_int_distribution<int> opDist(0, 11);
                        int operation = opDist(rng);
                        
                        switch (operation) {
                            case 0:
                                // Insert random text at cursor
                                editor.typeText(randomString(5));
                                break;
                            case 1:
                                // Delete character at cursor
                                if (col < editor.getBuffer().getLine(line).length()) {
                                    editor.backspace();
                                }
                                break;
                            case 2:
                                // Insert a new line
                                editor.insertLine(editor.getCursorLine(), "");
                                break;
                            case 3:
                                // Move cursor
                                editor.setCursor(
                                    std::min(line + 1, editor.getBuffer().lineCount() - 1),
                                    0
                                );
                                break;
                            case 4:
                                // Delete character
                                if (editor.hasSelection()) editor.deleteSelection(); else editor.backspace();
                                break;
                            case 5:
                                // Insert text
                                editor.typeText(randomString(5));
                                break;
                            case 6:
                                // Insert line
                                editor.insertLine(editor.getCursorLine(), randomString(rng() % 20));
                                break;
                            case 7:
                                // Move cursor
                                editor.setCursor(
                                    rng() % editor.getBuffer().lineCount(),
                                    rng() % (editor.getBuffer().lineLength(editor.getCursorLine()) + 1)
                                );
                                break;
                            case 8:
                                // Toggle syntax highlighting
                                editor.enableSyntaxHighlighting(!editor.isSyntaxHighlightingEnabled());
                                break;
                            case 9:
                                // Save (simulated)
                                try {
                                    editor.saveFile();
                                } catch (...) {
                                    // Ignore save errors in fuzzing
                                }
                                break;
                            case 10:
                                // Undo
                                editor.undo();
                                break;
                            case 11:
                                // Redo
                                editor.redo();
                                break;
                        }
                    }
                } catch (const EditorException& e) {
                    // EditorException is expected for some invalid files
                    // Just verify it has a meaningful message
                    EXPECT_FALSE(e.what() == nullptr || std::string(e.what()).empty())
                        << "EditorException has empty message";
                } catch (const std::exception& e) {
                    FAIL() << "Unexpected standard exception: " << e.what();
                } catch (...) {
                    FAIL() << "Unknown exception during file operations";
                }
                
                // Clean up temp file
                cleanupTempFile(filename);
                
            } catch (const std::exception& e) {
                // Handle exceptions during fuzzer setup (not from the SUT)
                std::cerr << "Warning: Fuzzer setup exception: " << e.what() << std::endl;
                if (!filename.empty()) {
                    cleanupTempFile(filename);
                }
            }
        }
    }
};

// Google Test suite for syntax highlighting fuzzing
TEST(FuzzTesting, SyntaxHighlightingFuzz) {
    SyntaxHighlightingFuzzer fuzzer(42); // Fixed seed for reproducibility
    fuzzer.fuzzCppHighlighter(10); // Reduced iterations for regular test runs
}

TEST(FuzzTesting, SyntaxHighlightingManagerFuzz) {
    SyntaxHighlightingFuzzer fuzzer(43); // Different seed
    fuzzer.fuzzSyntaxHighlightingManager(10); // Reduced iterations for regular test runs
}

TEST(FuzzTesting, FileLoadingFuzz) {
    FileIOFuzzer fuzzer(44); // Different seed
    fuzzer.fuzzFileLoading(5); // Reduced iterations for regular test runs
}

// Long-running comprehensive fuzzing test
// Disabled by default - run explicitly when needed
TEST(FuzzTesting, DISABLED_ComprehensiveFuzzing) {
    // Use random seed for more thorough testing
    unsigned int seed = std::random_device{}();
    std::cout << "Using random seed: " << seed << std::endl;
    
    SyntaxHighlightingFuzzer shFuzzer(seed);
    FileIOFuzzer fileIOFuzzer(seed + 1);
    
    // Run extended fuzzing
    shFuzzer.fuzzCppHighlighter(100);
    shFuzzer.fuzzSyntaxHighlightingManager(100);
    fileIOFuzzer.fuzzFileLoading(50);
} 