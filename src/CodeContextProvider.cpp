#include "CodeContextProvider.hpp"
#include "EditorErrorReporter.h"
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <unordered_set>
#include <utility>
#include <regex>
#include <cctype>
#include <fstream>
#include <cmath>

namespace fs = std::filesystem;
namespace ai_editor {

CodeContextProvider::CodeContextProvider(
    std::shared_ptr<ICodebaseIndex> codebaseIndex,
    std::shared_ptr<IProjectKnowledgeBase> knowledgeBase)
    : codebaseIndex_(codebaseIndex), knowledgeBase_(knowledgeBase)
{
    if (!codebaseIndex_) {
        throw std::invalid_argument("CodebaseIndex cannot be null");
    }
    
    // Register default symbol relevance scorer
    registerSymbolRelevanceScorer("default", 
        [](const CodeSymbol& symbol, const std::string& currentFile, size_t line, size_t column) {
            // Default scoring based on symbol type and proximity to cursor
            float score = 0.5f;  // Base score
            
            // Boost score for current file symbols
            if (symbol.filePath == currentFile) {
                score += 0.3f;
                
                // Boost score for symbols near cursor position
                const int lineDistance = std::abs(static_cast<int>(line) - static_cast<int>(symbol.line));
                if (lineDistance < 50) {
                    score += 0.2f * (1.0f - (lineDistance / 50.0f));
                }
            }
            
            // Adjust score based on symbol type
            switch (symbol.type) {
                case CodeSymbol::SymbolType::Class:
                case CodeSymbol::SymbolType::Struct:
                    score += 0.15f;
                    break;
                case CodeSymbol::SymbolType::Method:
                case CodeSymbol::SymbolType::Function:
                    score += 0.1f;
                    break;
                case CodeSymbol::SymbolType::Variable:
                    score += 0.05f;
                    break;
                default:
                    break;
            }
            
            return std::min(score, 1.0f);  // Cap at 1.0
        }
    );
    
    // Register default file relevance scorer
    registerFileRelevanceScorer("default",
        [](const std::string& filePath, const std::string& currentFile) {
            if (filePath == currentFile) {
                return 1.0f;  // Current file is most relevant
            }
            
            // Related files are somewhat relevant by default
            return 0.5f;
        }
    );
}

void CodeContextProvider::setProjectKnowledgeBase(std::shared_ptr<IProjectKnowledgeBase> knowledgeBase) {
    knowledgeBase_ = knowledgeBase;
}

std::shared_ptr<IProjectKnowledgeBase> CodeContextProvider::getProjectKnowledgeBase() const {
    return knowledgeBase_;
}

CodeContext CodeContextProvider::getContext(
    const std::string& filePath,
    size_t line,
    size_t column,
    const std::string& selectedText,
    const std::vector<std::string>& visibleFiles,
    const ContextOptions& options)
{
    CodeContext context;
    
    // Use the provided options or fallback to member variable
    ContextOptions effectiveOptions = options;
    if (options.maxTokens == 4000 && options.maxSnippets == 5) {
        // If default options were passed, use the member variable instead
        effectiveOptions = contextOptions_;
    }
    
    // Set basic context information
    context.currentFile = filePath;
    context.cursorLine = line;
    context.cursorColumn = column;
    context.selectedText = selectedText;
    context.visibleFiles = visibleFiles;
    
    // Skip context gathering if codebase index is null or file doesn't exist
    if (!codebaseIndex_ || !codebaseIndex_->getFileInfo(filePath)) {
        return context;
    }
    
    // Find the symbol at the current position
    context.currentSymbol = findSymbolAtPosition(filePath, line, column);
    
    // If we found a symbol, get related symbols and context
    if (context.currentSymbol) {
        context.relatedSymbols = findRelatedSymbols(
            *context.currentSymbol, 
            effectiveOptions.maxSymbols, 
            effectiveOptions);
    }
    
    // Find related files
    context.relatedFiles = findRelatedFiles(filePath, effectiveOptions);
    
    // Get project structure information
    context.importantProjectFiles = getImportantProjectFiles();
    context.projectLanguage = detectProjectLanguage();
    context.dependencies = getProjectDependencies();
    
    // Generate code snippets
    context.codeSnippets = generateCodeSnippets(context, effectiveOptions.maxSnippets, effectiveOptions);
    
    // Find relevant knowledge base entries if available
    if (knowledgeBase_ && effectiveOptions.includeKnowledgeBase) {
        context.knowledgeEntries = findRelevantKnowledgeEntries(context, effectiveOptions);
    }
    
    // Prune snippets based on relevance score
    pruneSnippetsByRelevance(context.codeSnippets, effectiveOptions.minRelevanceScore);
    
    // Trim context to fit within token limit
    trimContextToTokenLimit(context, effectiveOptions.maxTokens);
    
    return context;
}

std::string CodeContextProvider::generateContextualPrompt(
    const std::string& userPrompt,
    const CodeContext& context,
    const ContextOptions& options)
{
    std::stringstream prompt;
    
    // Track token count
    size_t estimatedTokens = 0;
    
    // Helper to add content if we're within token limits
    auto addContent = [&](const std::string& content) {
        size_t contentTokens = estimateTokenCount(content);
        if (estimatedTokens + contentTokens <= options.maxTokens) {
            prompt << content;
            estimatedTokens += contentTokens;
            return true;
        }
        return false;
    };
    
    // Add project information first
    std::stringstream projectInfo;
    projectInfo << "Project information:\n";
    projectInfo << "- Primary language: " << context.projectLanguage << "\n";
    
    if (!context.dependencies.empty()) {
        projectInfo << "- Dependencies: ";
        for (size_t i = 0; i < context.dependencies.size(); ++i) {
            if (i > 0) projectInfo << ", ";
            projectInfo << context.dependencies[i];
        }
        projectInfo << "\n";
    }
    
    if (!context.importantProjectFiles.empty()) {
        projectInfo << "- Key project files:\n";
        for (const auto& file : context.importantProjectFiles) {
            projectInfo << "  - " << file << "\n";
        }
    }
    projectInfo << "\n";
    
    addContent(projectInfo.str());
    
    // Add information about the current file and position
    std::stringstream fileInfo;
    fileInfo << "I'm working on file: " << context.currentFile << " at line " 
             << (context.cursorLine + 1) << ", column " << (context.cursorColumn + 1) << "\n\n";
    
    addContent(fileInfo.str());
    
    // Add information about the current symbol if available
    if (context.currentSymbol) {
        std::stringstream symbolInfo;
        symbolInfo << "Current symbol context:\n"
                   << getSymbolSummary(*context.currentSymbol) << "\n\n";
        
        addContent(symbolInfo.str());
    }
    
    // Add selected text if any
    if (!context.selectedText.empty()) {
        std::stringstream selectedTextInfo;
        selectedTextInfo << "Selected text:\n```\n" << context.selectedText << "\n```\n\n";
        
        addContent(selectedTextInfo.str());
    }
    
    // Add code snippets (limiting to maxContextSnippets)
    if (!context.codeSnippets.empty()) {
        std::stringstream snippetsInfo;
        snippetsInfo << "Relevant code context:\n";
        
        // Sort snippets by relevance score (highest first)
        auto sortedSnippets = context.codeSnippets;
        std::sort(sortedSnippets.begin(), sortedSnippets.end(),
                 [](const auto& a, const auto& b) {
                     return a.relevanceScore > b.relevanceScore;
                 });
        
        size_t snippetCount = 0;
        for (const auto& snippet : sortedSnippets) {
            if (snippetCount >= options.maxContextSnippets) {
                break;
            }
            
            // Create snippet content
            std::stringstream snippetContent;
            snippetContent << "From file: " << snippet.filePath;
            if (!snippet.symbolName.empty()) {
                snippetContent << " (symbol: " << snippet.symbolName << ")";
            }
            
            snippetContent << " lines " << (snippet.startLine + 1) << "-" << (snippet.endLine + 1) 
                           << " [relevance: " << snippet.relevanceScore << "]:\n"
                           << "```\n" << snippet.content << "\n```\n\n";
            
            // Add if within token limit
            if (addContent(snippetContent.str())) {
                snippetCount++;
            } else {
                // If we can't add more content, break out of the loop
                break;
            }
        }
    }
    
    // Add information about related symbols if any
    if (!context.relatedSymbols.empty()) {
        std::stringstream relatedInfo;
        relatedInfo << "Related symbols:\n";
        
        for (const auto& symbol : context.relatedSymbols) {
            relatedInfo << "- " << symbol.name << " (" 
                        << getSymbolTypeString(symbol.type) << ")"
                        << " in " << symbol.filePath << "\n";
        }
        relatedInfo << "\n";
        
        addContent(relatedInfo.str());
    }
    
    // Add information about related files if any
    if (!context.relatedFiles.empty()) {
        std::stringstream relatedFilesInfo;
        relatedFilesInfo << "Related files:\n";
        
        for (const auto& file : context.relatedFiles) {
            relatedFilesInfo << "- " << file << "\n";
        }
        relatedFilesInfo << "\n";
        
        addContent(relatedFilesInfo.str());
    }
    
    // Extract key terms for search context
    auto keyTerms = extractKeyTerms(context);
    if (!keyTerms.empty()) {
        std::stringstream termsInfo;
        termsInfo << "Key terms in context: ";
        
        for (size_t i = 0; i < keyTerms.size(); ++i) {
            if (i > 0) termsInfo << ", ";
            termsInfo << keyTerms[i];
        }
        termsInfo << "\n\n";
        
        addContent(termsInfo.str());
    }
    
    // Add knowledge base entries if available
    if (!context.knowledgeEntries.empty()) {
        prompt << "\nProject Knowledge Base Information:\n";
        for (const auto& entry : context.knowledgeEntries) {
            prompt << "- [" << knowledgeCategoryToString(entry.category) << "] " 
                   << entry.title << "\n";
            
            if (!entry.customCategory.empty()) {
                prompt << "  Category: " << entry.customCategory << "\n";
            }
            
            if (!entry.tags.empty()) {
                prompt << "  Tags: ";
                for (size_t i = 0; i < entry.tags.size(); ++i) {
                    prompt << entry.tags[i];
                    if (i < entry.tags.size() - 1) {
                        prompt << ", ";
                    }
                }
                prompt << "\n";
            }
            
            prompt << "  Content:\n";
            prompt << "  ```\n  " << entry.content << "\n  ```\n\n";
        }
    }
    
    // Add the user's prompt at the end
    std::stringstream userPromptInfo;
    userPromptInfo << "My question is: " << userPrompt;
    
    addContent(userPromptInfo.str());
    
    return prompt.str();
}

std::string CodeContextProvider::getSymbolSummary(const CodeSymbol& symbol)
{
    std::stringstream summary;
    
    // Symbol type and name
    summary << getSymbolTypeString(symbol.type) << ": " << symbol.name << "\n";
    
    // File and location
    summary << "Location: " << symbol.filePath << ":" 
            << (symbol.lineNumber + 1) << ":" << (symbol.columnNumber + 1) << "\n";
    
    // Signature if available
    if (!symbol.signature.empty()) {
        summary << "Signature: " << symbol.signature << "\n";
    }
    
    // Documentation if available
    if (!symbol.documentation.empty()) {
        summary << "Documentation: " << symbol.documentation << "\n";
    }
    
    // Namespace if available
    if (!symbol.namespace_.empty()) {
        summary << "Namespace: " << symbol.namespace_ << "\n";
    }
    
    // Parent if available
    if (symbol.parentId && codebaseIndex_) {
        auto parent = codebaseIndex_->getSymbol(*symbol.parentId);
        if (parent) {
            summary << "Parent: " << parent->name << " (" 
                    << getSymbolTypeString(parent->type) << ")\n";
        }
    }
    
    return summary.str();
}

std::optional<CodeSymbol> CodeContextProvider::findSymbolAtPosition(
    const std::string& filePath,
    size_t line,
    size_t column)
{
    if (!codebaseIndex_) {
        return std::nullopt;
    }
    
    // Get all symbols in the file
    auto fileSymbols = codebaseIndex_->findSymbolsInFile(filePath);
    
    // Find the innermost symbol containing the position
    // Sort by hierarchy depth to find the most specific symbol
    std::sort(fileSymbols.begin(), fileSymbols.end(), [this](const CodeSymbol& a, const CodeSymbol& b) {
        return getSymbolDepth(a) > getSymbolDepth(b);
    });
    
    // Find symbols containing the cursor position
    for (const auto& symbol : fileSymbols) {
        // Skip symbol if it doesn't have a position or we can't get references
        if (symbol.lineNumber < 0 || symbol.columnNumber < 0) {
            continue;
        }
        
        // Get symbol references to find definition and end location
        auto references = codebaseIndex_->getSymbolReferences(symbol.id);
        
        // Find the definition reference, which should have line/column info
        auto defIt = std::find_if(references.begin(), references.end(),
            [](const SymbolReference& ref) { return ref.isDefinition; });
        
        if (defIt == references.end()) {
            // Use the symbol's own line/column info if no definition reference found
            if (line == symbol.lineNumber || 
                (line > symbol.lineNumber && isSymbolMultiLine(symbol))) {
                return symbol;
            }
        } else {
            // Check if cursor is within the symbol's definition
            if (line == defIt->lineNumber || 
                (line > defIt->lineNumber && line <= getSymbolEndLine(symbol))) {
                return symbol;
            }
        }
    }
    
    return std::nullopt;
}

std::vector<CodeSymbol> CodeContextProvider::findRelatedSymbols(
    const CodeSymbol& symbol,
    size_t maxRelated,
    const ContextOptions& options)
{
    std::vector<CodeSymbol> relatedSymbols;
    
    if (!codebaseIndex_) {
        return relatedSymbols;
    }
    
    // Track which symbols we've already added to avoid duplicates
    std::unordered_set<std::string> addedSymbolIds;
    addedSymbolIds.insert(symbol.id);
    
    // Collect candidate symbols with their relevance scores
    std::vector<std::pair<CodeSymbol, float>> candidates;
    
    // Add parent symbol if available
    if (symbol.parentId) {
        auto parent = codebaseIndex_->getSymbol(*symbol.parentId);
        if (parent && addedSymbolIds.find(parent->id) == addedSymbolIds.end()) {
            float relevance = calculateSymbolRelevance(*parent, symbol.filePath, symbol.lineNumber, symbol.columnNumber);
            candidates.push_back({*parent, relevance});
            addedSymbolIds.insert(parent->id);
        }
    }
    
    // Add child symbols if available
    for (const auto& childId : symbol.childIds) {
        auto child = codebaseIndex_->getSymbol(childId);
        if (child && addedSymbolIds.find(child->id) == addedSymbolIds.end()) {
            float relevance = calculateSymbolRelevance(*child, symbol.filePath, symbol.lineNumber, symbol.columnNumber);
            candidates.push_back({*child, relevance});
            addedSymbolIds.insert(child->id);
        }
    }
    
    // Add symbols with relationships to this symbol
    if (options.includeRelationships) {
        auto outboundRelations = codebaseIndex_->getSymbolRelations(symbol.id, std::nullopt, false);
        auto inboundRelations = codebaseIndex_->getSymbolRelations(symbol.id, std::nullopt, true);
        
        // Add symbols from outbound relationships (this symbol -> other symbols)
        for (const auto& relation : outboundRelations) {
            if (addedSymbolIds.find(relation.targetSymbolId) == addedSymbolIds.end()) {
                auto targetSymbol = codebaseIndex_->getSymbol(relation.targetSymbolId);
                if (targetSymbol) {
                    float relevance = calculateSymbolRelevance(
                        *targetSymbol, symbol.filePath, symbol.lineNumber, symbol.columnNumber);
                    
                    // Adjust relevance based on relation type
                    if (relation.type == SymbolRelation::CALLS) {
                        relevance *= 1.2f; // Boost calls as they're usually important
                    } else if (relation.type == SymbolRelation::INHERITS) {
                        relevance *= 1.1f; // Boost inheritance
                    }
                    
                    candidates.push_back({*targetSymbol, relevance});
                    addedSymbolIds.insert(targetSymbol->id);
                }
            }
        }
        
        // Add symbols from inbound relationships (other symbols -> this symbol)
        for (const auto& relation : inboundRelations) {
            if (addedSymbolIds.find(relation.sourceSymbolId) == addedSymbolIds.end()) {
                auto sourceSymbol = codebaseIndex_->getSymbol(relation.sourceSymbolId);
                if (sourceSymbol) {
                    float relevance = calculateSymbolRelevance(
                        *sourceSymbol, symbol.filePath, symbol.lineNumber, symbol.columnNumber);
                    
                    // Adjust relevance based on relation type
                    if (relation.type == SymbolRelation::CALLS) {
                        relevance *= 1.1f; // Boost calls somewhat
                    } else if (relation.type == SymbolRelation::INHERITS) {
                        relevance *= 1.2f; // Boost inheritance more for inbound
                    }
                    
                    candidates.push_back({*sourceSymbol, relevance});
                    addedSymbolIds.insert(sourceSymbol->id);
                }
            }
        }
    }
    
    // Sort candidates by relevance score
    std::sort(candidates.begin(), candidates.end(),
             [](const auto& a, const auto& b) {
                 return a.second > b.second; // Sort by relevance (highest first)
             });
    
    // Take the top maxRelated candidates
    for (const auto& [symbol, score] : candidates) {
        if (relatedSymbols.size() >= maxRelated) {
            break;
        }
        relatedSymbols.push_back(symbol);
    }
    
    return relatedSymbols;
}

std::vector<std::string> CodeContextProvider::findRelatedFiles(
    const std::string& filePath,
    const ContextOptions& options)
{
    std::vector<std::string> relatedFiles;
    
    if (!codebaseIndex_) {
        return relatedFiles;
    }
    
    // Collect candidate files with their relevance scores
    std::vector<std::pair<std::string, float>> candidates;
    
    fs::path path(filePath);
    std::string extension = path.extension().string();
    std::string stemName = path.stem().string();
    fs::path parentPath = path.parent_path();
    
    // Check if this is a C/C++ header or implementation file
    bool isHeader = (extension == ".h" || extension == ".hpp" || extension == ".hxx" || extension == ".hh");
    bool isImplementation = (extension == ".c" || extension == ".cpp" || extension == ".cxx" || extension == ".cc");
    
    // Find corresponding header/implementation files
    if (isHeader || isImplementation) {
        std::vector<std::string> possibleExtensions;
        
        if (isHeader) {
            // Look for implementation files
            possibleExtensions = {".cpp", ".cxx", ".cc", ".c"};
        } else {
            // Look for header files
            possibleExtensions = {".hpp", ".hxx", ".hh", ".h"};
        }
        
        // Get all files in the codebase
        auto allFiles = codebaseIndex_->getAllFiles();
        
        // Find files with the same stem name and matching extensions
        for (const auto& file : allFiles) {
            fs::path candidatePath(file.path);
            
            // Skip the current file
            if (candidatePath == path) {
                continue;
            }
            
            // Check if the stems match
            if (candidatePath.stem().string() == stemName) {
                // Check if the extension is one we're looking for
                std::string fileExt = candidatePath.extension().string();
                auto it = std::find(possibleExtensions.begin(), possibleExtensions.end(), fileExt);
                
                if (it != possibleExtensions.end()) {
                    // Calculate relevance for this file
                    float relevance = calculateFileRelevance(file.path, filePath);
                    
                    // Boost relevance for header/implementation pairs
                    relevance *= 1.5f;
                    
                    candidates.push_back({file.path, relevance});
                }
            }
        }
    }
    
    // Find files in the same directory
    auto allFiles = codebaseIndex_->getAllFiles();
    for (const auto& file : allFiles) {
        fs::path candidatePath(file.path);
        
        // Skip the current file and any already added
        if (candidatePath == path || 
            std::find(relatedFiles.begin(), relatedFiles.end(), file.path) != relatedFiles.end()) {
            continue;
        }
        
        // Check if in the same directory
        if (candidatePath.parent_path() == parentPath) {
            float relevance = calculateFileRelevance(file.path, filePath);
            candidates.push_back({file.path, relevance});
        }
    }
    
    // Sort candidates by relevance
    std::sort(candidates.begin(), candidates.end(),
             [](const auto& a, const auto& b) {
                 return a.second > b.second; // Sort by relevance (highest first)
             });
    
    // Take top files up to max limit
    for (const auto& [file, score] : candidates) {
        if (relatedFiles.size() >= options.maxRelatedFiles) {
            break;
        }
        relatedFiles.push_back(file);
    }
    
    return relatedFiles;
}

std::vector<CodeContext::ContextSnippet> CodeContextProvider::generateCodeSnippets(
    const CodeContext& context,
    size_t maxSnippets,
    const ContextOptions& options)
{
    std::vector<CodeContext::ContextSnippet> snippets;
    
    if (!codebaseIndex_) {
        return snippets;
    }
    
    // Collect candidate snippets with their relevance scores
    std::vector<CodeContext::ContextSnippet> candidates;
    
    // Add a snippet for the current symbol if available
    if (context.currentSymbol) {
        auto currentSymbol = *context.currentSymbol;
        
        // Try to get the content from the codebase index's file cache
        auto fileInfo = codebaseIndex_->getFileInfo(currentSymbol.filePath);
        if (fileInfo) {
            // We need to get the content and line range for the symbol
            size_t startLine = currentSymbol.lineNumber;
            size_t endLine = getSymbolEndLine(currentSymbol);
            
            // Get the snippet content from the file
            std::string content = getFileSnippet(currentSymbol.filePath, startLine, endLine);
            
            if (!content.empty()) {
                CodeContext::ContextSnippet snippet;
                snippet.filePath = currentSymbol.filePath;
                snippet.symbolName = currentSymbol.name;
                snippet.content = content;
                snippet.startLine = startLine;
                snippet.endLine = endLine;
                snippet.relevanceScore = 1.0f; // Current symbol has highest relevance
                
                candidates.push_back(snippet);
            }
        }
    }
    
    // Add snippets for related symbols
    for (const auto& symbol : context.relatedSymbols) {
        // Get the symbol's content
        size_t startLine = symbol.lineNumber;
        size_t endLine = getSymbolEndLine(symbol);
        
        std::string content = getFileSnippet(symbol.filePath, startLine, endLine);
        
        if (!content.empty()) {
            CodeContext::ContextSnippet snippet;
            snippet.filePath = symbol.filePath;
            snippet.symbolName = symbol.name;
            snippet.content = content;
            snippet.startLine = startLine;
            snippet.endLine = endLine;
            
            // Calculate relevance for this symbol
            float relevance = calculateSymbolRelevance(
                symbol, context.currentFile, context.cursorLine, context.cursorColumn);
            snippet.relevanceScore = relevance;
            
            candidates.push_back(snippet);
        }
    }
    
    // Add code near the cursor position if we don't have enough snippets
    if (candidates.size() < maxSnippets && !context.currentSymbol) {
        // No symbol at cursor, but still want to show surrounding code
        size_t startLine = context.cursorLine > 10 ? context.cursorLine - 10 : 0;
        size_t endLine = startLine + 20; // Show about 20 lines
        
        std::string content = getFileSnippet(context.currentFile, startLine, endLine);
        
        if (!content.empty()) {
            CodeContext::ContextSnippet snippet;
            snippet.filePath = context.currentFile;
            snippet.symbolName = ""; // No specific symbol
            snippet.content = content;
            snippet.startLine = startLine;
            snippet.endLine = endLine;
            snippet.relevanceScore = 0.9f; // High relevance for cursor surroundings
            
            candidates.push_back(snippet);
        }
    }
    
    // Sort candidates by relevance
    std::sort(candidates.begin(), candidates.end(),
             [](const auto& a, const auto& b) {
                 return a.relevanceScore > b.relevanceScore;
             });
    
    // Take top snippets up to max limit
    for (const auto& snippet : candidates) {
        if (snippets.size() >= maxSnippets) {
            break;
        }
        snippets.push_back(snippet);
    }
    
    return snippets;
}

// Helper methods

std::string CodeContextProvider::getSymbolTypeString(CodeSymbol::SymbolType type)
{
    switch (type) {
        case CodeSymbol::SymbolType::FUNCTION: return "function";
        case CodeSymbol::SymbolType::METHOD: return "method";
        case CodeSymbol::SymbolType::CLASS: return "class";
        case CodeSymbol::SymbolType::STRUCT: return "struct";
        case CodeSymbol::SymbolType::VARIABLE: return "variable";
        case CodeSymbol::SymbolType::FIELD: return "field";
        case CodeSymbol::SymbolType::ENUM: return "enum";
        case CodeSymbol::SymbolType::INTERFACE: return "interface";
        case CodeSymbol::SymbolType::NAMESPACE: return "namespace";
        case CodeSymbol::SymbolType::MODULE: return "module";
        case CodeSymbol::SymbolType::PACKAGE: return "package";
        case CodeSymbol::SymbolType::FILE: return "file";
        default: return "unknown";
    }
}

int CodeContextProvider::getSymbolDepth(const CodeSymbol& symbol)
{
    int depth = 0;
    auto currentSymbol = symbol;
    
    while (currentSymbol.parentId && codebaseIndex_) {
        auto parent = codebaseIndex_->getSymbol(*currentSymbol.parentId);
        if (!parent) {
            break;
        }
        
        depth++;
        currentSymbol = *parent;
    }
    
    return depth;
}

bool CodeContextProvider::isSymbolMultiLine(const CodeSymbol& symbol)
{
    // Implementation-specific logic to determine if a symbol spans multiple lines
    // This is a simple heuristic, might need adjustment for different languages
    switch (symbol.type) {
        case CodeSymbol::SymbolType::FUNCTION:
        case CodeSymbol::SymbolType::METHOD:
        case CodeSymbol::SymbolType::CLASS:
        case CodeSymbol::SymbolType::STRUCT:
        case CodeSymbol::SymbolType::ENUM:
        case CodeSymbol::SymbolType::INTERFACE:
        case CodeSymbol::SymbolType::NAMESPACE:
            return true;  // These types typically span multiple lines
            
        case CodeSymbol::SymbolType::VARIABLE:
        case CodeSymbol::SymbolType::FIELD:
            return false; // These types typically fit on one line
            
        default:
            return false;
    }
}

size_t CodeContextProvider::getSymbolEndLine(const CodeSymbol& symbol)
{
    // Get the definition from symbol references
    auto references = codebaseIndex_->getSymbolReferences(symbol.id);
    
    // Find the definition reference
    auto defIt = std::find_if(references.begin(), references.end(),
        [](const SymbolReference& ref) { return ref.isDefinition; });
    
    // Start with the symbol's own line number
    size_t startLine = symbol.lineNumber;
    
    // If we found a definition reference, use its line number
    if (defIt != references.end()) {
        startLine = defIt->lineNumber;
    }
    
    // Estimate the end line based on symbol type
    size_t estimatedLines = 0;
    
    switch (symbol.type) {
        case CodeSymbol::SymbolType::FUNCTION:
        case CodeSymbol::SymbolType::METHOD:
            estimatedLines = 10; // Typical small function/method
            break;
            
        case CodeSymbol::SymbolType::CLASS:
        case CodeSymbol::SymbolType::STRUCT:
            estimatedLines = 20; // Typical small class/struct
            break;
            
        case CodeSymbol::SymbolType::ENUM:
        case CodeSymbol::SymbolType::INTERFACE:
            estimatedLines = 5; // Typical small enum/interface
            break;
            
        case CodeSymbol::SymbolType::NAMESPACE:
            estimatedLines = 30; // Typical namespace
            break;
            
        case CodeSymbol::SymbolType::VARIABLE:
        case CodeSymbol::SymbolType::FIELD:
        default:
            estimatedLines = 1; // Single line
            break;
    }
    
    // For more accurate end line detection, we could look at:
    // 1. Child symbols (the latest child's end would be our end)
    // 2. References to this symbol within its own file
    // 3. Parse the file content to find matching braces
    
    return startLine + estimatedLines;
}

std::string CodeContextProvider::getFileSnippet(
    const std::string& filePath,
    size_t startLine,
    size_t endLine)
{
    // This would typically use the workspace manager or a file reader
    // For simplicity, we'll assume we can read the file directly
    
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return "";
    }
    
    std::string line;
    std::stringstream content;
    size_t currentLine = 0;
    
    // Skip lines until we reach the start line
    while (currentLine < startLine && std::getline(file, line)) {
        currentLine++;
    }
    
    // Read lines until we reach the end line
    while (currentLine <= endLine && std::getline(file, line)) {
        content << line << "\n";
        currentLine++;
    }
    
    return content.str();
}

// Add method to register a custom symbol relevance scorer
void CodeContextProvider::registerSymbolRelevanceScorer(
    const std::string& name,
    std::function<float(const CodeSymbol&, const std::string&, size_t, size_t)> scorer)
{
    symbolScorers_[name] = scorer;
}

// Add method to register a custom file relevance scorer
void CodeContextProvider::registerFileRelevanceScorer(
    const std::string& name,
    std::function<float(const std::string&, const std::string&)> scorer)
{
    fileScorers_[name] = scorer;
}

float CodeContextProvider::calculateSymbolRelevance(
    const CodeSymbol& symbol,
    const std::string& currentFile,
    size_t line,
    size_t column)
{
    if (symbolScorers_.empty()) {
        // Default scoring if no custom scorers
        return 0.5f;
    }
    
    float totalScore = 0.0f;
    
    // Apply all registered scorers and average the results
    for (const auto& [name, scorer] : symbolScorers_) {
        float score = scorer(symbol, currentFile, line, column);
        totalScore += score;
    }
    
    return totalScore / symbolScorers_.size();
}

float CodeContextProvider::calculateFileRelevance(
    const std::string& filePath,
    const std::string& currentFile)
{
    if (fileScorers_.empty()) {
        // Default scoring if no custom scorers
        return filePath == currentFile ? 1.0f : 0.5f;
    }
    
    float totalScore = 0.0f;
    
    // Apply all registered scorers and average the results
    for (const auto& [name, scorer] : fileScorers_) {
        float score = scorer(filePath, currentFile);
        totalScore += score;
    }
    
    return totalScore / fileScorers_.size();
}

void CodeContextProvider::trimContextToTokenLimit(CodeContext& context, size_t maxTokens)
{
    // Estimate token count for the current content
    size_t totalTokens = 0;
    
    // Tokens for current symbol
    if (context.currentSymbol) {
        totalTokens += estimateTokenCount(getSymbolSummary(*context.currentSymbol));
    }
    
    // Tokens for selected text
    totalTokens += estimateTokenCount(context.selectedText);
    
    // Tokens for snippets
    size_t snippetTokens = 0;
    for (const auto& snippet : context.codeSnippets) {
        snippetTokens += estimateTokenCount(snippet.content);
    }
    
    // Tokens for knowledge entries
    size_t knowledgeTokens = 0;
    for (const auto& entry : context.knowledgeEntries) {
        knowledgeTokens += estimateTokenCount(entry.title + entry.content);
    }
    
    // If we're over the limit, start removing the least relevant snippets and knowledge entries
    if ((totalTokens + snippetTokens + knowledgeTokens) > maxTokens) {
        // Sort snippets by relevance (highest first)
        std::sort(context.codeSnippets.begin(), context.codeSnippets.end(),
            [](const CodeContext::ContextSnippet& a, const CodeContext::ContextSnippet& b) {
                return a.relevanceScore > b.relevanceScore;
            });
        
        // Sort knowledge entries by relevance (highest first)
        std::sort(context.knowledgeEntries.begin(), context.knowledgeEntries.end(),
            [](const KnowledgeEntry& a, const KnowledgeEntry& b) {
                return a.relevanceScore > b.relevanceScore;
            });
        
        // Calculate how much we need to reduce
        size_t excessTokens = (totalTokens + snippetTokens + knowledgeTokens) - maxTokens;
        
        // First reduce knowledge entries if they exist
        while (!context.knowledgeEntries.empty() && excessTokens > 0) {
            const auto& entry = context.knowledgeEntries.back();
            size_t entryTokens = estimateTokenCount(entry.title + entry.content);
            
            excessTokens = (entryTokens > excessTokens) ? 0 : (excessTokens - entryTokens);
            context.knowledgeEntries.pop_back();
        }
        
        // Then reduce code snippets if still needed
        while (!context.codeSnippets.empty() && excessTokens > 0) {
            const auto& snippet = context.codeSnippets.back();
            size_t snippetTokenCount = estimateTokenCount(snippet.content);
            
            excessTokens = (snippetTokenCount > excessTokens) ? 0 : (excessTokens - snippetTokenCount);
            context.codeSnippets.pop_back();
        }
    }
}

void CodeContextProvider::pruneSnippetsByRelevance(
    std::vector<CodeContext::ContextSnippet>& snippets,
    float minScore)
{
    // Erase snippets with relevance score below the minimum
    snippets.erase(
        std::remove_if(snippets.begin(), snippets.end(),
                      [minScore](const auto& snippet) {
                          return snippet.relevanceScore < minScore;
                      }),
        snippets.end());
    
    // Sort remaining snippets by relevance (highest first)
    std::sort(snippets.begin(), snippets.end(),
             [](const auto& a, const auto& b) {
                 return a.relevanceScore > b.relevanceScore;
             });
}

size_t CodeContextProvider::estimateTokenCount(const std::string& text)
{
    if (text.empty()) {
        return 0;
    }
    
    // Simple estimation: average token is ~4 characters
    // This is a rough approximation used by many token estimators
    const float avgCharsPerToken = 4.0f;
    
    return static_cast<size_t>(std::ceil(text.length() / avgCharsPerToken));
}

std::vector<std::string> CodeContextProvider::extractKeyTerms(const CodeContext& context)
{
    std::vector<std::string> terms;
    std::unordered_set<std::string> uniqueTerms;
    
    // Helper to add a term if it's not already in the set
    auto addTerm = [&](const std::string& term) {
        if (!term.empty() && uniqueTerms.find(term) == uniqueTerms.end()) {
            uniqueTerms.insert(term);
            terms.push_back(term);
        }
    };
    
    // Extract from current symbol
    if (context.currentSymbol) {
        addTerm(context.currentSymbol->name);
        
        // Add parts of the name if it contains camel case or underscores
        std::string name = context.currentSymbol->name;
        std::regex wordRegex("[A-Z][a-z0-9]+|[a-z0-9]+");
        
        std::sregex_iterator it(name.begin(), name.end(), wordRegex);
        std::sregex_iterator end;
        
        while (it != end) {
            std::string term = it->str();
            if (term.length() > 2) { // Ignore very short terms
                addTerm(term);
            }
            ++it;
        }
        
        // Add namespace
        if (!context.currentSymbol->namespace_.empty()) {
            addTerm(context.currentSymbol->namespace_);
        }
    }
    
    // Extract from selected text
    if (!context.selectedText.empty()) {
        std::regex wordRegex("[A-Za-z][A-Za-z0-9_]+");
        std::string text = context.selectedText;
        
        std::sregex_iterator it(text.begin(), text.end(), wordRegex);
        std::sregex_iterator end;
        
        while (it != end) {
            std::string term = it->str();
            if (term.length() > 2) { // Ignore very short terms
                addTerm(term);
            }
            ++it;
        }
    }
    
    // Extract from related symbols
    for (const auto& symbol : context.relatedSymbols) {
        addTerm(symbol.name);
    }
    
    return terms;
}

std::vector<std::string> CodeContextProvider::getImportantProjectFiles()
{
    if (!codebaseIndex_) {
        return {};
    }
    
    std::vector<std::string> importantFiles;
    
    // Get all files in the codebase
    auto allFiles = codebaseIndex_->getAllFiles();
    
    // Look for important files like:
    // - README files
    // - Build configuration files (CMakeLists.txt, package.json, etc.)
    // - Main entry point files (main.cpp, index.js, etc.)
    
    for (const auto& file : allFiles) {
        fs::path filePath(file.path);
        std::string filename = filePath.filename().string();
        std::string extension = filePath.extension().string();
        
        // Check for README files
        if (filename.find("README") != std::string::npos) {
            importantFiles.push_back(file.path);
        }
        // Check for build configuration files
        else if (filename == "CMakeLists.txt" || 
                 filename == "package.json" || 
                 filename == "Makefile" ||
                 filename == "build.gradle" ||
                 filename == "pom.xml") {
            importantFiles.push_back(file.path);
        }
        // Check for main entry points
        else if (filename == "main.cpp" || 
                 filename == "main.c" || 
                 filename == "main.java" ||
                 filename == "app.py" || 
                 filename == "index.js") {
            importantFiles.push_back(file.path);
        }
    }
    
    return importantFiles;
}

std::string CodeContextProvider::detectProjectLanguage()
{
    if (!codebaseIndex_) {
        return "unknown";
    }
    
    // Get all files in the codebase
    auto allFiles = codebaseIndex_->getAllFiles();
    
    // Count file extensions to determine the primary language
    std::unordered_map<std::string, int> extensionCounts;
    
    for (const auto& file : allFiles) {
        fs::path filePath(file.path);
        std::string extension = filePath.extension().string();
        
        // Convert to lowercase
        std::transform(extension.begin(), extension.end(), extension.begin(),
                       [](unsigned char c){ return std::tolower(c); });
        
        if (!extension.empty()) {
            // Remove the leading dot
            extension = extension.substr(1);
            extensionCounts[extension]++;
        }
    }
    
    // Find the most common extension
    std::string mostCommonExt;
    int maxCount = 0;
    
    for (const auto& [ext, count] : extensionCounts) {
        if (count > maxCount) {
            maxCount = count;
            mostCommonExt = ext;
        }
    }
    
    // Map extension to language name
    std::unordered_map<std::string, std::string> extensionToLanguage = {
        {"cpp", "C++"},
        {"hpp", "C++"},
        {"c", "C"},
        {"h", "C"},
        {"java", "Java"},
        {"py", "Python"},
        {"js", "JavaScript"},
        {"ts", "TypeScript"},
        {"html", "HTML"},
        {"css", "CSS"},
        {"rb", "Ruby"},
        {"go", "Go"},
        {"rs", "Rust"},
        {"php", "PHP"},
        {"cs", "C#"},
        {"swift", "Swift"},
        {"kt", "Kotlin"}
    };
    
    auto it = extensionToLanguage.find(mostCommonExt);
    if (it != extensionToLanguage.end()) {
        return it->second;
    }
    
    // If no mapping found, return the extension
    return mostCommonExt.empty() ? "unknown" : mostCommonExt;
}

std::vector<std::string> CodeContextProvider::getProjectDependencies()
{
    std::vector<std::string> dependencies;
    
    if (!codebaseIndex_) {
        return dependencies;
    }
    
    // Get all files in the codebase
    auto allFiles = codebaseIndex_->getAllFiles();
    
    // Look for dependency files
    for (const auto& file : allFiles) {
        fs::path filePath(file.path);
        std::string filename = filePath.filename().string();
        
        if (filename == "package.json" || 
            filename == "requirements.txt" || 
            filename == "Cargo.toml" ||
            filename == "pom.xml" ||
            filename == "build.gradle") {
            
            // Read the file to extract dependencies
            std::ifstream f(file.path);
            if (f.is_open()) {
                std::string content((std::istreambuf_iterator<char>(f)),
                                   std::istreambuf_iterator<char>());
                
                // This is a simplified approach - in a real implementation,
                // we would parse the specific file format to extract dependencies
                
                // For now, just indicate we found a dependency file
                dependencies.push_back("Found dependency file: " + filename);
            }
        }
    }
    
    return dependencies;
}

std::vector<KnowledgeEntry> CodeContextProvider::findRelevantKnowledgeEntries(
    const CodeContext& context,
    const ContextOptions& options) {
    
    if (!knowledgeBase_) {
        return {};
    }
    
    // Extract key terms from the context
    std::vector<std::string> contextTerms = extractKeyTerms(context);
    
    // Query the knowledge base for relevant entries
    std::vector<KnowledgeEntry> entries;
    if (options.knowledgeCategory.has_value()) {
        // Filter by specific category if provided
        entries = knowledgeBase_->findByCategory(options.knowledgeCategory.value(), options.maxKnowledgeEntries);
    } else {
        // Find entries relevant to the current context
        entries = knowledgeBase_->findRelevantForContext(contextTerms, options.maxKnowledgeEntries);
    }
    
    // Return entries limited by the max entries setting
    return entries.size() <= options.maxKnowledgeEntries 
        ? entries 
        : std::vector<KnowledgeEntry>(entries.begin(), entries.begin() + options.maxKnowledgeEntries);
}

} // namespace ai_editor 