#include <iostream>
#include <fstream>
#include <unistd.h>
#include <thread>
#include <chrono>
#include <grpcpp/grpcpp.h>
#include "helloworld.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using helloworld::Demo;
using helloworld::HelloReply;
using helloworld::HelloRequest;

class DemoClient
{
public:
    DemoClient(std::shared_ptr<Channel> channel) : stub_(Demo::NewStub(channel)) {}

    void SayHello(const std::string &file_path)
    {
        HelloRequest request;
        std::ifstream file(file_path, std::ios::binary);
        if (!file.is_open())
        {
            std::cerr << "Error: file not found" << std::endl;
            return;
        }

        file.seekg(0, std::ios::end);
        size_t size = file.tellg();
        // std::cout << "File size: " << size << " bytes." << std::endl;
        file.seekg(0, std::ios::beg);
        request.mutable_data()->resize(size);
        file.read(&(*request.mutable_data())[0], size);

        HelloReply reply;
        ClientContext context;
        auto start = std::chrono::steady_clock::now();
        Status status = stub_->SayHello(&context, request, &reply);
        auto end = std::chrono::steady_clock::now();
        if (status.ok())
        {
            std::cout << "Received " << reply.code() << " bytes." << std::endl;
        }
        else
        {
            std::cerr << "status code: " << status.error_code() << ": " << status.error_message() << std::endl;
            return;
        }

        std::chrono::duration<double> elapsed = end - start;
        double gb = size / 1024.0 / 1024.0 / 1024.0;

        auto used = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "Time taken: " << used << " ms" << std::endl;
        std::cout << "Bytes sent: " << size << std::endl;
        double transfer_speed = static_cast<double>(size) / used;
        std::cout << "Transfer speed: " << transfer_speed << " bytes/ms" << std::endl;
    }

private:
    std::unique_ptr<Demo::Stub> stub_;
};

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <file_path>" << std::endl;
        return 1;
    }

    std::string ip = "localhost";
    if (argc == 3)
    {
        ip = argv[2];
    }

    std::string file_path(argv[1]);

    // 将最大消息大小设置为 2 GB
    grpc::ChannelArguments args;
    args.SetMaxReceiveMessageSize(1.8 * 1024 * 1024 * 1024); // 2 GB
    args.SetMaxSendMessageSize(1.8 * 1024 * 1024 * 1024);    // 2 GB

    std::string server_address = ip + ":50051";
    DemoClient client(grpc::CreateCustomChannel(server_address, grpc::InsecureChannelCredentials(), args));
    client.SayHello(file_path);

    return 0;
}
