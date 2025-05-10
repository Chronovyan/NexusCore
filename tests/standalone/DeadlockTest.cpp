#include "src/Editor.h"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>
#include <random>
#include <algorithm>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <sstream>

// For thread naming
#ifdef _WIN32
#include <windows.h>
// For SetThreadDescription (Windows 10 1607+)
#pragma comment(lib, "synchronization.lib")
extern "C" {
    HRESULT WINAPI SetThreadDescription(HANDLE hThread, PCWSTR lpThreadDescription);
}
#elif defined(__linux__)
#include <pthread.h>
#endif

// Test parameters
constexpr int NUM_THREADS = 8;                 // Increased thread count
constexpr int ITERATIONS_PER_THREAD = 30;      // More iterations per thread
constexpr int MAX_TEST_DURATION_MS = 30000;    // 30 seconds timeout
constexpr int MAX_CONCURRENT_EDITORS = 20;     // Maximum number of concurrent editors

// Shared test state with atomic variables for thread safety
std::atomic<bool> test_failed{false};
std::atomic<int> editors_created{0};
std::atomic<int> threads_completed{0};
std::atomic<int> current_editor_count{0};      // Track active editors
std::atomic<int> next_thread_id{0};            // For unique thread IDs

// Synchronization primitives for editor creation limits
std::mutex editor_limit_mutex;
std::condition_variable editor_limit_cv;

// Thread-safe logging
std::mutex log_mutex;
void thread_log(const std::string& message) {
    std::lock_guard<std::mutex> lock(log_mutex);
    std::cout << "[Thread " << std::this_thread::get_id() << "] " << message << std::endl;
}

// Set thread name for debugging
void set_thread_name(const std::string& name) {
    #ifdef _WIN32
    // Convert to wide string for Windows API
    std::wstring wname(name.begin(), name.end());
    // Set thread name for Windows
    SetThreadDescription(GetCurrentThread(), wname.c_str());
    #elif defined(__linux__)
    // Set thread name for Linux (limited to 16 chars including null terminator)
    std::string truncated_name = name.substr(0, 15);
    pthread_setname_np(pthread_self(), truncated_name.c_str());
    #endif
    // Thread name doesn't need to be set for other platforms in this test
}

// Random number generator for thread behavior
thread_local std::mt19937 rng{std::random_device{}()};

// Thread-safe random number generation
int random_int(int min, int max) {
    std::uniform_int_distribution<int> dist(min, max);
    return dist(rng);
}

// Create different types of files for testing
std::string generate_test_file(int type) {
    switch (type % 4) {
        case 0: return "test.cpp";
        case 1: return "test.h";
        case 2: return "test.txt";
        case 3: return "test.hpp";
        default: return "test.cpp";
    }
}

// Generate varying content based on file type
void populate_editor(Editor& editor, const std::string& filename) {
    if (filename.find(".cpp") != std::string::npos || filename.find(".h") != std::string::npos) {
        editor.addLine("// This is a C++ test file");
        editor.addLine("#include <iostream>");
        editor.addLine("#include <vector>");
        editor.addLine("int main() {");
        editor.addLine("    std::cout << \"Hello, world!\" << std::endl;");
        editor.addLine("    return 0;");
        editor.addLine("}");
    } else {
        editor.addLine("This is a plain text file");
        editor.addLine("It doesn't have any syntax highlighting");
        editor.addLine("But we'll test it anyway");
    }
}

void test_thread_func(int thread_id) {
    try {
        // Set thread name for debugging
        std::stringstream name_ss;
        name_ss << "TestThread" << thread_id;
        set_thread_name(name_ss.str());
        
        thread_log("Starting...");
        
        // Memory ordering to ensure visibility of shared state
        std::atomic_thread_fence(std::memory_order_seq_cst);
        
        for (int i = 0; i < ITERATIONS_PER_THREAD && !test_failed.load(std::memory_order_acquire); ++i) {
            // Wait if we've reached the maximum number of concurrent editors
            {
                std::unique_lock<std::mutex> lock(editor_limit_mutex);
                editor_limit_cv.wait(lock, [&] {
                    return current_editor_count.load(std::memory_order_acquire) < MAX_CONCURRENT_EDITORS;
                });
                
                // Increment active editor count
                current_editor_count.fetch_add(1, std::memory_order_acq_rel);
            }
            
            // Report progress periodically
            if (i % 10 == 0) {
                thread_log("Iteration " + std::to_string(i) + "/" + std::to_string(ITERATIONS_PER_THREAD));
            }
            
            // Create an editor with varying characteristics
            Editor editor;
            editors_created.fetch_add(1, std::memory_order_release);
            
            // Exercise different features
            bool enable_highlighting = (random_int(0, 10) > 2);  // 80% chance
            editor.enableSyntaxHighlighting(enable_highlighting);
            
            // Set a filename with different extensions to test various highlighters
            std::string filename = generate_test_file(thread_id + i);
            editor.setFilename(filename);
            
            // Add content based on file type
            populate_editor(editor, filename);
            
            // Exercise editor operations
            if (random_int(0, 10) > 5) {
                // Force syntax highlighting update
                auto styles = editor.getHighlightingStyles();
            }
            
            if (random_int(0, 10) > 7) {
                // Perform some edits
                editor.setCursor(random_int(0, 3), random_int(0, 10));
                editor.typeText("TEST");
            }
            
            if (random_int(0, 10) > 8) {
                // Test undo/redo
                if (editor.canUndo()) {
                    editor.undo();
                }
                if (editor.canRedo()) {
                    editor.redo();
                }
            }
            
            // Small random delay to increase chance of thread interaction
            std::this_thread::sleep_for(std::chrono::milliseconds(random_int(1, 20)));
            
            // Decrement active editor count
            {
                std::lock_guard<std::mutex> lock(editor_limit_mutex);
                current_editor_count.fetch_sub(1, std::memory_order_acq_rel);
            }
            
            // Notify waiting threads that an editor slot is available
            editor_limit_cv.notify_one();
        }
        
        // Use memory fence before completing
        std::atomic_thread_fence(std::memory_order_seq_cst);
        
        thread_log("Completed successfully");
        threads_completed.fetch_add(1, std::memory_order_release);
    } 
    catch (const std::exception& e) {
        std::cerr << "Thread " << thread_id << " failed with exception: " << e.what() << std::endl;
        test_failed.store(true, std::memory_order_release);
    }
    catch (...) {
        std::cerr << "Thread " << thread_id << " failed with unknown exception." << std::endl;
        test_failed.store(true, std::memory_order_release);
    }
}

int main() {
    std::cout << "=== Enhanced Deadlock & Concurrency Verification Test ===" << std::endl;
    std::cout << "This test verifies thread safety with " << NUM_THREADS << " concurrent threads" << std::endl;
    std::cout << "creating up to " << MAX_CONCURRENT_EDITORS << " editors simultaneously." << std::endl;
    std::cout << std::endl;
    
    auto start_time = std::chrono::steady_clock::now();
    std::vector<std::thread> threads;
    
    try {
        // Create multiple threads with varied behavior
        std::cout << "Starting " << NUM_THREADS << " threads..." << std::endl;
        for (int i = 0; i < NUM_THREADS; ++i) {
            threads.emplace_back(test_thread_func, i);
            
            // Small stagger between thread creation
            if (i % 2 == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        }
        
        // Wait for all threads or timeout
        bool timeout_occurred = false;
        while (threads_completed.load(std::memory_order_acquire) < NUM_THREADS && 
               !test_failed.load(std::memory_order_acquire)) {
            auto current_time = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time);
            
            if (elapsed.count() > MAX_TEST_DURATION_MS) {
                std::cerr << "Test timed out after " << MAX_TEST_DURATION_MS << "ms - possible deadlock!" << std::endl;
                std::cerr << "Current state: " << threads_completed.load() << " of " << NUM_THREADS << " threads completed" << std::endl;
                std::cerr << "Active editors: " << current_editor_count.load() << std::endl;
                timeout_occurred = true;
                test_failed.store(true, std::memory_order_release);
                break;
            }
            
            // More responsive checking
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            
            // Periodically report progress
            if (elapsed.count() % 5000 < 100) {
                std::cout << "Progress: " << threads_completed.load() << " of " << NUM_THREADS 
                          << " threads completed, " << editors_created.load() << " editors created" << std::endl;
            }
        }
        
        // Join all threads that are still running
        for (auto& t : threads) {
            if (t.joinable()) {
                if (timeout_occurred) {
                    // If timeout occurred, threads may be deadlocked
                    t.detach(); // Just detach them in this test
                } else {
                    t.join();
                }
            }
        }
        
        if (test_failed.load(std::memory_order_acquire)) {
            std::cout << std::endl << "!!! Test FAILED (deadlock or other concurrency issue detected) !!!" << std::endl;
            return 1;
        } else if (timeout_occurred) {
             std::cout << std::endl << "!!! Test FAILED (timeout) !!!" << std::endl;
            return 1;
        }
        else {
            std::cout << std::endl << "All threads completed successfully." << std::endl;
            std::cout << "Total editors created: " << editors_created.load() << std::endl;
            std::cout << "Test PASSED" << std::endl;
            return 0;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Main thread caught exception: " << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "Main thread caught unknown exception." << std::endl;
        return 1;
    }
} 