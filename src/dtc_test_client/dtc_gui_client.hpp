#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commctrl.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <vector>
#include <memory>
#include "coinbase_dtc_core/core/dtc/protocol.hpp"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "ws2_32.lib")

// Window dimensions
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const int CONTROL_SPACING = 10;
const int BUTTON_WIDTH = 100;
const int BUTTON_HEIGHT = 25;

// Control IDs
enum ControlID
{
    ID_EDIT_SERVER_HOST = 1001,
    ID_EDIT_SERVER_PORT = 1002,
    ID_BTN_CONNECT = 1003,
    ID_BTN_DISCONNECT = 1004,
    ID_COMBO_SYMBOLS = 1005,
    ID_BTN_ACCOUNT_INFO = 1006,
    ID_BTN_LOAD_SYMBOLS = 1007,
    ID_COMBO_PRODUCT_TYPE = 1008,
    ID_BTN_SYMBOL_INFO = 1009,
    ID_BTN_DOM_DATA = 1010,
    ID_BTN_SUBSCRIBE = 1011,
    ID_BTN_UNSUBSCRIBE = 1012,
    ID_BTN_CLEAR_CONSOLE = 1013,
    ID_EDIT_CONSOLE = 1014,
    ID_STATUS_BAR = 1015,
    ID_EDIT_ACCOUNT_INFO = 1016
};

class DTCTestClientGUI
{
public:
    DTCTestClientGUI();
    ~DTCTestClientGUI();

    bool Initialize(HINSTANCE hInstance);
    void Run();
    void Shutdown();

private:
    // Window procedure
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // Event handlers
    void OnCreate(HWND hwnd);
    void OnCommand(HWND hwnd, WPARAM wParam);
    void OnClose(HWND hwnd);
    void OnPaint(HWND hwnd);

    // DTC Client functions
    void ConnectToServer();
    void DisconnectFromServer();
    void GetAccountInfo();
    void LoadAvailableSymbols();
    void GetSymbolInfo();
    void GetDOMData();
    void SubscribeToSymbol();
    void UnsubscribeFromSymbol();

    // UI Helper functions
    void UpdateConsole(const std::string &message);
    void UpdateStatus(const std::string &status);
    void UpdateAccountInfo(const std::string &info);
    void ClearConsole();
    std::string GetSelectedSymbol();

    // Network functions
    bool SendDTCMessage(const std::vector<uint8_t> &message);
    void ProcessIncomingData();
    void ProcessDTCMessages();
    void HandleDTCResponse(std::unique_ptr<open_dtc_server::core::dtc::DTCMessage> message);
    void GetRealAccountData();

    // Window and control handles
    HWND m_hwnd = nullptr;
    HWND m_btnConnect = nullptr;
    HWND m_btnDisconnect = nullptr;
    HWND m_btnAccountInfo = nullptr;
    HWND m_btnLoadSymbols = nullptr;
    HWND m_btnSymbolInfo = nullptr;
    HWND m_btnDOMData = nullptr;
    HWND m_btnSubscribe = nullptr;
    HWND m_btnUnsubscribe = nullptr;
    HWND m_btnClearConsole = nullptr;
    HWND m_comboSymbols = nullptr;
    HWND m_comboProductType = nullptr;
    HWND m_editConsole = nullptr;
    HWND m_editAccountInfo = nullptr;
    HWND m_statusBar = nullptr;
    HWND m_editServerHost = nullptr;
    HWND m_editServerPort = nullptr;

    // Application data
    HINSTANCE m_hInstance = nullptr;
    bool m_isConnected = false;
    SOCKET m_socket = INVALID_SOCKET;
    std::string m_serverHost = "127.0.0.1";
    int m_serverPort = 11099;
    std::vector<uint8_t> m_incomingBuffer; // Window constants
    static constexpr int WINDOW_WIDTH = 800;
    static constexpr int WINDOW_HEIGHT = 600;
    static constexpr int BUTTON_WIDTH = 120;
    static constexpr int BUTTON_HEIGHT = 30;
    static constexpr int CONTROL_SPACING = 10;

    // Control IDs
    enum
    {
        ID_BTN_CONNECT = 1001,
        ID_BTN_DISCONNECT,
        ID_BTN_ACCOUNT_INFO,
        ID_BTN_LOAD_SYMBOLS,
        ID_BTN_SYMBOL_INFO,
        ID_BTN_DOM_DATA,
        ID_BTN_SUBSCRIBE,
        ID_BTN_UNSUBSCRIBE,
        ID_BTN_CLEAR_CONSOLE,
        ID_COMBO_SYMBOLS,
        ID_EDIT_CONSOLE,
        ID_STATUS_BAR,
        ID_EDIT_SERVER_HOST,
        ID_EDIT_SERVER_PORT
    };
};
