/*
 * Copyright 2010 Jonathan R. Guthrie
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
// #include <iostream>

#include "jabberbot.hpp"

#include <fstream>
#include <cstdlib>
#include <sstream>

#include <syslog.h>

void JabberBot::Subscribe(const gloox::Message &stanza) {
  char keyValue[stanza.from().bare().size()+1];
  stanza.from().bare().copy(keyValue, std::string::npos);
  keyValue[stanza.from().bare().size()] = '\0';
  time_t last = 0;

  Dbt key(keyValue, stanza.from().bare().size()+1);
  Dbt data(&last, sizeof(time_t));
  m_subscriptionDb.put(NULL, &key, &data, 0);
  m_subscriptionDb.sync(0);

  time_t now = time(NULL);
  time_t t1 = random() % ((m_eod - now)/2);
  time_t t2 = random() % ((m_eod - now)/2);

  Upcoming newEntry(stanza.from().bare(), now + t1 + t2);
  m_upcomingQueue.push(newEntry);

  gloox::Message message(stanza.subtype(), stanza.from(), "You have been subscribed");
  m_client->send(message);
}

void JabberBot::Unsubscribe(const gloox::Message &stanza) {
  char keyValue[stanza.from().bare().size()+1];
  stanza.from().bare().copy(keyValue, std::string::npos);
  keyValue[stanza.from().bare().size()] = '\0';

  Dbt key(keyValue, stanza.from().bare().size()+1);
  m_subscriptionDb.del(NULL, &key, 0);
  m_subscriptionDb.sync(0);

  gloox::Message message(stanza.subtype(), stanza.from(), "You have been unsubscribed");
  m_client->send(message);
}

void JabberBot::Affirmation(const gloox::Message &stanza) {
  AffirmationList::size_type which = random() % m_affirmations.size();
  gloox::Message message(stanza.subtype(), stanza.from(), m_affirmations[which]);
  m_client->send(message);
}

void JabberBot::Status(const gloox::Message &stanza) {
  char keyValue[stanza.from().bare().size()+1];
  stanza.from().bare().copy(keyValue, std::string::npos);
  keyValue[stanza.from().bare().size()] = '\0';
  time_t last = 0;

  Dbt key(keyValue, stanza.from().bare().size()+1);
  Dbt data;
  data.set_data(&last);
  data.set_ulen(sizeof(time_t));
  int result = m_subscriptionDb.get(NULL, &key, &data, 0);

  if (0 != result) {
    gloox::Message message(stanza.subtype(), stanza.from(), "You are not subscribed");
    m_client->send(message);
  }
  else {
    gloox::Message message(stanza.subtype(), stanza.from(), "You are subscribed");
    m_client->send(message);
  }
}

void JabberBot::DefaultCommand(const gloox::Message &stanza) {
  // I figure the best thing to do is to send the help message if it doesn't recognize the command sent
  gloox::Message message(stanza.subtype(), stanza.from(), "I know how to do these things:\nsubscribe - send affirmation to you once per day\nunsubscribe - stop sending you daily affirmations\nstatus - tell you whether or not you're subscribed\naffirmation - send a random affirmation right now\nhelp - send this message\n\nCommands are not case sensitive");
  m_client->send(message);
}

CommandHandlerMap JabberBot::m_handlers;
AffirmationList JabberBot::m_affirmations;

JabberBot::JabberBot(const std::string &jid_string, const std::string &password, const std::string &affirmation_path, const std::string &db_path) : m_subscriptionDb(NULL, 0) {
  openlog("affirmations-bot", LOG_PID|LOG_CONS, LOG_DAEMON);

  Dbc *cursor;
  m_subscriptionDb.open(NULL, db_path.c_str(), NULL, DB_BTREE, DB_CREATE, 0);

  m_subscriptionDb.cursor(NULL, &cursor, 0);
  if (NULL != cursor) {
    Dbt key, data;
    time_t now = time(NULL);
    time_t bod = now - (now % 86400);
    m_eod = bod + 86400;

    while (0 == cursor->get(&key, &data, DB_NEXT)) {
      std::string user((char *)key.get_data());
      time_t last = *((time_t*) data.get_data());
      if (last < bod) {
	// He hasn't gotten an affirmation today
	time_t t1 = random() % ((m_eod - now)/2);
	time_t t2 = random() % ((m_eod - now)/2);
	
	Upcoming newEntry(user, now + t1 + t2);
	m_upcomingQueue.push(newEntry);
      }
      std::ostringstream message;
      message << "User " << user << " last got an affirmation at " << last;
      syslog(LOG_NOTICE, message.str().c_str());
    }
    cursor->close();
  }

  if (m_handlers.begin() == m_handlers.end()) {
    CommandHandler handler;
    handler.method = &JabberBot::Subscribe;
    m_handlers.insert(CommandHandlerMap::value_type("subscribe", handler));
    handler.method = &JabberBot::Unsubscribe;
    m_handlers.insert(CommandHandlerMap::value_type("unsubscribe", handler));
    handler.method = &JabberBot::Affirmation;
    m_handlers.insert(CommandHandlerMap::value_type("affirmation", handler));
    handler.method = &JabberBot::Status;
    m_handlers.insert(CommandHandlerMap::value_type("status", handler));
  }
  if (m_affirmations.empty()) {
    srand(time(NULL));
    std::string line;

    std::ifstream a(affirmation_path.c_str());
    while (getline(a, line)) { 
      std::ostringstream message;
      message << "The line is \"" << line << "\"" << std::endl;
      syslog(LOG_DEBUG, message.str().c_str());
      m_affirmations.push_back(line);
    }
  }

  // std::cout << "Creating the JID from the string \"" << jid_string << "\"" << std::endl;
  gloox::JID jid(jid_string);
  // std::cout << "Creating a client from the JID and the password \"" << password << "\"" << std::endl;
  m_client = new gloox::Client(jid, password);
  m_client->setServer("talk.google.com");
  // std::cout << "Registering the handlers" << std::endl;
  m_client->registerMessageHandler(this);
  m_client->logInstance().registerLogHandler(gloox::LogLevelDebug, gloox::LogAreaAll, this);
  // std::cout << "Running connect" << std::endl;
  m_client->rosterManager()->registerRosterListener(this, true);

  m_continueRunning = true;
  m_sendThread = new boost::thread(SendThreadFunction, this);
  // Start the thread here
  m_client->connect(true);
  // std::cout << "returned from connect" << std::endl;
  // std::cout << "The connection state is " << ((int) m_client->state()) << std::endl;
  m_continueRunning = false;
}

JabberBot::~JabberBot(void) {
  m_continueRunning = false;
  m_sendThread->join();
  std::ostringstream message;
  message << "Shutting down cleanly" << std::endl;
  syslog(LOG_NOTICE, message.str().c_str());
  m_subscriptionDb.close(0);
}


void JabberBot::ProcessSendList(void) {
  // syslog(LOG_NOTICE, "In Process Send List");
  bool updated = false;
  for (SendList::iterator i=m_sendList.begin(); i != m_sendList.end(); ++i) {
    gloox::RosterItem *ri = m_client->rosterManager()->getRosterItem(*i);
    if (nullptr != ri) {
      if (ri->online()) {
	AffirmationList::size_type which = random() % m_affirmations.size();
	updated = true;

	gloox::Message message(gloox::Message::MessageType::Chat, *i, m_affirmations[which]);
	m_client->send(message);
	// syslog(LOG_NOTICE, "Updating the database");
	time_t now = time(NULL);
	char keyValue[i->bare().size()+1];
	i->bare().copy(keyValue, std::string::npos);
	keyValue[i->bare().size()] = '\0';
	Dbt key(keyValue, i->bare().size()+1);
	Dbt data(&now, sizeof(time_t));
	m_subscriptionDb.put(NULL, &key, &data, 0);

	// syslog(LOG_NOTICE, "Removing the user from the send list");
	m_sendList.erase(i);
      }
    }
  }
  if (updated) {
    // syslog(LOG_NOTICE, "Updating the database");
    m_subscriptionDb.sync(0);
  }
}

void JabberBot::ShutDown(void) {
  m_continueRunning = false;
}

void JabberBot::RunSession(void) {
  // syslog(LOG_INFO, "In RunSession");
  while(m_continueRunning) {
    // syslog(LOG_INFO, "In continueRunning loop");
    time_t now = time(NULL);
    while (!m_upcomingQueue.empty() && (now > m_upcomingQueue.top().Next())) {
      // std::ostringstream message;
      // message << "Appending " << m_upcomingQueue.top().User().bare() << " to the list of people who get messages now" << std::endl;
      // syslog(LOG_NOTICE, message.str().c_str());
      m_sendList.insert(m_upcomingQueue.top().User());
      m_upcomingQueue.pop();
    }
    // syslog(LOG_INFO, "Calling ProcessSendList");
    ProcessSendList();

    if (m_eod < now) {
      time_t bod = m_eod;
      m_eod += 86400;

      m_sendList.clear();
      while (!m_upcomingQueue.empty()) {
	m_upcomingQueue.pop();
      }

      // syslog(LOG_INFO, "Updating the database");
      Dbc *cursor;
      m_subscriptionDb.cursor(NULL, &cursor, 0);
      if (NULL != cursor) {
	Dbt key, data;
	m_eod = bod + 86400;

	while (0 == cursor->get(&key, &data, DB_NEXT)) {
	  std::string user((char *)key.get_data());
	  time_t t1 = random() % 43200;
	  time_t t2 = random() % 43200;

	  Upcoming newEntry(user, bod + t1 + t2);
	  m_upcomingQueue.push(newEntry);
	  // std::ostringstream message;
	  // message << "It is now " << now << " and user " << user << " will next get a message at " << bod + t1 + t2;
	  // syslog(LOG_NOTICE, message.str().c_str());
	}
	cursor->close();
      }
    }
    // syslog(LOG_INFO, "Sleeping");
    sleep(1);
  }
  // syslog(LOG_INFO, "Returning from RunSession");
}


void *JabberBot::SendThreadFunction(void *instance) {
  // syslog(LOG_INFO, "In the SendThreadFunction");
  JabberBot *self = (JabberBot *)instance;
  // syslog(LOG_INFO, "Running RunSession");
  self->RunSession();
  // syslog(LOG_INFO, "Exiting SendThreadFunction");
  return nullptr;
}


void JabberBot::handleMessage(const gloox::Message &stanza, gloox::MessageSession *session) {
  // std::cout << "Message type: " << ((int) stanza.subtype()) << std::endl;
  if (0 < stanza.body().size()) {
    insensitiveString body(stanza.body().c_str());
    CommandHandlerMap::iterator h = m_handlers.find(body);

    if (h != m_handlers.end()) {
      (this->*h->second.method)(stanza);
    }
    else {
      DefaultCommand(stanza);
    }
  }
}


void JabberBot::handleLog(gloox::LogLevel level, gloox::LogArea area, const std::string &message) {
  // std::cout << "Log:  " << ((int)level) << ", " << ((int) area) << " \"" << message << "\"" << std::endl;
  // syslog(LOG_INFO, message.c_str());
}


bool JabberBot::handleSubscriptionRequest(const gloox::JID &jid, const std::string &msg) {
  // std::cout << jid.bare() < " is attempting to subscribe with messge \"" << msg << "\"" << std::endl;
  return true;
}


bool JabberBot::handleUnsubscriptionRequest(const gloox::JID &jid, const std::string &msg) {
  // std::cout << jid.bare() < " is attempting to unsubscribe with messge \"" << msg << "\"" << std::endl;
  return true;
}
