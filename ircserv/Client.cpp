#include "Client.hpp"
#include "Server.hpp"
#include <cstddef>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>

Client::Client(int fd)
: _fd(fd),
  _nickname(""),
  _username(""),
  _buffer(""),
  _authenticated(false),
  _pass_ok(false),
  _registered(false),
  _outbox("")
{}

Client::~Client() {}

int Client::getFd() const
{
    return _fd;
}

const std::string &Client::getNickname() const
{
    return _nickname;
}

const std::string &Client::getUsername() const
{
    return _username;
}

void Client::setNickname(const std::string &nick)
{
    _nickname = nick;
}

void Client::setUsername(const std::string &user)
{
    _username = user;
}

bool Client::isAuthenticated() const
{
    return _authenticated;
}

void Client::authenticate()
{
    _authenticated = true;
}

void Client::appendToBuffer(const std::string &data)
{
    _buffer += data;
}

std::string Client::extractLine()
{
    std::string::size_type pos = _buffer.find('\n');
    if (pos == std::string::npos)
        return "";
    
    std::string line = _buffer.substr(0, pos);
    if (!line.empty() && line[line.size()-1] == '\r')
        line.erase(line.size()-1);
    _buffer.erase(0, pos + 1);
    
    return line;
}

void Client::setPassOk(bool v)
{
    _pass_ok = v;
}

bool Client::hasPassOk() const
{
    return _pass_ok;
}

void Client::markRegistered()
{
    _registered = true;
}

bool Client::isRegistered() const
{
    return _registered;
}

void Client::queueSend(const std::string &data)
{
    _outbox += data;
    Server::instance()->enableWrite(_fd);
}

bool Client::hasPending() const
{
    return !_outbox.empty();
}

void Client::flushSend()
{
    while (!_outbox.empty())
    {
        ssize_t n = ::send(_fd, _outbox.data(), _outbox.size(), 0);
        if (n > 0)
            _outbox.erase(0, static_cast<size_t>(n));
        else
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            break;
        }
    }

    if (_outbox.empty())
        Server::instance()->disableWrite(_fd);
}
