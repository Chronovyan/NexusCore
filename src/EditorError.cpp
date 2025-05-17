#include "EditorError.h"

// Define static members of ErrorReporter
bool ErrorReporter::debugLoggingEnabled = false;
bool ErrorReporter::suppressAllWarnings = false;
EditorException::Severity ErrorReporter::severityThreshold = EditorException::Severity::Warning; 