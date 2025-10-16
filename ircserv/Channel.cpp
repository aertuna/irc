#include "Channel.hpp"
#include <sys/socket.h>
#include <string>

Channel::Channel(const std::string &name)
:   _name(name),
    _topic(""),
    _key(""),
    _limit(0),
    _inviteOnly(false),
    _topicRestricted(false)
{}

Channel::~Channel() {}

const std::string &Channel::getName() const
{ 
    return _name;
}

const std::string &Channel::getTopic() const
{
    return _topic;
}

void Channel::setTopic(const std::string &topic)
{
    _topic = topic;
}

void Channel::setKey(const std::string &key)
{
    _key = key;
}

const std::string &Channel::getKey() const
{
    return _key;
}

void Channel::setLimit(size_t limit)
{
    _limit = limit;
}

size_t Channel::getLimit() const
{
    return _limit;
}

void Channel::setInviteOnly(bool v)
{
    _inviteOnly = v;
}

bool Channel::isInviteOnly() const
{
    return _inviteOnly;
}

void Channel::setTopicRestricted(bool v)
{
    _topicRestricted = v;
}

bool Channel::isTopicRestricted() const
{
    return _topicRestricted;
}

bool Channel::addClient(Client *c)
{
    if (_clients.find(c) != _clients.end())
        return true;

    _clients.insert(c);
    removeInvitation(c);
    if (_operators.empty())
        _operators.insert(c);
    return true;
}

void Channel::removeClient(Client *c)
{
    _clients.erase(c);
    _operators.erase(c);
}

bool Channel::hasClient(Client *c) const
{
    return _clients.count(c) > 0;
}

const std::set<Client*>& Channel::getClients() const
{
    return _clients;
}

void Channel::addOperator(Client *c)
{
    _operators.insert(c);
}

void Channel::removeOperator(Client *c)
{
    _operators.erase(c);
}

bool Channel::isOperator(Client *c) const
{
    return _operators.count(c) > 0;
}

void Channel::invite(Client *c)
{
    _invited.insert(c);
}

bool Channel::isInvited(Client *c) const
{
    return _invited.count(c) > 0;
}

void Channel::removeInvitation(Client *c)
{
    _invited.erase(c);
}

void Channel::broadcast(Client *sender, const std::string &message)
{
    std::set<Client*>::const_iterator it = _clients.begin();
    for (; it != _clients.end(); ++it)
    {
        Client *c = *it;
        if (c == sender)
            continue;

        std::string raw = ":" + sender->getNickname() + " PRIVMSG " + _name + " :" + message + "\r\n";
        c->queueSend(raw);
    }
}
