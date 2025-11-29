# PowerShell script to migrate all coinbase files to new Logger system
# This automates the migration of all remaining coinbase source files

$coinbaseFiles = @(
    "c:\Daten\_Codding\coinbase-dtc-core\src\exchanges\coinbase\coinbase_feed.cpp",
    "c:\Daten\_Codding\coinbase-dtc-core\src\exchanges\coinbase\rest_client.cpp",
    "c:\Daten\_Codding\coinbase-dtc-core\src\exchanges\coinbase\websocket_client.cpp",
    "c:\Daten\_Codding\coinbase-dtc-core\src\exchanges\coinbase\websocket_client_old.cpp"
)

foreach ($filePath in $coinbaseFiles) {
    if (Test-Path $filePath) {
        Write-Host "Migrating $filePath..."
        
        $content = Get-Content $filePath -Raw
        
        # Replace include paths first
        $content = $content -replace '#include "coinbase_dtc_core/core/util/log\.hpp"', '#include "coinbase_dtc_core/core/util/advanced_log.hpp"'
        
        # Define replacement mappings for logging calls
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
            'open_dtc_server::util::simple_log\("\[COINBASE\]' = 'LOG_INFO("[COINBASE]'
            'open_dtc_server::util::simple_log\("\[FACTORY\]' = 'LOG_INFO("[FACTORY]'
            'util::simple_log\("\[' = 'LOG_INFO("['  # For shorter calls
        }

        # Apply replacements
        foreach ($pattern in $replacements.Keys) {
            $replacement = $replacements[$pattern]
            $content = $content -replace $pattern, $replacement
        }

        # Write back to file
        Set-Content -Path $filePath -Value $content
        
        Write-Host "  ✓ Completed migration for $(Split-Path $filePath -Leaf)"
    } else {
        Write-Host "  ⚠ File not found: $filePath"
    }
}

Write-Host "`n✅ All Coinbase file migrations completed!"