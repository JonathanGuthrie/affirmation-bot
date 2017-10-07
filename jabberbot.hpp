/*
 * Copyright 2017 Jonathan R. Guthrie
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#if !defined(_JABBERBOT_HPP_INCLUDED_)
#define _JABBERBOT_HPP_INCLUDED_

#include <boost/thread.hpp>

#include <gloox/client.h>
#include <gloox/message.h>
#include <gloox/messagehandler.h>
#include <gloox/loghandler.h>
#include <gloox/rostermanager.h>

#include <map>
#include <set>
#include <vector>
#include <queue>

#include <db_cxx.h>

#include "insensitive.hpp"

class JabberBot;

typedef struct {
  void (JabberBot::*method)(const gloox::Message &stanza);
} CommandHandler;

class Upcoming {
private:
  gloox::JID m_user;
  time_t m_next;

public:
  Upcoming(const gloox::JID &user, time_t next)  : m_user(user), m_next(next) {}
  const gloox::JID &User(void) const { return m_user; }
  const time_t Next(void) const { return m_next; }
};

typedef std::vector<Upcoming> UpcomingList;

class UpcomingCompare {
public :
  bool operator()( const Upcoming &a, const Upcoming &b ) {
    return a.Next() > b.Next();
  }
};

typedef std::map<insensitiveString, CommandHandler> CommandHandlerMap;
typedef std::vector<std::string> AffirmationList;
typedef std::priority_queue<Upcoming, UpcomingList, UpcomingCompare> UpcomingQueue;
typedef std::set<gloox::JID> SendList;

class JabberBot : public gloox::MessageHandler, gloox::LogHandler, gloox::RosterListener {
private:
  // Required to be an instance of the MessageHandler class
  virtual void handleMessage(const gloox::Message &stanza, gloox::MessageSession *session=0);
  // Required to be an instance of the LogHandler class
  virtual void handleLog(gloox::LogLevel level, gloox::LogArea area, const std::string &message);
  // Required to be an instance of the RosterListener class
  virtual void handleItemAdded(const gloox::JID &jid) {}
  virtual void handleItemSubscribed(const gloox::JID &jid) {}
  virtual void handleItemRemoved(const gloox::JID &jid) {}
  virtual void handleItemUpdated(const gloox::JID &jid) {}
  virtual void handleItemUnsubscribed(const gloox::JID &jid) {}
  virtual void handleRoster(const gloox::Roster &roster) {}
  virtual void handleRosterPresence(const gloox::RosterItem &item, const std::string &resource, gloox::Presence::PresenceType presence, const std::string &msg) {}
  virtual void handleSelfPresence(const gloox::RosterItem &item, const std::string &resource, gloox::Presence::PresenceType presence, const std::string &msg) {}
  virtual bool handleSubscriptionRequest(const gloox::JID &jid, const std::string &msg);
  virtual bool handleUnsubscriptionRequest(const gloox::JID &jid, const std::string &msg);
  virtual void handleNonrosterPresence(const gloox::Presence &presence) {}
  virtual void handleRosterError(const gloox::IQ &iq) {}

  void DefaultCommand(const gloox::Message &stanza);
  void Subscribe(const gloox::Message &stanza);
  void Unsubscribe(const gloox::Message &stanza);
  void Affirmation(const gloox::Message &stanza);
  void Status(const gloox::Message &stanza);
  void ProcessSendList(void);
  static void *SendThreadFunction(void *);

  gloox::Client *m_client;
  static CommandHandlerMap m_handlers;
  static AffirmationList m_affirmations;
  Db m_subscriptionDb;
  UpcomingQueue m_upcomingQueue;
  SendList m_sendList;
  time_t m_eod;
  bool m_continueRunning;
  boost::thread *m_sendThread = nullptr;

public:
  JabberBot(const std::string &jid_string, const std::string &password, const std::string &affirmation_path, const std::string &db_path);
  ~JabberBot(void);
  void ShutDown(void);
  void RunSession(void);
};


#endif // _JABBERSESSION_HPP_INCLUDED_
