#pragma once
#include <iostream>

#include <cstring>
class ccl_string
{
public:
//    ccl_string();
//    ccl_string(const char*);
//    ccl_string(const ccl_string&);
//    ccl_string(const std::string&);
//
//    ~ccl_string();
//    const char* c_str() const;
//    ccl_string& operator=(const ccl_string&);
//    operator std::string () const;
//    ccl_string operator+(const ccl_string&);
//    ccl_string operator+(const char*);
//    ccl_string operator+(const std::string&);
//
//    friend bool operator> (const ccl_string &, const ccl_string &);
//    friend bool operator<= (const ccl_string &, const ccl_string &);
//
//    friend bool operator< (const ccl_string &, const ccl_string &);
//    friend bool operator>= (const ccl_string &, const ccl_string &);
//    friend std::ostream& operator<<(std::ostream&,
//                                    const ccl_string&);

    ~ccl_string() {
        delete [] str;
        str = 0;
        length = 0;
    }

    ccl_string() {
        str = new char[1];
        str[0] = '\0';
        length = 0;
    }

    ccl_string(const char *_str) {
        length = strlen(_str);
        str = new char[length + 1];
        strcpy(str, _str);
    }

    ccl_string(const ccl_string& _str){
        length = _str.length;
        str = new char[length + 1];
        strcpy(str, _str.str);
    }

    ccl_string(const std::string& _str){
        length = _str.length();
        str = new char[length + 1];
        strcpy(str, _str.c_str());
    }

    ccl_string& operator=(const ccl_string& _str) {
        length = _str.length;
        delete [] str;
        str = new char[length + 1];
        strcpy(str, _str.str);
        return *this;
    }

    const char* c_str() const {
        return str;
    };

    operator std::string () const
    {
        return std::string(str);
    }

    friend std::ostream& operator<<(std::ostream& out,
                                     const ccl_string& _str) {
        out << _str.str;
        return out;
    }

    ccl_string operator+(const char* _str) {
        char* tmp_str = new char[length + strlen(_str) + 1];
        strcpy(tmp_str, str);
        strcpy(&tmp_str[length], _str);
        ccl_string res(tmp_str);
        delete[] tmp_str;
        return res;
    }

    ccl_string operator+(const ccl_string& _str) {
        return (*this + _str.c_str());
    }

    ccl_string operator+(const std::string& _str) {
        return (*this + _str.c_str());
    }

    friend std::string operator+(const std::string &str1, const ccl_string &str2) {
        return (str1 + str2.c_str());
    }

    friend bool operator> (const ccl_string &str1, const ccl_string &str2) {
        return strcmp(str1.c_str(), str2.c_str()) > 0;
    }

    friend bool operator<= (const ccl_string &str1, const ccl_string &str2) {
        return strcmp(str1.c_str(), str2.c_str()) <= 0;
    }

    friend bool operator< (const ccl_string &str1, const ccl_string &str2) {
        return strcmp(str1.c_str(), str2.c_str()) < 0;
    }

    friend bool operator>= (const ccl_string &str1, const ccl_string &str2) {
        return strcmp(str1.c_str(), str2.c_str()) >= 0;
    }

    friend bool operator== (const ccl_string &str1, const ccl_string &str2) {
        return strcmp(str1.c_str(), str2.c_str()) == 0;
    }

private:
    size_t length;
    char* str;
};
