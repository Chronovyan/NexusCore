#include "MyersDiff.h"
#include "AppDebugLog.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <map>
#include <stack>

// Definitions for template functions are in the header (MyersDiff.h)
// Non-template functions that were defined inline in the header are also not re-defined here.

// Explicit template instantiations
template std::vector<MyersDiff::EditScriptItem> MyersDiff::computeEditScript<std::string>(
    const std::vector<std::string>& seq1, 
    const std::vector<std::string>& seq2);

template std::vector<MyersDiff::EditScriptItem> MyersDiff::computeEditScript<char>(
    const std::vector<char>& seq1, 
    const std::vector<char>& seq2);