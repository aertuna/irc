#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <set>
#include "Client.hpp"

class Channel
{
    private:
        std::string _name;
        std::string _topic;
        std::string _key;
        size_t _limit;
        bool _inviteOnly;
        bool _topicRestricted;
        std::set<Client*> _clients;
        std::set<Client*> _operators;
        std::set<Client*> _invited;
    public:
        Channel(const std::string &name);
        ~Channel();

        const std::string &getName() const;
        const std::string &getTopic() const;
        void setTopic(const std::string &topic);

        void setKey(const std::string &key);
        const std::string &getKey() const;
        void setLimit(size_t limit);
        size_t getLimit() const;
        void setInviteOnly(bool v);
        bool isInviteOnly() const;
        void setTopicRestricted(bool v);
        bool isTopicRestricted() const;

        bool addClient(Client *c);
        void removeClient(Client *c);
        bool hasClient(Client *c) const;
        const std::set<Client*>& getClients() const;

        void addOperator(Client *c);
        void removeOperator(Client *c);
        bool isOperator(Client *c) const;

        void invite(Client *c);
        bool isInvited(Client *c) const;
        void removeInvitation(Client *c);

        void broadcast(Client *sender, const std::string &message);
};

#endif
