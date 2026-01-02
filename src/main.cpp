#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <string>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

int main(int argc, char* argv[]) {
    // Disable output buffering
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    // Creating the Server Socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Failed to create server socket: " << std::endl;
        return 1;
    }

    // Since the tester restarts your program quite often, setting SO_REUSEADDR
    // ensures that we don't run into 'Address already in use' errors
    int reuse = 1;

    // The setsockopt() function provides an application program with the
    // means to control socket behavior. An application program can use
    // setsockopt() to allocate buffer space, control timeouts, or permit
    // socket data broadcasts. The <sys/socket.h> header defines the
    // socket-level options available to setsockopt().

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        close(server_fd);
        std::cerr << "setsockopt failed: " << std::endl;
        return 1;
    }

    // Defining Server Address
    struct sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(9092);

    // bind address info with a socket, that define the nature of the socket itself.
    if (bind(server_fd, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) != 0) {
        close(server_fd);
        std::cerr << "Failed to bind to port 9092" << std::endl;
        return 1;
    }

    int connection_backlog = 5;
    
    // listen() marks the socket referred to by sockfd as a passive
    // socket, that is, as a socket that will be used to accept incoming
    // connection requests using accept(2).
    
    // The sockfd argument is a file descriptor that refers to a socket
    // of type SOCK_STREAM or SOCK_SEQPACKET.

    // The backlog argument defines the maximum length to which the queue
    // of pending connections for sockfd may grow.  If a connection
    // request arrives when the queue is full, the client may receive an
    // error with an indication of ECONNREFUSED or, if the underlying
    // protocol supports retransmission, the request may be ignored so
    // that a later reattempt at connection succeeds.

    if (listen(server_fd, connection_backlog) != 0) {
        close(server_fd);
        std::cerr << "listen failed" << std::endl;
        return 1;
    }

    std::cout << "Waiting for a client to connect...\n";

    // Defining client address holder, we did not defined any criteria for client connection for now.
    struct sockaddr_in client_addr{};
    socklen_t client_addr_len = sizeof(client_addr);

    std::cerr << "Logs from your program will appear here!\n";
    
    // Accept Client Connection, we will respond with a fd, that can be used to send data to server fd.
    int client_fd = accept(server_fd, reinterpret_cast<struct sockaddr*>(&client_addr), &client_addr_len);
    std::cout << "Client connected\n";

    // --- Kafka request ----
    // message_size (4) + header: `request_api_key` (2 bytes) + `request_api_key` (2 bytes) + `correlation_id` (4 bytes)
    int32_t request_message_size_ = htonl(0);

    char buffer[4096];

    // Receive Data from Client
    ::read(client_fd, buffer, sizeof(buffer));

    // ---- Kafka response ----
    // message_size (4 bytes) + header: correlation_id (4 bytes)
    int32_t message_size_ = *(u_int32_t*)(buffer);
    int16_t request_api_key = *(u_int16_t*) (buffer + 4);
    int16_t request_api_version = *(u_int16_t*) (buffer + 6);
    int32_t correlation_id_n_ = *(u_int32_t*)(buffer + 8);

    std::cout << "request_api_key: " << ntohs(request_api_key) << std::endl;
    std::cout << "request_api_version: " << ntohs(request_api_version) << std::endl;

    std::cout << "correlation_id: " << ntohl( correlation_id_n_ ) << std::endl;
    
    int16_t response_error_code_ = htons(35);

    char response[10];
    std::memcpy(response, &message_size_, 4);
    std::memcpy(response + 4, &correlation_id_n_, 4);
    std::memcpy(response + 8, &response_error_code_, 2);

    // Respond to client fd.
    ssize_t sent = send(client_fd, &response, sizeof(response), 0);

    if(sent != sizeof(response)){
        std::cerr << "Failed to send response\n";
    }

    close(client_fd);
    close(server_fd);
    return 0;
}