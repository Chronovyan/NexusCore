$filePath = "c:\Users\HydraPony\dev\AI-First TextEditor\src\EditorDemoWindow.h"
$content = [System.IO.File]::ReadAllText($filePath)

# Remove the first TabState definition (keeping only the one from TabState.h)
$pattern = '(?s)    // Tab state structure\s+struct TabState \{[^}]+\}\s+// Member variables\s+std::vector<TabState> tabs_;'
$replacement = '    // Using TabState from TabState.h\n    // Tab management is handled by tabManager_'

$newContent = [regex]::Replace($content, $pattern, $replacement)

# Remove the second TabState definition if it still exists
$pattern2 = '(?s)    // Tab state structure\s+struct TabState \{[^}]+\}\s+// Member variables\s+std::vector<TabState> tabs_;'
$newContent = [regex]::Replace($newContent, $pattern2, '')

# Replace the original file with the cleaned content
[System.IO.File]::WriteAllText($filePath, $newContent)

Write-Host "File has been updated successfully."
