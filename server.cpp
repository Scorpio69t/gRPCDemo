#include <iostream>
#include <fstream>
#include <ctime>
#include <unistd.h>
#include <chrono>
#include <grpcpp/grpcpp.h>
#include "helloworld.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using helloworld::Demo;
using helloworld::HelloReply;
using helloworld::HelloRequest;

class DemoServiceImpl final : public Demo::Service
{
public:
    Status SayHello(ServerContext *context, const HelloRequest *request, HelloReply *reply) override
    {
        std::ofstream file("received_data.bin", std::ios::binary);
        size_t size = request->data().length();
        std::cout << "Received " << size << " bytes." << std::endl;
        file.write(request->data().c_str(), size);
        file.close();

        reply->set_code(static_cast<int32_t>(size));
        return Status::OK;
    }
};

void runServer()
{
    std::string server_address("0.0.0.0:50051");
    DemoServiceImpl service;

    // 将最大消息大小设置为 2 GB
    ServerBuilder builder;
    builder.SetMaxReceiveMessageSize(1.8 * 1024 * 1024 * 1024); // 2 GB
    builder.SetMaxSendMessageSize(1.8 * 1024 * 1024 * 1024);    // 2 GB
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;

    server->Wait();
}

int main()
{
    runServer();
    return 0;
}
