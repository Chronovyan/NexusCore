@echo off
setlocal

echo Building OpenAIClientTest with debug logging...

REM Create build directory
if not exist build_debug mkdir build_debug

REM Create logs directory if it doesn't exist
if not exist logs mkdir logs

REM Create a debug version of AppDebugLog.h with iostream
echo Creating debug log header...
(
echo #ifndef APP_DEBUG_LOG_H
echo #define APP_DEBUG_LOG_H
echo.
echo #include ^<fstream^>
echo #include ^<iostream^>
echo #include ^<string^>
echo #include ^<chrono^>
echo #include ^<iomanip^>
echo #include ^<sstream^>
echo.
echo class AppDebugLog {
echo public:
echo     static AppDebugLog^& getInstance^(^) {
echo         static AppDebugLog instance;
echo         return instance;
echo     }
echo.
echo     void initialize^(const std::string^& appName^) {
echo         if ^(isInitialized^) return;
echo         
echo         try {
echo             // Create logs directory if it doesn't exist
echo             std::string logFilePath = "logs/" + appName + ".log";
echo             
echo             logFile.open^(logFilePath, std::ios::out^);
echo             
echo             if ^(logFile.is_open^(^)^) {
echo                 isInitialized = true;
echo                 log^("Log initialized for " + appName^);
echo                 log^("Log file: " + logFilePath^);
echo             }
echo             else {
echo                 // Fallback to console if file can't be created
echo                 std::cerr ^<^< "Failed to open log file: " ^<^< logFilePath ^<^< std::endl;
echo             }
echo         }
echo         catch ^(const std::exception^& e^) {
echo             std::cerr ^<^< "Error initializing log: " ^<^< e.what^(^) ^<^< std::endl;
echo         }
echo     }
echo.
echo     void log^(const std::string^& message^) {
echo         if ^(!isInitialized^) {
echo             std::cerr ^<^< "WARNING: Log not initialized: " ^<^< message ^<^< std::endl;
echo             return;
echo         }
echo         
echo         try {
echo             auto now = std::chrono::system_clock::now^(^);
echo             auto ms = std::chrono::duration_cast^<std::chrono::milliseconds^>^(now.time_since_epoch^(^)^).count^(^) %% 1000;
echo             
echo             std::time_t time = std::chrono::system_clock::to_time_t^(now^);
echo             std::tm tm;
echo             localtime_s^(^&tm, ^&time^);
echo             
echo             char buffer[32];
echo             strftime^(buffer, sizeof^(buffer^), "[%%H:%%M:%%S", ^&tm^);
echo             
echo             logFile ^<^< buffer ^<^< "." ^<^< std::setfill^('0'^) ^<^< std::setw^(3^) ^<^< ms ^<^< "] " 
echo                    ^<^< message ^<^< std::endl;
echo             logFile.flush^(^);
echo             
echo             // Also output to console for debugging
echo             std::cout ^<^< buffer ^<^< "." ^<^< std::setfill^('0'^) ^<^< std::setw^(3^) ^<^< ms ^<^< "] " 
echo                      ^<^< message ^<^< std::endl;
echo         }
echo         catch ^(const std::exception^& e^) {
echo             std::cerr ^<^< "Error writing to log: " ^<^< e.what^(^) ^<^< std::endl;
echo         }
echo     }
echo     
echo     void logError^(const std::string^& message^) {
echo         log^("ERROR: " + message^);
echo     }
echo.
echo     ~AppDebugLog^(^) {
echo         if ^(isInitialized ^&^& logFile.is_open^(^)^) {
echo             log^("Log closed"^);
echo             logFile.close^(^);
echo         }
echo     }
echo.
echo private:
echo     AppDebugLog^(^) : isInitialized^(false^) {}
echo     
echo     bool isInitialized;
echo     std::ofstream logFile;
echo };
echo.
echo // Convenience macros for logging
echo #define LOG_INIT^(appName^) AppDebugLog::getInstance^(^).initialize^(appName^)
echo #define LOG_DEBUG^(message^) AppDebugLog::getInstance^(^).log^(message^)
echo #define LOG_ERROR^(message^) AppDebugLog::getInstance^(^).logError^(message^)
echo.
echo #endif // APP_DEBUG_LOG_H
) > src\AppDebugLog.h

echo Creating debug OpenAIClientTest.cpp with hardcoded API key...
(
echo #include "OpenAI_API_Client.h"
echo #include "AppDebugLog.h"
echo #include ^<iostream^>
echo #include ^<string^>
echo.
echo using namespace ai_editor;
echo.
echo int main^(int argc, char** argv^) {
echo     // Initialize debug logging
echo     LOG_INIT^("OpenAIClientTest"^);
echo     LOG_DEBUG^("Starting OpenAIClientTest"^);
echo     
echo     try {
echo         // Use a placeholder API key - replace with a real one if needed
echo         std::string apiKey = "your_api_key_here";
echo         
echo         // For testing with environment variable
echo         const char* envApiKey = std::getenv^("OPENAI_API_KEY"^);
echo         if ^(envApiKey^) {
echo             LOG_DEBUG^("Using API key from environment variable"^);
echo             apiKey = envApiKey;
echo         } else {
echo             LOG_DEBUG^("Using placeholder API key"^);
echo         }
echo         
echo         // Create the OpenAI API client
echo         LOG_DEBUG^("Creating OpenAI API client"^);
echo         OpenAI_API_Client client^(apiKey^);
echo         LOG_DEBUG^("OpenAI API client created successfully"^);
echo         
echo         // Create a simple request
echo         LOG_DEBUG^("Setting up test request"^);
echo         std::vector^<ApiChatMessage^> messages;
echo         ApiChatMessage systemMsg;
echo         systemMsg.role = "system";
echo         systemMsg.content = "You are a helpful assistant.";
echo         
echo         ApiChatMessage userMsg;
echo         userMsg.role = "user";
echo         userMsg.content = "Hello, what can you do for me?";
echo         
echo         messages.push_back^(systemMsg^);
echo         messages.push_back^(userMsg^);
echo         
echo         std::vector^<ApiToolDefinition^> tools; // Empty tools vector
echo         
echo         // Log all request details
echo         LOG_DEBUG^("Request details:"^);
echo         LOG_DEBUG^(" - Model: gpt-3.5-turbo"^);
echo         LOG_DEBUG^(" - Messages: " + std::to_string^(messages.size^(^)^)^);
echo         for ^(const auto^& msg : messages^) {
echo             LOG_DEBUG^("   * " + msg.role + ": " + msg.content^);
echo         }
echo         
echo         // Send request to OpenAI
echo         LOG_DEBUG^("Sending request to OpenAI API..."^);
echo         ApiResponse response = client.sendChatCompletionRequest^(
echo             messages,
echo             tools,
echo             "gpt-3.5-turbo",
echo             0.7f,
echo             150
echo         ^);
echo         
echo         // Log and print result
echo         if ^(response.success^) {
echo             LOG_DEBUG^("Request successful"^);
echo             LOG_DEBUG^("Response content: " + response.content^);
echo             std::cout ^<^< "Response: " ^<^< response.content ^<^< std::endl;
echo             return 0;
echo         } else {
echo             LOG_ERROR^("Request failed: " + response.error_message^);
echo             std::cerr ^<^< "ERROR: " ^<^< response.error_message ^<^< std::endl;
echo             return 1;
echo         }
echo     }
echo     catch ^(const std::exception^& e^) {
echo         LOG_ERROR^("Exception occurred: " + std::string^(e.what^(^)^)^);
echo         std::cerr ^<^< "EXCEPTION: " ^<^< e.what^(^) ^<^< std::endl;
echo         return 1;
echo     }
echo     catch ^(...^) {
echo         LOG_ERROR^("Unknown exception occurred"^);
echo         std::cerr ^<^< "UNKNOWN EXCEPTION" ^<^< std::endl;
echo         return 1;
echo     }
echo }
) > src\OpenAIClientTest.cpp

REM Configure and build using CMake
cd build_debug
cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Debug ^
      -DCPR_FORCE_USE_SYSTEM_CURL=OFF ^
      -DBUILD_SHARED_LIBS=ON ^
      -DCPR_BUILD_TESTS=OFF ^
      -DCPR_BUILD_TESTS_SSL=OFF ^
      -DCPR_ENABLE_CERTIFICATE_OPTIMIZATION=OFF ^
      ..

if %ERRORLEVEL% neq 0 (
  echo CMake configuration failed.
  exit /b %ERRORLEVEL%
)

echo Building OpenAIClientTest...
cmake --build . --config Debug --target OpenAIClientTest

if %ERRORLEVEL% neq 0 (
  echo Build failed.
  cd ..
  exit /b %ERRORLEVEL%
)

cd ..
echo Build succeeded!
echo.
echo To run the test with your OpenAI API key:
echo.
echo   set OPENAI_API_KEY=your_api_key_here
echo   .\\build_debug\\Debug\\OpenAIClientTest.exe
echo.
echo Debug logs will be written to logs/OpenAIClientTest.log 