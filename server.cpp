#include<bits/stdc++.h>
using namespace std;

#include <thread>
#include <mutex>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "protocol.h"
#include "crypto.h"

class User{
    public:
        string name;
        // string key;
        int fd;

        User() {}  // ✅ default constructor
        User(string name, int fd){
        // User(string name, string key, int fd){
            this->name = name;
            // this->key = key;
            this->fd = fd;
        }
};
unordered_map<string, int> nameTofd; // name -> fd
unordered_map<int, User> fdToUser; // fd -> user
string g_shared_key;
mutex g_mutex;


// Raw send (no encryption — used for KEY exchange & control messages)
void rawSend(int fd, const string& packet) {
    string msg = packet + "\n";
    send(fd, msg.c_str(), msg.size(), 0);
}

// Encrypted send — data portion is XOR-encrypted, TYPE stays plain
// Packet format sent: TYPE|<encrypted_data>
void encSend(int fd, const string& type, const string& plainData) {
    string cipher = xorCipher(plainData, g_shared_key);
    // Encode cipher as hex so it's safely printable over the socket
    string hex;
    for (unsigned char c : cipher) {
        char buf[3];
        snprintf(buf, sizeof(buf), "%02x", c);
        hex += buf;
    }
    rawSend(fd, Protocol::build(type, hex));
}

// Broadcast an encrypted server announcement to all connected clients
void broadcastServer(const string& announcement, int exclude_fd = -1) {
    lock_guard<mutex> lock(g_mutex);
    for (auto &[fd, user] : fdToUser) {
        if (fd != exclude_fd)
            encSend(fd, Protocol::SERVER_MSG, announcement);
    }
}

// Broadcast a MSG (already-encrypted hex payload from sender)
// The server NEVER decrypts — it just forwards the hex blob
void broadcastMsg(const string& hexPayload, int sender_fd) {
    string senderName;
    {
        lock_guard<mutex> lock(g_mutex);
        senderName = fdToUser.count(sender_fd) ? fdToUser[sender_fd].name : "?";
    }
    // Wrap with sender tag: SERVER re-packages as  MSG|username:hexPayload
    string forwardMSG = Protocol::build(Protocol::MSG, senderName + ":" + hexPayload);
    // string forwardMSG = Protocol::build(Protocol::MSG, senderName + ":" + hexPayload);
    lock_guard<mutex> lock(g_mutex);
    for (auto &[fd, user] : fdToUser) {
        if (fd != sender_fd)
            rawSend(fd, forwardMSG);
    }
}

// ─────────────────────────────────────────────
//  CLEANUP — remove a disconnected client
// ─────────────────────────────────────────────
void removeClient(int fd) {
    // user u = fdToUser[fd];
    string username = fdToUser[fd].name;
    {
        lock_guard<mutex> lock(g_mutex);//LOCK
        // if (g_fd_to_username.count(fd))
        //     username = g_fd_to_username[fd];

        // g_fd_to_username.erase(fd);
        // g_username_to_fd.erase(username);
        
        // string name = fdTouser[fd].name; // get name first

        nameTofd.erase(username);            // remove name → fd
        fdToUser.erase(fd);              // remove fd → user


        //DOOOOOOOOOOOOOOOOOUBT
        // g_client_fds.erase(
        //     remove(g_client_fds.begin(), g_client_fds.end(), fd),
        //     g_client_fds.end()
        // );
        close(fd);
    }

    if (!username.empty()) {
        cout << "[SERVER] " << username << " disconnected.\n";
        broadcastServer(username + " has left the chat.");
    }
}

string buildUserList() {
    lock_guard<mutex> lock(g_mutex);

    if(fdToUser.empty()) return "No users online.";

    string list = "Online users: ";
    bool first = true;
    for(auto& [fd, user] : fdToUser) {
        if (!first) list += ", ";
        list += user.name;
        first = false;
    }
    return list;
}




// ─────────────────────────────────────────────
//  CLIENT HANDLER — runs in its own thread
// ─────────────────────────────────────────────
void handleClient(int fd) {
    char buf[7000];
    // char buf[4096];

    // ── Step 1: Send the shared encryption key to this client ──
    rawSend(fd, Protocol::build(Protocol::KEY, g_shared_key));

    // ── Step 2: Wait for JOIN|username ──
    string username;
    while (true) {
        memset(buf, 0, sizeof(buf));
        int n = recv(fd, buf, sizeof(buf) - 1, 0);

        //Disconnected before JOINing
        if (n <= 0) { removeClient(fd); return; }

        string packet(buf, n);
        // Strip trailing newline
        while (!packet.empty() && (packet.back() == '\n' || packet.back() == '\r'))
            packet.pop_back();

        if (Protocol::getType(packet) == Protocol::JOIN) {
            username = Protocol::getData(packet);
            if (username.empty()) {
                rawSend(fd, Protocol::build(Protocol::ERROR_MSG, "Username cannot be empty."));
                continue;
            }
            // Check for duplicate username
            lock_guard<mutex> lock(g_mutex);
            if (nameTofd.count(username)) {
                rawSend(fd, Protocol::build(Protocol::ERROR_MSG, "Username already taken."));
                username.clear();
                continue;
            }
            // g_fd_to_username[fd]      = username;
            // g_username_to_fd[username] = fd;

            nameTofd[username] = fd;
            fdToUser[fd] = User(username, fd);
            break;
        }
    }

    cout << "[SERVER] " << username << " joined from fd=" << fd << "\n";
    broadcastServer(username + " has joined the chat!", fd);
    encSend(fd, Protocol::SERVER_MSG, "Welcome, " + username + "! Type /list to see online users.");

    // ── Step 3: Main message loop ──
    while (true) {
        memset(buf, 0, sizeof(buf));
        int n = recv(fd, buf, sizeof(buf) - 1, 0);
        if (n <= 0) break;   // client disconnected

        string packet(buf, n);
        while (!packet.empty() && (packet.back() == '\n' || packet.back() == '\r'))
            packet.pop_back();

        string type = Protocol::getType(packet);
        string data = Protocol::getData(packet);

        // ── Broadcast message (data is hex-encoded encrypted payload) ──
        if (type == Protocol::MSG) {
            cout << "[SERVER] Forwarding encrypted MSG from " << username << " (server cannot read it)\n";
            broadcastMsg(data, fd);
        }

        // ── Private message: DMSG|receiver|hexPayload ──
        else if (type == Protocol::DMSG) {
            auto parts = Protocol::split(data, '|');
            // auto parts = Protocol::split(data, ':');
            if (parts.size() < 2) continue;
            string receiver    = parts[0];
            string hexPayload  = parts[1];

            int receiver_fd = -1;
            {
                lock_guard<mutex> lock(g_mutex);
                if (nameTofd.count(receiver))
                    receiver_fd = nameTofd[receiver];
            }

            if(receiver_fd == -1){
                encSend(fd, Protocol::ERROR_MSG, "User '" + receiver + "' not found.");
            }
            else{



                // Forward: DMSG|sender|hexPayload
                string fwd = Protocol::build(Protocol::DMSG, username + "|" + hexPayload);
                rawSend(receiver_fd, fwd);
                // Echo back to sender
                cout<<"[SERVER] Forwarding encrypted DMSG from " << username << " to " << receiver << " (server cannot read it)\n";
                encSend(fd, Protocol::SERVER_MSG, "[DM sent to " + receiver + "]");




            }
        }

        // ── List online users ──
        else if (type == Protocol::LIST) {
            string list = buildUserList();
            encSend(fd, Protocol::SERVER_MSG, list);
        }
    }

    removeClient(fd);
}

// int main(int argc, char* argv[]) {
int main() {
    int PORT = 8080;
    // if (argc > 1) PORT = stoi(argv[1]);

    // Generate shared encryption key
    // g_shared_key = generateKey(16);
    g_shared_key = generateKey();

    cout << "╔══════════════════════════════════════╗\n";
    cout << "║     SECURE CHAT SERVER               ║\n";
    cout << "╚══════════════════════════════════════╝\n";
    cout << "[SERVER] Encryption key: " << g_shared_key << "\n";
    cout << "[SERVER] Starting on port " << PORT << "...\n";

    // Create TCP socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); return 1; }




    // Allow port reuse (avoids "Address already in use" after restart)
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    // SO_REUSEADDR: if you restart the server quickly, the port isn't "in use" anymore
    // Without this you'd have to wait ~60 seconds for the OS to release the port





    // Bind
    struct sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind"); 
        return 1;
    }

    // Listen
    if (listen(server_fd, 10) < 0) { perror("listen"); return 1; }

    cout << "[SERVER] Listening for connections...\n\n";

    // Accept loop — spawn a thread per client
    while (true) {
        struct sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) { perror("accept"); continue; }

        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip, sizeof(ip));
        cout << "[SERVER] New connection from " << ip << " (fd=" << client_fd << ")\n";

        {
            lock_guard<mutex> lock(g_mutex);
            // g_client_fds.push_back(client_fd);
            
            // We don't know the username yet, so we can add to fdToUser with a ?
            fdToUser[client_fd] = User("?", client_fd);
        }

        thread(handleClient, client_fd).detach();
    }

    close(server_fd);
    return 0;
}

