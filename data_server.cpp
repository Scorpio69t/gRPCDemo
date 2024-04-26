#include <iostream>
#include <string>
#include <fstream>
#include <grpcpp/grpcpp.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/security/server_credentials.h>
#include "datatransfer.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::Status;

#define CHUNK_SIZE 300 * 1024 * 1024 // 300MB

class DataTransferServiceImpl final : public dataTransfer::DataTransfer::Service
{
    Status SendData(ServerContext *context, ServerReader<dataTransfer::DataChunk> *reader, dataTransfer::Reply *reply) override
    {
        dataTransfer::DataChunk chunk;
        std::ofstream file("received_data", std::ios::binary);
        int64_t len = 0;
        while (reader->Read(&chunk))
        {
            std::cout << "Received chunk of size: " << chunk.length() << std::endl;
            file.write(chunk.data().c_str(), chunk.length());
            len += chunk.length();
        }

        file.close();
        reply->set_length(len);

        return Status::OK;
    }
};

void RunServer()
{
    std::string server_address("0.0.0.0:50051");
    DataTransferServiceImpl service;

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    builder.SetMaxReceiveMessageSize(CHUNK_SIZE + 1);
    builder.SetMaxSendMessageSize(CHUNK_SIZE + 1);

    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;
    server->Wait();
}

int main(int argc, char **argv)
{
    RunServer();

    return 0;
}
