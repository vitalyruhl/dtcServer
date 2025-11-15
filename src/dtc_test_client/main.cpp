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

void setup_callbacks()
{
    client_.set_status_callback([this](const std::string &status)
                                { add_console_output("STATUS: " + status); });

    client_.set_account_callback([this](const dtc_test_client::AccountInfo &account)
                                 { add_console_output("ACCOUNT: " + account.account_id + " | Balance: $" +
                                                      std::to_string(account.balance)); });

    client_.set_symbol_callback([this](const dtc_test_client::SymbolInfo &symbol)
                                { add_console_output("SYMBOL: " + symbol.symbol + " | Price: $" +
                                                     std::to_string(symbol.price)); });

    client_.set_trade_callback([this](const std::string &symbol, double price, double size)
                               { add_console_output("TRADE: " + symbol + " | $" + std::to_string(price) +
                                                    " x " + std::to_string(size)); });
}

return ch;
}

tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
return 0;
#endif
}

void handle_menu_choice(char choice)
{
    if (choice == 0)
        return; // No input

    clear_screen();
    print_header();

    switch (choice)
    {
    case '1':
        handle_connect();
        break;
    case '2':
        handle_get_account_info();
        break;
    case '3':
        handle_get_symbols();
        break;
    case '4':
        handle_select_symbol();
        break;
    case '5':
        handle_get_symbol_info();
        break;
    case '6':
        handle_get_last_trade();
        break;
    case '7':
        handle_get_market_depth();
        break;
    case '8':
        handle_subscribe_symbol();
        break;
    case '9':
        handle_unsubscribe_symbol();
        break;
    case '0':
        handle_exit();
        break;
    default:
        add_console_output("Invalid choice: " + std::string(1, choice));
        break;
    }
}

void handle_connect()
{
    add_console_output("Connecting to DTC Server...");
    if (client_.connect("127.0.0.1", 11099))
    {
        add_console_output("✅ Connected successfully!");
    }
    else
    {
        add_console_output("❌ Connection failed!");
    }
}

void handle_get_account_info()
{
    add_console_output("Requesting account information...");
    if (client_.get_account_info())
    {
        add_console_output("✅ Account info retrieved!");
    }
    else
    {
        add_console_output("❌ Failed to get account info!");
    }
}

void handle_get_symbols()
{
    add_console_output("Fetching available symbols...");
    if (client_.get_available_symbols())
    {
        const auto &symbols = client_.get_symbols();
        add_console_output("✅ Found " + std::to_string(symbols.size()) + " symbols");
        add_console_output("Priority symbols: 10x-USD, STRK-USD");
    }
    else
    {
        add_console_output("❌ Failed to get symbols!");
    }
}

void handle_select_symbol()
{
    const auto &symbols = client_.get_symbols();
    if (symbols.empty())
    {
        add_console_output("❌ No symbols available. Get symbols first!");
        return;
    }

    add_console_output("Available symbols:");
    for (size_t i = 0; i < symbols.size(); i++)
    {
        std::string marker = (i == selected_symbol_index_) ? "► " : "  ";
        add_console_output(marker + std::to_string(i + 1) + ". " + symbols[i].symbol +
                           " (" + symbols[i].display_name + ")");
    }

    // Cycle to next symbol
    selected_symbol_index_ = (selected_symbol_index_ + 1) % symbols.size();
    add_console_output("✅ Selected: " + symbols[selected_symbol_index_].symbol);
}

void handle_get_symbol_info()
{
    std::string symbol = get_current_symbol();
    if (symbol == "None")
    {
        add_console_output("❌ No symbol selected!");
        return;
    }

    if (client_.get_symbol_info(symbol))
    {
        add_console_output("✅ Symbol info retrieved for " + symbol);
    }
    else
    {
        add_console_output("❌ Failed to get symbol info!");
    }
}

void handle_get_last_trade()
{
    std::string symbol = get_current_symbol();
    if (symbol == "None")
    {
        add_console_output("❌ No symbol selected!");
        return;
    }

    if (client_.get_symbol_last_trade(symbol))
    {
        add_console_output("✅ Last trade retrieved for " + symbol);
    }
    else
    {
        add_console_output("❌ Failed to get last trade!");
    }
}

void handle_get_market_depth()
{
    std::string symbol = get_current_symbol();
    if (symbol == "None")
    {
        add_console_output("❌ No symbol selected!");
        return;
    }

    if (client_.get_symbol_depth(symbol))
    {
        add_console_output("✅ Market depth (DOM) retrieved for " + symbol);
    }
    else
    {
        add_console_output("❌ Failed to get market depth!");
    }
}

void handle_subscribe_symbol()
{
    std::string symbol = get_current_symbol();
    if (symbol == "None")
    {
        add_console_output("❌ No symbol selected!");
        return;
    }

    if (client_.subscribe_to_symbol(symbol))
    {
        add_console_output("✅ Subscribed to real-time data for " + symbol);
    }
    else
    {
        add_console_output("❌ Failed to subscribe to " + symbol);
    }
}

void handle_unsubscribe_symbol()
{
    std::string symbol = get_current_symbol();
    if (symbol == "None")
    {
        add_console_output("❌ No symbol selected!");
        return;
    }

    if (client_.unsubscribe_from_symbol(symbol))
    {
        add_console_output("✅ Unsubscribed from " + symbol);
    }
    else
    {
        add_console_output("❌ Failed to unsubscribe from " + symbol);
    }
}

void handle_exit()
{
    add_console_output("Disconnecting and exiting...");
    client_.disconnect();
    running_ = false;
}

void add_console_output(const std::string &message)
{
    console_output_.push_back(message);

    // Keep only last 50 messages
    if (console_output_.size() > 50)
    {
        console_output_.erase(console_output_.begin());
    }
}
}
;

int main()
{
    std::cout << "Starting DTC Test Client GUI...\n\n";

    DTCTestGUI gui;
    gui.run();

    std::cout << "\nDTC Test Client closed. Press Enter to exit...";
    std::cin.get();

    return 0;
}