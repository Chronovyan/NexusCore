#include "TextOperation.h"
#include <sstream>

namespace ai_editor {

TextOperation TextOperation::createInsertion(int line, int column, const std::string& text) {
    TextOperation op;
    op.type_ = Type::INSERT;
    op.line_ = line;
    op.column_ = column;
    op.text_ = text;
    return op;
}

TextOperation TextOperation::createDeletion(int line, int column, const std::string& text, 
                                          int endLine, int endColumn) {
    TextOperation op;
    op.type_ = Type::DELETE;
    op.line_ = line;
    op.column_ = column;
    op.text_ = text;
    op.endLine_ = endLine == -1 ? line : endLine;
    op.endColumn_ = endColumn == -1 ? column + static_cast<int>(text.length()) : endColumn;
    return op;
}

TextOperation TextOperation::createReplacement(int line, int column, 
                                             const std::string& oldText, 
                                             const std::string& newText,
                                             int endLine, int endColumn) {
    TextOperation op;
    op.type_ = Type::REPLACE;
    op.line_ = line;
    op.column_ = column;
    op.oldText_ = oldText;
    op.text_ = newText;
    
    if (endLine == -1) {
        // Calculate end position based on old text
        size_t lastNewline = oldText.rfind('\n');
        if (lastNewline == std::string::npos) {
            op.endLine_ = line;
            op.endColumn_ = column + static_cast<int>(oldText.length());
        } else {
            // Count newlines to determine end line
            int newlineCount = 0;
            size_t pos = 0;
            while ((pos = oldText.find('\n', pos)) != std::string::npos) {
                ++newlineCount;
                ++pos;
            }
            op.endLine_ = line + newlineCount;
            op.endColumn_ = static_cast<int>(oldText.length() - lastNewline - 1);
        }
    } else {
        op.endLine_ = endLine;
        op.endColumn_ = endColumn;
    }
    
    return op;
}

std::string TextOperation::getDescription() const {
    std::ostringstream ss;
    
    switch (type_) {
        case Type::INSERT:
            ss << "INSERT " << text_.length() << " chars at " << (line_ + 1) << ":" << (column_ + 1);
            break;
            
        case Type::DELETE:
            ss << "DELETE " << text_.length() << " chars at " << (line_ + 1) << ":" << (column_ + 1);
            if (isMultiLine()) {
                ss << " to " << (endLine_ + 1) << ":" << (endColumn_ + 1);
            }
            break;
            
        case Type::REPLACE:
            ss << "REPLACE " << oldText_.length() << " chars with " << text_.length() 
               << " chars at " << (line_ + 1) << ":" << (column_ + 1);
            if (isMultiLine()) {
                ss << " to " << (endLine_ + 1) << ":" << (endColumn_ + 1);
            }
            break;
    }
    
    return ss.str();
}

} // namespace ai_editor
