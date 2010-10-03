#include <cstdlib>
#include <fstream>

#include <signal.h>
#include <unistd.h>

#include <jabber-bot/jabber-bot.hpp>

#include "jabberbotsession.hpp"

JabberBotSession *s;

void signal_handler(int signal) {
  s->ShutDown();
}

void usage(char *arg0) {
  std::cerr << "Usage:  " << arg0 << " -h HOSTNAME [-p PORT] [-y] [-d] -u USERNAME -w PASSWORD -r RESOURCE -e EMAIL -a AFFIRMATION_PATH -b DATABASE_PATH"<< std::endl;
}

int main(int argc, char **argv) {
  const std::string *host = NULL;      // The h option
  unsigned short port = 5222;          // The p option
  bool start_encrypted = false;        // The y option
  bool debug = false;                  // The d option
  const std::string *userid = NULL;    // The u option
  const std::string *password = NULL;  // The w option
  const std::string *resource = NULL;  // The r option
  const std::string *email = NULL;     // The e option
  const std::string *apath = NULL;     // The a option
  const std::string *dbpath = NULL;    // The b option

  int opt;
  while (-1 != (opt = getopt(argc, argv, "h:p:ydu:w:r:e:a:b:"))) {
    switch (opt) {
    case 'h':
      delete host;
      host = new std::string(optarg);
      break;

    case 'p':
      port = atoi(optarg);
      break;

    case 'y':
      start_encrypted = true;
      break;

    case 'd':
      debug = true;
      break;

    case 'u':
      delete userid;
      userid = new std::string(optarg);
      break;

    case 'w':
      delete password;
      password = new std::string(optarg);
      break;

    case 'r':
      delete resource;
      resource = new std::string(optarg);
      break;

    case 'e':
      delete email;
      email = new std::string(optarg);
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

  if ((NULL == host) ||
      (NULL == userid) ||
      (NULL == password) ||
      (NULL == resource) ||
      (NULL == email) ||
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

      s = new JabberBotSession(*host, port, start_encrypted, *userid, *password, *resource, *email, *apath, *dbpath);
      s->RunSession();
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

    s = new JabberBotSession(*host, port, start_encrypted, *userid, *password, *resource, *email, *apath, *dbpath);
    s->RunSession();
  }
  return EXIT_SUCCESS;
}
