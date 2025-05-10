#include <iostream>
#include <string>
#include <vector>

class TextBuffer {
private:
    std::vector<std::string> lines_;
public:
    TextBuffer() {
        lines_.push_back("");
    }
    
    void addLine(const std::string& text) {
        lines_.push_back(text);
    }
    
    size_t lineCount() const {
        return lines_.size();
    }
    
    std::string getLine(size_t lineIndex) {
        return lines_.at(lineIndex);
    }
    
    void joinLines(size_t lineIndex) {
        if (lineIndex >= lines_.size() - 1) {
            std::cerr << "Cannot join last line with next line" << std::endl;
            return;
        }
        
        lines_[lineIndex] += lines_[lineIndex + 1];
        lines_.erase(lines_.begin() + lineIndex + 1);
    }
    
    void printBuffer() {
        for (size_t i = 0; i < lines_.size(); ++i) {
            std::cout << i << ": \"" << lines_[i] << "\"" << std::endl;
        }
    }
};

int main() {
    TextBuffer buffer;
    std::cout << "Initial line count: " << buffer.lineCount() << std::endl;
    buffer.printBuffer();
    
    buffer.addLine("First line");
    buffer.addLine("Second line");
    
    std::cout << "Before join - Line count: " << buffer.lineCount() << std::endl;
    buffer.printBuffer();
    
    buffer.joinLines(0); // Join line 0 with line 1
    
    std::cout << "After join - Line count: " << buffer.lineCount() << std::endl;
    buffer.printBuffer();
    
    return 0;
} 