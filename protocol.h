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
        // static string getSenderName(const string& packet);
        static string build(const string& type, const string& data);
        static string buildDMsg(const string& receiver, const string& content);
};