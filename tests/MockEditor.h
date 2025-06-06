#ifndef MOCK_EDITOR_H
#define MOCK_EDITOR_H

#include <string>
#include <vector>
#include <memory>
#include <gmock/gmock.h>
#include "IEditorTestInterface.hpp"

// Mock implementation of IEditor using Google Mock
class MockEditor : public IEditorTestInterface {
public:
    virtual ~MockEditor() = default;

    // File Operations
    MOCK_METHOD(bool, openFile, (const std::string&), (override));
    MOCK_METHOD(bool, saveFile, (), (override));
    MOCK_METHOD(bool, saveFileAs, (const std::string&), (override));
    MOCK_METHOD(bool, isModified, (), (const, override));
    MOCK_METHOD(void, setModified, (bool), (override));
    MOCK_METHOD(std::string, getFilename, (), (const, override));
    MOCK_METHOD(void, setFilename, (const std::string&), (override));

    // Cursor Management
    MOCK_METHOD(void, setCursor, (size_t, size_t), (override));
    MOCK_METHOD(size_t, getCursorLine, (), (const, override));
    MOCK_METHOD(size_t, getCursorCol, (), (const, override));

    // Cursor Movement
    MOCK_METHOD(void, moveCursorUp, (), (override));
    MOCK_METHOD(void, moveCursorDown, (), (override));
    MOCK_METHOD(void, moveCursorLeft, (), (override));
    MOCK_METHOD(void, moveCursorRight, (), (override));
    MOCK_METHOD(void, moveCursorToLineStart, (), (override));
    MOCK_METHOD(void, moveCursorToLineEnd, (), (override));
    MOCK_METHOD(void, moveCursorToBufferStart, (), (override));
    MOCK_METHOD(void, moveCursorToBufferEnd, (), (override));

    // Text Editing
    MOCK_METHOD(void, typeText, (const std::string&), (override));
    MOCK_METHOD(void, newLine, (), (override));

    // Selection
    MOCK_METHOD(bool, hasSelection, (), (const, override));
    MOCK_METHOD(void, clearSelection, (), (override));
    MOCK_METHOD(std::string, getSelectedText, (), (const, override));
    MOCK_METHOD(void, setSelectionRange, (size_t, size_t, size_t, size_t), (override));

    // Clipboard
    MOCK_METHOD(void, setClipboardText, (const std::string&), (override));
};

#endif // MOCK_EDITOR_H
