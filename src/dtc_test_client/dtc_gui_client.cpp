#include "dtc_gui_client.hpp"
#include "coinbase_dtc_core/core/util/log.hpp"
#include <sstream>
#include <iomanip>
#include <chrono>

DTCTestClientGUI::DTCTestClientGUI()
{
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        MessageBoxA(nullptr, "Failed to initialize Winsock", "Error", MB_ICONERROR);
    }
}

DTCTestClientGUI::~DTCTestClientGUI()
{
    Shutdown();
    WSACleanup();
}

bool DTCTestClientGUI::Initialize(HINSTANCE hInstance)
{
    m_hInstance = hInstance;

    // Initialize Common Controls
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_WIN95_CLASSES | ICC_BAR_CLASSES;
    if (!InitCommonControlsEx(&icex))
    {
        MessageBoxA(nullptr, "Failed to initialize common controls", "Error", MB_ICONERROR);
        return false;
    }

    // Register window class
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"DTCTestClientClass";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);

    if (!RegisterClassW(&wc))
    {
        MessageBoxA(nullptr, "Failed to register window class", "Error", MB_ICONERROR);
        return false;
    }

    // Create main window
    m_hwnd = CreateWindowExW(
        0,
        L"DTCTestClientClass",
        L"DTC Test Client - Coinbase Bridge",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        nullptr, nullptr, hInstance, this);

    if (!m_hwnd)
    {
        MessageBoxA(nullptr, "Failed to create window", "Error", MB_ICONERROR);
        return false;
    }

    return true;
}

void DTCTestClientGUI::Run()
{
    MSG msg = {};
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

void DTCTestClientGUI::Shutdown()
{
    if (m_isConnected)
    {
        DisconnectFromServer();
    }
}

LRESULT CALLBACK DTCTestClientGUI::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    DTCTestClientGUI *pThis = nullptr;

    if (uMsg == WM_NCCREATE)
    {
        CREATESTRUCT *pCreate = (CREATESTRUCT *)lParam;
        pThis = (DTCTestClientGUI *)pCreate->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
    }
    else
    {
        pThis = (DTCTestClientGUI *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }

    if (pThis)
    {
        return pThis->HandleMessage(hwnd, uMsg, wParam, lParam);
    }

    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

LRESULT DTCTestClientGUI::HandleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
        OnCreate(hwnd);
        break;
    case WM_COMMAND:
        OnCommand(hwnd, wParam);
        break;
    case WM_CLOSE:
        OnClose(hwnd);
        break;
    case WM_PAINT:
        OnPaint(hwnd);
        break;
    case WM_SIZE:
        // Handle window resizing if needed
        break;
    default:
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

void DTCTestClientGUI::OnCreate(HWND hwnd)
{
    int x = CONTROL_SPACING;
    int y = CONTROL_SPACING;

    // Server connection controls
    CreateWindowA("STATIC", "Server Host:", WS_VISIBLE | WS_CHILD,
                  x, y, 80, 20, hwnd, nullptr, m_hInstance, nullptr);

    m_editServerHost = CreateWindowA("EDIT", m_serverHost.c_str(),
                                     WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                                     x + 85, y, 100, 20, hwnd, (HMENU)ID_EDIT_SERVER_HOST, m_hInstance, nullptr);

    CreateWindowA("STATIC", "Port:", WS_VISIBLE | WS_CHILD,
                  x + 195, y, 35, 20, hwnd, nullptr, m_hInstance, nullptr);

    m_editServerPort = CreateWindowA("EDIT", std::to_string(m_serverPort).c_str(),
                                     WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | ES_NUMBER,
                                     x + 235, y, 60, 20, hwnd, (HMENU)ID_EDIT_SERVER_PORT, m_hInstance, nullptr);

    y += 30;

    // Connection buttons
    m_btnConnect = CreateWindowA("BUTTON", "Connect",
                                 WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                                 x, y, BUTTON_WIDTH, BUTTON_HEIGHT, hwnd, (HMENU)ID_BTN_CONNECT, m_hInstance, nullptr);

    m_btnDisconnect = CreateWindowA("BUTTON", "Disconnect",
                                    WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | WS_DISABLED,
                                    x + BUTTON_WIDTH + CONTROL_SPACING, y, BUTTON_WIDTH, BUTTON_HEIGHT,
                                    hwnd, (HMENU)ID_BTN_DISCONNECT, m_hInstance, nullptr);

    y += BUTTON_HEIGHT + CONTROL_SPACING;

    // Symbol selection
    CreateWindowA("STATIC", "Symbol:", WS_VISIBLE | WS_CHILD,
                  x, y + 5, 50, 20, hwnd, nullptr, m_hInstance, nullptr);

    m_comboSymbols = CreateWindowA("COMBOBOX", "",
                                   WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_VSCROLL,
                                   x + 60, y, 200, 200, hwnd, (HMENU)ID_COMBO_SYMBOLS, m_hInstance, nullptr);

    // Add some default symbols
    SendMessageA(m_comboSymbols, CB_ADDSTRING, 0, (LPARAM) "BTC-USD");
    SendMessageA(m_comboSymbols, CB_ADDSTRING, 0, (LPARAM) "ETH-USD");
    SendMessageA(m_comboSymbols, CB_ADDSTRING, 0, (LPARAM) "STRK-USD");
    SendMessageA(m_comboSymbols, CB_SETCURSEL, 0, 0);

    y += 30;

    // Action buttons - Row 1
    m_btnAccountInfo = CreateWindowA("BUTTON", "Account Info",
                                     WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | WS_DISABLED,
                                     x, y, BUTTON_WIDTH, BUTTON_HEIGHT, hwnd, (HMENU)ID_BTN_ACCOUNT_INFO, m_hInstance, nullptr);

    m_btnLoadSymbols = CreateWindowA("BUTTON", "Load Symbols",
                                     WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | WS_DISABLED,
                                     x + BUTTON_WIDTH + CONTROL_SPACING, y, BUTTON_WIDTH, BUTTON_HEIGHT,
                                     hwnd, (HMENU)ID_BTN_LOAD_SYMBOLS, m_hInstance, nullptr);

    m_btnSymbolInfo = CreateWindowA("BUTTON", "Symbol Info",
                                    WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | WS_DISABLED,
                                    x + 2 * (BUTTON_WIDTH + CONTROL_SPACING), y, BUTTON_WIDTH, BUTTON_HEIGHT,
                                    hwnd, (HMENU)ID_BTN_SYMBOL_INFO, m_hInstance, nullptr);

    y += BUTTON_HEIGHT + CONTROL_SPACING;

    // Action buttons - Row 2
    m_btnDOMData = CreateWindowA("BUTTON", "DOM Data",
                                 WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | WS_DISABLED,
                                 x, y, BUTTON_WIDTH, BUTTON_HEIGHT, hwnd, (HMENU)ID_BTN_DOM_DATA, m_hInstance, nullptr);

    m_btnSubscribe = CreateWindowA("BUTTON", "Subscribe",
                                   WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | WS_DISABLED,
                                   x + BUTTON_WIDTH + CONTROL_SPACING, y, BUTTON_WIDTH, BUTTON_HEIGHT,
                                   hwnd, (HMENU)ID_BTN_SUBSCRIBE, m_hInstance, nullptr);

    m_btnUnsubscribe = CreateWindowA("BUTTON", "Unsubscribe",
                                     WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | WS_DISABLED,
                                     x + 2 * (BUTTON_WIDTH + CONTROL_SPACING), y, BUTTON_WIDTH, BUTTON_HEIGHT,
                                     hwnd, (HMENU)ID_BTN_UNSUBSCRIBE, m_hInstance, nullptr);

    y += BUTTON_HEIGHT + CONTROL_SPACING;

    // Clear console button
    m_btnClearConsole = CreateWindowA("BUTTON", "Clear Console",
                                      WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                                      x, y, BUTTON_WIDTH, BUTTON_HEIGHT, hwnd, (HMENU)ID_BTN_CLEAR_CONSOLE, m_hInstance, nullptr);

    y += BUTTON_HEIGHT + CONTROL_SPACING;

    // Console output
    CreateWindowA("STATIC", "Console Output:", WS_VISIBLE | WS_CHILD,
                  x, y, 100, 20, hwnd, nullptr, m_hInstance, nullptr);

    y += 25;

    m_editConsole = CreateWindowA("EDIT", "",
                                  WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
                                  x, y, WINDOW_WIDTH - 40, 200, hwnd, (HMENU)ID_EDIT_CONSOLE, m_hInstance, nullptr);

    // Status bar
    m_statusBar = CreateWindowA("msctls_statusbar32", "Ready",
                                WS_VISIBLE | WS_CHILD | SBARS_SIZEGRIP,
                                0, 0, 0, 0, hwnd, (HMENU)ID_STATUS_BAR, m_hInstance, nullptr);

    // Set initial status
    UpdateStatus("Ready - Not Connected");
    UpdateConsole("DTC Test Client initialized");
    UpdateConsole("Click 'Connect' to connect to DTC server");
}

void DTCTestClientGUI::OnCommand(HWND hwnd, WPARAM wParam)
{
    switch (LOWORD(wParam))
    {
    case ID_BTN_CONNECT:
        ConnectToServer();
        break;
    case ID_BTN_DISCONNECT:
        DisconnectFromServer();
        break;
    case ID_BTN_ACCOUNT_INFO:
        GetAccountInfo();
        break;
    case ID_BTN_LOAD_SYMBOLS:
        LoadAvailableSymbols();
        break;
    case ID_BTN_SYMBOL_INFO:
        GetSymbolInfo();
        break;
    case ID_BTN_DOM_DATA:
        GetDOMData();
        break;
    case ID_BTN_SUBSCRIBE:
        SubscribeToSymbol();
        break;
    case ID_BTN_UNSUBSCRIBE:
        UnsubscribeFromSymbol();
        break;
    case ID_BTN_CLEAR_CONSOLE:
        ClearConsole();
        break;
    }
}

void DTCTestClientGUI::OnClose(HWND hwnd)
{
    DestroyWindow(hwnd);
    PostQuitMessage(0);
}

void DTCTestClientGUI::OnPaint(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    EndPaint(hwnd, &ps);
}

void DTCTestClientGUI::ConnectToServer()
{
    // Get server host and port from edit controls
    char hostBuffer[256];
    char portBuffer[16];

    GetWindowTextA(m_editServerHost, hostBuffer, sizeof(hostBuffer));
    GetWindowTextA(m_editServerPort, portBuffer, sizeof(portBuffer));

    m_serverHost = hostBuffer;
    m_serverPort = atoi(portBuffer);

    UpdateConsole("Connecting to " + m_serverHost + ":" + std::to_string(m_serverPort) + "...");

    // Create socket
    m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_socket == INVALID_SOCKET)
    {
        UpdateConsole("ERROR: Failed to create socket");
        return;
    }

    // Connect to server
    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(m_serverPort);

    // Handle hostname resolution for both IP addresses and hostnames like "localhost"
    if (inet_pton(AF_INET, m_serverHost.c_str(), &serverAddr.sin_addr) != 1)
    {
        // inet_pton failed, try hostname resolution
        struct hostent *host = gethostbyname(m_serverHost.c_str());
        if (host == nullptr || host->h_addr_list[0] == nullptr)
        {
            UpdateConsole("ERROR: Cannot resolve hostname '" + m_serverHost + "'");
            closesocket(m_socket);
            m_socket = INVALID_SOCKET;
            return;
        }

        // Copy the resolved IP address
        memcpy(&serverAddr.sin_addr, host->h_addr_list[0], host->h_length);
        UpdateConsole("Resolved '" + m_serverHost + "' to IP address");
    }

    if (connect(m_socket, (sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        UpdateConsole("ERROR: Failed to connect to server");
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        return;
    }

    m_isConnected = true;
    UpdateConsole("✅ Connected successfully!");
    UpdateStatus("Connected to " + m_serverHost + ":" + std::to_string(m_serverPort));

    // Enable/disable buttons
    EnableWindow(m_btnConnect, FALSE);
    EnableWindow(m_btnDisconnect, TRUE);
    EnableWindow(m_btnAccountInfo, TRUE);
    EnableWindow(m_btnLoadSymbols, TRUE);
    EnableWindow(m_btnSymbolInfo, TRUE);
    EnableWindow(m_btnDOMData, TRUE);
    EnableWindow(m_btnSubscribe, TRUE);
    EnableWindow(m_btnUnsubscribe, TRUE);
}

void DTCTestClientGUI::DisconnectFromServer()
{
    if (m_socket != INVALID_SOCKET)
    {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
    }

    m_isConnected = false;
    UpdateConsole("Disconnected from server");
    UpdateStatus("Disconnected");

    // Enable/disable buttons
    EnableWindow(m_btnConnect, TRUE);
    EnableWindow(m_btnDisconnect, FALSE);
    EnableWindow(m_btnAccountInfo, FALSE);
    EnableWindow(m_btnLoadSymbols, FALSE);
    EnableWindow(m_btnSymbolInfo, FALSE);
    EnableWindow(m_btnDOMData, FALSE);
    EnableWindow(m_btnSubscribe, FALSE);
    EnableWindow(m_btnUnsubscribe, FALSE);
}

void DTCTestClientGUI::GetAccountInfo()
{
    if (!m_isConnected)
    {
        UpdateConsole("ERROR: Not connected to server");
        return;
    }

    UpdateConsole("📊 Requesting account information...");
    UpdateConsole("[MOCK] - Account Info Request - Function not implemented");
    UpdateConsole("Account Info:");
    UpdateConsole("  Exchange: [MOCK] Coinbase Advanced Trade");
    UpdateConsole("  Account Type: [MOCK] Demo Account (No real API connection)");
    UpdateConsole("  Status: [MOCK] Test Mode - Requires Coinbase API integration");
    UpdateConsole("  Note: All account data above is simulated");
}

void DTCTestClientGUI::LoadAvailableSymbols()
{
    if (!m_isConnected)
    {
        UpdateConsole("ERROR: Not connected to server");
        return;
    }

    UpdateConsole("📋 Loading available symbols...");
    UpdateConsole("[MOCK] - Symbol Request - Function not implemented");

    // Clear current symbols
    SendMessage(m_comboSymbols, CB_RESETCONTENT, 0, 0);

    // Add priority symbols first
    std::vector<std::string> prioritySymbols = {
        "BTC-USD", "ETH-USD", "STRK-USD", "ADA-USD", "SOL-USD",
        "DOT-USD", "LINK-USD", "UNI-USD", "AAVE-USD", "SUSHI-USD"};

    for (const auto &symbol : prioritySymbols)
    {
        SendMessageA(m_comboSymbols, CB_ADDSTRING, 0, (LPARAM)symbol.c_str());
        UpdateConsole("  [MOCK] Added: " + symbol);
    }

    SendMessage(m_comboSymbols, CB_SETCURSEL, 0, 0);
    UpdateConsole("✅ [MOCK] Loaded " + std::to_string(prioritySymbols.size()) + " simulated symbols");
}

void DTCTestClientGUI::GetSymbolInfo()
{
    std::string symbol = GetSelectedSymbol();
    if (symbol.empty())
    {
        UpdateConsole("ERROR: No symbol selected");
        return;
    }

    if (!m_isConnected)
    {
        UpdateConsole("ERROR: Not connected to server");
        return;
    }

    UpdateConsole("ℹ️ Getting symbol info for: " + symbol);

    // TODO: Implement actual DTC symbol info request
    UpdateConsole("Symbol Info for " + symbol + ":");
    UpdateConsole("  Full Name: " + symbol);
    UpdateConsole("  Type: Cryptocurrency Pair");
    UpdateConsole("  Base Currency: " + symbol.substr(0, symbol.find('-')));
    UpdateConsole("  Quote Currency: " + symbol.substr(symbol.find('-') + 1));
    UpdateConsole("  Min Order Size: 0.001");
    UpdateConsole("  Max Order Size: 10000");
    UpdateConsole("  Price Increment: 0.01");
}

void DTCTestClientGUI::GetDOMData()
{
    std::string symbol = GetSelectedSymbol();
    if (symbol.empty())
    {
        UpdateConsole("ERROR: No symbol selected");
        return;
    }

    if (!m_isConnected)
    {
        UpdateConsole("ERROR: Not connected to server");
        return;
    }

    UpdateConsole("📈 Getting DOM (Depth of Market) data for: " + symbol);

    // TODO: Implement actual DTC DOM request
    UpdateConsole("DOM Data for " + symbol + ":");
    UpdateConsole("  Bids:");
    UpdateConsole("    $45,250.00 x 0.5");
    UpdateConsole("    $45,249.50 x 1.2");
    UpdateConsole("    $45,249.00 x 0.8");
    UpdateConsole("  Asks:");
    UpdateConsole("    $45,251.00 x 0.7");
    UpdateConsole("    $45,251.50 x 1.0");
    UpdateConsole("    $45,252.00 x 0.9");
}

void DTCTestClientGUI::SubscribeToSymbol()
{
    std::string symbol = GetSelectedSymbol();
    if (symbol.empty())
    {
        UpdateConsole("ERROR: No symbol selected");
        return;
    }

    if (!m_isConnected)
    {
        UpdateConsole("ERROR: Not connected to server");
        return;
    }

    UpdateConsole("🔔 Subscribing to real-time data for: " + symbol);

    // TODO: Implement actual DTC subscription request
    UpdateConsole("✅ Subscribed to " + symbol);
    UpdateConsole("You will now receive real-time updates for this symbol");
}

void DTCTestClientGUI::UnsubscribeFromSymbol()
{
    std::string symbol = GetSelectedSymbol();
    if (symbol.empty())
    {
        UpdateConsole("ERROR: No symbol selected");
        return;
    }

    if (!m_isConnected)
    {
        UpdateConsole("ERROR: Not connected to server");
        return;
    }

    UpdateConsole("🔕 Unsubscribing from: " + symbol);

    // TODO: Implement actual DTC unsubscribe request
    UpdateConsole("✅ Unsubscribed from " + symbol);
}

void DTCTestClientGUI::UpdateConsole(const std::string &message)
{
    if (!m_editConsole)
        return;

    // Get current time
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "[%H:%M:%S] ");

    std::string timestampedMessage = ss.str() + message + "\r\n";

    // Get current text length
    int textLength = GetWindowTextLengthA(m_editConsole);

    // Set selection to end and append text
    SendMessage(m_editConsole, EM_SETSEL, textLength, textLength);
    SendMessageA(m_editConsole, EM_REPLACESEL, FALSE, (LPARAM)timestampedMessage.c_str());

    // Scroll to bottom
    SendMessage(m_editConsole, EM_SCROLLCARET, 0, 0);
}

void DTCTestClientGUI::UpdateStatus(const std::string &status)
{
    if (m_statusBar)
    {
        SendMessageA(m_statusBar, SB_SETTEXTA, 0, (LPARAM)status.c_str());
    }
}

void DTCTestClientGUI::ClearConsole()
{
    if (m_editConsole)
    {
        SetWindowTextA(m_editConsole, "");
        UpdateConsole("Console cleared");
    }
}

std::string DTCTestClientGUI::GetSelectedSymbol()
{
    if (!m_comboSymbols)
        return "";

    int selection = SendMessage(m_comboSymbols, CB_GETCURSEL, 0, 0);
    if (selection == CB_ERR)
        return "";

    char buffer[256];
    SendMessageA(m_comboSymbols, CB_GETLBTEXT, selection, (LPARAM)buffer);
    return std::string(buffer);
}

bool DTCTestClientGUI::SendDTCMessage(const std::vector<uint8_t> &message)
{
    if (!m_isConnected || m_socket == INVALID_SOCKET)
    {
        return false;
    }

    int result = send(m_socket, (const char *)message.data(), message.size(), 0);
    return result != SOCKET_ERROR;
}

void DTCTestClientGUI::ProcessIncomingData()
{
    // TODO: Implement incoming DTC message processing
}