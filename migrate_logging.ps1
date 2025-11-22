# PowerShell script to migrate all simple_log calls to new Logger macros
# This script automates the replacement of logging calls

$filePath = "c:\Daten\_Codding\coinbase-dtc-core\src\exchanges\coinbase\ssl_websocket_client.cpp"
$content = Get-Content $filePath -Raw

# Define replacement mappings
$replacements = @{
    'open_dtc_server::util::simple_log\("\[ERROR\]' = 'LOG_ERROR("[ERROR]'
    'open_dtc_server::util::simple_log\("\[WARNING\]' = 'LOG_WARN("[WARNING]'
    'open_dtc_server::util::simple_log\("\[SUCCESS\]' = 'LOG_INFO("[SUCCESS]'
    'open_dtc_server::util::simple_log\("\[INFO\]' = 'LOG_INFO("[INFO]'
    'open_dtc_server::util::simple_log\("\[DEBUG\]' = 'LOG_DEBUG("[DEBUG]'
    'open_dtc_server::util::simple_log\("\[WS\]' = 'LOG_INFO("[WS]'
    'open_dtc_server::util::simple_log\("\[WORKER\]' = 'LOG_INFO("[WORKER]'
    'open_dtc_server::util::simple_log\("\[PING\]' = 'LOG_DEBUG("[PING]'
    'open_dtc_server::util::simple_log\("\[SEND\]' = 'LOG_DEBUG("[SEND]'
    'open_dtc_server::util::simple_log\("\[DEVELOPMENT\]' = 'LOG_DEBUG("[DEVELOPMENT]'
}

# Apply replacements
foreach ($pattern in $replacements.Keys) {
    $replacement = $replacements[$pattern]
    $content = $content -replace $pattern, $replacement
}

# Write back to file
Set-Content -Path $filePath -Value $content

Write-Host "Migration completed for ssl_websocket_client.cpp"