#include "dtc_gui_client.hpp"
#include <iostream>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    try
    {
        DTCTestClientGUI client;

        if (!client.Initialize(hInstance))
        {
            MessageBoxA(nullptr, "Failed to initialize DTC Test Client", "Error", MB_ICONERROR);
            return 1;
        }

        client.Run();
        client.Shutdown();

        return 0;
    }
    catch (const std::exception &e)
    {
        std::string error = "Exception: ";
        error += e.what();
        MessageBoxA(nullptr, error.c_str(), "Error", MB_ICONERROR);
        return 1;
    }
    catch (...)
    {
        MessageBoxA(nullptr, "Unknown exception occurred", "Error", MB_ICONERROR);
        return 1;
    }
}