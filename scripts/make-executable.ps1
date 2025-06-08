# Make all scripts in the scripts directory executable
# This script is for Windows systems

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$scripts = Get-ChildItem -Path $scriptDir -Filter "*.sh" -Recurse

foreach ($script in $scripts) {
    $fullPath = $script.FullName
    
    # Remove any existing Zone.Identifier alternate data stream
    if (Get-Command -Name "Unblock-File" -ErrorAction SilentlyContinue) {
        Unblock-File -Path $fullPath -ErrorAction SilentlyContinue
    }
    
    # Set the script to be executable
    $acl = Get-Acl $fullPath
    $acl.SetAccessRuleProtection($true, $false)
    $rule = New-Object System.Security.AccessControl.FileSystemAccessRule("BUILTIN\Users", "FullControl", "Allow")
    $acl.AddAccessRule($rule)
    Set-Acl -Path $fullPath -AclObject $acl
    
    Write-Host "Made executable: $fullPath"
}

Write-Host "\n✅ All scripts have been made executable!"
