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
    std::cout << "  undo                      - Undoes the last action." << '\n';
    std::cout << "  redo                      - Redoes the last undone action." << '\n';
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

int main(int argc, char* argv[]) {
    Editor editor;
    if (argc > 1) {
        editor.openFile(argv[1]);
    }

    std::string line;
    editor.printView(std::cout); // Initial view

    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) {
            if (std::cin.eof()) {
                std::cout << "Exiting due to EOF." << std::endl;
            } else {
                std::cerr << "Error reading input." << std::endl;
            }
            break; // Exit on EOF or stream error
        }

        if (line.empty()) {
            continue; // Skip processing for empty line and re-prompt
        }

        if (line == "exit" || line == "quit") {
            if (editor.isModified()) {
                std::cout << "File has unsaved changes. Save before exiting? (yes/no/cancel): ";
                std::string response;
                if (!std::getline(std::cin, response)) {
                    break; // EOF or error, treat as cancel
                }
                if (response.empty()) { // Treat empty response as cancel or re-prompt
                    std::cout << "Invalid response. Exit cancelled." << std::endl;
                    continue;
                }
                if (response == "yes" || response == "y") {
                    editor.saveFile();
                    break;
                } else if (response == "no" || response == "n") {
                    break;
                } else {
                    std::cout << "Exit cancelled." << std::endl;
                    continue;
                }
            } else {
                break; // No changes, exit directly
            }
        } else if (line.rfind("open ", 0) == 0) {
            std::string filename = line.substr(5);
            editor.openFile(filename);
        } else if (line.rfind("saveas ", 0) == 0) {
            std::string filename = line.substr(7);
            editor.saveFile(filename);
        } else if (line == "save") {
            editor.saveFile();
        } else if (line == "undo") {
            editor.undo();
        } else if (line == "redo") {
            editor.redo();
        } else {
            editor.typeText(line); // Example: treat non-commands as text to type
        }
        editor.printView(std::cout); // Refresh view after command
    }

    return 0;
} 