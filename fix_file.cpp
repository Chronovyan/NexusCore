#include <iostream>
#include <fstream>
#include <string>
#include <vector>

int main() {
    // Read the file
    std::ifstream inFile("src/OpenAI_API_Client.cpp");
    if (!inFile) {
        std::cerr << "Could not open input file\n";
        return 1;
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(inFile, line)) {
        lines.push_back(line);
    }
    inFile.close();

    // Write the fixed file
    std::ofstream outFile("src/OpenAI_API_Client.cpp");
    if (!outFile) {
        std::cerr << "Could not open output file\n";
        return 1;
    }

    bool skipMode = false;
    for (size_t i = 0; i < lines.size(); ++i) {
        // Check for the beginning of the method to skip
        if (lines[i].find("RetryStatistics OpenAI_API_Client::getRetryStatistics() const") != std::string::npos) {
            skipMode = true;
            continue;
        }

        // Continue skipping until we hit the end of the method
        if (skipMode) {
            if (lines[i] == "}") {
                skipMode = false;
                continue;
            }
            continue;
        }

        // Write the line
        outFile << lines[i] << "\n";
    }

    outFile.close();
    std::cout << "File fixed successfully\n";
    return 0;
} 