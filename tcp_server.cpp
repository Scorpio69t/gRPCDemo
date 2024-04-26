#include <iostream>
#include <fstream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <sys/resource.h>
#include <unistd.h>
#include <chrono>

#define CHUNK_SIZE 3 * 1024 * 1024 // 3MB

using namespace std::chrono;

void printUsage(const struct rusage &start, const struct rusage &end)
{
    std::cout << "CPU Usage: User time: " << (end.ru_utime.tv_sec - start.ru_utime.tv_sec) + (end.ru_utime.tv_usec - start.ru_utime.tv_usec) / 1000000.0
              << "s, System time: " << (end.ru_stime.tv_sec - start.ru_stime.tv_sec) + (end.ru_stime.tv_usec - start.ru_stime.tv_usec) / 1000000.0 << "s" << std::endl;
    std::cout << "Max resident set size: " << (end.ru_maxrss - start.ru_maxrss) << " KB" << std::endl;
}

int main()
{
    int server_port = 8888;
    const char *output_file = "output_received_file";

    // Create socket
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0)
    {
        std::cerr << "Socket creation failed." << std::endl;
        return 1;
    }

    // Bind socket to IP / port
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(server_port);
    server_address.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_sock, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        std::cerr << "Bind failed." << std::endl;
        close(server_sock);
        return 1;
    }

    // Listen
    if (listen(server_sock, 10) < 0)
    {
        std::cerr << "Listen failed." << std::endl;
        close(server_sock);
        return 1;
    }

    std::cout << "Server is listening on port " << server_port << std::endl;

    // Accept connection
    struct sockaddr_in client_address;
    socklen_t client_len = sizeof(client_address);
    int client_sock = accept(server_sock, (struct sockaddr *)&client_address, &client_len);
    if (client_sock < 0)
    {
        std::cerr << "Accept failed." << std::endl;
        close(server_sock);
        return 1;
    }

    struct rusage usage_start, usage_end;
    getrusage(RUSAGE_SELF, &usage_start);

    auto start = high_resolution_clock::now();

    std::ofstream file(output_file, std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file for writing." << std::endl;
        close(client_sock);
        close(server_sock);
        return 1;
    }

    // Receive data
    char *buffer = new char[CHUNK_SIZE];
    int bytes_received;
    while ((bytes_received = recv(client_sock, buffer, sizeof(buffer), 0)) > 0)
    {
        file.write(buffer, bytes_received);
    }

    if (bytes_received < 0)
    {
        std::cerr << "Error in recv()." << std::endl;
    }
    else
    {
        std::cout << "File received successfully." << std::endl;
        long pos = file.tellp();
        auto end = high_resolution_clock::now();
        getrusage(RUSAGE_SELF, &usage_end);
        auto duration = duration_cast<milliseconds>(end - start);

        printUsage(usage_start, usage_end);
        std::cout << "Total transmission time: " << duration.count() << " ms" << std::endl;
        std::cout << "Total data transmitted: " << pos << " bytes" << std::endl;
        std::cout << "Transmission rate: " << (pos * 1000.0 / duration.count()) / 1024 / 1024 << " MB/s" << std::endl;
    }

    delete[] buffer;

    file.close();
    close(client_sock);
    close(server_sock);
    return 0;
}
