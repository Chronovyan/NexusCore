#pragma once

#include <memory>
#include <string>
#include <iostream>
#include "Injector.hpp"

namespace examples {

// Sample interfaces
struct ILogger {
    virtual ~ILogger() = default;
    virtual void log(const std::string& message) = 0;
};

struct IFileSystem {
    virtual ~IFileSystem() = default;
    virtual bool fileExists(const std::string& path) = 0;
    virtual std::string readFile(const std::string& path) = 0;
    virtual bool writeFile(const std::string& path, const std::string& content) = 0;
};

struct ITextBuffer {
    virtual ~ITextBuffer() = default;
    virtual void insertLine(int index, const std::string& text) = 0;
    virtual void deleteLine(int index) = 0;
    virtual std::string getLine(int index) const = 0;
    virtual int getLineCount() const = 0;
};

// Concrete implementations

// Simple console logger
class ConsoleLogger : public ILogger {
public:
    void log(const std::string& message) override {
        std::cout << "[INFO] " << message << std::endl;
    }
};

// Simple in-memory file system simulation
class MemoryFileSystem : public IFileSystem {
public:
    explicit MemoryFileSystem(const std::string& rootPath) 
        : _rootPath(rootPath) {}

    bool fileExists(const std::string& path) override {
        return _files.find(path) != _files.end();
    }

    std::string readFile(const std::string& path) override {
        if (!fileExists(path)) {
            throw std::runtime_error("File not found: " + path);
        }
        return _files[path];
    }

    bool writeFile(const std::string& path, const std::string& content) override {
        _files[path] = content;
        return true;
    }

private:
    std::string _rootPath;
    std::unordered_map<std::string, std::string> _files;
};

// Simple text buffer implementation
class SimpleTextBuffer : public ITextBuffer {
public:
    SimpleTextBuffer() {}

    void insertLine(int index, const std::string& text) override {
        if (index < 0 || index > static_cast<int>(_lines.size())) {
            throw std::out_of_range("Line index out of range");
        }
        _lines.insert(_lines.begin() + index, text);
    }

    void deleteLine(int index) override {
        if (index < 0 || index >= static_cast<int>(_lines.size())) {
            throw std::out_of_range("Line index out of range");
        }
        _lines.erase(_lines.begin() + index);
    }

    std::string getLine(int index) const override {
        if (index < 0 || index >= static_cast<int>(_lines.size())) {
            throw std::out_of_range("Line index out of range");
        }
        return _lines[index];
    }

    int getLineCount() const override {
        return static_cast<int>(_lines.size());
    }

private:
    std::vector<std::string> _lines;
};

// Editor that depends on the other components
class Editor {
public:
    Editor(std::shared_ptr<ILogger> logger,
           std::shared_ptr<IFileSystem> fileSystem,
           std::shared_ptr<ITextBuffer> textBuffer)
        : _logger(logger),
          _fileSystem(fileSystem),
          _textBuffer(textBuffer) {
        _logger->log("Editor initialized");
    }

    bool openFile(const std::string& path) {
        _logger->log("Opening file: " + path);
        if (!_fileSystem->fileExists(path)) {
            _logger->log("File does not exist: " + path);
            return false;
        }

        std::string content = _fileSystem->readFile(path);
        // Split content into lines and load into buffer
        // (simplified implementation)
        _textBuffer->insertLine(0, content);
        return true;
    }

    void run() {
        _logger->log("Editor is running");
        // Main editor loop would go here
    }

private:
    std::shared_ptr<ILogger> _logger;
    std::shared_ptr<IFileSystem> _fileSystem;
    std::shared_ptr<ITextBuffer> _textBuffer;
};

// Factory for more complex component creation
class TextBufferFactory {
public:
    static std::shared_ptr<ITextBuffer> createBuffer(di::Injector& injector) {
        auto logger = injector.resolve<ILogger>();
        logger->log("Creating a new text buffer");
        return std::make_shared<SimpleTextBuffer>();
    }
};

// Example usage:
inline void runExample() {
    di::Injector injector;

    // Register components
    injector.registerType<ILogger, ConsoleLogger>(di::Lifetime::Singleton);
    
    // Register with an instance that has constructor parameters
    injector.registerInstance<IFileSystem>(
        std::make_shared<MemoryFileSystem>("./workspace")
    );
    
    // Register with a factory function
    injector.registerFactory<ITextBuffer>(
        [](di::Injector& inj) {
            return TextBufferFactory::createBuffer(inj);
        },
        di::Lifetime::Transient
    );
    
    // Register the editor itself
    injector.registerType<Editor, Editor>(di::Lifetime::Transient);

    // Resolve and use the editor
    auto editor = injector.resolve<Editor>();
    editor->run();
    
    // We could also resolve individual components
    auto logger = injector.resolve<ILogger>();
    logger->log("Example completed");
}

} // namespace examples 