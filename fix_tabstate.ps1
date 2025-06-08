$filePath = "c:\Users\HydraPony\dev\AI-First TextEditor\src\EditorDemoWindow.h"
$content = [System.IO.File]::ReadAllText($filePath)

# Pattern to match the duplicate TabState definition and tab management code
$pattern = '(?s)    // Tab state structure\s+struct TabState \{[^}]+\}\s+// Member variables\s+std::vector<TabState> tabs_;'
$replacement = '    // Using TabState from TabState.h
    // Tab management is handled by tabManager_'

$newContent = [regex]::Replace($content, $pattern, $replacement)

# Write the updated content back to the file
[System.IO.File]::WriteAllText($filePath, $newContent)

Write-Host "File has been updated successfully."
