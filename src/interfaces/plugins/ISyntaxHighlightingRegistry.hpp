#pragma once

#include <string>
#include <vector>
#include <memory>

namespace ai_editor {

// Forward declarations
class TextBuffer;

/**
 * @struct SyntaxStyle
 * @brief Represents the styling information for a segment of text.
 */
struct SyntaxStyle {
    size_t startPos;       ///< Starting position in the line
    size_t length;         ///< Length of the styled segment
    uint32_t colorRGBA;    ///< Color in RGBA format
    bool isBold;           ///< Whether the text should be rendered in bold
    bool isItalic;         ///< Whether the text should be rendered in italic
    bool isUnderlined;     ///< Whether the text should be underlined
};

/**
 * @class SyntaxHighlighter
 * @brief Interface for syntax highlighter implementations.
 * 
 * This class defines the interface that all syntax highlighters must implement.
 * Syntax highlighters are responsible for providing styling information for text
 * based on the language syntax.
 */
class SyntaxHighlighter {
public:
    virtual ~SyntaxHighlighter() = default;
    
    /**
     * @brief Highlight a single line of text.
     * @param line The text line to highlight.
     * @param lineIndex The index of the line in the buffer.
     * @return A vector of SyntaxStyle objects describing the styling for segments of the line.
     */
    virtual std::unique_ptr<std::vector<SyntaxStyle>> highlightLine(
        const std::string& line, size_t lineIndex) const = 0;
    
    /**
     * @brief Highlight an entire text buffer.
     * @param buffer The text buffer to highlight.
     * @return A vector of vectors of SyntaxStyle objects, one vector per line.
     */
    virtual std::vector<std::vector<SyntaxStyle>> highlightBuffer(
        const TextBuffer& buffer) const = 0;
    
    /**
     * @brief Get the list of file extensions supported by this highlighter.
     * @return A vector of supported file extensions (without the leading dot).
     */
    virtual std::vector<std::string> getSupportedExtensions() const = 0;
    
    /**
     * @brief Get the name of the language this highlighter is for.
     * @return The language name, which is also used as the highlighter ID.
     */
    virtual std::string getLanguageName() const = 0;
};

/**
 * @class ISyntaxHighlightingRegistry
 * @brief Interface for a registry of syntax highlighters.
 * 
 * This registry manages syntax highlighters for different languages and provides
 * methods to register highlighters and retrieve them based on file extensions.
 */
class ISyntaxHighlightingRegistry {
public:
    virtual ~ISyntaxHighlightingRegistry() = default;
    
    /**
     * @brief Register a syntax highlighter for one or more file extensions.
     * @param highlighter The syntax highlighter to register.
     * @param fileExtensions The file extensions this highlighter should be associated with.
     * @return True if registration was successful, false otherwise.
     */
    virtual bool registerHighlighter(
        std::shared_ptr<SyntaxHighlighter> highlighter, 
        const std::vector<std::string>& fileExtensions) = 0;
    
    /**
     * @brief Unregister a syntax highlighter by its ID.
     * @param highlighterId The ID of the highlighter to unregister.
     * @return True if unregistration was successful, false otherwise.
     */
    virtual bool unregisterHighlighter(const std::string& highlighterId) = 0;
    
    /**
     * @brief Get a highlighter by its ID.
     * @param highlighterId The ID of the highlighter to retrieve.
     * @return A shared pointer to the highlighter, or nullptr if not found.
     */
    virtual std::shared_ptr<SyntaxHighlighter> getHighlighter(
        const std::string& highlighterId) = 0;
    
    /**
     * @brief Get a highlighter for a specific file extension.
     * @param fileExtension The file extension (without the leading dot).
     * @return A shared pointer to the appropriate highlighter, or nullptr if not found.
     */
    virtual std::shared_ptr<SyntaxHighlighter> getHighlighterForExtension(
        const std::string& fileExtension) = 0;
    
    /**
     * @brief Check if a highlighter exists for a specific file extension.
     * @param fileExtension The file extension (without the leading dot).
     * @return True if a highlighter exists for the extension, false otherwise.
     */
    virtual bool hasHighlighterForExtension(const std::string& fileExtension) = 0;
    
    /**
     * @brief Get all registered highlighter IDs.
     * @return A vector of highlighter IDs.
     */
    virtual std::vector<std::string> getAllHighlighterIds() = 0;
    
    /**
     * @brief Get all supported file extensions.
     * @return A vector of supported file extensions.
     */
    virtual std::vector<std::string> getSupportedFileExtensions() = 0;
};

} // namespace ai_editor 