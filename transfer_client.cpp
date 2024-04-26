#include <iostream>
#include <string>
#include <fstream>
#include <chrono>
#include <grpcpp/grpcpp.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include "transferfile.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientWriter;
using grpc::Status;
using transferfile::FileChunk;
using transferfile::FileUploadStatus;
using transferfile::TransferFile;

#define CHUNK_SIZE 3 * 1024 * 1024 // 3MB

class TransferClient
{
public:
    TransferClient(std::shared_ptr<Channel> channel) : stub_(TransferFile::NewStub(channel)) {}

    void uploadFile(const std::string &filename);

private:
    std::unique_ptr<TransferFile::Stub> stub_;
};

void TransferClient::uploadFile(const std::string &filename)
{
    FileChunk chunk;
    char *buffer = new char[CHUNK_SIZE];
    FileUploadStatus status;
    ClientContext context;
    std::ifstream infile;
    unsigned long len = 0;

    auto start = std::chrono::steady_clock::now();
    infile.open(filename, std::ios::binary | std::ios::in);
    if (!infile.is_open())
    {
        std::cerr << "Error: File not found" << std::endl;
        return;
    }

    std::unique_ptr<ClientWriter<FileChunk>> writer(stub_->UploadFile(&context, &status)); // Create a writer to send chunks of file
    while (!infile.eof())
    {
        infile.read(buffer, CHUNK_SIZE);
        chunk.set_buffer(buffer, infile.gcount());
        if (!writer->Write(chunk))
        {
            std::cerr << "Error: Failed to write chunk" << std::endl;
            break;
        }
        len += infile.gcount();
    }

    infile.close();
    delete[] buffer;

    writer->WritesDone();
    Status status1 = writer->Finish();
    if (status1.ok() && len == status.length())
    {
        auto end = std::chrono::steady_clock::now();
        std::cout << "File uploaded successfully" << std::endl;
        std::cout << "Time taken: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;
        std::cout << "File size: " << len << " bytes" << std::endl;
        auto speed = (len / std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()) * 1000 / 1024 / 1024;
        std::cout << "Speed: " << speed << " MB/s" << std::endl;
    }
    else
    {
        std::cerr << "Error: " << status1.error_message() << "len: " << len << " status.length(): " << status.length()
                  << std::endl;
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <filename> [server_ip]" << std::endl;
        return 1;
    }

    std::string filename(argv[1]);
    std::string server_ip = "localhost";

    if (argc == 3)
    {
        server_ip = argv[2];
    }

    TransferClient client(grpc::CreateChannel(server_ip + ":50051", grpc::InsecureChannelCredentials()));
    client.uploadFile(filename);

    return 0;
}
