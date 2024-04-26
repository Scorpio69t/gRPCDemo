#include <iostream>
#include <fstream>
#include <string>
#include <grpcpp/grpcpp.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include "datatransfer.grpc.pb.h"

using dataTransfer::DataChunk;
using dataTransfer::DataTransfer;
using dataTransfer::Reply;
using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientWriter;
using grpc::Status;

#define CHUNK_SIZE 300 * 1024 * 1024 // 300MB

class DataTransferClient
{
public:
    DataTransferClient(std::shared_ptr<Channel> channel)
        : stub_(DataTransfer::NewStub(channel)) {}

    void SendData(char *data, uint32_t len)
    {
        DataChunk chunk;
        ClientContext context;
        Reply reply;
        char *buffer = new char[CHUNK_SIZE];

        std::unique_ptr<ClientWriter<DataChunk>> writer(stub_->SendData(&context, &reply));
        if (writer == nullptr)
        {
            std::cerr << "Error: Failed to create writer" << std::endl;
            return;
        }

        int64_t bytes_sent = 0;
        chunk.set_length(len);
        while (bytes_sent < len)
        {
            uint32_t chunk_size = len - bytes_sent > CHUNK_SIZE ? CHUNK_SIZE : len - bytes_sent;
            memcpy(buffer, data + bytes_sent, chunk_size);
            chunk.set_data(buffer, chunk_size);
            if (!writer->Write(chunk))
            {
                std::cerr << "Error: Failed to write chunk" << std::endl;
                break;
            }
            bytes_sent += chunk_size;
        }

        delete[] buffer;

        writer->WritesDone();
        Status status = writer->Finish();
        if (!status.ok())
        {
            std::cerr << "Error: " << status.error_message() << std::endl;
            return;
        }
        std::cout << "Data sent successfully" << std::endl;

        return;
    }

private:
    std::unique_ptr<DataTransfer::Stub> stub_;
};

int main(int argc, char **argv)
{
    grpc::ChannelArguments args;
    args.SetMaxSendMessageSize(CHUNK_SIZE + 1);
    args.SetMaxReceiveMessageSize(CHUNK_SIZE + 1);

    DataTransferClient client(grpc::CreateCustomChannel("localhost:50051", grpc::InsecureChannelCredentials(), args));
    std::ifstream infile;
    infile.open("test.bin", std::ios::binary | std::ios::in);
    if (!infile.is_open())
    {
        std::cerr << "Error: File not found" << std::endl;
        return 1;
    }

    auto file_size = infile.tellg();
    infile.seekg(0, std::ios::end);
    file_size = infile.tellg() - file_size;
    infile.seekg(0, std::ios::beg);
    char *buffer = new char[file_size];
    infile.read(buffer, file_size);

    std::cout << "File size: " << file_size << " bytes" << std::endl;
    client.SendData(buffer, file_size);

    return 0;
}
