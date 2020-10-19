#pragma once

#include <cstring>
#include <iostream>
#include <string>

namespace ccl {

class string
{
public:

    ~string() {
        delete [] storage;
        storage = 0;
        length = 0;
    }

    string() {
        storage = new char[1];
        storage[0] = '\0';
        length = 0;
    }

    string(const char* str) {
        length = strlen(str);
        storage = new char[length + 1];
        strncpy(storage, str, length);
        storage[length] = '\0';
    }

    string(const string& str) {
        length = str.length;
        storage = new char[length + 1];
        strncpy(storage, str.storage, length);
        storage[length] = '\0';
    }

    string(const std::string& str){
        length = str.length();
        storage = new char[length + 1];
        strncpy(storage, str.c_str(), length);
        storage[length] = '\0';
    }

    string& operator= (const string& str) {
        length = str.length;
        delete [] storage;
        storage = new char[length + 1];
        strncpy(storage, str.storage, length);
        storage[length] = '\0';
        return *this;
    }

    const char* c_str() const {
        return storage;
    };

    operator std::string () const {
        return std::string(storage);
    }

    friend std::ostream& operator<< (std::ostream& out,
                                     const string& str) {
        out << str.storage;
        return out;
    }

    string operator+ (const char* str) {
        char* tmp_str = new char[length + strlen(str) + 1];
        strncpy(tmp_str, storage, length);
        strncpy(&tmp_str[length], str, strlen(str));
        tmp_str[length + strlen(str)] = '\0';
        string res(tmp_str);
        delete[] tmp_str;
        return res;
    }

    string operator+ (const string& str) {
        return (*this + str.c_str());
    }

    string operator+ (const std::string& str) {
        return (*this + str.c_str());
    }

    friend std::string operator+(const std::string &str1, const string &str2) {
        return (str1 + str2.c_str());
    }

    friend bool operator> (const string &str1, const string &str2) {
        return strcmp(str1.c_str(), str2.c_str()) > 0;
    }

    friend bool operator<= (const string &str1, const string &str2) {
        return strcmp(str1.c_str(), str2.c_str()) <= 0;
    }

    friend bool operator< (const string &str1, const string &str2) {
        return strcmp(str1.c_str(), str2.c_str()) < 0;
    }

    friend bool operator>= (const string &str1, const string &str2) {
        return strcmp(str1.c_str(), str2.c_str()) >= 0;
    }

    friend bool operator== (const string &str1, const string &str2) {
        return strcmp(str1.c_str(), str2.c_str()) == 0;
    }

private:
    size_t length;
    char* storage;
};

} // namespace ccl
