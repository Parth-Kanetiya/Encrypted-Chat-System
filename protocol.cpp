#include<bits/stdc++.h>
#include "protocol.h"
using namespace std;



const string Protocol::JOIN  = "JOIN";
const string Protocol::MSG   = "MSG";
const string Protocol::DMSG  = "DMSG";
const string Protocol::KEY   = "KEY";
const string Protocol::LEAVE = "LEAVE";
const string Protocol::ERROR_MSG = "ERROR";
const string Protocol::LIST  = "LIST";
const string Protocol::SERVER_MSG = "SERVER";

//splitter will be '|'
vector<string> Protocol::split(const string& packet, char splitter) {
    vector<string> result;
    stringstream ss(packet);
    string part;

    while (getline(ss, part, splitter)) {
        result.push_back(part);
    }

    return result;//it will be either 2 or 3 parts depending on the type of message
}
string Protocol::getType(string &packet) {
    return split(packet, '|')[0];
}
// string Protocol::getData(const string& packet) {
//     vector<string> parts = split(packet, '|');
//     if(parts.size() == 2) return parts[1];
//     else if(parts.size() == 3) return parts[2];

//     //else
//     return "";
// }
string Protocol::getData(const std::string& packet) {
        size_t pos = packet.find('|');
        return (pos != string::npos) ? packet.substr(pos + 1) : "";
}
// string Protocol::getSenderName(const string& packet) {
//     vector<string> parts = split(packet, '|');
//     if(parts.size() == 3) return parts[1];
//     else return "";
// }
string Protocol::build(const string& type, const string& data) {
    return type + "|" + data;
}
string Protocol::buildDMsg(const string& receiver, const string& data) {
    return DMSG + "|" + receiver + "|" + data;
}