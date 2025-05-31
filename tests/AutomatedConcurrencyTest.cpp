#include <gtest/gtest.h>
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
#include "../src/Editor.h"

// For thread naming on Windows platform
#ifdef _WIN32
#include <windows.h>
// For SetThreadDescription (Windows 10 1607+)
#pragma comment(lib, "synchronization.lib")
// Don't redeclare SetThreadDescription as it's already declared in Windows headers
#elif defined(__linux__)
#include <pthread.h>
#endif

class AutomatedConcurrencyTest : public ::testing::Test {
protected:
    // Test parameters
    static constexpr int NUM_THREADS = 8;
    static constexpr int ITERATIONS_PER_THREAD = 30;
    static constexpr int MAX_TEST_DURATION_MS = 30000;  // 30 seconds timeout
    static constexpr int MAX_CONCURRENT_EDITORS = 20;
    
    // Shared test state with atomic variables for thread safety
    std::atomic<bool> test_failed{false};
    std::atomic<int> editors_created{0};
    std::atomic<int> threads_completed{0};
    std::atomic<int> current_editor_count{0};
    std::atomic<int> next_thread_id{0};
    
    // Synchronization primitives for editor creation limits
    std::mutex editor_limit_mutex;
    std::condition_variable editor_limit_cv;
    
    // Thread-safe logging
    std::mutex log_mutex;
    std::vector<std::string> test_logs;
    
    void SetUp() override {
        // Reset atomic variables for each test
        test_failed.store(false);
        editors_created.store(0);
        threads_completed.store(0);
        current_editor_count.store(0);
        next_thread_id.store(0);
        
        // Clear logs
        test_logs.clear();
    }
    
    void TearDown() override {
        // Print out logs for debugging
        for (const auto& log : test_logs) {
            std::cout << log << std::endl;
        }
    }
    
    // Thread-safe logging
    void thread_log(const std::string& message) {
        std::lock_guard<std::mutex> lock(log_mutex);
        std::stringstream ss;
        ss << "[Thread " << std::this_thread::get_id() << "] " << message;
        test_logs.push_back(ss.str());
    }
    
    // Set thread name for debugging
    static void set_thread_name(const std::string& name) {
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
    
    // Thread-safe random number generation
    int random_int(int min, int max) {
        thread_local std::mt19937 rng{std::random_device{}()};
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
    
    // Thread worker function for concurrency testing - modified to work with std::thread
    void test_thread_func_impl(int thread_id) {
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
            std::stringstream ss;
            ss << "Thread " << thread_id << " failed with exception: " << e.what();
            thread_log(ss.str());
            test_failed.store(true, std::memory_order_release);
        }
        catch (...) {
            std::stringstream ss;
            ss << "Thread " << thread_id << " failed with unknown exception.";
            thread_log(ss.str());
            test_failed.store(true, std::memory_order_release);
        }
    }
    
    // Wrapper method to call the implementation from thread
    static void test_thread_func(AutomatedConcurrencyTest* instance, int thread_id) {
        instance->test_thread_func_impl(thread_id);
    }
};

/**
 * Test that multiple threads can create and use Editor instances without deadlocking
 */
TEST_F(AutomatedConcurrencyTest, NoDeadlockWithMultipleThreads) {
    // Starting time for test timeout
    auto start_time = std::chrono::steady_clock::now();
    
    // Create and start threads
    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.push_back(std::thread(&AutomatedConcurrencyTest::test_thread_func, this, next_thread_id.fetch_add(1)));
    }
    
    // Wait for threads to complete or timeout
    bool all_completed = false;
    while (!all_completed && !test_failed.load(std::memory_order_acquire)) {
        // Check for timeout
        auto current_time = std::chrono::steady_clock::now();
        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time).count();
        if (elapsed_ms > MAX_TEST_DURATION_MS) {
            std::cout << "Test timed out after " << elapsed_ms << " ms" << std::endl;
            test_failed.store(true, std::memory_order_release);
            break;
        }
        
        // Check if all threads have completed
        all_completed = (threads_completed.load(std::memory_order_acquire) == NUM_THREADS);
        
        // Short sleep to reduce busy waiting
        if (!all_completed) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    
    // Wait for threads to join
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    // Check results
    ASSERT_FALSE(test_failed.load()) << "Test failed due to exception in at least one thread";
    ASSERT_EQ(threads_completed.load(), NUM_THREADS) << "Not all threads completed successfully";
    
    // Report statistics
    std::cout << "Concurrency test statistics:" << std::endl;
    std::cout << "  Threads: " << NUM_THREADS << std::endl;
    std::cout << "  Editors created: " << editors_created.load() << std::endl;
    std::cout << "  Threads completed: " << threads_completed.load() << std::endl;
}

/**
 * Test that creating editors in rapid succession doesn't cause resource issues
 */
TEST_F(AutomatedConcurrencyTest, RapidEditorCreation) {
    // Number of editors to create in rapid succession
    const int EDITOR_COUNT = 100;
    
    // Create editors rapidly in a single thread
    for (int i = 0; i < EDITOR_COUNT; ++i) {
        Editor editor;
        
        // Set different filenames and enable/disable highlighting
        editor.setFilename(generate_test_file(i));
        editor.enableSyntaxHighlighting(i % 2 == 0);
        
        // Add some content
        populate_editor(editor, editor.getFilename());
        
        // Perform some quick operations
        editor.setCursor(random_int(0, 3), random_int(0, 10));
        editor.typeText("Test");
        
        // If this is a supported file type, trigger syntax highlighting
        if (editor.getCurrentHighlighter() != nullptr) {
            auto styles = editor.getHighlightingStyles();
        }
        
        // No need to sleep between iterations - we want to stress the system
    }
    
    // If we got here without exceptions, the test passes
    ASSERT_TRUE(true) << "Successfully created " << EDITOR_COUNT << " editors in rapid succession";
}

/**
 * Test creating and destroying editors across thread boundaries
 */
TEST_F(AutomatedConcurrencyTest, CrossThreadEditorTransfer) {
    // This is a challenging test: we create editors in one thread and use them in another
    std::vector<std::shared_ptr<Editor>> editors;
    std::mutex editors_mutex;
    std::condition_variable editors_cv;
    std::atomic<bool> producer_done{false};
    std::atomic<bool> consumer_done{false};
    std::atomic<bool> test_passed{true};
    
    // Thread that creates editors
    std::thread producer_thread([&]() {
        try {
            set_thread_name("ProducerThread");
            thread_log("Producer thread starting");
            
            // Create several editors
            for (int i = 0; i < 20 && !consumer_done.load(); ++i) {
                // Create a new editor
                auto editor = std::make_shared<Editor>();
                
                // Configure it
                editor->setFilename(generate_test_file(i));
                editor->enableSyntaxHighlighting(true);
                populate_editor(*editor, editor->getFilename());
                
                // Add it to the shared vector
                {
                    std::lock_guard<std::mutex> lock(editors_mutex);
                    editors.push_back(editor);
                    thread_log("Producer added editor #" + std::to_string(i));
                }
                
                // Notify consumer
                editors_cv.notify_one();
                
                // Small delay to simulate real-world timing
                std::this_thread::sleep_for(std::chrono::milliseconds(random_int(5, 20)));
            }
            
            producer_done.store(true);
            editors_cv.notify_all();
            thread_log("Producer thread finished");
        }
        catch (const std::exception& e) {
            thread_log(std::string("Producer thread exception: ") + e.what());
            test_passed.store(false);
        }
        catch (...) {
            thread_log("Producer thread unknown exception");
            test_passed.store(false);
        }
    });
    
    // Thread that uses editors
    std::thread consumer_thread([&]() {
        try {
            set_thread_name("ConsumerThread");
            thread_log("Consumer thread starting");
            
            while (!producer_done.load() || !editors.empty()) {
                std::shared_ptr<Editor> editor;
                
                // Try to get an editor
                {
                    std::unique_lock<std::mutex> lock(editors_mutex);
                    if (editors.empty()) {
                        // Wait for an editor to become available or for producer to finish
                        editors_cv.wait_for(lock, std::chrono::milliseconds(100), [&]() {
                            return !editors.empty() || producer_done.load();
                        });
                        
                        // If still no editors and producer is done, exit loop
                        if (editors.empty()) {
                            if (producer_done.load()) {
                                break;
                            }
                            continue;
                        }
                    }
                    
                    // Get the editor
                    editor = editors.back();
                    editors.pop_back();
                    thread_log("Consumer got an editor, " + std::to_string(editors.size()) + " left");
                }
                
                // Use the editor
                if (editor) {
                    // Perform various operations
                    editor->setCursor(0, 0);
                    editor->typeText("Modified by consumer");
                    
                    if (editor->canUndo()) {
                        editor->undo();
                    }
                    
                    // Get highlighting if supported
                    if (editor->getCurrentHighlighter() != nullptr) {
                        auto styles = editor->getHighlightingStyles();
                    }
                    
                    // Small delay to simulate processing
                    std::this_thread::sleep_for(std::chrono::milliseconds(random_int(10, 30)));
                }
            }
            
            consumer_done.store(true);
            thread_log("Consumer thread finished");
        }
        catch (const std::exception& e) {
            thread_log(std::string("Consumer thread exception: ") + e.what());
            test_passed.store(false);
        }
        catch (...) {
            thread_log("Consumer thread unknown exception");
            test_passed.store(false);
        }
    });
    
    // Wait for both threads to finish
    producer_thread.join();
    consumer_thread.join();
    
    // Check if the test passed
    ASSERT_TRUE(test_passed.load()) << "Cross-thread editor transfer test failed";
} 