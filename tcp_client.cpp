#include <iostream>
#include <fstream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <cstring>

#define CHUNK_SIZE 3 * 1024 * 1024 // 3MB

int main(int argc, char *argv[])
{
    const char *server_ip = "127.0.0.1";
    int server_port = 8888;
    std::string file_path = argv[1];

    // Create socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        std::cerr << "Socket creation failed." << std::endl;
        return 1;
    }

    // Define the server address
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(server_port);
    inet_pton(AF_INET, server_ip, &server_address.sin_addr);

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        std::cerr << "Connection to server failed." << std::endl;
        close(sock);
        return 1;
    }

    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file: " << file_path << std::endl;
        close(sock);
        return 1;
    }

    // Send file contents
    char *buffer = new char[CHUNK_SIZE];
    while (file.read(buffer, sizeof(buffer)) || file.gcount())
    {
        if (send(sock, buffer, file.gcount(), 0) < 0)
        {
            std::cerr << "Failed to send data." << std::endl;
            break;
        }
    }

    delete[] buffer;

    std::cout << "File sent successfully." << std::endl;

    file.close();
    close(sock);

    return 0;
}
