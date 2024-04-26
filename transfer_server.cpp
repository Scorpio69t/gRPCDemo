#include <iostream>
#include <string>
#include <fstream>
#include <grpcpp/grpcpp.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/security/server_credentials.h>
#include "transferfile.grpc.pb.h"
#include <sys/resource.h>
#include <unistd.h>
#include <chrono>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::Status;
using transferfile::FileChunk;
using transferfile::FileUploadStatus;
using transferfile::TransferFile;
using namespace std::chrono;

#define CHUNK_SIZE 3 * 1024 * 1024 // 3MB

class TransferServiceImpl final : public TransferFile::Service
{
public:
    Status UploadFile(ServerContext *context, ServerReader<FileChunk> *reader, FileUploadStatus *status) override;

protected:
    void printUsage(const struct rusage &start, const struct rusage &end)
    {
        std::cout << "CPU Usage: User time: " << (end.ru_utime.tv_sec - start.ru_utime.tv_sec) + (end.ru_utime.tv_usec - start.ru_utime.tv_usec) / 1000000.0
                  << "s, System time: " << (end.ru_stime.tv_sec - start.ru_stime.tv_sec) + (end.ru_stime.tv_usec - start.ru_stime.tv_usec) / 1000000.0 << "s" << std::endl;
        std::cout << "Max resident set size: " << (end.ru_maxrss - start.ru_maxrss) << " KB" << std::endl;
    }
};

Status TransferServiceImpl::UploadFile(ServerContext *context, ServerReader<FileChunk> *reader, FileUploadStatus *status)
{
    FileChunk chunk;
    std::ofstream outfile;
    const char *data;

    struct rusage usage_start, usage_end;
    getrusage(RUSAGE_SELF, &usage_start);

    auto start = high_resolution_clock::now();

    outfile.open("output.bin", std::ios::binary | std::ios::out | std::ios::trunc);
    if (!outfile.is_open())
    {
        std::cerr << "Error: Failed to open file" << std::endl;
        return Status::CANCELLED;
    }

    while (reader->Read(&chunk))
    {
        data = chunk.buffer().c_str();
        outfile.write(data, chunk.buffer().length());
    }

    long pos = outfile.tellp();
    status->set_length(pos);
    outfile.close();

    auto end = high_resolution_clock::now();
    getrusage(RUSAGE_SELF, &usage_end);
    auto duration = duration_cast<milliseconds>(end - start);

    printUsage(usage_start, usage_end);
    std::cout << "Total transmission time: " << duration.count() << " ms" << std::endl;
    std::cout << "Total data transmitted: " << pos << " bytes" << std::endl;
    std::cout << "Transmission rate: " << (pos * 1000.0 / duration.count()) / 1024 / 1024 << " MB/s" << std::endl;

    return Status::OK;
}

void RunServer()
{
    std::string server_address("0.0.0.0:50051");
    TransferServiceImpl service;

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;
    server->Wait();
}

int main(int argc, char **argv)
{
    RunServer();
    return 0;
}
