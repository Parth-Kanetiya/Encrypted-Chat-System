//    - Commands:
//        /msg <user> <text>   — private message
//        /list                — show online users
//        /quit                — disconnect

#include<bits/stdc++.h>
using namespace std;

#include <thread>
#include <mutex>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "crypto.h"
#include "protocol.h"

   
string g_key;// shared encryption key received from server
bool is_Running(true);// if client disconnects or server disconnects, we can close both send and receive loops by setting this to false.

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
    for(int i=0; i<(hex.size()-1); i+=2) {
        unsigned int byte;
        sscanf(hex.substr(i, 2).c_str(), "%02x", &byte);
        data += (char)byte;
    }
    return data;
}

//Send a raw packet (used for JOIN which is unencrypted by design)
void rawSend(int fd, const string& packet){
    string msg = packet + "\n";
    send(fd, msg.c_str(), msg.size(), 0);
}

//Encrypt plaintext and send
void encSend(int fd, const string& type, const string& plaintext){
    string cipher = xorCipher(plaintext, g_key);
    string hex    = toHex(cipher);
    rawSend(fd, Protocol::build(type, hex));
}


void receiveLoop(int fd){
    char buf[4096];
    while (is_Running) {

        memset(buf, 0, sizeof(buf));//clean buffer
        int n = recv(fd, buf, sizeof(buf) - 1, 0);
        if (n <= 0) {
            cout << "\n[!] Disconnected from server.\n";
            is_Running = false;
            break;
        }

        string packet(buf, n);
        // Strip newline
        while (!packet.empty() && (packet.back() == '\n' || packet.back() == '\r'))
            packet.pop_back();

        string type = Protocol::getType(packet);
        string data = Protocol::getData(packet);

        //for future use(if we want to change key during session)
        if(type == Protocol::KEY) {
            g_key = data;
            continue;
        }
        // SERVER_MSG — for server announcements(like user joined/left, or DM sent)
        else if(type == Protocol::SERVER_MSG) {
            string plain = xorCipher(fromHex(data), g_key);
            cout << "\n\033[33m[SERVER] " << plain << "\033[0m\n> " << flush; //yellow
            continue;
        }
        else if(type == Protocol::MSG){// broadcasted message: format is  MSG|sender:hexPayload
            int colon = data.find(':');//sender:hexPayload
            if (colon == string::npos) continue;

            string sender     = data.substr(0, colon);
            string hexPayload = data.substr(colon + 1);

            string plain = xorCipher(fromHex(hexPayload), g_key);//decrypt
            cout << "\n\033[36m" << sender << "\033[0m: " << plain << "\n> " << flush; //blue
            continue;
        }
        else if(type == Protocol::DMSG){// receive DM, format is:- DMSG|sender|hexPayload
            auto parts = Protocol::split(data, '|');
            if (parts.size() < 2) continue;
            string sender     = parts[0];
            string hexPayload = parts[1];
            string plain = xorCipher(fromHex(hexPayload), g_key);
            cout << "\n\033[35m[DM from " << sender << "]\033[0m " << plain << "\n> " << flush; //purple
            continue;
        }
        // ERROR msg
        else if(type == Protocol::ERROR_MSG) {
            string plain_error = xorCipher(fromHex(data), g_key);//decrypt error message
            cout << "\n\033[31m[ERROR] " << plain_error << "\033[0m\n> " << flush;//red
            continue;
        }
    }
}


int main() {
    // string SERVER_IP   = "10.196.60.233";
    // string SERVER_IP   = "10.196.63.250";
    // string SERVER_IP   = "172.21.218.228";
    string SERVER_IP   = "127.0.0.1";
    int SERVER_PORT = 8080;

    cout << "----------------------------------\n";
    cout << "|      CHAT CLIENT               |\n";
    cout << "----------------------------------\n\n";

    //Create socket 
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

    cout << "-----Connected to " << SERVER_IP << ":" << SERVER_PORT << "-------\n";

    //wait for server to send the key
    {
        char buf[256] = {};
        int n = recv(fd, buf, sizeof(buf) - 1, 0);
        if (n <= 0) { cout << "[!] Server closed connection.\n"; return 1; }

        string packet(buf, n);
        while (!packet.empty() && (packet.back() == '\n' || packet.back() == '\r'))
            packet.pop_back();


        if (Protocol::getType(packet) == Protocol::KEY)
            g_key = Protocol::getData(packet);
    }
    // cout << "Encryption key received.\n\n";

    //take Username and send to server as JOIN packet
    string username;
    while (true) {
        cout << "\nEnter username: ";
        
        getline(cin, username);

        if (username.empty()) { cout << "Username cannot be empty.\n"; continue; }

        //Send JOIN packet (username, without encryption)
        rawSend(fd, Protocol::build(Protocol::JOIN, username));

        //Wait for server response (ERROR or welcome msg)
        char buf[512] = {};
        int n = recv(fd, buf, sizeof(buf) - 1, 0);
        if (n <= 0) { cout << "[!] Server closed connection.\n"; return 1; }

        string packet(buf, n);
        while (!packet.empty() && (packet.back() == '\n' || packet.back() == '\r'))
            packet.pop_back();

        //Error msg(if username already exists or empty username)
        if (Protocol::getType(packet) == Protocol::ERROR_MSG) {
            cout << "[!] " << Protocol::getData(packet) << "\n";
            continue;
        }

        // Welcome message
        if (Protocol::getType(packet) == Protocol::SERVER_MSG) {
            string plain = xorCipher(fromHex(Protocol::getData(packet)), g_key);
            // cout << "[SERVER] " << plain << "\n\n";
            cout << "\n\033[33m[SERVER] " << plain << "\033[0m\n> " << flush; //yellow
        }

        break;
    }

    thread(receiveLoop, fd).detach();//thread to receive messages from server.

    cout << "-----------------------------------------\n";
    cout << "  Type a message and press Enter to send\n";
    cout << "  /msg <user> <text>  — Direct Message\n";
    cout << "  /list               — Get List of online users\n";
    cout << "  /quit               — To Disconnect\n";
    cout << "-----------------------------------------\n";


    string input;
    while (is_Running) {
        cout << "> ";

        getline(cin, input);

        if (input.empty()) continue;


        if (input == "/quit") {
            cout << "[+] Goodbye!\n";
            break;
        }

        // /list —> request user list from server
        if (input == "/list") {
            rawSend(fd, Protocol::build(Protocol::LIST, ""));
            continue;
        }

        // /msg <receiver> <message>
        if (input.substr(0, 5) == "/msg ") {
            stringstream ss(input.substr(5));
            string receiver, message;

            ss >> receiver;
            getline(ss, message);//since message can contains spaces.

            if(!message.empty() && message[0] == ' ')
                message = message.substr(1);

            if(receiver.empty() || message.empty()) {
                cout << "[!] Error: /msg <user> <message>\n";
                continue;
            }

            string cipher = xorCipher(message, g_key);
            string hex    = toHex(cipher);
            rawSend(fd, Protocol::build(Protocol::DMSG, receiver + "|" + hex));
            continue;
        }

        //Normal message for whole room
        encSend(fd, Protocol::MSG, input);
    }

    is_Running = false;
    close(fd);
    return 0;
}
