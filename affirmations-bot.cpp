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
#include <cstdlib>
#include <fstream>

#include <signal.h>
#include <unistd.h>

#include "jabberbot.hpp"

JabberBot *s;

void signal_handler(int signal) {
  s->ShutDown();
}

void usage(char *arg0) {
  std::cerr << "Usage:  " << arg0 << " -j JABBERID -w PASSWORD -a AFFIRMATION_PATH -b DATABASE_PATH"<< std::endl;
}

int main(int argc, char **argv) {
  bool debug = false;                   // The d option
  const std::string *jid_string = NULL; // The j option
  const std::string *password = NULL;   // The w option
  const std::string *apath = NULL;      // The a option
  const std::string *dbpath = NULL;     // The b option

  int opt;
  while (-1 != (opt = getopt(argc, argv, "dj:w:a:b:"))) {
    switch (opt) {
    case 'd':
      debug = true;
      break;

    case 'j':
      delete jid_string;
      jid_string = new std::string(optarg);
      break;

    case 'w':
      delete password;
      password = new std::string(optarg);
      break;

    case 'a':
      delete apath;
      apath = new std::string(optarg);
      break;

    case 'b':
      delete dbpath;
      dbpath = new std::string(optarg);
      break;

    default:
      usage(argv[0]);
      exit(EXIT_FAILURE);
    }
  }

  if ((NULL == jid_string) ||
      (NULL == password) ||
      (NULL == apath) ||
      (NULL == dbpath)) {
    usage(argv[0]);
    exit(EXIT_FAILURE);
  }

  if (!debug) {
    close(0);
    close(1);
    close(2);

    pid_t process_id = fork();

    switch(process_id) {
    case -1:
      break;

    case 0:
      setuid(1000);

      signal(SIGINT, signal_handler);
      signal(SIGTERM, signal_handler);

      s = new JabberBot(*jid_string, *password, *apath, *dbpath);
      delete s;

      unlink("var/run/affirmations-bot/affirmations-bot.pid");
      break;

    default:
      {
	setuid(1000);

	std::ofstream outfile("/var/run/affirmations-bot/affirmations-bot.pid", std::ios_base::out);
	outfile << process_id << std::endl;
      }
      break;
    }

  }
  else {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    s = new JabberBot(*jid_string, *password, *apath, *dbpath);
    delete s;
  }
  return EXIT_SUCCESS;
}
