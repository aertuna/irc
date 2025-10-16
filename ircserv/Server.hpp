#ifndef SERVER_HPP
#define SERVER_HPP

#include <map>
#include <vector>
#include <string>
#include <poll.h>
#include "Client.hpp"
#include "Channel.hpp"

class Server
{
    private:
        int _port;
        std::string _password;
        int _server_fd;
        bool _running;
        std::vector<pollfd> _pollfds;
        std::map<int, Client*> _clients;
        std::map<std::string, Channel*> _channels;

        static Server* s_instance;

        void initSocket();
        void acceptNewClient();
        void receiveClientMessage(int fd);
        void handleCommand(Client &client, const std::string &line);

    public:
        Server(int port, const std::string &password);
        ~Server();

        static Server* instance() { return s_instance; }

        void start();
        void stop();
        void run();

        std::map<std::string, Channel*>& getChannels();
        void removeClient(int fd);

        Channel* getChannel(const std::string &name);
        Client* getClientByNickname(const std::string &nick);

        Client* getClientByFd(int fd);
        void enableWrite(int fd);
        void disableWrite(int fd);

        const std::string &getPassword() const { return _password; }
};

#endif
