#ifndef COMMANDS_HPP
#define COMMANDS_HPP

#include <string>

class Server;
class Client;

struct Replies
{
    static void sendRaw(int fd, const std::string &raw);
    static void notice(int fd, const std::string &msg);
    static void numeric(int fd, const std::string &code, const std::string &msg);
};

class Commands
{
    public:
        static void pass(Server &server, Client &client, const std::string &args);
        static void nick(Server &server, Client &client, const std::string &args);
        static void user(Server &server, Client &client, const std::string &args);
        static void join(Server &server, Client &client, const std::string &args);
        static void privmsg(Server &server, Client &client, const std::string &args);
        static void kick(Server &server, Client &client, const std::string &args);
        static void invite(Server &server, Client &client, const std::string &args);
        static void topic(Server &server, Client &client, const std::string &args);
        static void mode(Server &server, Client &client, const std::string &args);

        static void ping(Server &server, Client &client, const std::string &args);
        static void cap(Server &server, Client &client, const std::string &args);
        static void notice(Server &server, Client &client, const std::string &args);
        static void who(Server &server, Client &client, const std::string &args);
        static void names(Server &server, Client &client, const std::string &args);
        static void quit(Server &server, Client &client, const std::string &args);

        static void tryRegister(Server &server, Client &client);
};

#endif
