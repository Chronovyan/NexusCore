#include "ErrorReporterStub.h"

namespace ai_editor {

void ErrorReporter::report(Severity severity, const std::string& message, const std::string& source) {
    const char* level = "UNKNOWN";
    switch (severity) {
        case Severity::DEBUG: level = "DEBUG"; break;
        case Severity::INFO: level = "INFO"; break;
        case Severity::WARNING: level = "WARNING"; break;
        case Severity::ERROR: level = "ERROR"; break;
        case Severity::CRITICAL: level = "CRITICAL"; break;
    }
    std::cerr << "[" << level << "] " << source << ": " << message << std::endl;
}

void ErrorReporter::logError(const std::string& message) {
    getInstance().report(Severity::ERROR, message, "TextBuffer");
}

} // namespace ai_editor
