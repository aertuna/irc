#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

class Client
{
    private:
        int _fd;
        std::string _nickname;
        std::string _username;
        std::string _buffer;
        bool _authenticated;

        bool _pass_ok;
        bool _registered;

        std::string _outbox;

    public:
        Client(int fd);
        ~Client();

        int getFd() const;
        const std::string &getNickname() const;
        const std::string &getUsername() const;
        void setNickname(const std::string &nick);
        void setUsername(const std::string &user);
        bool isAuthenticated() const;
        void authenticate();

        void appendToBuffer(const std::string &data);
        std::string extractLine();

        void setPassOk(bool v);
        bool hasPassOk() const;

        void markRegistered();
        bool isRegistered() const;

        void queueSend(const std::string &data);
        bool hasPending() const;
        void flushSend();
};

#endif
