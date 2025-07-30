#pragma once

#ifndef STRING_SPLITTER
#define STRING_SPLITTER

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cctype>

class StringSplitter {
public:
    static std::vector<std::string> split(const std::string& str, const std::string& delimiter = "", int maxsplit = -1) {
        std::vector<std::string> result;
        
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
    static std::vector<std::string> splitByWhitespace(const std::string& str, int maxsplit) {
        std::vector<std::string> result;
        std::stringstream ss(str);
        std::string token;
        int splits = 0;
        
        while (ss >> token && (maxsplit == -1 || splits < maxsplit)) {
            result.push_back(token);
            splits++;
        }
        
        // Handle remaining content when maxsplit is reached
        if (maxsplit != -1 && splits == maxsplit && ss >> token) {
            std::string remainder = token;
            std::string rest;
            while (ss >> rest) {
                remainder += " " + rest;
            }
            result.push_back(remainder);
        }
        
        return result;
    }
    
    static std::vector<std::string> splitByDelimiter(const std::string& str, const std::string& delimiter, int maxsplit) {
        std::vector<std::string> result;
        std::string::size_type start = 0;
        std::string::size_type end = 0;
        int splits = 0;
        
        while ((end = str.find(delimiter, start)) != std::string::npos && 
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
std::vector<std::string> split(const std::string& str, const std::string& delimiter = "", int maxsplit = -1) {
    return StringSplitter::split(str, delimiter, maxsplit);
}

#endif