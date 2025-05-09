#ifndef EDITOR_ERROR_H
#define EDITOR_ERROR_H

#include <string>
#include <stdexcept>
#include <iostream>

// Base class for all editor-specific exceptions
class EditorException : public std::runtime_error {
public:
    enum class Severity {
        Warning,    // Non-fatal but noteworthy
        Error,      // Operation failed but editor can continue
        Critical    // Serious error that may require termination or recovery
    };

    EditorException(const std::string& message, Severity severity = Severity::Error)
        : std::runtime_error(message), severity_(severity) {}

    // Get the severity level of the exception
    Severity getSeverity() const { return severity_; }

    // Get a string representation of the severity
    std::string getSeverityString() const {
        switch (severity_) {
            case Severity::Warning: return "Warning";
            case Severity::Error: return "Error";
            case Severity::Critical: return "Critical Error";
            default: return "Unknown";
        }
    }

    // Get a formatted error message with severity
    std::string getFormattedMessage() const {
        return getSeverityString() + ": " + what();
    }

private:
    Severity severity_;
};

// Specialized exceptions for different error categories
class TextBufferException : public EditorException {
public:
    TextBufferException(const std::string& message, Severity severity = Severity::Error)
        : EditorException("TextBuffer: " + message, severity) {}
};

class CommandException : public EditorException {
public:
    CommandException(const std::string& message, Severity severity = Severity::Error)
        : EditorException("Command: " + message, severity) {}
};

class SyntaxHighlightingException : public EditorException {
public:
    SyntaxHighlightingException(const std::string& message, Severity severity = Severity::Error)
        : EditorException("Syntax Highlighting: " + message, severity) {}
};

class FileOperationException : public EditorException {
public:
    FileOperationException(const std::string& message, Severity severity = Severity::Error)
        : EditorException("File Operation: " + message, severity) {}
};

// Error logging and reporting utility
class ErrorReporter {
public:
    // Log an exception to std::cerr (could be extended to log to file)
    static void logException(const EditorException& ex) {
        std::cerr << ex.getFormattedMessage() << std::endl;
    }

    // Log a general exception
    static void logException(const std::exception& ex) {
        std::cerr << "Exception: " << ex.what() << std::endl;
    }

    // Log an unknown exception
    static void logUnknownException(const std::string& context) {
        std::cerr << "Unknown exception in " << context << std::endl;
    }

    // Log a general error message
    static void logError(const std::string& message) {
        std::cerr << "Error: " << message << std::endl;
    }

    // Log a warning message
    static void logWarning(const std::string& message) {
        std::cerr << "Warning: " << message << std::endl;
    }
};

#endif // EDITOR_ERROR_H 