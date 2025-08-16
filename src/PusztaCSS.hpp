#pragma once
#include <string>
#include <cctype>
#include <stdexcept>
#include <map>
#include "lakys-string-helper.hpp"

using namespace std;

class CSSParser
{
private:
	const string str;
	size_t i = 0;

	// Skip whitespace
	void whitespace()
	{
		while(i < str.size() && std::isspace(static_cast<unsigned char>(str[i])))
		{
			++i;
		}
	}

	    // Parse and return a “word” (ident, number, #hex, …)
    std::string word() 
	{
        std::size_t start = i;
        while (i < str.size()) 
		{
            char c = str[i];
            if (std::isalnum(static_cast<unsigned char>(c)) || c == '#' || c == '-' || c == '.' || c == '%')
            {
                ++i;
            } 
			else 
			{
                break;
            }
        }
        if (i == start) 
		{
            throw std::runtime_error("CSSParser error: expected word at position " + std::to_string(i));
        }
        return str.substr(start, i - start);
    }

    // Expect exactly the given character, or throw
    void literal(char expected) 
	{
        if (i >= str.size() || str[i] != expected) 
		{
            throw std::runtime_error(std::string("CSSParser error: expected '") + expected +"' at position " + std::to_string(i));
        }
        ++i;
    }

	// Create a pair of the property and value from a line
	pair<string, string> pair()
	{
		string prop = word();
		whitespace();
		literal(':');
		whitespace();
		string val = word();

		return {to_lowercase(prop), val};
	}

	char ignore_until(string chars)
	{
		while(i < str.length())
		{
			if(contains(chars, string{str[i]}))
			{
				return str[i];
			}
			else
			{
				i += 1;
			}
		}

		return '\0';
	}

public:
	CSSParser(const string& str) : str(str) {}

	map<string, string> body()
	{
		map<string, string> pairs;

		while(i < str.length())
		{
			try
			{
				if (i >= str.size()) break;
				auto [prop, val] = pair();
				pairs[prop] = val;
				whitespace();
				literal(';');
				whitespace();
			}
			catch (...)
			{
				auto why = ignore_until(";");
				// Check if why is ;
				if(why == ';')
				{
					literal(';');
					whitespace();
				}
				else
				{
					break;
				}
			}

		}
		return pairs;
	}
};