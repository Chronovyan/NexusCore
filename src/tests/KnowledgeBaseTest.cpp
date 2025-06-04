#include "../ProjectKnowledgeBase.hpp"
#include <filesystem>
#include <iostream>
#include <vector>
#include <string>
#include <cassert>

using namespace ai_editor;

// Utility function to print a divider
void printDivider() {
    std::cout << "\n" << std::string(80, '-') << "\n\n";
}

// Utility function to print a knowledge entry
void printKnowledgeEntry(const KnowledgeEntry& entry) {
    std::cout << "[" << entry.id << "] " << entry.title << " (Relevance: " << entry.relevanceScore << ")\n";
    std::cout << "Category: " << knowledgeCategoryToString(entry.category);
    
    if (!entry.customCategory.empty()) {
        std::cout << " (" << entry.customCategory << ")";
    }
    
    std::cout << "\n";
    
    if (!entry.tags.empty()) {
        std::cout << "Tags: ";
        for (size_t i = 0; i < entry.tags.size(); ++i) {
            std::cout << entry.tags[i];
            if (i < entry.tags.size() - 1) {
                std::cout << ", ";
            }
        }
        std::cout << "\n";
    }
    
    if (!entry.created.empty()) {
        std::cout << "Created: " << entry.created << "\n";
        if (!entry.updated.empty() && entry.updated != entry.created) {
            std::cout << "Updated: " << entry.updated << "\n";
        }
    }
    
    std::cout << "\nContent:\n";
    std::cout << entry.content << "\n\n";
}

// Test function to verify knowledge base functionality
void runTests() {
    bool allTestsPassed = true;
    
    try {
        std::cout << "Creating knowledge base...\n";
        auto kb = std::make_shared<ProjectKnowledgeBase>();
        
        // Test 1: Add entries
        std::cout << "Test 1: Adding entries...\n";
        
        KnowledgeEntry entry1;
        entry1.title = "Coding Standards";
        entry1.content = "Always use 4 spaces for indentation.";
        entry1.tags = {"style", "formatting"};
        
        KnowledgeEntry entry2;
        entry2.title = "Architecture Overview";
        entry2.content = "This project uses a layered architecture with MVC pattern.";
        entry2.tags = {"architecture", "design"};
        
        // Add entries to knowledge base
        kb->addEntry(entry1);
        kb->addEntry(entry2);
        
        // Verify entry count
        std::cout << "Knowledge base has " << kb->getEntryCount() << " entries\n";
        
        // Test 2: Retrieve entries
        std::cout << "Test 2: Retrieving entries...\n";
        
        // Get entry by ID
        auto retrievedEntry1 = kb->getEntry(entry1.id);
        if (retrievedEntry1.has_value()) {
            std::cout << "Retrieved entry: " << retrievedEntry1->title << "\n";
        }
        
        // Test 3: Save and load
        std::cout << "Test 3: Save and load testing...\n";
        
        // Create a temporary directory for testing
        std::filesystem::path tempDir = std::filesystem::temp_directory_path() / "kb_test";
        std::filesystem::create_directories(tempDir);
        std::string kbFilePath = (tempDir / "test_kb.json").string();
        
        // Save the knowledge base
        bool saveResult = kb->saveToFile(kbFilePath);
        std::cout << "Save result: " << (saveResult ? "success" : "failure") << "\n";
        
        printDivider();
        std::cout << "Knowledge base test completed\n";
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed with exception: " << e.what() << "\n";
        allTestsPassed = false;
    }
    
    if (!allTestsPassed) {
        std::exit(1);
    }
}

int main() {
    std::cout << "KNOWLEDGE BASE TEST SUITE\n";
    printDivider();
    
    runTests();
    
    return 0;
} 