#pragma once

#include "TcpClientSocket.hpp"
#include "TcpSslSocket.hpp"

// Use multiple inheritance to get both client functionality and SSL functionality
class TcpSslClientSocket : public TcpClientSocket {
private:
    SSL_CTX* _sslContext;
    SSL* _ssl;
    bool _sslInitialized;

public:
    TcpSslClientSocket(const char* host, const short port)
        : TcpClientSocket(host, port), _sslContext(nullptr), _ssl(nullptr), _sslInitialized(false) {
        initializeSSL();
        setupSSLContext(true); // Client mode
        setTrustStore();
    }

    ~TcpSslClientSocket() {
        cleanup();
    }

    void openConnection() override {
        // First establish TCP connection using parent method
        TcpClientSocket::openConnection();
        
        if (!_connected) {
            return; // TCP connection failed
        }
        
        // Create SSL object and attach to socket
        if (!createSSL()) {
            _connected = false;
            return;
        }
        
        // Set hostname for SNI (Server Name Indication)
        if (SSL_set_tlsext_host_name(_ssl, _host) != 1) {
            sprintf_s(_message, "SSL_set_tlsext_host_name failed");
            _connected = false;
            return;
        }
        
        // Perform SSL handshake
        if (performSSLHandshake()) {
            _sslInitialized = true;
            sprintf_s(_message, "SSL connection established");
        } else {
            _connected = false;
        }
    }

    // Override data methods for SSL
    bool sendData(void* buf, size_t len) override {
        if (!_ssl) return false;
        return SSL_write(_ssl, buf, len) == (int)len;
    }

    bool receiveData(void* buf, size_t len) override {
        if (!_ssl) return false;
        return SSL_read(_ssl, buf, len) == (int)len;
    }

	int receiveDataInt(void* buf, size_t len) {
        if (!_ssl) return -1;
        //return SSL_read(_ssl, buf, len) == (int)len;

		int n = SSL_read(_ssl, buf, len);
		if (n <= 0) {
			int err = SSL_get_error(_ssl, n);
			// Optionally translate SSL errors to errno or your own error codes
			return (n < 0 ? -1 : 0);
		}
		return n;
    }

    bool isSSLConnected() const {
        return _ssl != nullptr && _sslInitialized;
    }

private:
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

    bool performSSLHandshake() {
        int result = SSL_connect(_ssl);
        
        if (result == 1) {
            return true; // Handshake successful
        }
        
        int ssl_error = SSL_get_error(_ssl, result);
        switch (ssl_error) {
            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_WANT_WRITE:
                sprintf_s(_message, "SSL handshake would block");
                break;
            case SSL_ERROR_ZERO_RETURN:
                sprintf_s(_message, "SSL connection closed");
                break;
            case SSL_ERROR_SYSCALL:
                sprintf_s(_message, "SSL syscall error");
                break;
            default:
                sprintf_s(_message, "SSL handshake failed with error: %d", ssl_error);
                break;
        }
        
        return false;
    }
};
