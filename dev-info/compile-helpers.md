# Compile Helpers

````bash

#build all
cmake --build build --config Release

# Build server (full rebuild)
cd C:\Daten\_Codding\coinbase-dtc-core\build; cmake --build . --config Release --target coinbase_dtc_server --clean-first

# Build GUI client (force rebuild)
cd C:\Daten\_Codding\coinbase-dtc-core\build; cmake --build . --config Release --target dtc_test_client_gui #--clean-first

# Run server with custom credentials path
cd C:\Daten\_Codding\coinbase-dtc-core\build\Release; .\coinbase_dtc_server.exe --credentials "C:\Daten\_Codding\coinbase-dtc-core\secrets\coinbase\cdp_api_key_ECDSA.json"

# start client
cd C:\Daten\_Codding\coinbase-dtc-core\build\Release; .\dtc_test_client_gui.exe
````