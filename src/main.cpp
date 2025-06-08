#include <iostream>
#include <string>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <memory>
#include <vector>
namespace fs = std::filesystem;

// Forward declarations
struct OpenFile;

// Include GLEW before any other OpenGL-related headers
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// Include ImGui
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

// Include EditorLib components
#include "Editor.h"
#include "TextBuffer.h"
#include "CommandManager.h"
#include "SyntaxHighlighter.h"
#include "SyntaxHighlightingManager.h"

// Platform-specific includes for file dialogs
#ifdef _WIN32
#include <windows.h>
#include <shobjidl.h> 
#endif

// Logger class to write to both console and file
class Logger {
public:
    Logger() {
        logFile.open("texteditor_log.txt", std::ios::out);
    }
    
    ~Logger() {
        if (logFile.is_open()) {
            logFile.close();
        }
    }
    
    void log(const std::string& message) {
        std::cout << message << std::endl;
        if (logFile.is_open()) {
            logFile << message << std::endl;
            logFile.flush();
        }
    }
    
    void error(const std::string& message) {
        std::cerr << "ERROR: " << message << std::endl;
        if (logFile.is_open()) {
            logFile << "ERROR: " << message << std::endl;
            logFile.flush();
        }
    }
    
private:
    std::ofstream logFile;
};

// Global logger
Logger logger;

// Callback for GLFW errors
static void glfw_error_callback(int error, const char* description) {
    logger.error("GLFW Error " + std::to_string(error) + ": " + description);
}

// Replace the simple file dialog functions with more robust ones
#ifdef _WIN32
// Windows implementation using modern COM-based file dialogs
bool openFileDialog(std::string& outPath) {
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) {
        return false;
    }

    IFileOpenDialog* pFileOpen;
    hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));
    if (SUCCEEDED(hr)) {
        // Set file types filter
        COMDLG_FILTERSPEC fileTypes[] = {
            { L"Text Files", L"*.txt" },
            { L"C++ Files", L"*.cpp;*.h;*.hpp" },
            { L"All Files", L"*.*" }
        };
        pFileOpen->SetFileTypes(ARRAYSIZE(fileTypes), fileTypes);

        // Show the dialog
        hr = pFileOpen->Show(NULL);
        if (SUCCEEDED(hr)) {
            IShellItem* pItem;
            hr = pFileOpen->GetResult(&pItem);
            if (SUCCEEDED(hr)) {
                PWSTR pszFilePath;
                hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
                if (SUCCEEDED(hr)) {
                    // Convert wide string to narrow string
                    int requiredSize = WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, NULL, 0, NULL, NULL);
                    if (requiredSize > 0) {
                        std::vector<char> buffer(requiredSize);
                        WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, buffer.data(), requiredSize, NULL, NULL);
                        outPath = buffer.data();
                    }
                    CoTaskMemFree(pszFilePath);
                }
                pItem->Release();
            }
        }
        pFileOpen->Release();
    }
    CoUninitialize();
    return !outPath.empty();
}

bool saveFileDialog(std::string& outPath) {
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) {
        return false;
    }

    IFileSaveDialog* pFileSave;
    hr = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_ALL, IID_IFileSaveDialog, reinterpret_cast<void**>(&pFileSave));
    if (SUCCEEDED(hr)) {
        // Set file types filter
        COMDLG_FILTERSPEC fileTypes[] = {
            { L"Text Files", L"*.txt" },
            { L"C++ Files", L"*.cpp;*.h;*.hpp" },
            { L"All Files", L"*.*" }
        };
        pFileSave->SetFileTypes(ARRAYSIZE(fileTypes), fileTypes);

        // Set default extension
        pFileSave->SetDefaultExtension(L"txt");

        // If a file is already open, use its directory as the starting location
        if (!outPath.empty()) {
            std::filesystem::path filePath(outPath);
            std::wstring directory = filePath.parent_path().wstring();
            
            IShellItem* pItem;
            hr = SHCreateItemFromParsingName(directory.c_str(), NULL, IID_PPV_ARGS(&pItem));
            if (SUCCEEDED(hr)) {
                pFileSave->SetFolder(pItem);
                pItem->Release();
            }
            
            // Set default filename
            std::wstring filename = filePath.filename().wstring();
            pFileSave->SetFileName(filename.c_str());
        }

        // Show the dialog
        hr = pFileSave->Show(NULL);
        if (SUCCEEDED(hr)) {
            IShellItem* pItem;
            hr = pFileSave->GetResult(&pItem);
            if (SUCCEEDED(hr)) {
                PWSTR pszFilePath;
                hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
                if (SUCCEEDED(hr)) {
                    // Convert wide string to narrow string
                    int requiredSize = WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, NULL, 0, NULL, NULL);
                    if (requiredSize > 0) {
                        std::vector<char> buffer(requiredSize);
                        WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, buffer.data(), requiredSize, NULL, NULL);
                        outPath = buffer.data();
                    }
                    CoTaskMemFree(pszFilePath);
                }
                pItem->Release();
            }
        }
        pFileSave->Release();
    }
    CoUninitialize();
    return !outPath.empty();
}
#else
// Simple fallback for non-Windows platforms
bool openFileDialog(std::string& outPath) {
    // This is a dummy implementation for non-Windows platforms
    // In a real application, you would use platform-specific APIs
    // like GTK's file chooser dialog on Linux
    outPath = "example.txt";
    return true;
}

bool saveFileDialog(std::string& outPath) {
    // This is a dummy implementation for non-Windows platforms
    if (outPath.empty()) {
        outPath = "example.txt";
    }
    return true;
}
#endif

// Fix SyntaxHighlighter implementation
class CPPSyntaxHighlighter : public SyntaxHighlighter {
public:
    std::unique_ptr<std::vector<SyntaxStyle>> highlightLine(const std::string& line, size_t lineIndex) const override {
        auto styles = std::make_unique<std::vector<SyntaxStyle>>();
        
        // Check for line comments
        size_t lineCommentPos = line.find("//");
        if (lineCommentPos != std::string::npos && !isInString(line, lineCommentPos)) {
            styles->push_back(SyntaxStyle(lineCommentPos, line.length(), SyntaxColor::Comment));
            return styles;
        }
        
        // Check for block comments /* ... */
        static bool inBlockComment = false;
        size_t blockCommentStartPos = line.find("/*");
        size_t blockCommentEndPos = line.find("*/");
        
        if (inBlockComment) {
            if (blockCommentEndPos != std::string::npos) {
                size_t commentEndPos = blockCommentEndPos + 2;
                styles->push_back(SyntaxStyle(0, commentEndPos, SyntaxColor::Comment));
                inBlockComment = false;
                // Process rest of the line after block comment if needed
            } else {
                // Entire line is a comment
                styles->push_back(SyntaxStyle(0, line.length(), SyntaxColor::Comment));
                return styles;
            }
        } else if (blockCommentStartPos != std::string::npos && !isInString(line, blockCommentStartPos)) {
            if (blockCommentEndPos != std::string::npos && blockCommentEndPos > blockCommentStartPos) {
                // Comment starts and ends on same line
                styles->push_back(SyntaxStyle(blockCommentStartPos, blockCommentEndPos + 2, SyntaxColor::Comment));
            } else {
                // Comment starts but doesn't end
                styles->push_back(SyntaxStyle(blockCommentStartPos, line.length(), SyntaxColor::Comment));
                inBlockComment = true;
                return styles;
            }
        }
        
        // Check for string literals
        for (size_t i = 0; i < line.length(); ++i) {
            if (line[i] == '"' && (i == 0 || line[i-1] != '\\')) {
                size_t quotePos = i;
                size_t endQuotePos = line.find('"', quotePos + 1);
                while (endQuotePos != std::string::npos && endQuotePos > 0 && line[endQuotePos-1] == '\\') {
                    endQuotePos = line.find('"', endQuotePos + 1);
                }
                
                if (endQuotePos != std::string::npos) {
                    styles->push_back(SyntaxStyle(quotePos, endQuotePos + 1, SyntaxColor::String));
                    i = endQuotePos;
                } else {
                    // String doesn't end on this line
                    styles->push_back(SyntaxStyle(quotePos, line.length(), SyntaxColor::String));
                    break;
                }
            }
        }
        
        // Check for preprocessor directives
        if (!line.empty() && line[0] == '#') {
            std::string code = line;
            if (code.find("#include") == 0 || 
                code.find("#define") == 0 || 
                code.find("#ifdef") == 0 || 
                code.find("#ifndef") == 0 || 
                code.find("#endif") == 0 || 
                code.find("#pragma") == 0) {
                    styles->push_back(SyntaxStyle(0, code.length(), SyntaxColor::Preprocessor));
                    return styles;
                }
        }
        
        // Check for keywords and identifiers
        std::vector<std::string> keywords = {
            "auto", "break", "case", "char", "const", "continue", "default", "do", "double",
            "else", "enum", "extern", "float", "for", "goto", "if", "int", "long", "register",
            "return", "short", "signed", "sizeof", "static", "struct", "switch", "typedef",
            "union", "unsigned", "void", "volatile", "while", "class", "namespace", "try",
            "catch", "throw", "template", "typename", "virtual", "inline", "bool", "new", "delete",
            "public", "private", "protected"
        };
        
        std::string word;
        size_t wordStart = 0;
        for (size_t i = 0; i < line.length(); ++i) {
            if (isalnum(line[i]) || line[i] == '_') {
                if (word.empty()) {
                    wordStart = i;
                }
                word += line[i];
            } else {
                if (!word.empty()) {
                    if (std::find(keywords.begin(), keywords.end(), word) != keywords.end()) {
                        styles->push_back(SyntaxStyle(wordStart, wordStart + word.length(), SyntaxColor::Keyword));
                    } else if (std::all_of(word.begin(), word.end(), [](char c) { return isdigit(c) || c == '.' || c == 'f'; })) {
                        styles->push_back(SyntaxStyle(wordStart, wordStart + word.length(), SyntaxColor::Number));
                    } else {
                        styles->push_back(SyntaxStyle(wordStart, wordStart + word.length(), SyntaxColor::Identifier));
                    }
                    word.clear();
                }
                
                if (isspace(line[i])) {
                    size_t whitespaceStart = i;
                    size_t whitespaceLength = 1;
                    while (i + 1 < line.length() && isspace(line[i + 1])) {
                        ++i;
                        ++whitespaceLength;
                    }
                    if (whitespaceLength > 0) {
                        styles->push_back(SyntaxStyle(whitespaceStart, whitespaceStart + whitespaceLength, SyntaxColor::Default));
                    }
                }
            }
        }
        
        // Handle the last word if it exists
        if (!word.empty()) {
            if (std::find(keywords.begin(), keywords.end(), word) != keywords.end()) {
                styles->push_back(SyntaxStyle(wordStart, wordStart + word.length(), SyntaxColor::Keyword));
            } else if (std::all_of(word.begin(), word.end(), [](char c) { return isdigit(c) || c == '.' || c == 'f'; })) {
                styles->push_back(SyntaxStyle(wordStart, wordStart + word.length(), SyntaxColor::Number));
            } else {
                styles->push_back(SyntaxStyle(wordStart, wordStart + word.length(), SyntaxColor::Identifier));
            }
        }
        
        return styles;
    }
    
    // Implement required virtual methods
    std::vector<std::vector<SyntaxStyle>> highlightBuffer(const ITextBuffer& buffer) const override {
        std::vector<std::vector<SyntaxStyle>> result;
        for (size_t i = 0; i < buffer.getLineCount(); ++i) {
            auto lineStyles = highlightLine(buffer.getLine(i), i);
            result.push_back(*lineStyles);
        }
        return result;
    }
    
    std::vector<std::string> getSupportedExtensions() const override {
        return {".cpp", ".h", ".hpp", ".c", ".cc"};
    }
    
    std::string getLanguageName() const override {
        return "C++";
    }

private:
    bool isInString(const std::string& line, size_t pos) const {
        bool inString = false;
        for (size_t i = 0; i < pos; ++i) {
            if (line[i] == '"' && (i == 0 || line[i-1] != '\\')) {
                inString = !inString;
            }
        }
        return inString;
    }
};

// Find and replace state
struct FindReplaceState {
    char findBuffer[256] = "";
    char replaceBuffer[256] = "";
    bool caseSensitive = false;
    bool wholeWord = false;
    bool isOpen = false;
    bool focusFind = false;
    bool focusReplace = false;
};

// File browser state
struct FileBrowserState {
    std::string currentPath;
    std::vector<fs::directory_entry> entries;
    bool isOpen = true;
    char searchBuffer[256] = "";
    
    FileBrowserState() {
        // Initialize with the current directory
        currentPath = fs::current_path().string();
        refreshEntries();
    }
    
    void refreshEntries() {
        entries.clear();
        
        try {
            // Get all entries in the current directory
            for (const auto& entry : fs::directory_iterator(currentPath)) {
                entries.push_back(entry);
            }
            
            // Sort entries: directories first, then files
            std::sort(entries.begin(), entries.end(), 
                [](const fs::directory_entry& a, const fs::directory_entry& b) {
                    // If both are directories or both are files, sort by name
                    if (a.is_directory() == b.is_directory()) {
                        return a.path().filename().string() < b.path().filename().string();
                    }
                    // Otherwise, directories come first
                    return a.is_directory();
                });
        } catch (const std::exception& e) {
            // Handle directory access errors
            entries.clear();
        }
    }
    
    void navigateToParent() {
        fs::path path(currentPath);
        fs::path parent = path.parent_path();
        
        // Make sure we don't go beyond the root
        if (parent != path) {
            currentPath = parent.string();
            refreshEntries();
        }
    }
    
    void navigateTo(const std::string& path) {
        currentPath = path;
        refreshEntries();
    }
};

// Keep only one OpenFile struct definition
struct OpenFile {
    std::string filename;
    std::string displayName;
    std::shared_ptr<TextBuffer> buffer;
    std::shared_ptr<CommandManager> commandManager;
    std::shared_ptr<SyntaxHighlightingManager> highlightingManager;
    std::unique_ptr<Editor> editor;
    bool modified;
    
    OpenFile(const std::string& path) 
        : filename(path), 
          modified(false) {
        
        // Extract display name from path
        displayName = path;
        size_t lastSlash = displayName.find_last_of("/\\");
        if (lastSlash != std::string::npos) {
            displayName = displayName.substr(lastSlash + 1);
        }
        
        // Create editor components
        buffer = std::make_shared<TextBuffer>();
        commandManager = std::make_shared<CommandManager>();
        highlightingManager = std::make_shared<SyntaxHighlightingManager>();
        
        // Create editor
        editor = std::make_unique<Editor>(buffer, commandManager, highlightingManager);
        
        // Enable syntax highlighting
        editor->enableSyntaxHighlighting(true);
        
        // Set the highlighter based on file extension
        std::string extension = "";
        size_t lastDot = path.find_last_of(".");
        if (lastDot != std::string::npos) {
            extension = path.substr(lastDot);
            std::transform(extension.begin(), extension.end(), extension.begin(),
                [](unsigned char c){ return std::tolower(c); });
        }
        
        // Set the appropriate highlighter based on extension
        if (extension == ".cpp" || extension == ".h" || extension == ".hpp" || extension == ".c") {
            editor->setHighlighter(std::make_shared<CPPSyntaxHighlighter>());
        }
        
        // If it's an existing file, load it
        if (!path.empty() && path != "untitled") {
            if (!editor->openFile(path)) {
                // Failed to open the file, create an empty buffer
                buffer->clear(false);
                editor->setFilename(path);
            }
        }
    }
};

int main(int argc, char** argv) {
    // Set GLFW error callback
    glfwSetErrorCallback(glfw_error_callback);
    
    logger.log("Starting TextEditor application...");
    
    // Initialize GLFW
    if (!glfwInit()) {
        logger.error("Failed to initialize GLFW");
        return 1;
    }
    
    logger.log("GLFW initialized successfully.");

    // Set OpenGL version and profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create a window
    logger.log("Creating GLFW window...");
    GLFWwindow* window = glfwCreateWindow(1024, 768, "TextEditor", NULL, NULL);
    if (!window) {
        logger.error("Failed to create GLFW window");
        glfwTerminate();
        return 1;
    }
    
    logger.log("GLFW window created successfully.");

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Initialize GLEW
    logger.log("Initializing GLEW...");
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        logger.error("Failed to initialize GLEW: " + std::string((const char*)glewGetErrorString(err)));
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    
    logger.log("GLEW initialized successfully. OpenGL version: " + 
               std::string((const char*)glGetString(GL_VERSION)));

    // Setup ImGui context
    logger.log("Setting up ImGui...");
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;  // Comment out or remove this line
    
    // Setup ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) {
        logger.error("Failed to initialize ImGui GLFW backend");
        ImGui::DestroyContext();
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    
    if (!ImGui_ImplOpenGL3_Init("#version 130")) {
        logger.error("Failed to initialize ImGui OpenGL3 backend");
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    
    logger.log("ImGui setup complete.");
    
    // Replace the single editor and state variables with collections for multiple files
    std::vector<OpenFile> openFiles;
    int currentFileIndex = -1;
    
    // Add a default untitled file
    openFiles.emplace_back("untitled");
    currentFileIndex = 0;
    
    // Find and replace state
    FindReplaceState findReplaceState;
    
    // File browser state
    FileBrowserState fileBrowserState;
    
    logger.log("Starting main loop...");

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Poll events
        glfwPollEvents();

        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Create a dockspace
        // ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
        
        // Menu bar
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("New", "Ctrl+N")) {
                    // Create a new untitled file
                    openFiles.emplace_back("untitled");
                    currentFileIndex = openFiles.size() - 1;
                }
                
                if (ImGui::MenuItem("Open", "Ctrl+O")) {
                    std::string filepath;
                    if (openFileDialog(filepath)) {
                        // Check if the file is already open
                        bool alreadyOpen = false;
                        for (size_t i = 0; i < openFiles.size(); i++) {
                            if (openFiles[i].filename == filepath) {
                                currentFileIndex = i;
                                alreadyOpen = true;
                                break;
                            }
                        }
                        
                        // If not already open, create a new tab
                        if (!alreadyOpen) {
                            openFiles.emplace_back(filepath);
                            currentFileIndex = openFiles.size() - 1;
                        }
                    }
                }
                
                if (ImGui::MenuItem("Save", "Ctrl+S", false, currentFileIndex >= 0)) {
                    if (currentFileIndex >= 0) {
                        auto& currentFile = openFiles[currentFileIndex];
                        if (currentFile.filename == "untitled") {
                            std::string filepath;
                            if (saveFileDialog(filepath)) {
                                if (currentFile.editor->saveFileAs(filepath)) {
                                    currentFile.filename = filepath;
                                    currentFile.displayName = filepath;
                                    size_t lastSlash = currentFile.displayName.find_last_of("/\\");
                                    if (lastSlash != std::string::npos) {
                                        currentFile.displayName = currentFile.displayName.substr(lastSlash + 1);
                                    }
                                    currentFile.modified = false;
                                    logger.log("Saved file: " + filepath);
                                } else {
                                    logger.error("Failed to save file: " + filepath);
                                }
                            }
                        } else {
                            if (currentFile.editor->saveFile()) {
                                currentFile.modified = false;
                                logger.log("Saved file: " + currentFile.filename);
                            } else {
                                logger.error("Failed to save file: " + currentFile.filename);
                            }
                        }
                    }
                }
                
                if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S", false, currentFileIndex >= 0)) {
                    if (currentFileIndex >= 0) {
                        auto& currentFile = openFiles[currentFileIndex];
                        std::string filepath = currentFile.filename;
                        if (saveFileDialog(filepath)) {
                            if (currentFile.editor->saveFileAs(filepath)) {
                                currentFile.filename = filepath;
                                currentFile.displayName = filepath;
                                size_t lastSlash = currentFile.displayName.find_last_of("/\\");
                                if (lastSlash != std::string::npos) {
                                    currentFile.displayName = currentFile.displayName.substr(lastSlash + 1);
                                }
                                currentFile.modified = false;
                                logger.log("Saved file as: " + filepath);
                            } else {
                                logger.error("Failed to save file as: " + filepath);
                            }
                        }
                    }
                }
                
                ImGui::Separator();
                
                if (ImGui::MenuItem("Close", "Ctrl+W", false, currentFileIndex >= 0)) {
                    if (currentFileIndex >= 0) {
                        // If file is modified, prompt to save
                        if (openFiles[currentFileIndex].modified) {
                            // In a real application, you would show a dialog here
                            // For now, we'll just close without saving
                        }
                        
                        // Remove the file
                        openFiles.erase(openFiles.begin() + currentFileIndex);
                        
                        // Update current file index
                        if (openFiles.empty()) {
                            // No files left, create a new untitled file
                            openFiles.emplace_back("untitled");
                            currentFileIndex = 0;
                        } else if (currentFileIndex >= openFiles.size()) {
                            // Select the last file
                            currentFileIndex = openFiles.size() - 1;
                        }
                    }
                }
                
                ImGui::Separator();
                
                if (ImGui::MenuItem("Exit", "Alt+F4")) {
                    glfwSetWindowShouldClose(window, GLFW_TRUE);
                }
                
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("Edit", currentFileIndex >= 0)) {
                auto& editor = openFiles[currentFileIndex].editor;
                
                if (ImGui::MenuItem("Undo", "Ctrl+Z", false, editor->canUndo())) {
                    editor->undo();
                }
                
                if (ImGui::MenuItem("Redo", "Ctrl+Y", false, editor->canRedo())) {
                    editor->redo();
                }
                
                ImGui::Separator();
                
                if (ImGui::MenuItem("Cut", "Ctrl+X", false, editor->hasSelection())) {
                    editor->cutSelection();
                    openFiles[currentFileIndex].modified = true;
                }
                
                if (ImGui::MenuItem("Copy", "Ctrl+C", false, editor->hasSelection())) {
                    editor->copySelection();
                }
                
                if (ImGui::MenuItem("Paste", "Ctrl+V")) {
                    editor->pasteAtCursor();
                    openFiles[currentFileIndex].modified = true;
                }
                
                ImGui::Separator();
                
                if (ImGui::MenuItem("Select All", "Ctrl+A")) {
                    editor->selectAll();
                }
                
                ImGui::Separator();
                
                if (ImGui::MenuItem("Find", "Ctrl+F")) {
                    findReplaceState.isOpen = true;
                    findReplaceState.focusFind = true;
                }
                
                if (ImGui::MenuItem("Replace", "Ctrl+H")) {
                    findReplaceState.isOpen = true;
                    findReplaceState.focusReplace = true;
                }
                
                if (ImGui::MenuItem("Find Next", "F3", false, strlen(findReplaceState.findBuffer) > 0)) {
                    if (currentFileIndex >= 0) {
                        editor->searchNext();
                    }
                }
                
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("View", currentFileIndex >= 0)) {
                auto& editor = openFiles[currentFileIndex].editor;
                
                bool syntaxHighlighting = editor->isSyntaxHighlightingEnabled();
                if (ImGui::MenuItem("Syntax Highlighting", NULL, syntaxHighlighting)) {
                    editor->enableSyntaxHighlighting(!syntaxHighlighting);
                }
                
                ImGui::Separator();
                
                if (ImGui::MenuItem("File Browser", NULL, fileBrowserState.isOpen)) {
                    fileBrowserState.isOpen = !fileBrowserState.isOpen;
                }
                
                ImGui::EndMenu();
            }
            
            ImGui::EndMainMenuBar();
        }
        
        // File tabs
        if (!openFiles.empty()) {
            ImGui::BeginChild("Tabs", ImVec2(ImGui::GetContentRegionAvail().x, 30), false);
            
            float tabWidth = 150.0f;
            float tabSpacing = 2.0f;
            float tabsStartX = ImGui::GetCursorPosX();
            float tabsEndX = tabsStartX + ImGui::GetContentRegionAvail().x;
            
            for (int i = 0; i < openFiles.size(); i++) {
                // Calculate tab position
                float tabX = tabsStartX + i * (tabWidth + tabSpacing);
                
                // Skip tabs that would go beyond the window
                if (tabX + tabWidth > tabsEndX) {
                    // In a real application, you would add tab scrolling
                    break;
                }
                
                ImGui::SetCursorPosX(tabX);
                
                // Tab style based on selection
                if (i == currentFileIndex) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
                } else {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_Button));
                }
                
                // Display the tab with a close button
                std::string tabName = openFiles[i].displayName;
                if (openFiles[i].modified) {
                    tabName += "*";
                }
                
                // Make tab name fit within tab width
                if (ImGui::CalcTextSize(tabName.c_str()).x > tabWidth - 30) {
                    // Truncate and add ellipsis
                    size_t maxChars = 0;
                    float width = 0;
                    for (size_t j = 0; j < tabName.length(); j++) {
                        width += ImGui::CalcTextSize(&tabName[j], &tabName[j+1]).x;
                        if (width > tabWidth - 40) {
                            break;
                        }
                        maxChars++;
                    }
                    
                    if (maxChars < tabName.length()) {
                        tabName = tabName.substr(0, maxChars) + "...";
                    }
                }
                
                if (ImGui::Button(tabName.c_str(), ImVec2(tabWidth - 25, 20))) {
                    currentFileIndex = i;
                }
                
                ImGui::PopStyleColor();
                
                // Close button
                ImGui::SameLine(tabX + tabWidth - 20);
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
                
                if (ImGui::Button(("x##" + std::to_string(i)).c_str(), ImVec2(20, 20))) {
                    // If file is modified, prompt to save
                    if (openFiles[i].modified) {
                        // In a real application, you would show a dialog here
                        // For now, we'll just close without saving
                    }
                    
                    // Remove the file
                    openFiles.erase(openFiles.begin() + i);
                    
                    // Update current file index
                    if (openFiles.empty()) {
                        // No files left, create a new untitled file
                        openFiles.emplace_back("untitled");
                        currentFileIndex = 0;
                    } else if (currentFileIndex >= openFiles.size() || currentFileIndex == i) {
                        // Select the last file
                        currentFileIndex = openFiles.size() - 1;
                    } else if (currentFileIndex > i) {
                        // Adjust current file index
                        currentFileIndex--;
                    }
                    
                    // Break the loop as we've modified the vector
                    break;
                }
                
                ImGui::PopStyleVar();
                ImGui::PopStyleColor();
            }
            
            ImGui::EndChild();
        }
        
        // File browser panel
        if (fileBrowserState.isOpen) {
            ImGui::Begin("File Browser", &fileBrowserState.isOpen);
            
            // Current path
            ImGui::Text("Path: %s", fileBrowserState.currentPath.c_str());
            
            // Navigation buttons
            if (ImGui::Button("Up")) {
                fileBrowserState.navigateToParent();
            }
            
            ImGui::SameLine();
            
            if (ImGui::Button("Refresh")) {
                fileBrowserState.refreshEntries();
            }
            
            // Search box
            ImGui::InputText("Search", fileBrowserState.searchBuffer, IM_ARRAYSIZE(fileBrowserState.searchBuffer));
            std::string searchTerm = fileBrowserState.searchBuffer;
            std::transform(searchTerm.begin(), searchTerm.end(), searchTerm.begin(),
                [](unsigned char c){ return std::tolower(c); });
            
            // Display entries
            ImGui::BeginChild("Entries", ImVec2(0, 0), true);
            
            for (const auto& entry : fileBrowserState.entries) {
                std::string filename = entry.path().filename().string();
                
                // Apply search filter
                if (!searchTerm.empty()) {
                    std::string lowerFilename = filename;
                    std::transform(lowerFilename.begin(), lowerFilename.end(), lowerFilename.begin(),
                        [](unsigned char c){ return std::tolower(c); });
                    
                    if (lowerFilename.find(searchTerm) == std::string::npos) {
                        continue;
                    }
                }
                
                // Display icon based on entry type
                if (entry.is_directory()) {
                    ImGui::Text("[D] %s", filename.c_str());
                    
                    if (ImGui::IsItemClicked()) {
                        // Navigate into the directory
                        fileBrowserState.navigateTo(entry.path().string());
                    }
                } else {
                    ImGui::Text("[F] %s", filename.c_str());
                    
                    if (ImGui::IsItemClicked()) {
                        // Open the file
                        std::string filepath = entry.path().string();
                        
                        // Check if the file is already open
                        bool alreadyOpen = false;
                        for (size_t i = 0; i < openFiles.size(); i++) {
                            if (openFiles[i].filename == filepath) {
                                currentFileIndex = i;
                                alreadyOpen = true;
                                break;
                            }
                        }
                        
                        // If not already open, create a new tab
                        if (!alreadyOpen) {
                            openFiles.emplace_back(filepath);
                            currentFileIndex = openFiles.size() - 1;
                        }
                    }
                }
            }
            
            ImGui::EndChild();
            
            ImGui::End();
        }
        
        // Status bar
        ImGui::Begin("StatusBar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoTitleBar);
        
        if (currentFileIndex >= 0) {
            auto& currentFile = openFiles[currentFileIndex];
            auto& editor = currentFile.editor;
            
            ImGui::Text("Line: %zu, Column: %zu", editor->getCursorLine() + 1, editor->getCursorCol() + 1);
            ImGui::SameLine(ImGui::GetWindowWidth() - 200);
            ImGui::Text("%s%s", 
                currentFile.filename == "untitled" ? "Untitled" : currentFile.filename.c_str(), 
                currentFile.modified ? " *" : "");
        } else {
            ImGui::Text("No file open");
        }
        
        ImGui::End();
        
        // Editor window with tabs
        ImGui::Begin("Editor", nullptr, ImGuiWindowFlags_HorizontalScrollbar);
        
        // Render file tabs inside the editor window
        if (!openFiles.empty()) {
            ImGui::BeginChild("Tabs", ImVec2(ImGui::GetContentRegionAvail().x, 30), false);
            
            float tabWidth = 150.0f;
            float tabSpacing = 2.0f;
            float tabsStartX = ImGui::GetCursorPosX();
            float tabsEndX = tabsStartX + ImGui::GetContentRegionAvail().x;
            
            for (int i = 0; i < openFiles.size(); i++) {
                // Calculate tab position
                float tabX = tabsStartX + i * (tabWidth + tabSpacing);
                
                // Skip tabs that would go beyond the window
                if (tabX + tabWidth > tabsEndX) {
                    // In a real application, you would add tab scrolling
                    break;
                }
                
                ImGui::SetCursorPosX(tabX);
                
                // Tab style based on selection
                if (i == currentFileIndex) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
                } else {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_Button));
                }
                
                // Display the tab with a close button
                std::string tabName = openFiles[i].displayName;
                if (openFiles[i].modified) {
                    tabName += "*";
                }
                
                // Make tab name fit within tab width
                if (ImGui::CalcTextSize(tabName.c_str()).x > tabWidth - 30) {
                    // Truncate and add ellipsis
                    size_t maxChars = 0;
                    float width = 0;
                    for (size_t j = 0; j < tabName.length(); j++) {
                        width += ImGui::CalcTextSize(&tabName[j], &tabName[j+1]).x;
                        if (width > tabWidth - 40) {
                            break;
                        }
                        maxChars++;
                    }
                    
                    if (maxChars < tabName.length()) {
                        tabName = tabName.substr(0, maxChars) + "...";
                    }
                }
                
                if (ImGui::Button(tabName.c_str(), ImVec2(tabWidth - 25, 20))) {
                    currentFileIndex = i;
                }
                
                ImGui::PopStyleColor();
                
                // Close button
                ImGui::SameLine(tabX + tabWidth - 20);
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
                
                if (ImGui::Button(("x##" + std::to_string(i)).c_str(), ImVec2(20, 20))) {
                    // If file is modified, prompt to save
                    if (openFiles[i].modified) {
                        // In a real application, you would show a dialog here
                        // For now, we'll just close without saving
                    }
                    
                    // Remove the file
                    openFiles.erase(openFiles.begin() + i);
                    
                    // Update current file index
                    if (openFiles.empty()) {
                        // No files left, create a new untitled file
                        openFiles.emplace_back("untitled");
                        currentFileIndex = 0;
                    } else if (currentFileIndex >= openFiles.size() || currentFileIndex == i) {
                        // Select the last file
                        currentFileIndex = openFiles.size() - 1;
                    } else if (currentFileIndex > i) {
                        // Adjust current file index
                        currentFileIndex--;
                    }
                    
                    // Break the loop as we've modified the vector
                    break;
                }
                
                ImGui::PopStyleVar();
                ImGui::PopStyleColor();
            }
            
            ImGui::EndChild();
        }
        
        // Editor content
        if (currentFileIndex >= 0) {
            auto& currentFile = openFiles[currentFileIndex];
            auto& editor = currentFile.editor;
            
            // Render ImGui
            ImGui::Render();
            int display_w, display_h;
            glfwGetFramebufferSize(window, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        } else {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No file open");
        }
        
        ImGui::End();
        
        // Find and Replace dialog
        if (findReplaceState.isOpen) {
            ImGui::SetNextWindowSize(ImVec2(400, 0), ImGuiCond_FirstUseEver);
            if (ImGui::Begin("Find and Replace", &findReplaceState.isOpen)) {
                // Find input
                if (findReplaceState.focusFind) {
                    ImGui::SetKeyboardFocusHere();
                    findReplaceState.focusFind = false;
                }
                
                bool findEnterPressed = ImGui::InputText("Find", findReplaceState.findBuffer, 
                                                       IM_ARRAYSIZE(findReplaceState.findBuffer), 
                                                       ImGuiInputTextFlags_EnterReturnsTrue);
                
                // Replace input
                if (findReplaceState.focusReplace) {
                    ImGui::SetKeyboardFocusHere();
                    findReplaceState.focusReplace = false;
                }
                
                bool replaceEnterPressed = ImGui::InputText("Replace", findReplaceState.replaceBuffer, 
                                                          IM_ARRAYSIZE(findReplaceState.replaceBuffer), 
                                                          ImGuiInputTextFlags_EnterReturnsTrue);
                
                // Options
                ImGui::Checkbox("Case Sensitive", &findReplaceState.caseSensitive);
                ImGui::SameLine();
                ImGui::Checkbox("Whole Word", &findReplaceState.wholeWord);
                
                // Buttons
                if (ImGui::Button("Find Next") || findEnterPressed) {
                    if (currentFileIndex >= 0 && strlen(findReplaceState.findBuffer) > 0) {
                        // Search for the text
                        std::string searchTerm = findReplaceState.findBuffer;
                        if (currentFileIndex >= 0) {
                            auto& currentFile = openFiles[currentFileIndex];
                            auto& editor = currentFile.editor;
                            editor->search(searchTerm, findReplaceState.caseSensitive);
                        }
                    }
                }
                
                ImGui::SameLine();
                
                if (ImGui::Button("Replace")) {
                    if (currentFileIndex >= 0 && strlen(findReplaceState.findBuffer) > 0) {
                        // Replace the current occurrence
                        std::string searchTerm = findReplaceState.findBuffer;
                        std::string replaceTerm = findReplaceState.replaceBuffer;
                        
                        if (currentFileIndex >= 0) {
                            auto& currentFile = openFiles[currentFileIndex];
                            auto& editor = currentFile.editor;
                            if (editor->replace(searchTerm, replaceTerm, findReplaceState.caseSensitive)) {
                                currentFile.modified = true;
                            }
                        }
                    }
                }
                
                ImGui::SameLine();
                
                if (ImGui::Button("Replace All")) {
                    if (currentFileIndex >= 0 && strlen(findReplaceState.findBuffer) > 0) {
                        // Replace all occurrences
                        std::string searchTerm = findReplaceState.findBuffer;
                        std::string replaceTerm = findReplaceState.replaceBuffer;
                        
                        if (currentFileIndex >= 0) {
                            auto& currentFile = openFiles[currentFileIndex];
                            auto& editor = currentFile.editor;
                            if (editor->replaceAll(searchTerm, replaceTerm, findReplaceState.caseSensitive)) {
                                currentFile.modified = true;
                            }
                        }
                    }
                }
                
                ImGui::SameLine();
                
                if (ImGui::Button("Close")) {
                    findReplaceState.isOpen = false;
                }
                
                ImGui::End();
            } else {
                findReplaceState.isOpen = false;
            }
        }
    }

    logger.log("Cleaning up...");
    
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    
    logger.log("TextEditor application terminated successfully.");
    
    return 0;
}