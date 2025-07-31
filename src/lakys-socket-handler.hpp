#pragma once

#ifndef LAKYS_SOCKET_HANDLER_HPP
#define LAKYS_SOCKET_HANDLER_HPP

#define _HAS_STD_BYTE 0

#include <string>
#include <iostream>
#include <stdexcept>
#include "StringSplitter.hpp"
#include "PusztaParser.hpp"
#include "socket/TcpSslClientSocket.hpp"
#include "lakys-file-loader.hpp"
#include <map>

using namespace std;

class HTTP
{
private:
    string scheme;
    string url;
    string host;
    string path;
    int port;

    string body;
    string headers;
    map<string, string> header_map;


    string cert_file;

    int status;
    string status_message;

    int redirect_depth = 0;
    int max_depth = 10;

public:

    bool should_parse;
    
    HTTP() : scheme(""), url(""), host(""), path(""), port(80) {}

    void set_cert_file(string cert_file)
    {
        this->cert_file = cert_file;
    }

    string get_url()
    {
        return this->url;
    }

    void set(const string& url)
    {
        // Split by scheme
        vector<string> scheme_parts = split(url, "://");
        if(scheme_parts.size() != 2)
        {
            cerr << "Invalid URL format: " << url << endl;
            throw invalid_argument("Invalid URL format");
        }
        
        this->scheme = scheme_parts[0];
        string url_without_scheme = scheme_parts[1];

        // Fixed logic: scheme must be either
        vector<string> valid_schemes = {"http", "https", "file", "view-source:http", "view-source:https"};
        if(this->scheme.empty() || url_without_scheme.empty() || find(valid_schemes.begin(), valid_schemes.end(), this->scheme) == valid_schemes.end())
        {
            cerr << "Invalid URL format or unsupported scheme: " << url << endl;
            throw invalid_argument("Invalid URL format or unsupported scheme");
        }

        // Set default ports
        if(this->scheme == "http")
        {
            this->port = 80;
        }
        else if(this->scheme == "https")
        {
            this->port = 443;
        }

        cout << "URL without scheme: " << url_without_scheme << endl;

        // Split the URL without scheme to get host and path
        vector<string> parts = split(url_without_scheme, "/");
        
        if(parts.empty())
        {
            cerr << "Invalid URL format: " << url << endl;
            throw invalid_argument("Invalid URL format");
        }

        this->host = parts[0];

        // Check for custom port
        if(contains(host, ":"))
        {
            vector<string> host_parts = split(host, ":");
            this->host = host_parts[0];
            try {
                this->port = stoi(host_parts[1]);
            } catch (const invalid_argument& e) {
                cerr << "Invalid port number in URL: " << url << endl;
                throw invalid_argument("Invalid port number in URL");
            }
        }
        
        // Construct the path
        if(parts.size() > 1)
        {
            this->path = "/";
            for(size_t i = 1; i < parts.size(); i++)
            {
                this->path += parts[i];
                if(i < parts.size() - 1) this->path += "/";
            }
        }
        else
        {
            this->path = "/";
        }

        this->url = this->scheme + "://" + this->host + this->path;
    }

    string request()
    {
        if(this->scheme == "https" || this->scheme == "view-source:https")
        {
            return request_https();
        }
        else if(this->scheme == "file")
        {
            std::cout << this->path << std::endl;
            return load_file(this->path);
        }
        else
        {
            return request_http();
        }
    }

	string request_https()
	{
        TcpSslClientSocket socket(this->host.c_str(), this->port);
		

		socket.openConnection();

        if (socket.isSSLConnected()) {

            std::cout << "SSL connection established!" << std::endl;

            

            string request;
            request += "GET " + this->path + " HTTP/1.1\r\n";
            request += "Host: " + this->host + "\r\n";
            request += "Connection: close\r\n";
            request += "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/138.0.0.0 Safari/537.36\r\n";

            request += "\r\n";
            cout << request;

            if(!socket.sendData((void*)request.c_str(), request.size()))
            {
                cerr << "Failed to send request to " << this->host << endl;
                return "";
            }


            std::string response_str;
            const size_t CHUNK = 4096;
            std::vector<char> buf(CHUNK);
            int n;

            while ((n = socket.receiveDataInt(buf.data(), CHUNK)) > 0) {
                response_str.append(buf.data(), n);
            }

            cout << "Response from " << this->host << ":\n" << response_str << endl;

            socket.closeConnection();

            if(response_str.empty())
            {
                cerr << "No response received from " << this->host << endl;
                return "";
            }

            // Seperate headers and body
            return start_parsing(response_str);
        }
        else
        {
            std::cout << "SSL connection failed: " << socket.getMessage() << std::endl;
            return "SSL Connection could not be established.";
        }
		


	}


    string request_http()
	{

        TcpClientSocket socket(this->host.c_str(), this->port);		

		socket.openConnection();

        if(this->scheme == "https")
        {
            std::cout << "SSL connection established!" << std::endl;

        }
        

        string request;
        request += "GET " + this->path + " HTTP/1.1\r\n";
        request += "Host: " + this->host + "\r\n";
        request += "Connection: close\r\n";
        request += "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/138.0.0.0 Safari/537.36\r\n";

        request += "\r\n";
        cout << request;

        if(!socket.sendData((void*)request.c_str(), request.size()))
        {
            cerr << "Failed to send request to " << this->host << endl;
            return "";
        }


        std::string response_str;
        const size_t CHUNK = 4096;
        std::vector<char> buf(CHUNK);
        int n;

        while ((n = socket.receiveDataInt(buf.data(), CHUNK)) > 0) {
            response_str.append(buf.data(), n);
        }

        cout << "Response from " << this->host << ":\n" << response_str << endl;

        socket.closeConnection();

        if(response_str.empty())
        {
            cerr << "No response received from " << this->host << endl;
            return "";
        }

        if(response_str.empty())
        {
            cerr << "No response received from " << this->host << endl;
            return "";
        }

        return start_parsing(response_str);


	}

    string start_parsing(string response_str)
    {
        cout << "Staring parsing...";

        size_t header_end = response_str.find("\r\n\r\n");
        if(header_end == string::npos)
        {
            cerr << "Invalid response format from " << this->host << endl;
            return "";
        }
        this->headers = response_str.substr(0, header_end);
        this->body = response_str.substr(header_end + 4); // Skip the "\r\n\r\n"
        cout << "Headers:\n" << this->headers << endl;
        cout << "Body:\n" << this->body << endl;

        // Parse headers into a map
        vector<string> header_lines = split(this->headers, "\r\n");

        // HTTP version and status code
        if(header_lines.empty())
        {
            cerr << "No headers found in response from " << this->host << endl;
            return "";
        }
        string first_line = header_lines[0];
        vector<string> first_line_parts = split(first_line, " ");
        if(first_line_parts.size() < 3)
        {
            cerr << "Invalid first line in response from " << this->host << endl;
            return "";
        }
        this->status = stoi(first_line_parts[1]);
        this->status_message = first_line_parts[2];

        header_lines.erase(header_lines.begin());

        for(const string& line : header_lines)
        {
            size_t colon_pos = line.find(": ");
            if(colon_pos != string::npos)
            {
                string key = line.substr(0, colon_pos);
                string value = line.substr(colon_pos + 2);
                this->header_map[key] = value;
            }
        }

        if(this->status == 200)
        {
            cout << "HTTP Status: " << this->status << " " << this->status_message << endl;
            cout << "rendering time" << endl;
            if(!this->body.empty())
            {
                if(this->scheme == "view-source:http" || this->scheme == "view-source:https")
                {
                    this->should_parse = false;
                    return this->body;
                }
                else
                {
                    this->should_parse = true;
                    return this->body;
                }

            }
            else
            {
                cout << "No body";
                return "Body not found. Tags:\n\n" + this->headers;
            }
        }
        else if (this->status == 301 || this->status == 302) {
            if (this->redirect_depth < this->max_depth) {
                std::string location = this->header_map["Location"];
                // Trim whitespace
                location.erase(location.find_last_not_of(" \n\r\t") + 1);
                location.erase(0, location.find_first_not_of(" \n\r\t"));
                if (location.empty()) {
                    std::cerr << "No Location header found for red irect" << std::endl;
                    return "No Location header found for redirect";
                }

                // Check if the location is relative
                if (location[0] != '/' && location.find("://") == std::string::npos) {
                    // If it's relative, prepend the host
                    location = this->scheme + "://" + this->host + location;
                }

                std::cout << "Redirecting to: " << location << std::endl;
                try {
                    this->set(location);
                } catch (const std::exception& e) {
                    std::cerr << "Failed to set redirect URL: " << e.what() << std::endl;
                    return "Invalid Location header for redirect";
                }
                this->redirect_depth += 1;
                return this->request(); // Always return the result!
            } else {
                return "Maximum amount of redirects reached.\nThe website you're visiting might've fallen into a redirect loop; Please try again.";
            }
        }
        else
        {
            cerr << "HTTP Error " << this->status << ": " << this->status_message << endl;
            return "HTTP Error " + to_string(this->status) + ": " + this->status_message;
        }



    }
};

#endif