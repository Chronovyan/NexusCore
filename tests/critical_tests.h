#ifndef CRITICAL_TESTS_H
#define CRITICAL_TESTS_H

// Define a test tag for critical tests that should run in the pre-commit hook
// Usage: TEST_F(MyFixture, CRITICAL(MyTest)) { ... }

#define CRITICAL(test_name) test_name ## _Critical

// Define a Google Test filter for critical tests
// Usage in command line: --gtest_filter="*_Critical*"
// This matches any test with "_Critical" in its name

#endif // CRITICAL_TESTS_H 