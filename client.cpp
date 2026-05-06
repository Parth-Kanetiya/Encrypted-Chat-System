// ═══════════════════════════════════════════════════════
//  SECURE CHAT CLIENT — client.cpp
//
//  Features:
//    - Connects to server, registers username
//    - Receives shared encryption key from server
//    - Encrypts all outgoing messages with XOR cipher
//    - Decrypts all incoming messages
//    - Separate thread for receiving (non-blocking UI)
//    - Commands:
//        /msg <user> <text>   — private message
//        /list                — show online users
//        /quit                — disconnect
// ═══════════════════════════════════════════════════════
#include<bits/stdc++.h>
using namespace std;

#include <thread>
#include <mutex>

// POSIX socket headers
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "crypto.h"
#include "protocol.h"

// ─────────────────────────────────────────────
//  GLOBALS
// ─────────────────────────────────────────────    
string g_key;           // shared encryption key from server


//Check if we can replace this with a simple bool is_Running = true; and remove the atomic header and include
// is_Running = true;
// atomic<bool> is_Running(true);
bool is_Running(true);




// ─────────────────────────────────────────────
//  HEX ENCODE / DECODE
//  (used to safely transmit XOR-ciphered bytes)
// ─────────────────────────────────────────────
string toHex(const string& data) {
    string hex;
    for (unsigned char c:data) {
        char buf[3];
        snprintf(buf, sizeof(buf), "%02x", c);
        hex += buf;
    }
    return hex;
}

string fromHex(const string& hex) {
    string data;
    for (int i=0; i<(hex.size()-1); i+=2) {
    // for (size_t i = 0; i + 1 < hex.size(); i += 2) {
        unsigned int byte;
        sscanf(hex.substr(i, 2).c_str(), "%02x", &byte);
        data += (char)byte;
    }
    return data;
}

// Send a raw packet (used for JOIN which is unencrypted by design)
void rawSend(int fd, const string& packet) {
    string msg = packet + "\n";
    send(fd, msg.c_str(), msg.size(), 0);
}

// Encrypt plaintext and send as TYPE|hexCipher
void encSend(int fd, const string& type, const string& plaintext) {
    string cipher = xorCipher(plaintext, g_key);
    string hex    = toHex(cipher);
    rawSend(fd, Protocol::build(type, hex));
}

// ─────────────────────────────────────────────
//  RECEIVE THREAD — runs in background
//  Parses and displays incoming messages
// ─────────────────────────────────────────────
void receiveLoop(int fd) {
    char buf[4096];
    while (is_Running) {
        memset(buf, 0, sizeof(buf));
        int n = recv(fd, buf, sizeof(buf) - 1, 0);
        if (n <= 0) {
            cout << "\n[!] Disconnected from server.\n";
            is_Running = false;
            break;
        }


        
        string packet(buf, n);
        // string packet(buf);
        


        // Strip newline
        while (!packet.empty() && (packet.back() == '\n' || packet.back() == '\r'))
            packet.pop_back();

        string type = Protocol::getType(packet);
        string data = Protocol::getData(packet);






        //--------------Doooooooooooooooooooubt-------------------------------------
        // KEY — store the key (unencrypted, first message)
        if (type == Protocol::KEY) {
            g_key = data;
            // Key is set, signal main thread by printing a note
            // (main thread waits for key before allowing user to type)
            continue;
        }

        // SERVER_MSG — server announcements, decrypted
        if (type == Protocol::SERVER_MSG) {
            string plain = xorCipher(fromHex(data), g_key);
            cout << "\n\033[33m[SERVER] " << plain << "\033[0m\n> " << flush;
            continue;
        }

        // MSG — broadcast: format is  MSG|sender:hexPayload
        if (type == Protocol::MSG) {
            size_t colon = data.find(':');//sender:hexPayload
            if (colon == string::npos) continue;

            string sender     = data.substr(0, colon);
            string hexPayload = data.substr(colon + 1);
            // CLIENT decrypts the payload — server never could
            string plain = xorCipher(fromHex(hexPayload), g_key);
            cout << "\n\033[36m" << sender << "\033[0m: " << plain << "\n> " << flush;
            continue;
        }

        // if (type == Protocol::MSG) {
        //     auto parts = Protocol::split(packet, '|');
        //     if (parts.size() < 3) continue; // Ensure we have type, sender, and payload
            
        //     string sender     = parts[1];
        //     string hexPayload = parts[2];
            
        //     // CLIENT decrypts the payload
        //     string plain = xorCipher(fromHex(hexPayload), g_key);
        //     cout << "\n\033[36m" << sender << "\033[0m: " << plain << "\n> " << flush;
        //     continue;
        // }





        // DMSG — private message: format is  DMSG|sender|hexPayload
        if (type == Protocol::DMSG) {
            auto parts = Protocol::split(data, '|');
            if (parts.size() < 2) continue;
            string sender     = parts[0];
            string hexPayload = parts[1];
            string plain = xorCipher(fromHex(hexPayload), g_key);
            cout << "\n\033[35m[PM from " << sender << "]\033[0m " << plain << "\n> " << flush;
            continue;
        }

        // if (type == Protocol::DMSG) {
        //     auto parts = Protocol::split(packet, '|');
        //     if (parts.size() < 3) continue;
            
        //     string sender     = parts[1];
        //     string hexPayload = parts[2];
            
        //     string plain = xorCipher(fromHex(hexPayload), g_key);
        //     cout << "\n\033[35m[PM from " << sender << "]\033[0m " << plain << "\n> " << flush;
        //     continue;
        // }


        // ERROR
        if (type == Protocol::ERROR_MSG) {
            string plain_error = xorCipher(fromHex(data), g_key);
            cout << "\n\033[31m[ERROR] " << plain_error << "\033[0m\n> " << flush;
            continue;
        }
    }
}

// ─────────────────────────────────────────────
//  MAIN
// ─────────────────────────────────────────────
int main() {
// int main(int argc, char* argv[]) {
    // string SERVER_IP   = "10.196.60.233";
    // string SERVER_IP   = "10.196.63.250";
    // string SERVER_IP   = "172.21.218.228";
    string SERVER_IP   = "127.0.0.1";
    int         SERVER_PORT = 8080;

    // if (argc > 1) SERVER_IP   = argv[1];
    // if (argc > 2) SERVER_PORT = stoi(argv[2]);

    cout << "╔══════════════════════════════════════╗\n";
    cout << "║     SECURE CHAT CLIENT               ║\n";
    cout << "╚══════════════════════════════════════╝\n";

    // ── Create socket and connect ──
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); return 1; }

    struct sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP.c_str(), &server_addr.sin_addr);

    if (connect(fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        cout << "[!] Could not connect to " << SERVER_IP << ":" << SERVER_PORT << "\n";
        return 1;
    }

    cout << "[+] Connected to " << SERVER_IP << ":" << SERVER_PORT << "\n";

    // ── Receive KEY from server before anything else ──
    {
        char buf[256] = {};
        int n = recv(fd, buf, sizeof(buf) - 1, 0);
        if (n <= 0) { cout << "[!] Server closed connection.\n"; return 1; }
        string packet(buf, n);

        //-------------CAN REMOVE THIS-------------------
        while (!packet.empty() && (packet.back() == '\n' || packet.back() == '\r'))
            packet.pop_back();


        if (Protocol::getType(packet) == Protocol::KEY)
            g_key = Protocol::getData(packet);
    }
    cout << "[+] Encryption key received. All messages are now encrypted.\n\n";

    // ── Register username ──
    string username;
    while (true) {
        cout << "Enter username: ";
        
        getline(cin, username);
        //Check if we can replace this with a simple cin >> username; and remove the getline above
        // cin>>username;

        

        if (username.empty()) { cout << "Username cannot be empty.\n"; continue; }

        // Send JOIN (username is NOT encrypted — server needs it in plaintext to register)
        rawSend(fd, Protocol::build(Protocol::JOIN, username));

        // Wait for server response (ERROR or SERVER_MSG/welcome)
        char buf[512] = {};
        int n = recv(fd, buf, sizeof(buf) - 1, 0);
        if (n <= 0) { cout << "[!] Server closed connection.\n"; return 1; }

        string packet(buf, n);
        while (!packet.empty() && (packet.back() == '\n' || packet.back() == '\r'))
            packet.pop_back();

        if (Protocol::getType(packet) == Protocol::ERROR_MSG) {
            cout << "[!] " << Protocol::getData(packet) << "\n";
            continue;
        }

        // Welcome message (SERVER_MSG, encrypted)
        if (Protocol::getType(packet) == Protocol::SERVER_MSG) {
            string plain = xorCipher(fromHex(Protocol::getData(packet)), g_key);
            cout << "[SERVER] " << plain << "\n\n";
        }



        break;
    }

    // ── Start receive thread ──
    
    thread(receiveLoop, fd).detach();



    // ── Print help ──
    cout << "──────────────────────────────────────\n";
    cout << "  Type a message and press Enter to send\n";
    cout << "  /msg <user> <text>  — private message\n";
    cout << "  /list               — online users\n";
    cout << "  /quit               — disconnect\n";
    cout << "──────────────────────────────────────\n";

    // ── Main input loop ──
    string input;
    while (is_Running) {
        cout << "> ";

        getline(cin, input);
        // if (!getline(cin, input)) break;
        // Check if we can replace this with a simple cin >> input; and remove the getline above



        if (input.empty()) continue;

        // /quit
        if (input == "/quit") {
            cout << "[+] Goodbye!\n";
            break;
        }

        // /list — request user list from server
        if (input == "/list") {
            rawSend(fd, Protocol::build(Protocol::LIST, ""));
            continue;
        }

        // /msg <receiver> <message>
        if (input.substr(0, 5) == "/msg ") {
            istringstream ss(input.substr(5));
            string receiver, message;
            ss >> receiver;
            getline(ss, message);
            if (!message.empty() && message[0] == ' ')
                message = message.substr(1);

            if (receiver.empty() || message.empty()) {
                cout << "[!] Error: /msg <user> <message>\n";
                continue;
            }

            // Encrypt the message payload
            string cipher = xorCipher(message, g_key);
            string hex    = toHex(cipher);
            rawSend(fd, Protocol::build(Protocol::DMSG, receiver + "|" + hex));
            continue;
        }

        // Broadcast message — encrypt before sending
        encSend(fd, Protocol::MSG, input);
    }

    is_Running = false;
    close(fd);
    return 0;
}
