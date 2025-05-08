#include <iostream>
#include <string>
#include <vector>
#include <sstream> // Required for std::stringstream
#include <limits>  // Required for std::numeric_limits
#include "Editor.h" // Include Editor instead

// Helper function to print available commands
void printHelp() {
    std::cout << "Available commands:" << '\n';
    std::cout << "  add <text>                - Adds text as a new line at the end." << '\n';
    std::cout << "  insert <index> <text>     - Inserts text at the given 0-based line index." << '\n';
    std::cout << "  delete <index>            - Deletes the line at the given 0-based index." << '\n';
    std::cout << "  replace <index> <text>    - Replaces the line at the given 0-based index." << '\n';
    std::cout << "  view                      - Prints the entire buffer with cursor position." << '\n';
    std::cout << "  lines                     - Shows the current number of lines." << '\n';
    std::cout << "  clear                     - Clears all lines from the buffer (resets cursor to 0,0)." << '\n';
    std::cout << "  save <filename>           - Saves the buffer content to a file." << '\n';
    std::cout << "  load <filename>           - Loads content from a file (clears existing, resets cursor)." << '\n';
    std::cout << "  cursor                    - Shows current cursor [line, col]." << '\n';
    std::cout << "  setcursor <line> <col>    - Sets cursor to [line, col]." << '\n';
    std::cout << "  cu                        - Moves cursor Up." << '\n';
    std::cout << "  cd                        - Moves cursor Down." << '\n';
    std::cout << "  cl                        - Moves cursor Left." << '\n';
    std::cout << "  cr                        - Moves cursor Right." << '\n';
    std::cout << "  home                      - Moves cursor to start of line." << '\n';
    std::cout << "  end                       - Moves cursor to end of line." << '\n';
    std::cout << "  top                       - Moves cursor to start of buffer." << '\n';
    std::cout << "  bottom                    - Moves cursor to end of buffer." << '\n';
    std::cout << "  nextword                  - Moves cursor to next word." << '\n';
    std::cout << "  prevword                  - Moves cursor to previous word." << '\n';
    std::cout << "  type <text>               - Inserts text at the cursor position." << '\n';
    std::cout << "  backspace                 - Deletes the character before the cursor." << '\n';
    std::cout << "  del                       - Deletes the character at the cursor position." << '\n';
    std::cout << "  newline                   - Inserts a line break at the cursor position." << '\n';
    std::cout << "  join                      - Joins the current line with the next line." << '\n';
    std::cout << "  selstart                  - Starts text selection at current cursor position." << '\n';
    std::cout << "  selend                    - Ends text selection at current cursor position." << '\n';
    std::cout << "  selclear                  - Clears current selection." << '\n';
    std::cout << "  selshow                   - Shows selected text." << '\n';
    std::cout << "  cut                       - Cuts selected text to clipboard." << '\n';
    std::cout << "  copy                      - Copies selected text to clipboard." << '\n';
    std::cout << "  paste                     - Pastes clipboard content at cursor position." << '\n';
    std::cout << "  delword                   - Deletes word at cursor position." << '\n';
    std::cout << "  selword                   - Selects word at cursor position." << '\n';
    std::cout << "  help                      - Shows this help message." << '\n';
    std::cout << "  quit / exit               - Exits the editor." << '\n';
    std::cout << "---------------------------------------------------------------------" << '\n';
}

// Helper function to get the rest of the line from stringstream
std::string getRestOfLine(std::stringstream& ss) {
    std::string remaining_text;
    std::string part;
    // Consume the first space after the command/index if any, before reading text
    if (ss.peek() == ' ') {
        ss.ignore();
    }
    std::getline(ss, remaining_text);
    return remaining_text;
}

int main() {
    Editor editor; // Use Editor instead of TextBuffer
    std::string line;
    bool running = true;

    std::cout << "--- Mini C++ Text Editor --- (type 'help' for commands)" << '\n';

    while (running) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) {
            // Handle EOF (e.g., Ctrl+D on Linux, Ctrl+Z then Enter on Windows)
            if (std::cin.eof()) {
                std::cout << "EOF detected. Exiting." << std::endl;
                break; 
            }
            // Other stream errors
            std::cout << "Error reading input. Exiting." << std::endl;
            break;
        }

        if (line.empty()) {
            continue;
        }

        std::stringstream ss(line);
        std::string command;
        ss >> command;

        try {
            if (command == "add") {
                std::string text_to_add = getRestOfLine(ss);
                editor.addLine(text_to_add); // Call editor method
                std::cout << "Line added." << '\n';
            } else if (command == "insert") {
                size_t index;
                if (!(ss >> index)) {
                    std::cerr << "Error: Missing index for insert." << '\n';
                    std::cerr << "Usage: insert <index> <text>" << '\n';
                    continue;
                }
                std::string text_to_insert = getRestOfLine(ss);
                editor.insertLine(index, text_to_insert); // Call editor method
                std::cout << "Line inserted at " << index << "." << '\n';
            } else if (command == "delete") {
                size_t index;
                if (!(ss >> index)) {
                    std::cerr << "Error: Missing index for delete." << '\n';
                    std::cerr << "Usage: delete <index>" << '\n';
                    continue;
                }
                editor.deleteLine(index); // Call editor method
                std::cout << "Line " << index << " deleted." << '\n';
            } else if (command == "replace") {
                size_t index;
                if (!(ss >> index)) {
                    std::cerr << "Error: Missing index for replace." << '\n';
                    std::cerr << "Usage: replace <index> <text>" << '\n';
                    continue;
                }
                std::string text_to_replace = getRestOfLine(ss);
                editor.replaceLine(index, text_to_replace); // Call editor method
                std::cout << "Line " << index << " replaced." << '\n';
            } else if (command == "view") { // Changed from "print" to "view"
                std::cout << "--- Buffer View ---" << '\n';
                editor.printView(std::cout); // Use editor's view method
                std::cout << "-------------------" << '\n';
            } else if (command == "lines") {
                std::cout << "Total lines: " << editor.getBuffer().lineCount() << '\n';
            } else if (command == "clear") {
                TextBuffer& buf = editor.getBuffer();
                while(!buf.isEmpty()){
                    buf.deleteLine(0);
                }
                // Ensure buffer is not entirely empty for cursor validity, and reset cursor
                if (buf.isEmpty()) buf.addLine(""); 
                editor.setCursor(0,0);
                std::cout << "Buffer cleared. Cursor reset to [0,0]." << '\n';
            } else if (command == "save") {
                std::string filename;
                if (!(ss >> filename)) {
                    std::cerr << "Error: Missing filename for save." << '\n';
                    std::cerr << "Usage: save <filename>" << '\n';
                    continue;
                }
                if (editor.getBuffer().saveToFile(filename)) {
                    std::cout << "Buffer saved to " << filename << "." << '\n';
                } else {
                    std::cout << "Failed to save buffer to " << filename << "." << '\n';
                }
            } else if (command == "load") {
                std::string filename;
                if (!(ss >> filename)) {
                    std::cerr << "Error: Missing filename for load." << '\n';
                    std::cerr << "Usage: load <filename>" << '\n';
                    continue;
                }
                if (editor.getBuffer().loadFromFile(filename)) {
                    // After loading, ensure buffer is not empty and reset cursor
                    if(editor.getBuffer().isEmpty()) editor.getBuffer().addLine("");
                    editor.setCursor(0,0);
                    std::cout << "Buffer loaded from " << filename << ". Cursor reset to [0,0]." << '\n';
                } else {
                    std::cout << "Failed to load buffer from " << filename << "." << '\n';
                }
            } else if (command == "cursor") {
                 std::cout << "Cursor at: [" << editor.getCursorLine() << ", " << editor.getCursorCol() << "]" << '\n';
            } else if (command == "setcursor") {
                size_t r_line, r_col;
                if (!(ss >> r_line >> r_col)) {
                    std::cerr << "Error: Missing line and column for setcursor." << '\n';
                    std::cerr << "Usage: setcursor <line> <col>" << '\n';
                    continue;
                }
                editor.setCursor(r_line, r_col);
                std::cout << "Cursor set to: [" << editor.getCursorLine() << ", " << editor.getCursorCol() << "] (clamped if necessary)" << '\n';
            } else if (command == "cu") {
                editor.moveCursorUp();
                std::cout << "Cursor at: [" << editor.getCursorLine() << ", " << editor.getCursorCol() << "]" << '\n';
            } else if (command == "cd") {
                editor.moveCursorDown();
                std::cout << "Cursor at: [" << editor.getCursorLine() << ", " << editor.getCursorCol() << "]" << '\n';
            } else if (command == "cl") {
                editor.moveCursorLeft();
                std::cout << "Cursor at: [" << editor.getCursorLine() << ", " << editor.getCursorCol() << "]" << '\n';
            } else if (command == "cr") {
                editor.moveCursorRight();
                std::cout << "Cursor at: [" << editor.getCursorLine() << ", " << editor.getCursorCol() << "]" << '\n';
            } else if (command == "home") {
                editor.moveCursorToLineStart();
                std::cout << "Cursor at: [" << editor.getCursorLine() << ", " << editor.getCursorCol() << "]" << '\n';
            } else if (command == "end") {
                editor.moveCursorToLineEnd();
                std::cout << "Cursor at: [" << editor.getCursorLine() << ", " << editor.getCursorCol() << "]" << '\n';
            } else if (command == "top") {
                editor.moveCursorToBufferStart();
                std::cout << "Cursor at: [" << editor.getCursorLine() << ", " << editor.getCursorCol() << "]" << '\n';
            } else if (command == "bottom") {
                editor.moveCursorToBufferEnd();
                std::cout << "Cursor at: [" << editor.getCursorLine() << ", " << editor.getCursorCol() << "]" << '\n';
            } else if (command == "nextword") {
                editor.moveCursorToNextWord();
                std::cout << "Cursor at: [" << editor.getCursorLine() << ", " << editor.getCursorCol() << "]" << '\n';
            } else if (command == "prevword") {
                editor.moveCursorToPrevWord();
                std::cout << "Cursor at: [" << editor.getCursorLine() << ", " << editor.getCursorCol() << "]" << '\n';
            } else if (command == "type") {
                std::string textToType = getRestOfLine(ss);
                if (textToType.empty()) {
                    std::cerr << "Error: Missing text for 'type' command." << '\n';
                    std::cerr << "Usage: type <text>" << '\n';
                    continue;
                }
                editor.typeText(textToType);
                std::cout << "Text inserted. Cursor at: [" << editor.getCursorLine() << ", " << editor.getCursorCol() << "]" << '\n';
            } else if (command == "backspace") {
                editor.backspace();
                std::cout << "Backspace performed. Cursor at: [" << editor.getCursorLine() << ", " << editor.getCursorCol() << "]" << '\n';
            } else if (command == "del") {
                editor.deleteForward();
                std::cout << "Delete performed. Cursor at: [" << editor.getCursorLine() << ", " << editor.getCursorCol() << "]" << '\n';
            } else if (command == "newline") {
                editor.newLine();
                std::cout << "Line split. Cursor at: [" << editor.getCursorLine() << ", " << editor.getCursorCol() << "]" << '\n';
            } else if (command == "join") {
                editor.joinWithNextLine();
                std::cout << "Lines joined. Cursor at: [" << editor.getCursorLine() << ", " << editor.getCursorCol() << "]" << '\n';
            } else if (command == "selstart") {
                editor.setSelectionStart();
                std::cout << "Selection started at: [" << editor.getCursorLine() << ", " << editor.getCursorCol() << "]" << '\n';
            } else if (command == "selend") {
                editor.setSelectionEnd();
                std::cout << "Selection ended at: [" << editor.getCursorLine() << ", " << editor.getCursorCol() << "]" << '\n';
            } else if (command == "selclear") {
                editor.clearSelection();
                std::cout << "Selection cleared." << '\n';
            } else if (command == "selshow") {
                if (editor.hasSelection()) {
                    std::cout << "Selected text: \"" << editor.getSelectedText() << "\"" << '\n';
                } else {
                    std::cout << "No active selection." << '\n';
                }
            } else if (command == "cut") {
                if (editor.hasSelection()) {
                    editor.cutSelectedText();
                    std::cout << "Text cut. Cursor at: [" << editor.getCursorLine() << ", " << editor.getCursorCol() << "]" << '\n';
                } else {
                    std::cout << "No active selection to cut." << '\n';
                }
            } else if (command == "copy") {
                if (editor.hasSelection()) {
                    editor.copySelectedText();
                    std::cout << "Text copied." << '\n';
                } else {
                    std::cout << "No active selection to copy." << '\n';
                }
            } else if (command == "paste") {
                editor.pasteText();
                std::cout << "Text pasted. Cursor at: [" << editor.getCursorLine() << ", " << editor.getCursorCol() << "]" << '\n';
            } else if (command == "delword") {
                editor.deleteWord();
                std::cout << "Word deleted. Cursor at: [" << editor.getCursorLine() << ", " << editor.getCursorCol() << "]" << '\n';
            } else if (command == "selword") {
                editor.selectWord();
                if (editor.hasSelection()) {
                    std::cout << "Word selected: \"" << editor.getSelectedText() << "\"" << '\n';
                } else {
                    std::cout << "No word at cursor position to select." << '\n';
                }
            } else if (command == "help") {
                printHelp();
            } else if (command == "quit" || command == "exit") {
                std::cout << "Exiting editor." << '\n';
                running = false;
            } else {
                std::cerr << "Unknown command: " << command << ". Type 'help' for a list of commands." << '\n';
            }
        } catch (const std::out_of_range& e) {
            std::cerr << "Error: " << e.what() << '\n';
            // After an error (e.g. bad index), it might be good to ensure the cursor is still valid.
            // editor.setCursor(editor.getCursorLine(), editor.getCursorCol()); // This just re-validates current
        } catch (const std::exception& e) {
            std::cerr << "An unexpected error occurred: " << e.what() << '\n';
        }
    }

    return 0;
} 