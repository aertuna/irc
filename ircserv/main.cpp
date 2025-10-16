#include "Server.hpp"
#include <csignal>
#include <cstdlib>
#include <iostream>

Server *g_server = 0;

void handleSig(int)
{
    if (g_server)
    {
        std::cout << "\n[Server] Shutting down..." << std::endl;
        g_server->stop();
    }
}

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        std::cerr << "Usage: ./ircserv <port> <password>" << std::endl;
        return 1;
    }

    int port = std::atoi(argv[1]);
    std::string pass = argv[2];

    Server srv(port, pass);
    g_server = &srv;
    std::signal(SIGINT, handleSig);
    std::signal(SIGTERM, handleSig);

    try
    {
        srv.start();
        srv.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Fatal: " << e.what() << std::endl;
        return 2;
    }
    return 0;
}
