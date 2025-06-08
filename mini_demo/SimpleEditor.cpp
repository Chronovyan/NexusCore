#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>

class SimpleEditor {
public:
    SimpleEditor() {
        // Initialize with an empty document
        lines_.push_back("");
    }
    
    bool loadFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << filename << std::endl;
            return false;
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();
        
        setText(buffer.str());
        filename_ = filename;
        isModified_ = false;
        
        std::cout << "Loaded file: " << filename << std::endl;
        return true;
    }
    
    bool saveFile(const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open file for writing: " << filename << std::endl;
            return false;
        }
        
        std::string content = joinLines();
        file << content;
        file.close();
        
        filename_ = filename;
        isModified_ = false;
        
        std::cout << "Saved file: " << filename << std::endl;
        return true;
    }
    
    void setText(const std::string& text) {
        splitIntoLines(text);
        cursorLine_ = 0;
        cursorColumn_ = 0;
        isModified_ = false;
    }
    
    std::string getText() const {
        return joinLines();
    }
    
    void insertCharacter(char c) {
        // Insert character at cursor position
        if (c == '\n') {
            // Split the line at cursor position
            std::string currentLine = lines_[cursorLine_];
            std::string leftPart = currentLine.substr(0, cursorColumn_);
            std::string rightPart = currentLine.substr(cursorColumn_);
            
            lines_[cursorLine_] = leftPart;
            lines_.insert(lines_.begin() + cursorLine_ + 1, rightPart);
            
            cursorLine_++;
            cursorColumn_ = 0;
        } else {
            // Insert character at cursor position
            lines_[cursorLine_].insert(cursorColumn_, 1, c);
            cursorColumn_++;
        }
        
        isModified_ = true;
    }
    
    void deleteCharacter() {
        // Delete character at cursor position
        if (cursorColumn_ > 0) {
            // Delete character before cursor
            lines_[cursorLine_].erase(cursorColumn_ - 1, 1);
            cursorColumn_--;
            isModified_ = true;
        } else if (cursorLine_ > 0) {
            // Join with previous line
            int prevLineLength = lines_[cursorLine_ - 1].length();
            lines_[cursorLine_ - 1] += lines_[cursorLine_];
            lines_.erase(lines_.begin() + cursorLine_);
            
            cursorLine_--;
            cursorColumn_ = prevLineLength;
            isModified_ = true;
        }
    }
    
    void moveCursorLeft() {
        if (cursorColumn_ > 0) {
            cursorColumn_--;
        } else if (cursorLine_ > 0) {
            cursorLine_--;
            cursorColumn_ = lines_[cursorLine_].length();
        }
    }
    
    void moveCursorRight() {
        if (cursorColumn_ < lines_[cursorLine_].length()) {
            cursorColumn_++;
        } else if (cursorLine_ < lines_.size() - 1) {
            cursorLine_++;
            cursorColumn_ = 0;
        }
    }
    
    void moveCursorUp() {
        if (cursorLine_ > 0) {
            cursorLine_--;
            if (cursorColumn_ > lines_[cursorLine_].length()) {
                cursorColumn_ = lines_[cursorLine_].length();
            }
        }
    }
    
    void moveCursorDown() {
        if (cursorLine_ < lines_.size() - 1) {
            cursorLine_++;
            if (cursorColumn_ > lines_[cursorLine_].length()) {
                cursorColumn_ = lines_[cursorLine_].length();
            }
        }
    }
    
    void displayText() const {
        // Clear the screen
        std::cout << "\033[2J\033[1;1H";
        
        // Display status line
        std::cout << "Simple Editor | " 
                  << (filename_.empty() ? "Untitled" : filename_) 
                  << (isModified_ ? " [modified]" : "") << std::endl;
        std::cout << "Line: " << (cursorLine_ + 1) << ", Col: " << (cursorColumn_ + 1) << std::endl;
        std::cout << std::string(80, '-') << std::endl;
        
        // Display text with cursor
        for (size_t i = 0; i < lines_.size(); i++) {
            std::cout << std::setw(4) << (i + 1) << " | ";
            
            if (i == cursorLine_) {
                // Print line with cursor
                std::string line = lines_[i];
                for (size_t j = 0; j < line.length(); j++) {
                    if (j == cursorColumn_) {
                        std::cout << "\033[7m" << line[j] << "\033[0m"; // Invert color for cursor
                    } else {
                        std::cout << line[j];
                    }
                }
                
                // If cursor is at the end of the line, show it
                if (cursorColumn_ >= line.length()) {
                    std::cout << "\033[7m \033[0m";
                }
                
                std::cout << std::endl;
            } else {
                // Print line without cursor
                std::cout << lines_[i] << std::endl;
            }
        }
        
        // If the cursor is on a new line at the end
        if (cursorLine_ >= lines_.size()) {
            std::cout << "\033[7m \033[0m" << std::endl;
        }
        
        std::cout << std::string(80, '-') << std::endl;
        std::cout << "Commands: ^S:Save, ^Q:Quit, ^O:Open" << std::endl;
    }
    
    const std::string& getFilename() const {
        return filename_;
    }
    
    bool isModified() const {
        return isModified_;
    }
    
private:
    void splitIntoLines(const std::string& text) {
        lines_.clear();
        
        std::stringstream ss(text);
        std::string line;
        
        while (std::getline(ss, line)) {
            lines_.push_back(line);
        }
        
        // Ensure there's at least one line
        if (lines_.empty()) {
            lines_.push_back("");
        }
    }
    
    std::string joinLines() const {
        std::stringstream ss;
        
        for (size_t i = 0; i < lines_.size(); i++) {
            ss << lines_[i];
            // Add newline for all lines except the last one
            if (i < lines_.size() - 1) {
                ss << "\n";
            }
        }
        
        return ss.str();
    }
    
    std::vector<std::string> lines_;
    std::string filename_;
    bool isModified_ = false;
    
    int cursorLine_ = 0;
    int cursorColumn_ = 0;
};

int main(int argc, char** argv) {
    SimpleEditor editor;
    
    // Load a file if provided
    if (argc > 1) {
        editor.loadFile(argv[1]);
    } else {
        // Set some sample text
        editor.setText("// Welcome to Simple Text Editor\n\n"
                       "This is a simple demo of the editor capabilities.\n"
                       "You can type text and navigate with cursor keys.\n\n"
                       "Enjoy!");
    }
    
    bool running = true;
    while (running) {
        // Display the editor content
        editor.displayText();
        
        // Get user input
        char c;
        std::cin.get(c);
        
        // Handle special keys
        if (c == 27) {  // ESC key
            // Get the next characters for arrow keys
            char seq[2];
            if (std::cin.read(seq, 2)) {
                if (seq[0] == '[') {
                    switch (seq[1]) {
                        case 'A': // Up arrow
                            editor.moveCursorUp();
                            break;
                        case 'B': // Down arrow
                            editor.moveCursorDown();
                            break;
                        case 'C': // Right arrow
                            editor.moveCursorRight();
                            break;
                        case 'D': // Left arrow
                            editor.moveCursorLeft();
                            break;
                    }
                }
            }
        } else if (c == 127 || c == 8) {  // Backspace
            editor.deleteCharacter();
        } else if (c == 19) {  // Ctrl+S
            if (editor.getFilename().empty()) {
                std::cout << "Enter filename to save: ";
                std::string filename;
                std::cin >> filename;
                editor.saveFile(filename);
            } else {
                editor.saveFile(editor.getFilename());
            }
        } else if (c == 17) {  // Ctrl+Q
            if (editor.isModified()) {
                std::cout << "File is modified. Save before quitting? (y/n): ";
                char choice;
                std::cin >> choice;
                if (choice == 'y' || choice == 'Y') {
                    if (editor.getFilename().empty()) {
                        std::cout << "Enter filename to save: ";
                        std::string filename;
                        std::cin >> filename;
                        editor.saveFile(filename);
                    } else {
                        editor.saveFile(editor.getFilename());
                    }
                }
            }
            running = false;
        } else if (c == 15) {  // Ctrl+O
            std::cout << "Enter filename to open: ";
            std::string filename;
            std::cin >> filename;
            editor.loadFile(filename);
        } else {
            editor.insertCharacter(c);
        }
    }
    
    return 0;
} 