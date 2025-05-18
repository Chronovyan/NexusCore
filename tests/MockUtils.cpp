#include "MockUtils.h"

// This file is intentionally left empty since we're just declaring the extern symbols
// in the header. The actual implementation lives in the GMock libraries, and we're 
// just ensuring the linker can find the symbols when linking our tests.

// Note: The symbols are already defined in the GMock library, we don't need to define them again here.
// Attempted to define them here was causing duplicate symbol errors during linking. 