#include<bits/stdc++.h>
#include "crypto.h"
using namespace std;    

string xorCipher(const string& data,const string& key) {
    // if (key.empty()) return data;
    string result = data;
    int keyLength = key.size();
    for (int i = 0; i < data.size(); i++)
        result[i] = data[i] ^ key[i%keyLength];
    return result;
}

string generateKey() {
    int len = 16;
    string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    string key;
    srand((unsigned)time(nullptr));//seed for the random number generator
    for (int i = 0; i < len; i++)
        key += chars[rand()%chars.size()];
    return key;
}