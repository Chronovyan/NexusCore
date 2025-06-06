#ifndef IEDITOR_TEST_INTERFACE_HPP
#define IEDITOR_TEST_INTERFACE_HPP

#include <string>
#include <vector>

// Simplified IEditor interface for testing
class IEditorTestInterface {
public:
    virtual ~IEditorTestInterface() = default;

    // Basic operations
    virtual bool openFile(const std::string& filename) = 0;
    virtual bool saveFile() = 0;
    virtual bool saveFileAs(const std::string& filename) = 0;
    virtual bool isModified() const = 0;
    virtual void setModified(bool modified) = 0;
    virtual std::string getFilename() const = 0;
    virtual void setFilename(const std::string& filename) = 0;

    // Cursor management
    virtual void setCursor(size_t line, size_t col) = 0;
    virtual size_t getCursorLine() const = 0;
    virtual size_t getCursorCol() const = 0;

    // Cursor movement
    virtual void moveCursorUp() = 0;
    virtual void moveCursorDown() = 0;
    virtual void moveCursorLeft() = 0;
    virtual void moveCursorRight() = 0;
    virtual void moveCursorToLineStart() = 0;
    virtual void moveCursorToLineEnd() = 0;
    virtual void moveCursorToBufferStart() = 0;
    virtual void moveCursorToBufferEnd() = 0;

    // Text editing
    virtual void typeText(const std::string& text) = 0;
    virtual void newLine() = 0;

    // Selection
    virtual bool hasSelection() const = 0;
    virtual void clearSelection() = 0;
    virtual std::string getSelectedText() const = 0;
    virtual void setSelectionRange(size_t startLine, size_t startCol, size_t endLine, size_t endCol) = 0;

    // Clipboard
    virtual void setClipboardText(const std::string& text) = 0;
};

#endif // IEDITOR_TEST_INTERFACE_HPP
