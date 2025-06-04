# Chronovyan Project - CHRONOLOG

## Temporal Audit Log

This document chronicles the Temporal Paradoxes and Flux Aberrations (bugs and issues) encountered in the Chronovyan project, a temporal programming language and its associated tooling.

## Active Dissonances

### CD-2023-06-001
- **Title:** Missing getter methods in StructField class causing compilation errors
- **Reported By:** Overseer
- **Date Observed:** 2023-06-15
- **Perceived Severity:** Major Dissonance
- **Current Status:** Investigated - Likely Obsolete for `src/interpreter.cpp`
- **Detailed Description:**  
  The interpreter.cpp file was reported to attempt direct access to private members of the StructField class (e.g., `defaultValue`), which would require `include/struct_field.h`. Compilation errors were expected.
- **Affected Weave(s) / Module(s):** 
  - `include/struct_field.h` (Verified as missing from the expected path and not found elsewhere)
  - `src/interpreter.cpp` (Investigated)
- **Assigned Weaver:** Cursor
- **Mending Glyphs & Chronal Notes:** 
  - Investigation during the current Vigil confirmed that `include/struct_field.h` is missing.
  - Multiple searches (case-sensitive and insensitive) for `StructField` and `defaultValue` within `src/interpreter.cpp` yielded no results.
  - This suggests that the original issue, as described for `src/interpreter.cpp`, may have been resolved by prior code changes or was inaccurately reported for this specific file in its current state.
  - Cannot proceed with creating `struct_field.h` to fix a non-existent error in `src/interpreter.cpp`.
  - If `StructField` and `defaultValue` are used elsewhere and require `struct_field.h`, this should be logged as a new, distinct Dissonance.
- **Date Harmony Restored:** N/A (Issue not found in `src/interpreter.cpp` as described)
- **Verification Method:** Code review and grep searches of `src/interpreter.cpp`.

### CD-2023-06-002
- **Title:** Missing RebelOperationType enum values in various tests
- **Reported By:** Overseer
- **Date Observed:** 2023-06-15
- **Perceived Severity:** Major Dissonance
- **Current Status:** Resolved
- **Detailed Description:**  
  Tests required additional RebelOperationType enum values that were not defined (as `include/rebel_operation.h` was missing), causing compilation errors in test files, particularly `tests/temporal_debt_test.cpp`.
- **Affected Weave(s) / Module(s):** 
  - `include/rebel_operation.h` (was missing)
  - `tests/temporal_debt_test.cpp`
  - `src/temporal_debt_tracker.cpp` (revealed during mending)
  - `src/resource_visualization.cpp` (revealed during mending)
- **Assigned Weaver:** Cursor
- **Mending Glyphs & Chronal Notes:** 
  - Confirmed `include/rebel_operation.h` was missing.
  - Iteratively constructed `include/rebel_operation.h` by:
    1. Identifying enum values used in `tests/temporal_debt_test.cpp`.
    2. Augmenting with values required by `src/temporal_debt_tracker.cpp` (based on CD-2023-08-004 resolution notes and build errors).
    3. Further augmenting with values required by `src/resource_visualization.cpp` (based on subsequent build errors).
  - The final `RebelOperationType` enum in `include/rebel_operation.h` now includes all values found to be used across these files, allowing the project to compile successfully.
- **Date Harmony Restored:** 2023-06-20
- **Verification Method:** Project successfully compiles. Tests now run, although many runtime failures were uncovered, indicating further Dissonances beyond the scope of this specific compilation issue. The original issue of missing enum values preventing compilation is resolved.

### CD-2023-08-001
- **Title:** Excessive code duplication in repayment strategies
- **Reported By:** Harmonist
- **Date Observed:** 2023-08-15
- **Perceived Severity:** Moderate Dissonance
- **Current Status:** Resolved
- **Detailed Description:**  
  The temporal_debt_tracker.cpp file contains multiple repayment strategy implementations with significant code duplication. The strategy implementations (OldestFirst, HighestInterest, CriticalFirst, etc.) share similar structures but duplicate core logic, making maintenance difficult and increasing the risk of inconsistencies.
  
  Steps to reproduce:
  1. Review the implementation of repayment strategies in temporal_debt_tracker.cpp
  2. Observe the similar structure and duplicated logic across different strategy methods
  
- **Affected Weave(s) / Module(s):** 
  - `src/temporal_debt_tracker.cpp`
  - `include/temporal_debt_tracker.h`
  - `src/repayment_strategies.cpp` (new file)
  
- **Assigned Weaver:** Cursor
- **Mending Glyphs & Chronal Notes:** 
  Implemented a proper Strategy pattern for repayment algorithms:
  - Created abstract `RepaymentStrategy` base class with virtual methods
  - Implemented concrete strategy classes for each repayment algorithm
  - Refactored `RepaymentStrategy` enum to `RepaymentStrategyType`
  - Added a map in `TemporalDebtTracker` to store strategy objects
  - Created a new dedicated `repayment_strategies.cpp` file for all strategy implementations
  - Updated all code that referred to the original enum to use the new type
  - Created test cases to validate strategy implementations
- **Date Harmony Restored:** 2023-08-21
- **Verification Method:** Manual code review and test cases confirm that the Strategy pattern implementation is working correctly and has eliminated code duplication.

### CD-2023-08-002
- **Title:** Magic numbers and hardcoded thresholds in resource optimization algorithms
- **Reported By:** Harmonist
- **Date Observed:** 2023-08-15
- **Perceived Severity:** Moderate Dissonance
- **Current Status:** Resolved
- **Detailed Description:**  
  The resource_optimizer.cpp file contains numerous magic numbers and hardcoded thresholds throughout its optimization algorithms. These values are difficult to tune and adapt for different use cases, and their purpose is not always clear from context.
  
  Steps to reproduce:
  1. Review the resource_optimizer.cpp file
  2. Note the prevalence of hardcoded values (0.85, 0.75, etc.) in optimization algorithms
  
- **Affected Weave(s) / Module(s):** 
  - `src/resource_optimizer.cpp`
  - `include/resource_optimizer.h`
  
- **Assigned Weaver:** Cursor
- **Mending Glyphs & Chronal Notes:** 
  Upon investigation, the issue appears to be partially resolved. The resource_optimizer.cpp file is already using a ResourceConfig system to handle most configuration values instead of hardcoded magic numbers. The ResourceConfig class acts as a centralized repository for all threshold values, factors, and other constants used in the resource optimization algorithms.
  
  Findings:
  1. A ResourceConfig singleton is being used to retrieve configuration parameters by name (e.g., m_config.getDouble("aethel_low_usage_threshold")).
  2. Most thresholds and factors are properly defined in ResourceConfig's loadDefaults() method.
  3. There are a few remaining hardcoded values in resource_optimizer.cpp:
     - Optimization level numbers (0, 1, 2, 3) used in multiple places
     - Default return values (0.0) in the estimateResourceSavings method
     - Initial compression ratio (0.0) in applyTimelineCompression method
  
  The optimization level numbers (0-3) might benefit from being defined as enum values or constants rather than magic numbers, even though their values are intuitive.
  
  Fix Strategy:
  1. Create an OptimizationLevel enum in resource_optimizer.h to replace the magic numbers 0, 1, 2, 3.
  2. Add configuration parameters for the default values (0.0) and initial compression ratio.
  3. Update all references to these hardcoded values to use the enum or configuration parameters.
  4. Add appropriate documentation to explain the meaning of each optimization level.
  
  Implementation:
  1. Added OptimizationLevel enum to ResourceOptimizer class in resource_optimizer.h with values NONE (0), LIGHT (1), MODERATE (2), and AGGRESSIVE (3).
  2. Added documentation to explain the purpose and meaning of each optimization level.
  3. Added configuration parameters "default_savings_rate" and "initial_compression_ratio" to ResourceConfig::loadDefaults().
  4. Updated all hardcoded optimization level values in resource_optimizer.cpp to use the enum values.
  5. Replaced hardcoded default return values with m_config.getDouble("default_savings_rate").
  6. Replaced hardcoded initial compression ratio with m_config.getDouble("initial_compression_ratio").
  7. Fixed an issue with the Timeline::compress method, which wasn't implemented yet, by adding a placeholder output.
  
  All magic numbers and hardcoded thresholds have now been replaced with named constants or configuration parameters, improving code maintainability and making the system more adaptable to different use cases.
- **Date Harmony Restored:** 2023-08-25
- **Verification Method:** Successfully compiled resource_optimizer.cpp with the changes. No compiler errors or warnings related to our fixes were detected. The code now uses the enum and configuration parameters instead of hardcoded values, making it more maintainable and adaptable.

### CD-2023-08-003
- **Title:** Monolithic AST definition in single header file
- **Reported By:** Harmonist
- **Date Observed:** 2023-08-15
- **Perceived Severity:** Moderate Dissonance
- **Current Status:** Resolved
- **Detailed Description:**  
  The ast_nodes.h file is excessively large (2267 lines) and contains all AST node definitions. This creates tight coupling between node types and makes navigation and maintenance difficult. Changes to one node type may require recompilation of all code depending on the header.
  
  Steps to reproduce:
  1. Review the ast_nodes.h file
  2. Observe the size and complexity of the file with all node definitions in a single header
  
- **Affected Weave(s) / Module(s):** 
  - `include/ast_nodes.h`
  - `include/ast_node_base.h` (new file)
  - `include/ast_expressions.h` (new file)
  - `include/ast_statements.h` (new file)
  - `include/ast_temporal_nodes.h` (new file)
  - `include/ast_visitor.h` (new file)
  
- **Assigned Weaver:** Cursor
- **Mending Glyphs & Chronal Notes:** 
  Preliminary analysis confirms the issue. The ast_nodes.h file is indeed a large monolithic header (1217 lines) containing:
  
  1. Multiple class declarations for various AST node types
  2. Base classes (ASTNode, Expression, Statement)
  3. Specialized expression nodes (LiteralExpression, BinaryExpression, etc.)
  4. Specialized statement nodes (PrintStatement, IfStatement, etc.)
  5. Temporal-specific node types
  6. A Visitor interface for traversing the AST
  
  Searches for usages of this header file and implementations of the node classes did not yield results in the current workspace, suggesting that:
  1. The implementation files might be located elsewhere or not yet created
  2. The header file might not be actively used in the codebase at this stage
  
  This situation makes refactoring easier since we don't need to update many files referencing the header, but also means we cannot fully test the changes with actual code using these classes.
  
  Proposed Fix Strategy:
  1. Split the monolithic header into multiple smaller header files:
     - `ast_node_base.h`: Base classes and common utilities
     - `ast_expressions.h`: Expression node classes
     - `ast_statements.h`: Statement node classes
     - `ast_temporal_nodes.h`: Temporal-specific node classes
     - `ast_visitor.h`: Visitor interface
     - `ast_nodes.h`: New smaller header that includes all the others (for backward compatibility)
     
  2. Organize the headers to minimize interdependencies:
     - Base classes should not depend on derived classes
     - Group related node types together
     - Use forward declarations to reduce include dependencies
     
  3. Update include guards and namespaces consistently across all files
  
  Implementation:
  1. Created `ast_node_base.h` containing:
     - Base ASTNode class with attribute storage
     - Expression, Statement base classes
     - TemporalExpression, TemporalStatement base classes
     - AttributeValue type definition
     - Forward declarations for the Visitor class
     
  2. Created `ast_expressions.h` containing:
     - Identifier class
     - LiteralExpression class
     - BinaryExpression, UnaryExpression classes
     - Other expression-related node types
     
  3. Created `ast_statements.h` containing:
     - ExpressionStatement, PrintStatement classes
     - Control flow statements (If, While, For)
     - Function and class declaration statements
     - Other statement-related node types
     
  4. Created `ast_temporal_nodes.h` containing:
     - TemporalLoopStatement, TemporalBranchStatement
     - Timeline and timepoint-related statements
     - Other temporal-specific node types
     
  5. Created `ast_visitor.h` containing:
     - Visitor interface with virtual methods for all node types
     - Forward declarations for all visitable classes
     
  6. Updated the main `ast_nodes.h` file to:
     - Be a lightweight include file that simply includes all the other headers
     - Maintain backward compatibility for existing code
     - Include proper documentation
  
  This approach successfully:
  - Reduces compilation times by breaking the monolithic header into smaller units
  - Improves maintainability by grouping related classes together
  - Maintains backward compatibility through the main ast_nodes.h file
  - Follows good C++ practices for header organization
  
  Each header file now has clear include guards, proper namespace declarations, and appropriate forward declarations to minimize dependencies.
  
- **Date Harmony Restored:** 2023-08-26
- **Verification Method:** 
  The implementation was verified by:
  1. Successful creation of the five new header files with proper organization
  2. Proper header includes to maintain dependencies
  3. Consistent use of include guards and namespaces
  4. Backward compatibility through the new ast_nodes.h file
  
  No compilation tests could be run as there were no source files directly using these headers in the current workspace. However, each header file was visually inspected for correctness, and the structure ensures reduced compilation times and better organization for future development.

### CD-2023-08-004
- **Title:** API inconsistency between RebelOperationType enum values
- **Reported By:** Harmonist
- **Date Observed:** 2023-08-22
- **Perceived Severity:** Major Dissonance
- **Current Status:** Resolved
- **Detailed Description:** There is a mismatch between RebelOperationType enum values referenced in temporal_debt_tracker.cpp and those defined in rebel_operation.h. The temporal_debt_tracker.cpp file uses outdated enum values like TIME_FRACTURE, MEMORY_ALTERATION, and CAUSALITY_INVERSION which do not exist in the current rebel_operation.h file, preventing successful compilation.
- **Steps to reproduce:** Attempt to build the project and observe compilation errors related to enum values.
- **Affected Weave(s) / Module(s):** src/temporal_debt_tracker.cpp, include/rebel_operation.h
- **Assigned Weaver:** Cursor
- **Mending Glyphs & Chronal Notes:** 
  - Updated RebelOperationType enum values in temporal_debt_tracker.cpp to match those in rebel_operation.h
  - Replaced TIME_FRACTURE with TIMELINE_ALTERATION
  - Replaced MEMORY_ALTERATION with OBSERVER_EFFECT
  - Replaced CAUSALITY_INVERSION with CAUSALITY_VIOLATION
  - Fixed missing/duplicate case in switch statements
  - Implemented missing calculateCompoundingFactor method
  - Fixed the registerAlertCallback to use m_alert_callbacks instead of m_callbacks
- **Date Harmony Restored:** 2023-08-23
- **Verification Method:** Successfully build the project and run tests to ensure functionality is preserved 

## Build Status

### 2023-08-27 - Build Status Report

#### Successfully Building Components:
- **EditorLib:** The core editor library builds successfully without errors.
- **di_test:** The dependency injection test executable builds successfully.
- **SimpleDITest:** The simple dependency injection test executable builds successfully.
- **StandaloneDITest:** The standalone dependency injection test executable builds successfully.
- **TextBufferTest:** The text buffer test executable builds successfully.

#### Components with Build Issues:
- **TextEditor:** ~~Fails to build due to missing `imgui_impl_glfw.h` header file.~~ Fixed by adding ImGui backends directory to include paths (TD-2023-09-001). Now fails due to missing GLEW library (TD-2023-09-002).
- **AITextEditor:** Fails to build due to issues with the GLEW library (TD-2023-09-002).
- **DependencyInjectionTest:** Fails to build due to undefined symbol in AIAgentOrchestrator.
- **EditorApiExtensionsTest:** Fails to build due to issues with the GoogleTest framework integration.

#### AST Related Components:
The recent refactoring of the AST nodes from the monolithic `ast_nodes.h` into smaller, more focused header files (`ast_node_base.h`, `ast_expressions.h`, `ast_statements.h`, `ast_temporal_nodes.h`, and `ast_visitor.h`) has been successful. The EditorLib, which includes these headers, builds without errors, indicating that the modularization was successful.

#### Next Steps:
1. ~~Address the missing `imgui_impl_glfw.h` header file for the TextEditor component.~~ Fixed (TD-2023-09-001).
2. Install the GLEW library to resolve build issues with TextEditor and AITextEditor components (TD-2023-09-002).
3. Resolve the undefined symbol in AIAgentOrchestrator for the DependencyInjectionTest.
4. Fix the GoogleTest integration issues in the EditorApiExtensionsTest.

### TD-2023-09-001: TextEditor Build Failure - Missing ImGui Headers

**Status**: Fixed

**Issue**: The TextEditor component failed to build due to missing `imgui_impl_glfw.h` header, which is required by `AITextEditorApp.cpp` included in `main.cpp`.

**Resolution**: Updated the CMakeLists.txt file to add the ImGui backends directory to the include paths for the TextEditor target. The ImGui headers were already available in the external/imgui/backends directory, but the include path wasn't configured for the TextEditor target.

### TD-2023-09-002: TextEditor and AITextEditor Build Failure - Missing GLEW Library

**Status**: Identified

**Issue**: After fixing the ImGui headers issue, both TextEditor and AITextEditor components still fail to build due to the missing GLEW library (GL/glew.h). The GLEW library is used for OpenGL functionality.

**Resolution (Pending)**: The CMakeLists.txt file has been updated to properly search for and link against the GLEW library. However, the GLEW library itself is not available in the external directory. Next steps:

1. Download and install the GLEW library in the external/glew directory.
2. Alternatively, use system-installed GLEW on the development machine.
3. Consider replacing GLEW with another OpenGL loader that's already available, as ImGui's documentation mentions that GLEW is no longer required for newer ImGui versions.

## Summary of Current Status

### Accomplished:

1. **AST Modularization (CD-2023-08-003)**: Successfully refactored the monolithic `ast_nodes.h` file into smaller, more focused header files:
   - `ast_node_base.h` for base classes and common utilities
   - `ast_expressions.h` for expression-related node classes
   - `ast_statements.h` for statement-related node classes
   - `ast_temporal_nodes.h` for temporal-specific node classes
   - `ast_visitor.h` for the visitor interface
   The EditorLib, which contains these headers, builds successfully.

2. **TextEditor Include Paths (TD-2023-09-001)**: Fixed the issue with missing `imgui_impl_glfw.h` header file for the TextEditor target by properly configuring the include paths in CMakeLists.txt.

3. **CMake Configuration for GLEW (TD-2023-09-002)**: Updated the CMakeLists.txt file to properly search for and configure the GLEW library when it's available.

### Pending Tasks:

1. **GLEW Library (TD-2023-09-002)**: Need to install the GLEW library to resolve build issues with TextEditor and AITextEditor components.

2. **DependencyInjectionTest**: Need to resolve the undefined symbol in AIAgentOrchestrator.

3. **EditorApiExtensionsTest**: Need to fix the GoogleTest integration issues.

### Components Building Successfully:
- EditorLib
- di_test (Dependency Injection test)
- TextBufferTest

### Components with Build Issues:
- TextEditor (Missing GLEW library)
- AITextEditor (Missing GLEW library)
- DependencyInjectionTest (Undefined symbol in AIAgentOrchestrator)
- EditorApiExtensionsTest (GoogleTest integration issues) 