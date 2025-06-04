#include "test_file_utilities.h"
#include <algorithm>
#include <iostream>
#include <sstream>

namespace TestFileGenerator {

std::string generateFile(
    size_t sizeInBytes, 
    const std::string& filename,
    ContentPattern pattern,
    LineEnding lineEnding
) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Unable to create test file: " + filename);
    }
    
    size_t currentSize = 0;
    
    // Select appropriate line ending
    std::string endLine;
    switch (lineEnding) {
        case LineEnding::CRLF: endLine = "\r\n"; break;
        case LineEnding::CR: endLine = "\r"; break;
        case LineEnding::LF: endLine = "\n"; break;
        case LineEnding::MIXED:
            // Will be handled case by case for each line
            endLine = "\n"; // Default for conditional cases
            break;
    }
    
    // Generate content based on pattern
    switch (pattern) {
        case ContentPattern::SEQUENTIAL_NUMBERS: {
            for (size_t i = 0; currentSize < sizeInBytes; ++i) {
                std::string line;
                
                // Create line with sequential number
                line = std::to_string(i);
                
                // Add appropriate line ending
                if (lineEnding == LineEnding::MIXED) {
                    // Alternate line endings
                    switch (i % 3) {
                        case 0: line += "\n"; break;
                        case 1: line += "\r\n"; break;
                        case 2: line += "\r"; break;
                    }
                } else {
                    line += endLine;
                }
                
                // Write to file and update size tracker
                if (!file.write(line.c_str(), line.size())) {
                    file.close();
                    throw std::runtime_error("Error writing to file: " + filename);
                }
                currentSize += line.size();
            }
            break;
        }
        
        case ContentPattern::REPEATED_TEXT: {
            const std::vector<std::string> baseTexts = {
                "This is a test line for performance evaluation. ",
                "The quick brown fox jumps over the lazy dog. ",
                "Lorem ipsum dolor sit amet, consectetur adipiscing elit. ",
                "Async logging with queue overflow policy tests. ",
                "Large file testing is essential for editor performance. "
            };
            
            size_t textIdx = 0;
            while (currentSize < sizeInBytes) {
                std::string line = baseTexts[textIdx % baseTexts.size()];
                textIdx++;
                
                // Add appropriate line ending
                if (lineEnding == LineEnding::MIXED) {
                    // Alternate line endings
                    switch (textIdx % 3) {
                        case 0: line += "\n"; break;
                        case 1: line += "\r\n"; break;
                        case 2: line += "\r"; break;
                    }
                } else {
                    line += endLine;
                }
                
                // Write to file and update size tracker
                if (!file.write(line.c_str(), line.size())) {
                    file.close();
                    throw std::runtime_error("Error writing to file: " + filename);
                }
                currentSize += line.size();
            }
            break;
        }
        
        case ContentPattern::RANDOM_TEXT: {
            std::random_device rd;
            std::mt19937 gen(rd());
            
            // Create distributions for different aspects
            std::uniform_int_distribution<> lengthDist(20, 200);  // Line length
            std::uniform_int_distribution<> charDist(32, 126);    // ASCII printable chars
            
            size_t lineCount = 0;
            while (currentSize < sizeInBytes) {
                // Generate random line length
                size_t lineLength = lengthDist(gen);
                std::string line;
                
                // Generate random text for this line
                for (size_t i = 0; i < lineLength; ++i) {
                    line += static_cast<char>(charDist(gen));
                }
                
                // Add appropriate line ending
                if (lineEnding == LineEnding::MIXED) {
                    // Alternate line endings
                    switch (lineCount % 3) {
                        case 0: line += "\n"; break;
                        case 1: line += "\r\n"; break;
                        case 2: line += "\r"; break;
                    }
                } else {
                    line += endLine;
                }
                
                // Write to file and update size tracker
                if (!file.write(line.c_str(), line.size())) {
                    file.close();
                    throw std::runtime_error("Error writing to file: " + filename);
                }
                currentSize += line.size();
                lineCount++;
            }
            break;
        }
        
        case ContentPattern::CODE_LIKE: {
            // Templates for C++-like code
            std::vector<std::string> codeTemplates = {
                "#include <iostream>",
                "#include <vector>",
                "#include <string>",
                "#include <algorithm>",
                "",
                "// This is a test function {0}",
                "void function{0}(int value) {",
                "    // Process the input value",
                "    for (int i = 0; i < value; i++) {",
                "        if (i % {1} == 0) {",
                "            std::cout << \"Value: \" << i << std::endl;",
                "        }",
                "    }",
                "}",
                "",
                "class TestClass{0} {",
                "private:",
                "    int value_{1};",
                "    std::string name_;",
                "",
                "public:",
                "    TestClass{0}(int value, const std::string& name) ",
                "        : value_(value), name_(name) {}",
                "",
                "    void process() {",
                "        std::vector<int> numbers;",
                "        for (int i = 0; i < value_; ++i) {",
                "            numbers.push_back(i * {1});",
                "        }",
                "        ",
                "        // Sort the numbers",
                "        std::sort(numbers.begin(), numbers.end());",
                "        ",
                "        // Print the result",
                "        std::cout << name_ << \": \";",
                "        for (auto n : numbers) {",
                "            std::cout << n << \" \";",
                "        }",
                "        std::cout << std::endl;",
                "    }",
                "};",
                ""
            };
            
            size_t counter = 0;
            size_t lineCount = 0;
            
            while (currentSize < sizeInBytes) {
                for (auto& templ : codeTemplates) {
                    std::string line = templ;
                    
                    // Replace template placeholders
                    size_t pos;
                    while ((pos = line.find("{0}")) != std::string::npos) {
                        line.replace(pos, 3, std::to_string(counter));
                    }
                    
                    while ((pos = line.find("{1}")) != std::string::npos) {
                        line.replace(pos, 3, std::to_string(counter * 10 + 1));
                    }
                    
                    // Add appropriate line ending
                    if (lineEnding == LineEnding::MIXED) {
                        // Alternate line endings
                        switch (lineCount % 3) {
                            case 0: line += "\n"; break;
                            case 1: line += "\r\n"; break;
                            case 2: line += "\r"; break;
                        }
                    } else {
                        line += endLine;
                    }
                    
                    // Write to file and update size tracker
                    if (!file.write(line.c_str(), line.size())) {
                        file.close();
                        throw std::runtime_error("Error writing to file: " + filename);
                    }
                    currentSize += line.size();
                    lineCount++;
                    
                    if (currentSize >= sizeInBytes) break;
                }
                counter++;
            }
            break;
        }
        
        case ContentPattern::MIXED_LINE_LENGTHS: {
            std::random_device rd;
            std::mt19937 gen(rd());
            
            // Create distributions for different line lengths
            std::vector<size_t> lineLengths = {5, 10, 15, 20, 30, 50, 75, 100, 200, 500, 1000};
            std::uniform_int_distribution<> lengthIndex(0, lineLengths.size() - 1);
            std::uniform_int_distribution<> charDist(65, 90);  // A-Z
            
            size_t lineCount = 0;
            while (currentSize < sizeInBytes) {
                // Select random line length from predefined options
                size_t length = lineLengths[lengthIndex(gen)];
                
                // Generate line content (repeating characters for efficiency)
                char c = static_cast<char>(charDist(gen));
                std::string line(length, c);
                
                // Add appropriate line ending
                if (lineEnding == LineEnding::MIXED) {
                    // Alternate line endings
                    switch (lineCount % 3) {
                        case 0: line += "\n"; break;
                        case 1: line += "\r\n"; break;
                        case 2: line += "\r"; break;
                    }
                } else {
                    line += endLine;
                }
                
                // Write to file and update size tracker
                if (!file.write(line.c_str(), line.size())) {
                    file.close();
                    throw std::runtime_error("Error writing to file: " + filename);
                }
                currentSize += line.size();
                lineCount++;
            }
            break;
        }
        
        case ContentPattern::MIXED_LINE_ENDINGS: {
            // This is handled by the MIXED lineEnding option
            // Default to REPEATED_TEXT pattern with MIXED line endings
            LineEnding mixedEnding = LineEnding::MIXED;
            return generateFile(sizeInBytes, filename, ContentPattern::REPEATED_TEXT, mixedEnding);
        }
    }
    
    // Check if file was written successfully
    if (file.bad()) {
        file.close();
        throw std::runtime_error("Error writing to file: " + filename);
    }
    
    // Close file and return
    file.close();
    
    // Output some diagnostics about the generated file
    std::cout << "Generated file: " << filename << std::endl;
    std::cout << "  Size: " << currentSize << " bytes" << std::endl;
    
    return filename;
}

} // namespace TestFileGenerator

namespace MemoryTracker {

size_t getCurrentMemoryUsage() {
    size_t memoryUsage = 0;
    
#ifdef _WIN32
    // Windows implementation
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        memoryUsage = pmc.WorkingSetSize;
    }
#elif defined(__APPLE__)
    // macOS implementation
    struct mach_task_basic_info info;
    mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
    
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info, &infoCount) == KERN_SUCCESS) {
        memoryUsage = info.resident_size;
    }
#else
    // Linux implementation
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        // Convert kilobytes to bytes
        memoryUsage = usage.ru_maxrss * 1024;
    }
    
    // Alternative using /proc/self/statm
    FILE* file = fopen("/proc/self/statm", "r");
    if (file) {
        unsigned long size, resident, share, text, lib, data, dt;
        if (fscanf(file, "%lu %lu %lu %lu %lu %lu %lu", &size, &resident, &share, &text, &lib, &data, &dt) == 7) {
            // resident set size in bytes = resident * page size
            memoryUsage = resident * sysconf(_SC_PAGESIZE);
        }
        fclose(file);
    }
#endif
    
    return memoryUsage;
}

} // namespace MemoryTracker 