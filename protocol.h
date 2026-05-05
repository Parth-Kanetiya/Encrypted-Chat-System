#include<bits/stdc++.h>
using namespace std;

class Protocol {
    public:
        static const string JOIN;
        static const string MSG;
        static const string DMSG;//Direct message
        // static const string PMSG;//Direct message
        static const string KEY;
        static const string LEAVE;
        static const string ERROR_MSG;
        static const string LIST;
        static const string SERVER_MSG;

        static vector<string> split(const string& packet, char splitter);
        static string getType(string &packet);
        static string getData(const string& packet);
        // static vector<string> split(const string& packet, char splitter);
        static string getSenderName(const string& packet);
        static string build(const string& type, const string& data);
        static string buildDMsg(const string& receiver, const string& content);
};


// #ifndef PROTOCOL_H
// #define PROTOCOL_H

// #include <string>
// #include <vector>

// class Protocol {
// public:
//     static const std::string JOIN;
//     static const std::string MSG;
//     static const std::string DMSG;
//     static const std::string KEY;
//     static const std::string LEAVE;
//     static const std::string ERROR_MSG;
//     static const std::string LIST;
//     static const std::string SERVER_MSG;

//     static std::vector<std::string> split(const std::string& packet, char splitter);
//     static std::string getType(std::string &packet);
//     static std::string getData(const std::string& packet);
//     static std::string getSenderName(const std::string& packet);
//     static std::string build(const std::string& type, const std::string& data);
//     static std::string buildDMsg(const std::string& receiver, const std::string& content);
// };

// #endif