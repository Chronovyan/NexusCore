#include "src/Editor.h"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>

// Test parameters
constexpr int NUM_THREADS = 4;
constexpr int ITERATIONS_PER_THREAD = 20;
constexpr int MAX_TEST_DURATION_MS = 30000; // 30 seconds

std::atomic<bool> test_failed{false};
std::atomic<int> editors_created{0};
std::atomic<int> threads_completed{0};

void test_thread_func(int thread_id) {
    try {
        std::cout << "Thread " << thread_id << " starting...\n";
        
        for (int i = 0; i < ITERATIONS_PER_THREAD && !test_failed; ++i) {
            // Create an editor
            Editor editor;
            editors_created++;
            
            // Exercise syntax highlighting functionality
            editor.enableSyntaxHighlighting(true);
            editor.setFilename("test.cpp");
            
            // Add some content
            editor.addLine("// This is a test");
            editor.addLine("int main() {");
            editor.addLine("    return 0;");
            editor.addLine("}");
            
            // Force syntax highlighting update
            auto styles = editor.getHighlightingStyles();
            
            // Small delay to increase chance of thread interaction
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        
        std::cout << "Thread " << thread_id << " completed successfully.\n";
        threads_completed++; // Increment the completed threads counter
    } 
    catch (const std::exception& e) {
        std::cerr << "Thread " << thread_id << " failed with exception: " << e.what() << std::endl;
        test_failed = true;
    }
    catch (...) {
        std::cerr << "Thread " << thread_id << " failed with unknown exception." << std::endl;
        test_failed = true;
    }
}

int main() {
    std::cout << "=== Deadlock Verification Test ===" << std::endl;
    std::cout << "This test verifies that the editor doesn't deadlock" << std::endl;
    std::cout << "when creating editors and using syntax highlighting." << std::endl;
    std::cout << std::endl;
    
    auto start_time = std::chrono::steady_clock::now();
    std::vector<std::thread> threads;
    
    try {
        // Create multiple threads that initialize Editor objects
        std::cout << "Starting " << NUM_THREADS << " threads..." << std::endl;
        for (int i = 0; i < NUM_THREADS; ++i) {
            threads.emplace_back(test_thread_func, i);
        }
        
        // Wait for all threads or timeout
        bool timeout_occurred = false;
        while (threads_completed < NUM_THREADS && !test_failed) {
            auto current_time = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time);
            
            if (elapsed.count() > MAX_TEST_DURATION_MS) {
                std::cerr << "Test timed out after " << MAX_TEST_DURATION_MS << "ms - possible deadlock!" << std::endl;
                timeout_occurred = true;
                test_failed = true;
                break;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        // Join all threads that are still running
        for (auto& t : threads) {
            if (t.joinable()) {
                if (timeout_occurred) {
                    // If timeout occurred, threads may be deadlocked
                    // In a real production scenario, we would use platform-specific
                    // techniques to forcibly terminate the threads
                    t.detach(); // Just detach them in this test
                } else {
                    t.join();
                }
            }
        }
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        std::cout << std::endl;
        std::cout << "Test completed in " << duration.count() << "ms" << std::endl;
        std::cout << "Editors successfully created: " << editors_created << std::endl;
        std::cout << "Threads completed: " << threads_completed << " of " << NUM_THREADS << std::endl;
        
        if (test_failed) {
            std::cout << "TEST FAILED: Issues detected during test!" << std::endl;
            return 1;
        } else {
            std::cout << "TEST PASSED: No deadlocks or exceptions detected." << std::endl;
            return 0;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "Test failed with unknown exception." << std::endl;
        return 1;
    }
} 