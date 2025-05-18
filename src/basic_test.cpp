#include <iostream>
#include <string>
#include <vector>
#include <memory>

// A simple test to verify that compilation works correctly
class SimpleTest {
public:
    SimpleTest(const std::string& name) : name_(name) {}
    
    void run() {
        std::cout << "Running test: " << name_ << std::endl;
        std::cout << "Test successful!" << std::endl;
    }
    
private:
    std::string name_;
};

int main() {
    std::cout << "Testing C++ compilation..." << std::endl;
    
    // Create and run a simple test
    auto test = std::make_unique<SimpleTest>("Basic Compilation Test");
    test->run();
    
    std::cout << "All tests completed!" << std::endl;
    return 0;
} 