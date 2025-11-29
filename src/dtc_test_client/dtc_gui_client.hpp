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
#include <map>
#include <unordered_set>
#include "coinbase_dtc_core/core/dtc/protocol.hpp"
#include <fstream>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "ws2_32.lib")

// Window dimensions
const int WINDOW_WIDTH = 1000; // Increased width for market data panel
const int WINDOW_HEIGHT = 750; // Increased height for market data panel
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
    ID_EDIT_ACCOUNT_INFO = 1016,
    ID_CHK_HIDE_DELISTED = 1017
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
    void UpdateMarketData(const std::string &symbol, double bid, double ask, double last, double volume); // NEW
    void UpdateDOMPanelDisplay(const std::string &symbol);
    void RenderMarketDataPanel(const std::string &symbol);
    void ClearConsole();
    std::string GetSelectedSymbol();
    void InitGuiLogger();
    void WriteGuiLog(const std::string &line);
    std::string GetExecutableDir();
    void RotateGuiLogsIfNeeded();

    // Network functions
    bool SendDTCMessage(const std::vector<uint8_t> &message);
    void ProcessIncomingData();
    void ProcessDTCMessages();
    void HandleDTCResponse(std::unique_ptr<open_dtc_server::core::dtc::DTCMessage> message);
    void GetRealAccountData();
    void UpdateDOMFromIncrement(uint16_t symbol_id, uint8_t side, uint16_t position, double price, double size, uint64_t ts);

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
    HWND m_editMarketData = nullptr; // NEW: Market data display area
    HWND m_editSymbolInfo = nullptr; // NEW: Symbol info panel
    HWND m_statusBar = nullptr;
    HWND m_editServerHost = nullptr;
    HWND m_editServerPort = nullptr;
    HWND m_chkHideDelisted = nullptr;

    // Application data
    HINSTANCE m_hInstance = nullptr;
    bool m_isConnected = false;
    SOCKET m_socket = INVALID_SOCKET;
    std::string m_serverHost = "127.0.0.1";
    int m_serverPort = 11099;
    std::vector<uint8_t> m_incomingBuffer;

    // Market data tracking
    struct MarketDataInfo
    {
        std::string symbol;
        double bid_price = 0.0;
        double ask_price = 0.0;
        double last_price = 0.0;
        double volume = 0.0;
        std::string last_update_time;
        bool is_subscribed = false;
    } m_currentMarketData;

    // Feature flags
    bool m_enableMockData = false;

    // Symbol to ID mapping for market data subscriptions
    std::map<std::string, uint16_t> m_symbolToIdMap;
    std::map<uint16_t, std::string> m_idToSymbolMap;

    // Delisted tracking (client-side)
    std::unordered_set<std::string> m_delistedSymbols;
    bool m_hideDelisted = true;
    void AddDelistedSymbol(const std::string &sym) { m_delistedSymbols.insert(sym); }
    bool IsDelisted(const std::string &sym) const { return m_delistedSymbols.find(sym) != m_delistedSymbols.end(); }
    void RefreshSymbolCombo();
    bool IsLikelyDelisted(const std::string &sym) const;

    // Last reject reason per symbol
    std::map<std::string, std::string> m_lastRejectReason;
    void UpdateSymbolInfoPanel(const std::string &symbol);

    // Cache of latest SecurityDefinitionResponse per symbol for richer panel info
    std::map<std::string, open_dtc_server::core::dtc::SecurityDefinitionResponse> m_symbolInfoCache;

    // DOM (Depth of Market) storage keyed by DTC symbol_id
    struct DOMBook
    {
        std::vector<std::pair<double, double>> bids; // index = position, value = {price, size}
        std::vector<std::pair<double, double>> asks; // index = position, value = {price, size}
        uint64_t last_update_ts = 0;
    };
    std::map<uint16_t, DOMBook> m_domBooks;

    // GUI logging (independent file logger with rotation)
    std::string m_guiLogPath;
    std::ofstream m_guiLogFile;
    size_t m_guiMaxSizeBytes = 50ull * 1024ull * 1024ull; // 50MB
    int m_guiMaxBackups = 5;
};
