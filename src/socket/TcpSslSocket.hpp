#pragma once

#include "TcpSocket.hpp"
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509.h>

class TcpSslSocket : public TcpSocket {
protected:
    SSL_CTX* _sslContext;
    SSL* _ssl;
    bool _sslInitialized;

    TcpSslSocket(const char* host, const short port)
        : TcpSocket(host, port), _sslContext(nullptr), _ssl(nullptr), _sslInitialized(false) {
        initializeSSL();
    }

    ~TcpSslSocket() {
        cleanup();
    }

    bool initializeSSL() {
        // Initialize OpenSSL library
        SSL_library_init();
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();
        return true;
    }

    void cleanup() {
        if (_ssl) {
            SSL_shutdown(_ssl);
            SSL_free(_ssl);
            _ssl = nullptr;
        }
        if (_sslContext) {
            SSL_CTX_free(_sslContext);
            _sslContext = nullptr;
        }
    }

public:
    // Now these can properly override the virtual methods
    bool sendData(void* buf, size_t len) override {
        if (!_ssl) return false;
        return SSL_write(_ssl, buf, len) == (int)len;
    }

    bool receiveData(void* buf, size_t len) override {
        if (!_ssl) return false;
        return SSL_read(_ssl, buf, len) == (int)len;
    }

    // SSL-specific methods
    bool setupSSLContext(bool isClient = true) {
        const SSL_METHOD* method;
        
        if (isClient) {
            method = TLS_client_method();
        } else {
            method = TLS_server_method();
        }
        
        _sslContext = SSL_CTX_new(method);
        if (!_sslContext) {
            sprintf_s(_message, "SSL_CTX_new failed");
            return false;
        }
        
        // Set minimum TLS version
        SSL_CTX_set_min_proto_version(_sslContext, TLS1_2_VERSION);
        return true;
    }

    bool createSSL() {
        if (!_sslContext) return false;
        
        _ssl = SSL_new(_sslContext);
        if (!_ssl) {
            sprintf_s(_message, "SSL_new failed");
            return false;
        }
        
        // Attach socket to SSL
        if (SSL_set_fd(_ssl, _sock) != 1) {
            sprintf_s(_message, "SSL_set_fd failed");
            return false;
        }
        
        return true;
    }

    void setTrustStore() {
        if (_sslContext) {
			if (SSL_CTX_load_verify_locations(_sslContext, "assets/ca_cert.pem", nullptr) != 1) {
				fprintf(stderr, "Failed to load CA bundle\n");
				// handle error
			}
			SSL_CTX_set_verify(_sslContext, SSL_VERIFY_PEER, nullptr);
        }
    }

    bool isSSLConnected() const {
        return _ssl != nullptr && _sslInitialized;
    }
};
