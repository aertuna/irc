#include "Commands.hpp"
#include "Server.hpp"
#include "Channel.hpp"
#include "Client.hpp"
#include <sstream>
#include <sys/socket.h>
#include <cstdlib>
#include <algorithm>
#include <cctype>
#include <unistd.h>
#include <iostream>
#include <map>


void Replies::sendRaw(int fd, const std::string &raw)
{
    Client* c = Server::instance()->getClientByFd(fd);
    if (!c) return;
    c->queueSend(raw);
}

void Replies::notice(int fd, const std::string &msg)
{
    sendRaw(fd, ":ircserv NOTICE * :" + msg + "\r\n");
}

void Replies::numeric(int fd, const std::string &code, const std::string &msg)
{
    sendRaw(fd, ":ircserv " + code + " " + msg + "\r\n");
}


static std::string trim(const std::string &s)
{
    std::string::size_type a = 0, b = s.size();

    while (a < b && (s[a] == ' ' || s[a] == '\t'))
        ++a;

    while (b > a && (s[b-1] == ' ' || s[b-1] == '\t'))
        --b;
    return s.substr(a, b-a);
}

static std::string trim_leading_colon(std::string s)
{
    if (!s.empty() && s[0] == ':')
        s.erase(0,1);
    return s;
}

void Commands::tryRegister(Server &server, Client &client)
{
    (void)server;
    if (!client.hasPassOk())
        return;
    if (client.getNickname().empty() || client.getUsername().empty())
        return;
    if (client.isRegistered())
        return;

    client.markRegistered();
    client.authenticate();
    Replies::numeric(client.getFd(), "001",
        (client.getNickname().empty() ? "Welcome" : client.getNickname() + " :Welcome"));
}

void Commands::pass(Server &server, Client &client, const std::string &args)
{
    if (args == server.getPassword())
    {
        client.setPassOk(true);
        tryRegister(server, client);
    }
    else
    {
        Replies::numeric(client.getFd(), "464", ":Password incorrect");
    }
}

void Commands::nick(Server &server, Client &client, const std::string &args)
{
    std::string nick = trim(args);
    if (nick.empty())
    {
        Replies::numeric(client.getFd(),"431",":No nickname given");
        return;
    }
    
    client.setNickname(nick);
    tryRegister(server, client);
}

void Commands::user(Server &server, Client &client, const std::string &args)
{
    std::istringstream iss(args);
    std::string username;
    iss >> username;
    
    if (username.empty())
    {
        Replies::numeric(client.getFd(),"461","USER :Not enough parameters");
        return;
    }
    client.setUsername(username);
    tryRegister(server, client);
}

void Commands::join(Server &server, Client &client, const std::string &args)
{
    if (!client.isAuthenticated())
    {
        Replies::numeric(client.getFd(),"451",":You have not registered");
        return;
    }
    
    std::istringstream iss(args);
    std::string channelName, key;
    iss >> channelName >> key;

    if (channelName.empty() || channelName[0] != '#')
    {
        Replies::numeric(client.getFd(),"403",":No such channel");
        return;
    }

    Channel *ch = server.getChannel(channelName);

    if (ch->isInviteOnly() && !ch->isInvited(&client) && !ch->isOperator(&client))
    {
        Replies::numeric(client.getFd(), "473", channelName + " :Invite-only channel");
        return;
    }

    if (ch->getLimit() != 0 && ch->getClients().size() >= ch->getLimit())
    {
        Replies::numeric(client.getFd(), "471", channelName + " :Channel is full");
        return;
    }

    if (!ch->getKey().empty() && key != ch->getKey())
    {
        Replies::numeric(client.getFd(),"475", channelName + " :Cannot join channel (+k)");
        return;
    }

    ch->addClient(&client);

    std::string joinMsg = ":" + (client.getNickname().empty() ? "anon" : client.getNickname()) + " JOIN " + ch->getName() + "\r\n";
    const std::set<Client*>& clients = ch->getClients();
    std::set<Client*>::const_iterator it = clients.begin();
    
    for (; it != clients.end(); ++it)
        Replies::sendRaw((*it)->getFd(), joinMsg);

    if (!ch->getTopic().empty())
        Replies::numeric(client.getFd(), "332", ch->getName() + " :" + ch->getTopic());
    Commands::names(server, client, ch->getName());
}

void Commands::privmsg(Server &server, Client &client, const std::string &args)
{
    if (!client.isAuthenticated())
    {
        Replies::numeric(client.getFd(),"451",":You have not registered");
        return;
    }

    std::istringstream iss(args);
    std::string target; iss >> target;
    std::string message; std::getline(iss, message);
    
    if (!message.empty() && message[0] == ' ')
        message.erase(0,1);

    Channel *ch = 0;
    if (!target.empty() && target[0] == '#') {
        std::map<std::string, Channel*>& chans = server.getChannels();
        std::map<std::string, Channel*>::iterator itc = chans.find(target);
        if (itc != chans.end()) ch = itc->second;
    }
    
    if (ch && ch->hasClient(&client))
    {
        ch->broadcast(&client, message);
    }
    else
    {
        Replies::numeric(client.getFd(),"401", target + " :No such nick/channel");
    }
}

void Commands::kick(Server &server, Client &client, const std::string &args)
{
    std::istringstream iss(args);
    std::string channelName, targetNick; iss >> channelName >> targetNick;

    if (channelName.empty() || targetNick.empty())
    {
        Replies::numeric(client.getFd(), "461", "KICK :Not enough parameters");
        return;
    }

    Channel *ch = 0;
    {
        std::map<std::string, Channel*>& chans = server.getChannels();
        std::map<std::string, Channel*>::iterator itc = chans.find(channelName);
        if (itc != chans.end()) ch = itc->second;
    }

    if (!ch)
    {
        Replies::numeric(client.getFd(),"403", channelName + " :No such channel");
        return;
    }

    if (!ch->hasClient(&client) || !ch->isOperator(&client))
    {
        Replies::numeric(client.getFd(),"482", channelName + " :You're not channel operator");
        return;
    }

    Client *target = server.getClientByNickname(targetNick);
    if (!target || !ch->hasClient(target))
    {
        Replies::numeric(client.getFd(),"441", targetNick + " " + channelName + " :They aren't on that channel");
        return;
    }

    ch->removeClient(target);
    std::string raw = ":" + client.getNickname() + " KICK " + ch->getName() + " " + targetNick + "\r\n";
    const std::set<Client*>& clients = ch->getClients();
    for (std::set<Client*>::const_iterator it = clients.begin(); it != clients.end(); ++it)
        Replies::sendRaw((*it)->getFd(), raw);
    Replies::sendRaw(target->getFd(), raw);
}


void Commands::invite(Server &server, Client &client, const std::string &args)
{
    std::istringstream iss(args);
    std::string channelName, targetNick; iss >> channelName >> targetNick;

    Channel *ch = 0;
    {
        std::map<std::string, Channel*>& chans = server.getChannels();
        std::map<std::string, Channel*>::iterator itc = chans.find(channelName);
        if (itc != chans.end()) ch = itc->second;
    }
    
    if (!ch || !ch->isOperator(&client))
    {
        Replies::numeric(client.getFd(),"482",channelName + " :You're not channel operator");
        return;
    }

    Client *target = server.getClientByNickname(targetNick);
    
    if (!target)
    {
        Replies::numeric(client.getFd(),"401", targetNick + " :No such nick");
        return;
    }

    ch->invite(target);
    std::string raw = ":" + client.getNickname() + " INVITE " + targetNick + " " + ch->getName() + "\r\n";
    Replies::sendRaw(target->getFd(), raw);
}

void Commands::topic(Server &server, Client &client, const std::string &args)
{
    std::istringstream iss(args);
    std::string channelName; iss >> channelName;
    std::string topic; std::getline(iss, topic);
    
    if (!topic.empty() && topic[0]==' ')
        topic.erase(0,1);

    Channel *ch = 0;
    {
        std::map<std::string, Channel*>& chans = server.getChannels();
        std::map<std::string, Channel*>::iterator itc = chans.find(channelName);
        if (itc != chans.end()) ch = itc->second;
    }
    
    if (!ch)
    {
        Replies::numeric(client.getFd(),"403",":No such channel");
        return;
    }
    
    if (ch->isTopicRestricted() && !ch->isOperator(&client) && !topic.empty())
    {
        Replies::numeric(client.getFd(),"482", channelName + " :You're not channel operator");
        return;
    }

    if (topic.empty())
    {
        if (ch->getTopic().empty())
            Replies::numeric(client.getFd(), "331", channelName + " :No topic is set");
        else
            Replies::numeric(client.getFd(), "332", channelName + " :" + ch->getTopic());
        return;
    }
    else
    {
        ch->setTopic(topic);
        std::string raw = ":" + client.getNickname() + " TOPIC " + ch->getName() + " :" + ch->getTopic() + "\r\n";
        const std::set<Client*>& clients = ch->getClients();
        std::set<Client*>::const_iterator it = clients.begin();
        
        for (; it != clients.end(); ++it)
            Replies::sendRaw((*it)->getFd(), raw);
    }
}

void Commands::mode(Server &server, Client &client, const std::string &args)
{
    (void)server;
    std::istringstream iss(args);
    std::string channelName; iss >> channelName;

    Channel *ch = 0;
    {
        std::map<std::string, Channel*>& chans = server.getChannels();
        std::map<std::string, Channel*>::iterator itc = chans.find(channelName);
        if (itc != chans.end()) ch = itc->second;
    }
    
    if (!ch)
    {
        Replies::numeric(client.getFd(),"403",":No such channel");
        return;
    }
    
    std::string modes; iss >> modes;
    std::string param; iss >> param;

    if (!ch->isOperator(&client))
    {
        Replies::numeric(client.getFd(),"482",channelName + " :You're not channel operator");
        return;
    }

    int sign = +1;
    
    for (size_t i = 0; i < modes.size(); ++i)
    {
        char c = modes[i];
        if (c == '+')
        {
            sign = +1;
            continue;
        }
        if (c == '-')
        {
            sign = -1;
            continue;
        }
        if (c == 'i')
            ch->setInviteOnly(sign > 0);
        
        else if (c == 't')
            ch->setTopicRestricted(sign > 0);
        
        else if (c=='l')
        {
            if (sign > 0)
            {
                int lim = std::atoi(param.c_str());
                if (lim < 0)
                    lim = 0;
                ch->setLimit((size_t)lim);
            }
            else
                ch->setLimit(0);
        }
        else if (c=='k')
        {
            if (sign>0)
                ch->setKey(param);
            else
                ch->setKey("");
        }
        else if (c=='o')
        {
            Client *t = 0;
            if (!param.empty())
                t = server.getClientByNickname(param);
            if (t)
            {
                if (sign>0)
                    ch->addOperator(t);
                else
                    ch->removeOperator(t);
            }
        }
    }

    std::string reply = ":" + client.getNickname() + " MODE " + ch->getName() + " " + modes;
    
    if (!param.empty())
        reply += " " + param;
    reply += "\r\n";
    
    const std::set<Client*>& clients = ch->getClients();
    std::set<Client*>::const_iterator it = clients.begin();
    
    for (; it != clients.end(); ++it)
        Replies::sendRaw((*it)->getFd(), reply);
}

void Commands::ping(Server &, Client &client, const std::string &args)
{
    std::string token = trim_leading_colon(trim(args));
    
    if (token.empty())
        token = "ping";
    Replies::sendRaw(client.getFd(), ":ircserv PONG ircserv :" + token + "\r\n");
}

void Commands::cap(Server &, Client &client, const std::string &args)
{
    std::istringstream iss(args);
    std::string sub; iss >> sub;
    
    for (size_t i=0;i<sub.size();++i)
        sub[i] = (char)std::toupper((unsigned char)sub[i]);

    if (sub == "LS") 
        Replies::sendRaw(client.getFd(), ":ircserv CAP * LS :\r\n");
    else if (sub == "LIST")
        Replies::sendRaw(client.getFd(), ":ircserv CAP * LIST :\r\n");
    else if (sub == "REQ")
    {
        std::string rest; std::getline(iss, rest);
        
        if (!rest.empty() && rest[0]==' ')
            rest.erase(0,1);
        Replies::sendRaw(client.getFd(), ":ircserv CAP * NAK :" + rest + "\r\n");
    }
    else if (sub == "END"){}
    else
        Replies::sendRaw(client.getFd(), ":ircserv CAP * NAK :\r\n");
}

void Commands::notice(Server &server, Client &client, const std::string &args)
{
    std::istringstream iss(args);
    std::string target; iss >> target;

    std::string message; std::getline(iss, message);
    if (!message.empty() && message[0] == ' ')
        message.erase(0, 1);
    if (!message.empty() && message[0] == ':')
        message.erase(0, 1);

    if (target.empty() || message.empty())
        return;

    if (!target.empty() && target[0] == '#')
    {
        Channel *ch = 0;
        {
            std::map<std::string, Channel*>& chans = server.getChannels();
            std::map<std::string, Channel*>::iterator itc = chans.find(target);
            if (itc != chans.end()) ch = itc->second;
        }
        if (!ch || !ch->hasClient(&client))
            return;

        const std::set<Client*>& clients = ch->getClients();
        for (std::set<Client*>::const_iterator it = clients.begin(); it != clients.end(); ++it) {
            Client *rcv = *it;
            if (rcv == &client) continue;
            std::string raw = ":" + (client.getNickname().empty() ? "anon" : client.getNickname())
                            + " NOTICE " + ch->getName() + " :" + message + "\r\n";
            Replies::sendRaw(rcv->getFd(), raw);
        }
        return;
    }

    Client *rcv = server.getClientByNickname(target);
    if (!rcv)
        return;
    
    std::string raw = ":" + (client.getNickname().empty() ? "anon" : client.getNickname())
                    + " NOTICE " + target + " :" + message + "\r\n";
    Replies::sendRaw(rcv->getFd(), raw);
}

void Commands::who(Server &server, Client &client, const std::string &args)
{
    std::istringstream iss(args);
    std::string mask; iss >> mask;

    const std::string me = client.getNickname().empty() ? "*" : client.getNickname();
    const int fd = client.getFd();

    std::string endMask = mask.empty() ? "*" : mask;
    std::string endLine = ":ircserv 315 " + me + " " + endMask + " :End of /WHO list\r\n";

    if (!mask.empty() && mask[0] == '#')
    {
        Channel *ch = 0;
        {
            std::map<std::string, Channel*>& chans = server.getChannels();
            std::map<std::string, Channel*>::iterator itc = chans.find(mask);
            if (itc != chans.end()) ch = itc->second;
        }
        if (ch)
        {
            const std::set<Client*>& clients = ch->getClients();
            
            for (std::set<Client*>::const_iterator it = clients.begin(); it != clients.end(); ++it)
            {
                Client *c = *it;
                std::string nick = c->getNickname().empty() ? "anon" : c->getNickname();
                std::string user = c->getUsername().empty() ? "user" : c->getUsername();
                std::string flags = "H";
                if (ch->isOperator(c))
                    flags += "@";

                std::string line = ":ircserv 352 " + me + " " + mask + " " + user
                                 + " localhost ircserv " + nick + " " + flags
                                 + " :0 " + nick + "\r\n";
                Replies::sendRaw(fd, line);
            }
        }
        Replies::sendRaw(fd, endLine);
        return;
    }

    if (!mask.empty())
    {
        Client *t = server.getClientByNickname(mask);
        if (t)
        {
            std::string nick = t->getNickname().empty() ? "anon" : t->getNickname();
            std::string user = t->getUsername().empty() ? "user" : t->getUsername();
            std::string line = ":ircserv 352 " + me + " * " + user
                             + " localhost ircserv " + nick + " H"
                             + " :0 " + nick + "\r\n";
            Replies::sendRaw(fd, line);
        }
    }
    Replies::sendRaw(fd, endLine);
}

void Commands::names(Server &server, Client &client, const std::string &args)
{
    std::istringstream iss(args);
    std::string channels; iss >> channels;

    const std::string me = client.getNickname().empty() ? "*" : client.getNickname();
    const int fd = client.getFd();

    const std::string send_end_all = ":ircserv 366 " + me + " * :End of /NAMES list\r\n";
    const std::string send_end = " :End of /NAMES list\r\n";

    if (!channels.empty())
    {
        std::string rest = channels;
        while (!rest.empty())
        {
            std::string chan;
            std::string::size_type pos = rest.find(',');
            if (pos == std::string::npos)
            {
                chan = rest;
                rest.clear();
            }
            else
            {
                chan = rest.substr(0, pos);
                rest.erase(0, pos + 1);
            }

            if (!chan.empty() && chan[0] == '#') {
                Channel *ch = 0;
                {
                    std::map<std::string, Channel*>& chans = server.getChannels();
                    std::map<std::string, Channel*>::iterator itc = chans.find(chan);
                    if (itc != chans.end()) ch = itc->second;
                }
                if (ch) {
                    std::string nicks;
                    const std::set<Client*>& clients = ch->getClients();
                    for (std::set<Client*>::const_iterator it = clients.begin(); it != clients.end(); ++it) {
                        Client *c = *it;
                        std::string nick = c->getNickname().empty() ? "anon" : c->getNickname();
                        if (!nicks.empty()) nicks += " ";
                        if (ch->isOperator(c)) nicks += "@";
                        nicks += nick;
                    }
                    Replies::sendRaw(fd, ":ircserv 353 " + me + " = " + chan + " :" + nicks + "\r\n");
                }
                Replies::sendRaw(fd, ":ircserv 366 " + me + " " + chan + send_end);
            }
        }
        return;
    }
    Replies::sendRaw(fd, send_end_all);
}

void Commands::quit(Server &server, Client &client, const std::string &args)
{
    std::string message = args;
    if (!message.empty() && message[0] == ':')
        message.erase(0, 1);

    std::string quitMsg = ":" + (client.getNickname().empty() ? "anon" : client.getNickname())
                        + " QUIT :" + (message.empty() ? "Client Quit" : message) + "\r\n";

    std::map<std::string, Channel*> channels = server.getChannels();
    for (std::map<std::string, Channel*>::iterator it = channels.begin(); it != channels.end(); ++it)
    {
        Channel *ch = it->second;
        if (ch->hasClient(&client))
        {
            const std::set<Client*>& clients = ch->getClients();
            for (std::set<Client*>::const_iterator cit = clients.begin(); cit != clients.end(); ++cit)
            {
                Client *c = *cit;
                if (c != &client)
                    Replies::sendRaw(c->getFd(), quitMsg);
            }
            ch->removeClient(&client);
        }
    }

    int fd = client.getFd();
    server.removeClient(fd);
    std::cout << "[Server] Client quit fd=" << fd << std::endl;
}
