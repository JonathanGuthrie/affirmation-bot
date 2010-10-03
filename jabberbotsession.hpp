#if !defined(_JABBERBOTSESSION_HPP_INCLUDED_)
#define _JABBERBOTSESSION_HPP_INCLUDED_

#include "jabber-bot/jabber-bot.hpp"

#include <map>
#include <set>
#include <vector>
#include <queue>

#include <db_cxx.h>

class JabberBotSession;

typedef struct {
  void (JabberBotSession::*method)(const std::string *from, MessageStanza::MessageTypes type, const std::string *id);
} CommandHandler;

class Upcoming;

typedef std::vector<Upcoming> UpcomingList;

class Upcoming {
private:
  std::string m_user;
  time_t m_next;

public:
  Upcoming(std::string user, time_t next)  : m_user(user), m_next(next) {}
  friend class UpcomingCompare;
  const std::string GetUser(void) const { return m_user; }
  const time_t GetNext(void) const { return m_next; }
};

class UpcomingCompare {
public :
  bool operator()( const Upcoming &a, const Upcoming &b ) {
    return a.m_next < b.m_next;
  }
};

typedef std::map<std::string, CommandHandler> CommandHandlerMap;
typedef std::vector<std::string> AffirmationList;
typedef std::set<std::string> ResourceSet;
typedef std::map<std::string, ResourceSet> UserStatusType;
typedef std::priority_queue<Upcoming, UpcomingList, UpcomingCompare> UpcomingQueue;
typedef std::set<std::string> SendList;

class JabberException : public std::exception {
private:
    const std::string message;
    
public:
    JabberException(const std::string error) : message(error) {
    }

    virtual const char* what() const throw() {
	return message.c_str();
    }

    virtual ~JabberException() throw() {
    }
};


class JabberBotSession {
private:
  JabberSession *m_session;
  static int HandlePresenceRequest(void *context, const PresenceStanza &request, class JabberSession *session);
  static int HandleMessageRequest(void *context, const MessageStanza &request, class JabberSession *session);

  void DefaultCommand(const std::string *from, MessageStanza::MessageTypes type, const std::string *id);
  void Subscribe(const std::string *from, MessageStanza::MessageTypes type, const std::string *id);
  void Unsubscribe(const std::string *from, MessageStanza::MessageTypes type, const std::string *id);
  void Affirmation(const std::string *from, MessageStanza::MessageTypes type, const std::string *id);
  void Status(const std::string *from, MessageStanza::MessageTypes type, const std::string *id);
  void SetAvailable(const std::string *from);
  void SetUnavailable(const std::string *from);
  void ProcessSendList(void);
  void SetPresence(PresenceStanza::PresenceTypes type, const std::string *to = NULL, const std::string *status = NULL);

  static CommandHandlerMap m_handlers;
  static AffirmationList m_affirmations;
  UserStatusType m_userStatuses;
  Db m_subscriptionDb;
  UpcomingQueue m_upcomingQueue;
  SendList m_sendList;
  unsigned m_id;
  time_t m_eod;
  bool m_continueRunning;

public:
  JabberBotSession(const std::string &host, unsigned short port, bool isSecure, const std::string &userid, const std::string &password, const std::string &resource, const std::string &email, const std::string &affirmation_path, const std::string &db_path);
  void RunSession(void);
  ~JabberBotSession(void);
  void ShutDown(void) { m_continueRunning = false; }

};


#endif // _JABBERSESSION_HPP_INCLUDED_