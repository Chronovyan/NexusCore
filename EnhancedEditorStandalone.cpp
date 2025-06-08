#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <ctime>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// Simple namespace to avoid conflicts
namespace ai_editor_demo {

// Forward declarations
void glfw_error_callback(int error, const char* description);
bool initializeGlfw();
bool initializeImGui(GLFWwindow* window, const char* glsl_version);
void cleanupImGui();
void renderEditor(bool* p_open);
void processEditor();

// Application state
struct AppState {
    bool isRunning = true;
    bool showEditor = true;
    float editorPanelWidth = 0.7f; // 70% of window width for editor, 30% for AI panel
    
    // Editor state
    std::vector<std::string> lines;
    std::string filename;
    std::string language = "cpp"; // Default language
    bool isModified = false;
    int cursorLine = 0;
    int cursorColumn = 0;
    bool showLineNumbers = true;
    bool enableSyntaxHighlighting = true;
    bool showAIPanel = true;
    
    // AI Assistant state
    struct ChatMessage {
        std::string query;
        std::string response;
    };
    
    std::vector<ChatMessage> chatHistory;
    std::vector<std::string> suggestions;
    bool isThinking = false;
    char inputBuffer[1024] = "";
    
    // Constructor
    AppState() {
        // Initialize with some sample C++ code
        const char* sampleCode = 
            "#include <iostream>\n\n"
            "// A simple C++ program\n"
            "int main() {\n"
            "    std::cout << \"Hello, AI-Enhanced Editor!\" << std::endl;\n"
            "    \n"
            "    // Calculate Fibonacci numbers\n"
            "    int n = 10;\n"
            "    int a = 0, b = 1;\n"
            "    \n"
            "    std::cout << \"Fibonacci sequence:\" << std::endl;\n"
            "    for (int i = 0; i < n; i++) {\n"
            "        std::cout << a << \" \";\n"
            "        int temp = a;\n"
            "        a = b;\n"
            "        b = temp + b;\n"
            "    }\n"
            "    \n"
            "    return 0;\n"
            "}\n";
        
        // Split the sample code into lines
        std::istringstream stream(sampleCode);
        std::string line;
        while (std::getline(stream, line)) {
            lines.push_back(line);
        }
        
        // If lines is empty, add at least one empty line
        if (lines.empty()) {
            lines.push_back("");
        }
    }
};

// Global application state
AppState g_state;

// Syntax highlighting definitions
struct SyntaxHighlighting {
    struct Rule {
        std::string pattern;
        ImVec4 color;
    };
    
    std::vector<std::pair<std::string, ImVec4>> keywords;
    std::vector<Rule> rules;
    ImVec4 defaultColor = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
};

// Syntax highlighting for C++
SyntaxHighlighting cppHighlighting() {
    SyntaxHighlighting hl;
    
    // Keywords
    const char* cppKeywords[] = {
        "auto", "break", "case", "char", "const", "continue", "default", "do", "double",
        "else", "enum", "extern", "float", "for", "goto", "if", "int", "long", "register",
        "return", "short", "signed", "sizeof", "static", "struct", "switch", "typedef",
        "union", "unsigned", "void", "volatile", "while", "class", "namespace", "template",
        "new", "this", "delete", "public", "protected", "private", "virtual", "friend",
        "inline", "explicit", "operator", "bool", "try", "catch", "throw", "using", "true",
        "false", "nullptr"
    };
    
    ImVec4 keywordColor = ImVec4(0.5f, 0.5f, 1.0f, 1.0f); // Blue for keywords
    
    for (const char* keyword : cppKeywords) {
        hl.keywords.push_back(std::make_pair(keyword, keywordColor));
    }
    
    // Patterns
    hl.rules.push_back({"//.*", ImVec4(0.5f, 0.5f, 0.5f, 1.0f)}); // Line comments
    hl.rules.push_back({"[0-9]+", ImVec4(0.0f, 0.7f, 0.7f, 1.0f)}); // Numbers
    hl.rules.push_back({"\".*\"", ImVec4(0.9f, 0.6f, 0.0f, 1.0f)}); // Strings
    hl.rules.push_back({"'.'", ImVec4(0.9f, 0.6f, 0.0f, 1.0f)}); // Char literals
    
    return hl;
}

// Syntax highlighting for Python
SyntaxHighlighting pythonHighlighting() {
    SyntaxHighlighting hl;
    
    // Keywords
    const char* pythonKeywords[] = {
        "and", "as", "assert", "break", "class", "continue", "def", "del", "elif", "else",
        "except", "False", "finally", "for", "from", "global", "if", "import", "in", "is",
        "lambda", "None", "nonlocal", "not", "or", "pass", "raise", "return", "True", "try",
        "while", "with", "yield"
    };
    
    ImVec4 keywordColor = ImVec4(0.5f, 0.5f, 1.0f, 1.0f); // Blue for keywords
    
    for (const char* keyword : pythonKeywords) {
        hl.keywords.push_back(std::make_pair(keyword, keywordColor));
    }
    
    // Patterns
    hl.rules.push_back({"#.*", ImVec4(0.5f, 0.5f, 0.5f, 1.0f)}); // Comments
    hl.rules.push_back({"[0-9]+", ImVec4(0.0f, 0.7f, 0.7f, 1.0f)}); // Numbers
    hl.rules.push_back({"\".*\"", ImVec4(0.9f, 0.6f, 0.0f, 1.0f)}); // Double-quoted strings
    hl.rules.push_back({"'.*'", ImVec4(0.9f, 0.6f, 0.0f, 1.0f)}); // Single-quoted strings
    
    return hl;
}

// Get the appropriate syntax highlighting for the current language
SyntaxHighlighting getHighlighting() {
    if (g_state.language == "cpp") {
        return cppHighlighting();
    } else if (g_state.language == "python") {
        return pythonHighlighting();
    } else {
        // Default plain text
        SyntaxHighlighting hl;
        return hl;
    }
}

// GLFW error callback
void glfw_error_callback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

// Initialize GLFW
bool initializeGlfw() {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }
    
    return true;
}

// Initialize ImGui
bool initializeImGui(GLFWwindow* window, const char* glsl_version) {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    
    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    
    // Setup style
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
    style.Colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    style.Colors[ImGuiCol_Border] = ImVec4(0.40f, 0.40f, 0.40f, 0.50f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    
    return true;
}

// Cleanup ImGui
void cleanupImGui() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

// Render the menu bar
void renderMenuBar() {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New", "Ctrl+N")) {
                g_state.lines.clear();
                g_state.lines.push_back("");
                g_state.filename.clear();
                g_state.cursorLine = 0;
                g_state.cursorColumn = 0;
                g_state.isModified = false;
            }
            
            if (ImGui::MenuItem("Open", "Ctrl+O")) {
                // Open file dialog would go here
                // For now, we'll mock loading a sample file
                g_state.lines.clear();
                g_state.lines.push_back("#include <iostream>");
                g_state.lines.push_back("");
                g_state.lines.push_back("int main() {");
                g_state.lines.push_back("    std::cout << \"Hello, World!\" << std::endl;");
                g_state.lines.push_back("    return 0;");
                g_state.lines.push_back("}");
                g_state.filename = "sample.cpp";
                g_state.language = "cpp";
                g_state.isModified = false;
            }
            
            if (ImGui::MenuItem("Save", "Ctrl+S", false, !g_state.filename.empty())) {
                // Save the current file
                g_state.isModified = false;
            }
            
            if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) {
                // Save As dialog would go here
                g_state.filename = "sample_saved.cpp";
                g_state.isModified = false;
            }
            
            ImGui::Separator();
            
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                g_state.isRunning = false;
            }
            
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo", "Ctrl+Z", false, false)) {} // Not implemented yet
            if (ImGui::MenuItem("Redo", "Ctrl+Y", false, false)) {} // Not implemented yet
            
            ImGui::Separator();
            
            if (ImGui::MenuItem("Cut", "Ctrl+X", false, false)) {} // Not implemented yet
            if (ImGui::MenuItem("Copy", "Ctrl+C", false, false)) {} // Not implemented yet
            if (ImGui::MenuItem("Paste", "Ctrl+V", false, false)) {} // Not implemented yet
            
            ImGui::Separator();
            
            if (ImGui::MenuItem("Select All", "Ctrl+A", false, false)) {} // Not implemented yet
            
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Show Line Numbers", nullptr, &g_state.showLineNumbers);
            ImGui::MenuItem("Syntax Highlighting", nullptr, &g_state.enableSyntaxHighlighting);
            ImGui::MenuItem("Show AI Assistant", nullptr, &g_state.showAIPanel);
            
            ImGui::Separator();
            
            if (ImGui::BeginMenu("Language")) {
                if (ImGui::MenuItem("C++", nullptr, g_state.language == "cpp")) {
                    g_state.language = "cpp";
                }
                if (ImGui::MenuItem("Python", nullptr, g_state.language == "python")) {
                    g_state.language = "python";
                }
                if (ImGui::MenuItem("Plain Text", nullptr, g_state.language == "text")) {
                    g_state.language = "text";
                }
                ImGui::EndMenu();
            }
            
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("AI")) {
            if (ImGui::MenuItem("Ask Question", "Ctrl+Space")) {
                // Focus on AI input field
            }
            
            if (ImGui::MenuItem("Generate Code", "Ctrl+G")) {
                // Generate code example
                const char* query = "Generate a function to calculate the Fibonacci sequence";
                g_state.isThinking = true;
                g_state.chatHistory.push_back({query, ""});
                
                // Simulate AI thinking and generating a response
                std::string response = "Here's a simple function to calculate Fibonacci numbers:";
                g_state.chatHistory.back().response = response;
                
                if (g_state.language == "cpp") {
                    g_state.suggestions.push_back(
                        "int fibonacci(int n) {\n"
                        "    if (n <= 1) return n;\n"
                        "    return fibonacci(n-1) + fibonacci(n-2);\n"
                        "}"
                    );
                    
                    g_state.suggestions.push_back(
                        "// Iterative solution (more efficient)\n"
                        "int fibonacci(int n) {\n"
                        "    if (n <= 1) return n;\n"
                        "    int prev = 0, curr = 1;\n"
                        "    for (int i = 2; i <= n; i++) {\n"
                        "        int next = prev + curr;\n"
                        "        prev = curr;\n"
                        "        curr = next;\n"
                        "    }\n"
                        "    return curr;\n"
                        "}"
                    );
                } else if (g_state.language == "python") {
                    g_state.suggestions.push_back(
                        "def fibonacci(n):\n"
                        "    if n <= 1:\n"
                        "        return n\n"
                        "    return fibonacci(n-1) + fibonacci(n-2)"
                    );
                }
                
                g_state.isThinking = false;
            }
            
            ImGui::EndMenu();
        }
        
        ImGui::EndMenuBar();
    }
}

// Check if a character is part of a word
bool isWordChar(char c) {
    return std::isalnum(c) || c == '_';
}

// Render a line with syntax highlighting
void renderLineWithSyntaxHighlighting(const std::string& line, int lineNumber) {
    SyntaxHighlighting hl = getHighlighting();
    
    // Simple approach: check each word against keywords
    std::string word;
    bool inWord = false;
    ImVec4 currentColor = hl.defaultColor;
    
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    
    for (size_t i = 0; i <= line.length(); i++) {
        char c = (i < line.length()) ? line[i] : ' '; // Add space at end to process last word
        
        // Apply rule-based highlighting (simple version)
        bool highlightedByRule = false;
        if (!inWord) {
            for (const auto& rule : hl.rules) {
                // Very simple pattern matching (not regex)
                if (i + rule.pattern.length() <= line.length()) {
                    bool match = true;
                    for (size_t j = 0; j < rule.pattern.length(); j++) {
                        if (rule.pattern[j] == '.') continue; // Wildcard
                        if (rule.pattern[j] != line[i + j]) {
                            match = false;
                            break;
                        }
                    }
                    
                    if (match) {
                        // Render highlighted text
                        ImGui::TextColored(rule.color, "%.*s", (int)rule.pattern.length(), line.c_str() + i);
                        ImGui::SameLine(0.0f, 0.0f);
                        i += rule.pattern.length() - 1; // Skip the highlighted text
                        highlightedByRule = true;
                        break;
                    }
                }
            }
        }
        
        if (highlightedByRule) {
            continue;
        }
        
        // Word-based highlighting
        if (isWordChar(c)) {
            if (!inWord) {
                inWord = true;
                word = c;
            } else {
                word += c;
            }
        } else {
            if (inWord) {
                // Check if the word is a keyword
                bool isKeyword = false;
                for (const auto& kw : hl.keywords) {
                    if (word == kw.first) {
                        ImGui::TextColored(kw.second, "%s", word.c_str());
                        ImGui::SameLine(0.0f, 0.0f);
                        isKeyword = true;
                        break;
                    }
                }
                
                if (!isKeyword) {
                    ImGui::TextColored(hl.defaultColor, "%s", word.c_str());
                    ImGui::SameLine(0.0f, 0.0f);
                }
                
                inWord = false;
                word.clear();
            }
            
            if (i < line.length()) {
                ImGui::TextColored(hl.defaultColor, "%c", c);
                ImGui::SameLine(0.0f, 0.0f);
            }
        }
    }
    
    ImGui::PopStyleVar();
    
    // Handle empty line
    if (line.empty()) {
        ImGui::TextUnformatted("");
    }
}

// Render the editor panel
void renderEditorPanel() {
    // Calculate line numbers column width
    float lineNumberWidth = 0.0f;
    if (g_state.showLineNumbers) {
        int lineCount = static_cast<int>(g_state.lines.size());
        int digits = 1;
        while (lineCount >= 10) {
            lineCount /= 10;
            digits++;
        }
        lineNumberWidth = ImGui::CalcTextSize("0").x * (digits + 1);
    }
    
    // Line numbers area
    ImGui::BeginChild("LineNumbers", ImVec2(lineNumberWidth, 0), false);
    
    for (int i = 0; i < static_cast<int>(g_state.lines.size()); i++) {
        ImGui::Text("%*d", 3, i + 1);
    }
    
    ImGui::EndChild();
    
    ImGui::SameLine();
    
    // Editor content area
    ImGui::BeginChild("EditorContent", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
    
    // Render each line
    for (int i = 0; i < static_cast<int>(g_state.lines.size()); i++) {
        // Highlight the current line
        if (i == g_state.cursorLine) {
            ImVec2 lineStart = ImGui::GetCursorScreenPos();
            ImVec2 lineEnd = ImVec2(lineStart.x + ImGui::GetContentRegionAvail().x, 
                                   lineStart.y + ImGui::GetTextLineHeight());
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            drawList->AddRectFilled(lineStart, lineEnd, IM_COL32(50, 50, 50, 100));
        }
        
        // Render line with or without syntax highlighting
        if (g_state.enableSyntaxHighlighting && g_state.language != "text") {
            renderLineWithSyntaxHighlighting(g_state.lines[i], i);
        } else {
            ImGui::TextUnformatted(g_state.lines[i].c_str());
        }
    }
    
    ImGui::EndChild();
}

// Render the AI assistant panel
void renderAIAssistantPanel() {
    ImGui::Text("AI Assistant");
    ImGui::Separator();
    
    // Chat history display
    ImGui::BeginChild("ChatHistory", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() * 4), true);
    
    for (const auto& chat : g_state.chatHistory) {
        // User query
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.8f, 0.0f, 1.0f));
        ImGui::TextWrapped("You: %s", chat.query.c_str());
        ImGui::PopStyleColor();
        
        ImGui::Spacing();
        
        // AI response
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
        ImGui::TextWrapped("AI: %s", chat.response.c_str());
        ImGui::PopStyleColor();
        
        ImGui::Separator();
    }
    
    // Show "thinking" animation if the AI is processing
    if (g_state.isThinking) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        
        // Animate dots based on time
        float time = ImGui::GetTime();
        int dots = (int)(fmodf(time, 1.0f) * 3.0f);
        std::string thinking = "AI is thinking";
        for (int i = 0; i < dots; i++) {
            thinking += ".";
        }
        ImGui::TextWrapped("%s", thinking.c_str());
        
        ImGui::PopStyleColor();
    }
    
    ImGui::SetScrollHereY(1.0f); // Auto-scroll to bottom
    ImGui::EndChild();
    
    // Suggestions section (if any)
    if (!g_state.suggestions.empty()) {
        ImGui::Text("Suggestions:");
        ImGui::BeginChild("Suggestions", ImVec2(0, ImGui::GetFrameHeightWithSpacing() * 2), true);
        
        for (size_t i = 0; i < g_state.suggestions.size(); i++) {
            if (ImGui::Selectable(g_state.suggestions[i].c_str(), false)) {
                // Apply the suggestion by inserting it at the cursor position
                std::string suggestion = g_state.suggestions[i];
                
                // Split the suggestion into lines
                std::vector<std::string> suggestionLines;
                std::istringstream stream(suggestion);
                std::string line;
                while (std::getline(stream, line)) {
                    suggestionLines.push_back(line);
                }
                
                if (!suggestionLines.empty()) {
                    // Insert the suggestion at the cursor position
                    if (g_state.cursorLine < g_state.lines.size()) {
                        // Get the current line
                        std::string currentLine = g_state.lines[g_state.cursorLine];
                        
                        // Split the line at the cursor position
                        std::string beforeCursor = currentLine.substr(0, g_state.cursorColumn);
                        std::string afterCursor = "";
                        if (g_state.cursorColumn < currentLine.length()) {
                            afterCursor = currentLine.substr(g_state.cursorColumn);
                        }
                        
                        // Replace the current line with the first line of the suggestion
                        g_state.lines[g_state.cursorLine] = beforeCursor + suggestionLines[0];
                        
                        // Insert the rest of the suggestion lines
                        for (size_t j = 1; j < suggestionLines.size(); j++) {
                            g_state.lines.insert(g_state.lines.begin() + g_state.cursorLine + j, suggestionLines[j]);
                        }
                        
                        // Add the rest of the original line to the last line of the suggestion
                        g_state.lines[g_state.cursorLine + suggestionLines.size() - 1] += afterCursor;
                        
                        // Update cursor position
                        g_state.cursorLine += suggestionLines.size() - 1;
                        g_state.cursorColumn = suggestionLines.back().length();
                        
                        g_state.isModified = true;
                    }
                }
                
                // Clear suggestions after applying
                g_state.suggestions.clear();
            }
        }
        
        ImGui::EndChild();
    }
    
    // Input box
    ImGui::Text("Ask the AI:");
    ImGui::SetNextItemWidth(-1.0f);
    bool inputSubmitted = ImGui::InputText("##AIInput", g_state.inputBuffer, sizeof(g_state.inputBuffer), 
                                         ImGuiInputTextFlags_EnterReturnsTrue);
    
    ImGui::SameLine();
    if (ImGui::Button("Send") || inputSubmitted) {
        if (strlen(g_state.inputBuffer) > 0) {
            // Process the query
            std::string query = g_state.inputBuffer;
            g_state.inputBuffer[0] = '\0'; // Clear input
            
            g_state.isThinking = true;
            g_state.chatHistory.push_back({query, ""});
            
            // Generate a mock response
            std::string response;
            if (query.find("hello") != std::string::npos || query.find("hi") != std::string::npos) {
                response = "Hello! I'm your AI coding assistant. How can I help you with your code today?";
            } else if (query.find("how") != std::string::npos && query.find("sort") != std::string::npos) {
                response = "There are many sorting algorithms, like bubble sort, quick sort, merge sort, etc. "
                           "Each has different performance characteristics. Would you like me to show you an example?";
                
                if (g_state.language == "cpp") {
                    g_state.suggestions.push_back(
                        "// Bubble sort implementation\n"
                        "void bubbleSort(int arr[], int n) {\n"
                        "    for (int i = 0; i < n-1; i++) {\n"
                        "        for (int j = 0; j < n-i-1; j++) {\n"
                        "            if (arr[j] > arr[j+1]) {\n"
                        "                // Swap elements\n"
                        "                int temp = arr[j];\n"
                        "                arr[j] = arr[j+1];\n"
                        "                arr[j+1] = temp;\n"
                        "            }\n"
                        "        }\n"
                        "    }\n"
                        "}"
                    );
                } else if (g_state.language == "python") {
                    g_state.suggestions.push_back(
                        "# Bubble sort implementation\n"
                        "def bubble_sort(arr):\n"
                        "    n = len(arr)\n"
                        "    for i in range(n):\n"
                        "        for j in range(0, n-i-1):\n"
                        "            if arr[j] > arr[j+1]:\n"
                        "                # Swap elements\n"
                        "                arr[j], arr[j+1] = arr[j+1], arr[j]\n"
                        "    return arr"
                    );
                }
            } else {
                response = "I understand you're asking about \"" + query + "\". How can I assist you with this?";
            }
            
            g_state.chatHistory.back().response = response;
            g_state.isThinking = false;
        }
    }
}

// Render the status bar
void renderStatusBar() {
    ImGui::Separator();
    
    ImGui::Text("Line: %d, Col: %d | %s | %s%s", 
                g_state.cursorLine + 1, 
                g_state.cursorColumn + 1, 
                g_state.language.c_str(),
                g_state.filename.empty() ? "Untitled" : g_state.filename.c_str(),
                g_state.isModified ? " *" : "");
    
    ImGui::SameLine(ImGui::GetWindowWidth() - 150);
    
    float splitRatio = g_state.editorPanelWidth * 100.0f;
    ImGui::SetNextItemWidth(100);
    if (ImGui::SliderFloat("##Split", &splitRatio, 10.0f, 90.0f, "%.0f%%")) {
        g_state.editorPanelWidth = splitRatio / 100.0f;
    }
}

// Render the editor window
void renderEditor(bool* p_open) {
    ImGui::SetNextWindowSize(ImVec2(1000, 700), ImGuiCond_FirstUseEver);
    
    if (!ImGui::Begin("AI-Enhanced Text Editor", p_open, ImGuiWindowFlags_MenuBar)) {
        ImGui::End();
        return;
    }
    
    // Render menu bar
    renderMenuBar();
    
    // Calculate the panel sizes based on the window width
    float windowWidth = ImGui::GetContentRegionAvail().x;
    float editorWidth = windowWidth * g_state.editorPanelWidth;
    float assistantWidth = windowWidth - editorWidth - ImGui::GetStyle().ItemSpacing.x;
    
    // Create a split layout with editor on the left and AI panel on the right
    ImGui::BeginChild("EditorPanel", ImVec2(editorWidth, 0), true);
    renderEditorPanel();
    ImGui::EndChild();
    
    ImGui::SameLine();
    
    if (g_state.showAIPanel) {
        ImGui::BeginChild("AIPanel", ImVec2(assistantWidth, 0), true);
        renderAIAssistantPanel();
        ImGui::EndChild();
    }
    
    // Render status bar at the bottom
    renderStatusBar();
    
    ImGui::End();
}

// Process keyboard input for the editor
void processEditor() {
    ImGuiIO& io = ImGui::GetIO();
    
    // Handle keyboard shortcuts
    bool ctrl = io.KeyCtrl;
    
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_S)) {
        // Save
        g_state.isModified = false;
    }
    
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_N)) {
        // New file
        g_state.lines.clear();
        g_state.lines.push_back("");
        g_state.filename.clear();
        g_state.cursorLine = 0;
        g_state.cursorColumn = 0;
        g_state.isModified = false;
    }
    
    // Arrow key navigation
    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
        if (g_state.cursorLine > 0) {
            g_state.cursorLine--;
            // Adjust cursor column if the new line is shorter
            if (g_state.cursorColumn > g_state.lines[g_state.cursorLine].length()) {
                g_state.cursorColumn = g_state.lines[g_state.cursorLine].length();
            }
        }
    }
    
    if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
        if (g_state.cursorLine < g_state.lines.size() - 1) {
            g_state.cursorLine++;
            // Adjust cursor column if the new line is shorter
            if (g_state.cursorColumn > g_state.lines[g_state.cursorLine].length()) {
                g_state.cursorColumn = g_state.lines[g_state.cursorLine].length();
            }
        }
    }
    
    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) {
        if (g_state.cursorColumn > 0) {
            g_state.cursorColumn--;
        } else if (g_state.cursorLine > 0) {
            g_state.cursorLine--;
            g_state.cursorColumn = g_state.lines[g_state.cursorLine].length();
        }
    }
    
    if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
        if (g_state.cursorColumn < g_state.lines[g_state.cursorLine].length()) {
            g_state.cursorColumn++;
        } else if (g_state.cursorLine < g_state.lines.size() - 1) {
            g_state.cursorLine++;
            g_state.cursorColumn = 0;
        }
    }
}

} // namespace ai_editor_demo

// Main entry point
int main(int argc, char** argv) {
    using namespace ai_editor_demo;
    
    // Initialize GLFW
    if (!initializeGlfw()) {
        return 1;
    }
    
    // Decide GL version
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    
    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1280, 720, "AI-Enhanced Text Editor", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return 1;
    }
    
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync
    
    // Initialize GLEW
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        glfwTerminate();
        return 1;
    }
    
    // Initialize ImGui
    if (!initializeImGui(window, glsl_version)) {
        glfwTerminate();
        return 1;
    }
    
    // Main loop
    while (g_state.isRunning && !glfwWindowShouldClose(window)) {
        glfwPollEvents();
        
        // Process keyboard input
        processEditor();
        
        // Start the ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        // Render the editor window
        renderEditor(&g_state.showEditor);
        
        if (!g_state.showEditor) {
            g_state.isRunning = false;
        }
        
        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        glfwSwapBuffers(window);
    }
    
    // Cleanup
    cleanupImGui();
    
    glfwDestroyWindow(window);
    glfwTerminate();
    
    return 0;
} 