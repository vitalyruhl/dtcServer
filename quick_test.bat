@echo off
echo 🔄 Quick API Key Activation Test...
echo Testing at: %date% %time%
echo.

.\out\build\windows-vs2022\Debug\test_permissions.exe | findstr /C:"SUCCESS" /C:"Key Permissions" /C:"List Accounts"

echo.
echo 💡 If still showing 401, wait another 10 minutes and run this script again.
echo 📱 Check your CDP dashboard at: https://portal.cdp.coinbase.com/projects/api-keys
pause