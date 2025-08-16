#pragma once

#ifndef STRING_SPLITTER
#define STRING_SPLITTER

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <locale>

using namespace std;

class StringSplitter {
public:
    static vector<string> split(const string& str, const string& delimiter = "", int maxsplit = -1) {
        vector<string> result;
        
        if (str.empty()) {
            return result;
        }
        
        // Default whitespace splitting (like Python's split())
        if (delimiter.empty()) {
            return splitByWhitespace(str, maxsplit);
        }
        
        // Specific delimiter splitting
        return splitByDelimiter(str, delimiter, maxsplit);
    }

private:
    static vector<string> splitByWhitespace(const string& str, int maxsplit) {
        vector<string> result;
        stringstream ss(str);
        string token;
        int splits = 0;
        
        while (ss >> token && (maxsplit == -1 || splits < maxsplit)) {
            result.push_back(token);
            splits++;
        }
        
        // Handle remaining content when maxsplit is reached
        if (maxsplit != -1 && splits == maxsplit && ss >> token) {
            string remainder = token;
            string rest;
            while (ss >> rest) {
                remainder += " " + rest;
            }
            result.push_back(remainder);
        }
        
        return result;
    }
    
    static vector<string> splitByDelimiter(const string& str, const string& delimiter, int maxsplit) {
        vector<string> result;
        string::size_type start = 0;
        string::size_type end = 0;
        int splits = 0;
        
        while ((end = str.find(delimiter, start)) != string::npos && 
               (maxsplit == -1 || splits < maxsplit)) {
            result.push_back(str.substr(start, end - start));
            start = end + delimiter.length();
            splits++;
        }
        
        // Add the remaining part
        result.push_back(str.substr(start));
        
        return result;
    }
};

// Convenience function for easier usage
vector<string> split(const string& str, const string& delimiter = "", int maxsplit = -1) {
    return StringSplitter::split(str, delimiter, maxsplit);
}

bool contains(const string& haystack, const string& needle) {
    return haystack.find(needle) != string::npos;
}

string to_lowercase(string in)
{
    locale loc;
    string lowercase = "";
    for(auto letter : in)
    {
        if(isalpha(letter, loc))
        {
            lowercase += tolower(letter, loc);
        }
        else
        {
            lowercase += letter;
        }
    }

    return lowercase;
}

#endif