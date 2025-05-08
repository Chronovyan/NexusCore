#ifndef EDITOR_TESTABLE_FIXED_H
#define EDITOR_TESTABLE_FIXED_H

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <functional>
#include "../src/Editor.h"

// Testable editor interface that wraps the main loop
class EditorTestableFixed {
public:
    // Run the editor with predefined inputs and return the result
    static bool runWithInputs(const std::vector<std::string>& inputs, std::string& output) {
        Editor editor;
        std::stringstream outputStream;
        
        outputStream << "--- Mini C++ Text Editor --- (type 'help' for commands)" << '\n';
        
        // Process each input line
        for (const auto& inputLine : inputs) {
            outputStream << "> " << inputLine << "\n";
            
            if (inputLine.empty()) {
                continue;
            }
            
            std::stringstream ss(inputLine);
            std::string command;
            ss >> command;
            
            try {
                // Check for quit command first
                if (command == "quit" || command == "exit") {
                    outputStream << "Exiting editor." << '\n';
                    break; // Stop processing more commands
                }

                processCommand(editor, command, ss, outputStream);
            } catch (const std::out_of_range& e) {
                outputStream << "Error: " << e.what() << '\n';
            } catch (const std::exception& e) {
                outputStream << "An unexpected error occurred: " << e.what() << '\n';
            }
        }
        
        output = outputStream.str();
        return true;
    }
    
    // Run editor with inputs and perform checkpoints at specified indices
    static bool runWithCheckpoints(
        const std::vector<std::string>& inputs, 
        const std::vector<std::pair<size_t, std::function<void(const Editor&)>>>& checkpoints,
        std::string& output) 
    {
        Editor editor;
        std::stringstream outputStream;
        
        outputStream << "--- Mini C++ Text Editor --- (type 'help' for commands)" << '\n';
        
        // Process each input line
        for (size_t i = 0; i < inputs.size(); ++i) {
            const auto& inputLine = inputs[i];
            outputStream << "> " << inputLine << "\n";
            
            if (inputLine.empty()) {
                continue;
            }
            
            std::stringstream ss(inputLine);
            std::string command;
            ss >> command;
            
            try {
                // Check for quit command first
                if (command == "quit" || command == "exit") {
                    outputStream << "Exiting editor." << '\n';
                    break; // Stop processing more commands
                }

                processCommand(editor, command, ss, outputStream);
            } catch (const std::out_of_range& e) {
                outputStream << "Error: " << e.what() << '\n';
            } catch (const std::exception& e) {
                outputStream << "An unexpected error occurred: " << e.what() << '\n';
            }
            
            // Execute checkpoints for this index
            for (const auto& checkpoint : checkpoints) {
                if (checkpoint.first == i) {
                    checkpoint.second(editor);
                }
            }
        }
        
        output = outputStream.str();
        return true;
    }
    
private:
    // Helper function to get the rest of the line from stringstream
    static std::string getRestOfLine(std::stringstream& ss) {
        std::string remaining_text;
        // Consume the first space after the command/index if any
        if (ss.peek() == ' ') {
            ss.ignore();
        }
        std::getline(ss, remaining_text);
        return remaining_text;
    }
    
    // Process a single command (copied from main.cpp with modifications for testing)
    static void processCommand(Editor& editor, const std::string& command, 
                              std::stringstream& ss, std::stringstream& out) 
    {
        if (command == "add") {
            std::string text_to_add = getRestOfLine(ss);
            editor.addLine(text_to_add);
            out << "Line added." << '\n';
        } else if (command == "insert") {
            size_t index;
            if (!(ss >> index)) {
                out << "Error: Missing index for insert." << '\n';
                out << "Usage: insert <index> <text>" << '\n';
                return;
            }
            std::string text_to_insert = getRestOfLine(ss);
            editor.insertLine(index, text_to_insert);
            out << "Line inserted at " << index << "." << '\n';
        } else if (command == "delete") {
            size_t index;
            if (!(ss >> index)) {
                out << "Error: Missing index for delete." << '\n';
                out << "Usage: delete <index>" << '\n';
                return;
            }
            editor.deleteLine(index);
            out << "Line " << index << " deleted." << '\n';
        } else if (command == "replace") {
            size_t index;
            if (!(ss >> index)) {
                out << "Error: Missing index for replace." << '\n';
                out << "Usage: replace <index> <text>" << '\n';
                return;
            }
            std::string text_to_replace = getRestOfLine(ss);
            editor.replaceLine(index, text_to_replace);
            out << "Line " << index << " replaced." << '\n';
        } else if (command == "view") {
            out << "--- Buffer View ---" << '\n';
            editor.printView(out);
            out << "-------------------" << '\n';
        } else if (command == "lines") {
            out << "Total lines: " << editor.getBuffer().lineCount() << '\n';
        } else if (command == "clear") {
            TextBuffer& buf = editor.getBuffer();
            while(!buf.isEmpty()){
                buf.deleteLine(0);
            }
            if (buf.isEmpty()) buf.addLine(""); 
            editor.setCursor(0,0);
            out << "Buffer cleared. Cursor reset to [0,0]." << '\n';
        } else if (command == "save") {
            std::string filename;
            if (!(ss >> filename)) {
                out << "Error: Missing filename for save." << '\n';
                out << "Usage: save <filename>" << '\n';
                return;
            }
            // For testing, we'll just simulate the save rather than actually writing to a file
            out << "Buffer saved to " << filename << "." << '\n';
        } else if (command == "load") {
            std::string filename;
            if (!(ss >> filename)) {
                out << "Error: Missing filename for load." << '\n';
                out << "Usage: load <filename>" << '\n';
                return;
            }
            // For testing, we'll just simulate the load rather than actually reading from a file
            out << "Simulated load from " << filename << ". (For testing only)" << '\n';
        } else if (command == "cursor") {
            out << "Cursor at: [" << editor.getCursorLine() << ", " << editor.getCursorCol() << "]" << '\n';
        } else if (command == "setcursor") {
            size_t r_line, r_col;
            if (!(ss >> r_line >> r_col)) {
                out << "Error: Missing line and column for setcursor." << '\n';
                out << "Usage: setcursor <line> <col>" << '\n';
                return;
            }
            editor.setCursor(r_line, r_col);
            out << "Cursor set to: [" << editor.getCursorLine() << ", " << editor.getCursorCol() << "]" << '\n';
        } else if (command == "cu") {
            editor.moveCursorUp();
            out << "Cursor at: [" << editor.getCursorLine() << ", " << editor.getCursorCol() << "]" << '\n';
        } else if (command == "cd") {
            editor.moveCursorDown();
            out << "Cursor at: [" << editor.getCursorLine() << ", " << editor.getCursorCol() << "]" << '\n';
        } else if (command == "cl") {
            editor.moveCursorLeft();
            out << "Cursor at: [" << editor.getCursorLine() << ", " << editor.getCursorCol() << "]" << '\n';
        } else if (command == "cr") {
            editor.moveCursorRight();
            out << "Cursor at: [" << editor.getCursorLine() << ", " << editor.getCursorCol() << "]" << '\n';
        } else if (command == "home") {
            editor.moveCursorToLineStart();
            out << "Cursor at: [" << editor.getCursorLine() << ", " << editor.getCursorCol() << "]" << '\n';
        } else if (command == "end") {
            editor.moveCursorToLineEnd();
            out << "Cursor at: [" << editor.getCursorLine() << ", " << editor.getCursorCol() << "]" << '\n';
        } else if (command == "top") {
            editor.moveCursorToBufferStart();
            out << "Cursor at: [" << editor.getCursorLine() << ", " << editor.getCursorCol() << "]" << '\n';
        } else if (command == "bottom") {
            editor.moveCursorToBufferEnd();
            out << "Cursor at: [" << editor.getCursorLine() << ", " << editor.getCursorCol() << "]" << '\n';
        } else if (command == "nextword") {
            editor.moveCursorToNextWord();
            out << "Cursor at: [" << editor.getCursorLine() << ", " << editor.getCursorCol() << "]" << '\n';
        } else if (command == "prevword") {
            editor.moveCursorToPrevWord();
            out << "Cursor at: [" << editor.getCursorLine() << ", " << editor.getCursorCol() << "]" << '\n';
        } else if (command == "type") {
            std::string textToType = getRestOfLine(ss);
            if (textToType.empty()) {
                out << "Error: Missing text for 'type' command." << '\n';
                out << "Usage: type <text>" << '\n';
                return;
            }
            editor.typeText(textToType);
            out << "Text inserted. Cursor at: [" << editor.getCursorLine() << ", " << editor.getCursorCol() << "]" << '\n';
        } else if (command == "backspace") {
            editor.backspace();
            out << "Backspace performed. Cursor at: [" << editor.getCursorLine() << ", " << editor.getCursorCol() << "]" << '\n';
        } else if (command == "del") {
            editor.deleteForward();
            out << "Delete performed. Cursor at: [" << editor.getCursorLine() << ", " << editor.getCursorCol() << "]" << '\n';
        } else if (command == "newline") {
            editor.newLine();
            out << "Line split. Cursor at: [" << editor.getCursorLine() << ", " << editor.getCursorCol() << "]" << '\n';
        } else if (command == "join") {
            editor.joinWithNextLine();
            out << "Lines joined. Cursor at: [" << editor.getCursorLine() << ", " << editor.getCursorCol() << "]" << '\n';
        } else if (command == "selstart") {
            editor.setSelectionStart();
            out << "Selection started at: [" << editor.getCursorLine() << ", " << editor.getCursorCol() << "]" << '\n';
        } else if (command == "selend") {
            editor.setSelectionEnd();
            out << "Selection ended at: [" << editor.getCursorLine() << ", " << editor.getCursorCol() << "]" << '\n';
        } else if (command == "selclear") {
            editor.clearSelection();
            out << "Selection cleared." << '\n';
        } else if (command == "selshow") {
            if (editor.hasSelection()) {
                out << "Selected text: \"" << editor.getSelectedText() << "\"" << '\n';
            } else {
                out << "No active selection." << '\n';
            }
        } else if (command == "cut") {
            if (editor.hasSelection()) {
                editor.cutSelectedText();
                out << "Text cut. Cursor at: [" << editor.getCursorLine() << ", " << editor.getCursorCol() << "]" << '\n';
            } else {
                out << "No active selection to cut." << '\n';
            }
        } else if (command == "copy") {
            if (editor.hasSelection()) {
                editor.copySelectedText();
                out << "Text copied." << '\n';
            } else {
                out << "No active selection to copy." << '\n';
            }
        } else if (command == "paste") {
            editor.pasteText();
            out << "Text pasted. Cursor at: [" << editor.getCursorLine() << ", " << editor.getCursorCol() << "]" << '\n';
        } else if (command == "delword") {
            editor.deleteWord();
            out << "Word deleted. Cursor at: [" << editor.getCursorLine() << ", " << editor.getCursorCol() << "]" << '\n';
        } else if (command == "selword") {
            editor.selectWord();
            if (editor.hasSelection()) {
                out << "Word selected: \"" << editor.getSelectedText() << "\"" << '\n';
            } else {
                out << "No word at cursor position to select." << '\n';
            }
        } else if (command == "undo") {
            if (editor.undo()) {
                out << "Action undone." << '\n';
            } else {
                out << "Nothing to undo." << '\n';
            }
        } else if (command == "redo") {
            if (editor.redo()) {
                out << "Action redone." << '\n';
            } else {
                out << "Nothing to redo." << '\n';
            }
        } else if (command == "help") {
            out << "[Help message displayed - truncated for tests]" << '\n';
        } else if (command == "quit" || command == "exit") {
            // Handle these directly in runWithInputs and runWithCheckpoints instead
            out << "Exiting editor..." << '\n';
        } else {
            out << "Unknown command: " << command << ". Type 'help' for a list of commands." << '\n';
        }
    }
};

#endif // EDITOR_TESTABLE_FIXED_H 