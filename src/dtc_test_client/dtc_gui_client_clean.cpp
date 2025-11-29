// Temporäre Datei - Bereinigte Version ohne LoadCoinbaseAccountInfo
// Nur die letzten 10 Zeilen der HandleDTCResponse Funktion:

default:
UpdateConsole("[INFO] Received DTC message type: " +
              std::to_string(static_cast<uint16_t>(message->get_type())));
break;
}
}