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
#include "jabberbotsession.hpp"

// #include <iostream>
#include <fstream>
#include <cstdlib>
#include <sstream>

static void split_identifier(const std::string *from, std::string &account_id, std::string &resource) {
  size_t sep = from->find_first_of('/');
  if (sep != std::string::npos) {
    account_id = from->substr(0, sep);
    resource = from->substr(sep+1);
  }
  else {
    account_id = *from;
    resource = "";
  }
}


void JabberBotSession::SetAvailable(const std::string *from) {
  // Split the string into username/resource
  std::string account_id, resource;
  split_identifier(from, account_id, resource);
  UserStatusType::iterator i =  m_userStatuses.find(account_id);
  if (i == m_userStatuses.end()) {
    // Not found, need to add the user
    ResourceSet value;
    if ("" != resource) {
      value.insert(resource);
    }
    m_userStatuses.insert(UserStatusType::value_type(account_id, value));
  }
  else {
    if ("" != resource) {
      i->second.insert(resource);
    }
  }
  // std::cout << "Account " << account_id << " has become available at resource " << resource << std::endl;
}


void JabberBotSession::SetUnavailable(const std::string *from) {
  // Split the string into username/resource
  std::string account_id, resource;
  split_identifier(from, account_id, resource);
  // std::cout << "Account " << account_id << " has become unavailable at resource " << resource << std::endl;
  UserStatusType::iterator i =  m_userStatuses.find(account_id);
  if (i != m_userStatuses.end()) {
    if ("" == resource) {
      // If no resource is given and there are no resource in the user's resource set, then remove the user
      // If no resource is given and there are resources in the user's resource set, do nothing
      if (i->second.empty()) {
	m_userStatuses.erase(i);
      }
    }
    else {
      // If a resource is given then remove it from the resource set, if the resource set is now empty, remove the account
      i->second.erase(resource);
      if (i->second.empty()) {
	m_userStatuses.erase(i);
      }
    }
  }
  // No else because if I don't have a listing for this user then I can't remove it
}


int JabberBotSession::HandlePresenceRequest(void *context, const PresenceStanza &request, class JabberSession *session) {
  JabberBotSession *bot = static_cast<JabberBotSession *>(context);

  switch(request.Type()) {
    case PresenceStanza::Subscribe:
      bot->SetPresence(PresenceStanza::Subscribed, request.From());
      bot->SetPresence(PresenceStanza::Subscribe, request.From());
      break;

    case PresenceStanza::Unsubscribe:
      bot->SetPresence(PresenceStanza::Unsubscribed, request.From());
      bot->SetPresence(PresenceStanza::Unsubscribe, request.From());
      break;

    case PresenceStanza::Available:
      bot->SetAvailable(request.From());
      break;

    case PresenceStanza::Unavailable:
      bot->SetUnavailable(request.From());
      break;

    default:
      break;
  }

  return 0;
}


int JabberBotSession::HandleMessageRequest(void *context, const MessageStanza &request, class JabberSession *session) {
  JabberBotSession *bot = static_cast<JabberBotSession *>(context);

  // Sometimes a server will send us a message.  I know it's a server if it doesn't have an "@" in the
  // name.  If I get a message like that, I don't want to use the normal logic because my responses
  // would just confuse it.
  if ((NULL != request.From()) && (std::string::npos != request.From()->find_first_of('@'))) {
    if (NULL != request.Body()) {
      CommandHandlerMap::iterator h = bot->m_handlers.find(*request.Body());
      if (h != bot->m_handlers.end()) {
	(bot->*h->second.method)(request.From(), request.Type(), request.Id());
      }
      else {
	bot->DefaultCommand(request.From(), request.Type(), request.Id());
      }
    }
  }
  return 0;
}

void JabberBotSession::Subscribe(const std::string *from, MessageStanza::MessageTypes type, const std::string *id) {
  std::string account_id, resource;
  split_identifier(from, account_id, resource);
  char keyValue[account_id.size()+1];
  account_id.copy(keyValue, std::string::npos);
  keyValue[account_id.size()] = '\0';
  time_t last = 0;

  Dbt key(keyValue, account_id.size()+1);
  Dbt data(&last, sizeof(time_t));
  m_subscriptionDb.put(NULL, &key, &data, 0);
  m_subscriptionDb.sync(0);

  time_t now = time(NULL);
  time_t t1 = random() % ((m_eod - now)/2);
  time_t t2 = random() % ((m_eod - now)/2);

  Upcoming newEntry(account_id, now + t1 + t2);
  m_upcomingQueue.push(newEntry);

  MessageStanza message;
  message.Type(type);
  message.To(from);
  message.Thread(id);
  message.Body("You have been subscribed");
  m_session->SendMessage(message, false);
}

void JabberBotSession::Unsubscribe(const std::string *from, MessageStanza::MessageTypes type, const std::string *id) {
  std::string account_id, resource;
  split_identifier(from, account_id, resource);
  char keyValue[account_id.size()+1];
  account_id.copy(keyValue, std::string::npos);
  keyValue[account_id.size()] = '\0';

  Dbt key(keyValue, account_id.size()+1);
  m_subscriptionDb.del(NULL, &key, 0);
  m_subscriptionDb.sync(0);

  MessageStanza message;
  message.Type(type);
  message.To(from);
  message.Thread(id);
  message.Body("You have been unsubscribed");
  m_session->SendMessage(message, false);
}

void JabberBotSession::Affirmation(const std::string *from, MessageStanza::MessageTypes type, const std::string *id) {
  AffirmationList::size_type which = random() % m_affirmations.size();
  MessageStanza message;
  message.Type(type);
  message.To(from);
  message.Thread(id);
  message.Body(m_affirmations[which]);
  m_session->SendMessage(message, false);
}

void JabberBotSession::Status(const std::string *from, MessageStanza::MessageTypes type, const std::string *id) {
  std::string account_id, resource;
  split_identifier(from, account_id, resource);
  char keyValue[account_id.size()+1];
  account_id.copy(keyValue, std::string::npos);
  keyValue[account_id.size()] = '\0';
  time_t last = 0;

  Dbt key(keyValue, account_id.size()+1);
  Dbt data;
  data.set_data(&last);
  data.set_ulen(sizeof(time_t));
  int result = m_subscriptionDb.get(NULL, &key, &data, 0);

  MessageStanza message;
  message.Type(type);
  message.To(from);
  message.Thread(id);
  if (0 != result) {
    message.Body("You are not subscribed");
  }
  else {
    message.Body("You are subscribed");
  }
  m_session->SendMessage(message, false);
}

void JabberBotSession::DefaultCommand(const std::string *from, MessageStanza::MessageTypes type, const std::string *id) {
  // I figure the best thing to do is to send the help message if it doesn't recognize the command sent
  MessageStanza message;
  message.Type(type);
  message.To(from);
  message.Thread(id);
  message.Body("I know how to do these things:\nsubscribe - send affirmation to you once per day\nunsubscribe - stop sending you daily affirmations\nstatus - tell you whether or not you're subscribed\naffirmation - send a random affirmation right now\nhelp - send this message\n\nCommands are case sensitive");
  m_session->SendMessage(message, false);
}

CommandHandlerMap JabberBotSession::m_handlers;
AffirmationList JabberBotSession::m_affirmations;

JabberBotSession::JabberBotSession(const std::string &host, unsigned short port, bool isSecure, const std::string &userid, const std::string &password, const std::string &resource, const std::string &email, const std::string &affirmation_path, const std::string &db_path) : m_subscriptionDb(NULL, 0) {
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
      // std::cout << "User " << user << " last got an affirmation at " << last << std::endl;
    }
    cursor->close();
  }

  if (m_handlers.begin() == m_handlers.end()) {
    CommandHandler handler;
    handler.method = &JabberBotSession::Subscribe;
    m_handlers.insert(CommandHandlerMap::value_type("subscribe", handler));
    handler.method = &JabberBotSession::Unsubscribe;
    m_handlers.insert(CommandHandlerMap::value_type("unsubscribe", handler));
    handler.method = &JabberBotSession::Affirmation;
    m_handlers.insert(CommandHandlerMap::value_type("affirmation", handler));
    handler.method = &JabberBotSession::Status;
    m_handlers.insert(CommandHandlerMap::value_type("status", handler));
  }
  if (m_affirmations.empty()) {
    srand(time(NULL));
    std::string line;

    std::ifstream a(affirmation_path.c_str());
    while (getline(a, line)) {
      // std::cout << "The line is \"" << line << "\"" << std::endl;
      m_affirmations.push_back(line);
    }
  }

  m_session = new JabberSession(host, port, isSecure);

  // SYZYGY
  m_session->Register(this, HandlePresenceRequest);
  m_session->Register(this, HandleMessageRequest);
  JabberIqAuth login_stanza(userid, password, resource);
  const Stanza *response = m_session->SendMessage(login_stanza, true);
  // std::cout << "The first login response is " << *(response->render(NULL)) << std::endl;
  if (200 != response->Error()) {
    // std::cout << "The first login response is " << response->ErrorMessage() << std::endl;
    JabberIqRegister register_stanza(userid, password, email);
    response = m_session->SendMessage(register_stanza, true);
    // std::cout << "The register response is " << *(response->render(NULL)) << std::endl;
    if (200 != response->Error()) {
      // std::cout << "The register response is " << response->ErrorMessage() << std::endl;
    }
    else {
      JabberIqAuth login2_stanza(userid, password, resource);
      response = m_session->SendMessage(login2_stanza, true);
      // std::cout << "The second login response is " << *(response->render(NULL)) << std::endl;
      if (200 != response->Error()) {
	// std::cout << "The second login failed with a message of " << response->ErrorMessage() << std::endl;
      }
    }
  }
  delete response;

  m_id = 0;
  m_continueRunning = true;
}

JabberBotSession::~JabberBotSession(void) {
  // std::cout << "Shutting down cleanly" << std::endl;
  m_subscriptionDb.close(0);
}


void JabberBotSession::ProcessSendList(void) {
  bool updated = false;
  for (SendList::iterator i=m_sendList.begin(); i != m_sendList.end(); ++i) {
    UserStatusType::iterator j =  m_userStatuses.find(*i);
    if (j != m_userStatuses.end()) {
      AffirmationList::size_type which = random() % m_affirmations.size();
      updated = true;
      if (!j->second.empty()) {
	for (ResourceSet::iterator k=j->second.begin(); k != j->second.end(); ++k) {
	  std::ostringstream to;
	  to << *i << "/" << *k;
	  std::ostringstream id;
	  id << m_id;
	  m_id++;

	  MessageStanza message;
	  message.Type(MessageStanza::Normal);
	  message.To(to.str());
	  message.Body(m_affirmations[which]);
	  m_session->SendMessage(message, false);
	}
      }
      else {
	std::ostringstream ss;
	ss << m_id;
	m_id++;

	MessageStanza message;
	message.Type(MessageStanza::Normal);
	message.To(*i);
	message.Body(m_affirmations[which]);
	m_session->SendMessage(message, false);
      }
      time_t now = time(NULL);
      char keyValue[i->size()+1];
      i->copy(keyValue, std::string::npos);
      keyValue[i->size()] = '\0';
      Dbt key(keyValue, i->size()+1);
      Dbt data(&now, sizeof(time_t));
      m_subscriptionDb.put(NULL, &key, &data, 0);

      m_sendList.erase(i);
    }
  }
  if (updated) {
    m_subscriptionDb.sync(0);
  }
}

void JabberBotSession::RunSession(void) {
  SetPresence(PresenceStanza::Available);

  JabberIqRoster rosterQuery;
  m_session->SendMessage(rosterQuery, false);

  while(m_continueRunning) {
    time_t now = time(NULL);
    while (!m_upcomingQueue.empty() && (now > m_upcomingQueue.top().Next())) {
      // std::cout << "Appending " << m_upcomingQueue.top().User() << " to the list of people who get messages now" << std::endl;
      m_sendList.insert(m_upcomingQueue.top().User());
      m_upcomingQueue.pop();
    }
    ProcessSendList();

    if (m_eod < now) {
      time_t bod = m_eod;
      m_eod += 86400;

      m_sendList.clear();
      while (!m_upcomingQueue.empty()) {
	m_upcomingQueue.pop();
      }

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
	}
	cursor->close();
      }
    }
    sleep(1);
  }
}


void JabberBotSession::SetPresence(PresenceStanza::PresenceTypes type, const std::string *to, const std::string *status) {
  PresenceStanza presenceNotification;
  presenceNotification.Type(type);
  presenceNotification.To(to);
  presenceNotification.Status(status);
  m_session->SendMessage(presenceNotification, false);
}
