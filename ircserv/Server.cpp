#include "Server.hpp"
#include "Commands.hpp"
#include "Channel.hpp"
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sstream>
#include <cctype>

Server* Server::s_instance = 0;

Server::Server(int port, const std::string &password)
: _port(port), _password(password), _server_fd(-1), _running(false)
{
    s_instance = this;
}

Server::~Server()
{
    stop();
    std::map<std::string, Channel*>::iterator itc = _channels.begin();
    for (; itc  !=  _channels.end(); ++itc)
        delete itc->second;
    std::map<int, Client*>::iterator it = _clients.begin();
    for (; it != _clients.end(); ++it)
        delete it->second;
}

void Server::initSocket()
{
    _server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_server_fd < 0)
        throw std::runtime_error("socket failed");

    int opt = 1;
    setsockopt(_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr; std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(_port);

    if (bind(_server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
        throw std::runtime_error("bind failed");
    if (listen(_server_fd, 128) < 0)
        throw std::runtime_error("listen failed");
    fcntl(_server_fd, F_SETFL, O_NONBLOCK);

    pollfd p; p.fd = _server_fd; p.events = POLLIN; p.revents = 0;
    _pollfds.push_back(p);
    std::cout << "[Server] Listening on " << _port << std::endl;
}

void Server::start()
{
    initSocket(); _running = true;
}

void Server::stop()
{
    for (size_t i = 0; i < _pollfds.size(); ++i)
        close(_pollfds[i].fd);
    _pollfds.clear();
    _server_fd = -1;
    _running = false;
}

void Server::acceptNewClient()
{
    sockaddr_in cli; socklen_t len = sizeof(cli);
    int cfd = accept(_server_fd, (struct sockaddr*)&cli, &len);
    if (cfd < 0)
        return;
    fcntl(cfd, F_SETFL, O_NONBLOCK);
    pollfd p; p.fd = cfd; p.events = POLLIN; p.revents = 0;
    _pollfds.push_back(p);
    _clients[cfd] = new Client(cfd);
    std::cout << "[Server] Client connected fd=" << cfd << std::endl;
}

void Server::receiveClientMessage(int fd)
{
    char buf[512];
    int n = recv(fd, buf, sizeof(buf)-1, 0);
    if (n <= 0)
    {
        std::cout << "[Server] Client disconnected fd=" << fd << std::endl;
        removeClient(fd);
        return;
    }

    buf[n] = '\0';
    std::map<int, Client*>::iterator itc = _clients.find(fd);
    if (itc == _clients.end())
        return;
    Client *cl = itc->second;
    cl->appendToBuffer(buf);

    while (true)
    {
        std::map<int, Client*>::iterator it_now = _clients.find(fd);
        if (it_now == _clients.end())
            return;

        cl = it_now->second;
        std::string line = cl->extractLine();
        if (line.empty())
            break;

        handleCommand(*cl, line);
    }
}

Channel* Server::getChannel(const std::string &name)
{
    std::map<std::string, Channel*>::iterator it = _channels.find(name);
    if (it != _channels.end())
        return it->second;
    Channel *ch = new Channel(name);
    _channels[name] = ch;
    return ch;
}

Client* Server::getClientByNickname(const std::string &nick)
{
    std::map<int, Client*>::iterator it = _clients.begin();
    for (; it != _clients.end(); ++it)
    {
        if (it->second && it->second->getNickname() == nick)
            return it->second;
    }
    return 0;
}

Client* Server::getClientByFd(int fd)
{
    std::map<int, Client*>::iterator it = _clients.find(fd);
    if (it == _clients.end()) return 0;
    return it->second;
}

std::map<std::string, Channel*>& Server::getChannels()
{
    return _channels;
}

void Server::enableWrite(int fd)
{
    for (size_t i = 0; i < _pollfds.size(); ++i)
    {
        if (_pollfds[i].fd == fd)
        {
            _pollfds[i].events |= POLLOUT;
            return;
        }
    }
}

void Server::disableWrite(int fd)
{
    for (size_t i = 0; i < _pollfds.size(); ++i)
    {
        if (_pollfds[i].fd == fd)
        {
            _pollfds[i].events &= ~POLLOUT;
            return;
        }
    }
}

void Server::removeClient(int fd)
{
    for (size_t i = 0; i < _pollfds.size(); ++i)
    {
        if (_pollfds[i].fd == fd)
        {
            close(_pollfds[i].fd);
            _pollfds.erase(_pollfds.begin() + i);
            break;
        }
    }

    std::map<int, Client*>::iterator it = _clients.find(fd);
    if (it == _clients.end())
        return;

    Client* victim = it->second;

    std::vector<std::string> emptyChannels;
    for (std::map<std::string, Channel*>::iterator itc = _channels.begin();
         itc != _channels.end(); ++itc)
    {
        Channel* ch = itc->second;
        if (ch && ch->hasClient(victim))
        {
            ch->removeClient(victim);
            if (ch->getClients().empty())
                emptyChannels.push_back(itc->first);
        }
    }

    delete victim;
    _clients.erase(it);

    for (size_t k = 0; k < emptyChannels.size(); ++k)
    {
        std::map<std::string, Channel*>::iterator cit = _channels.find(emptyChannels[k]);
        if (cit != _channels.end())
        {
            delete cit->second;
            _channels.erase(cit);
        }
    }
}

void Server::handleCommand(Client &client, const std::string &line)
{
    std::istringstream iss(line);
    std::string cmd; iss >> cmd;
    std::string args; std::getline(iss, args);
    
    if (!args.empty() && args[0]==' ')
        args.erase(0,1);
    for (size_t i = 0; i < cmd.size(); ++i)
        cmd[i] = (char)toupper(cmd[i]);

    if (cmd == "PASS")
        Commands::pass(*this, client, args);
    else if (cmd == "NICK")
        Commands::nick(*this, client, args);
    else if (cmd == "USER")
        Commands::user(*this, client, args);
    else if (cmd == "JOIN")
        Commands::join(*this, client, args);
    else if (cmd == "PRIVMSG")
        Commands::privmsg(*this, client, args);
    else if (cmd == "KICK")
        Commands::kick(*this, client, args);
    else if (cmd == "INVITE")
        Commands::invite(*this, client, args);
    else if (cmd == "TOPIC")
        Commands::topic(*this, client, args);
    else if (cmd == "MODE")
        Commands::mode(*this, client, args);
    else if (cmd == "PING")
        Commands::ping(*this, client, args);
    else if (cmd == "CAP")
        Commands::cap(*this, client, args);
    else if (cmd == "NOTICE")
        Commands::notice(*this, client, args);
    else if (cmd == "WHO")
        Commands::who(*this, client, args);
    else if (cmd == "NAMES")
        Commands::names(*this, client, args);
    else if (cmd == "QUIT")
        Commands::quit(*this, client, args);
    else
        std::cout << "[Server] Unknown command: " << cmd << std::endl;
}

void Server::run()
{
    while (_running)
    {
        if (_pollfds.empty())
            break;

        int ret = poll(&_pollfds[0], _pollfds.size(), 1000);
        if (ret < 0)
            continue;

        for (size_t i = 0; i < _pollfds.size(); ++i)
        {
            short rev = _pollfds[i].revents;
            if (!rev)
                continue;

            int fd = _pollfds[i].fd;

            if (rev & POLLIN)
            {
                if (fd == _server_fd)
                    acceptNewClient();
                else
                    receiveClientMessage(fd);
            }
            if (rev & POLLOUT)
            {
                Client* c = getClientByFd(fd);
                if (c)
                    c->flushSend();
            }
        }
    }
}
