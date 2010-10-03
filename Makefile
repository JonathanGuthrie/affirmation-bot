CC=g++
CXXFLAGS=-Wall -g `pkg-config libxml++-2.6 --cflags`
LDFLAGS=-ldb_cxx -L/usr/local/lib -ljabber-bot

affirmations-bot: affirmations-bot.o jabberbotsession.o

%.d: %.cpp
	@set -e; rm -f $@; \
	$(CC) -MM $(CPPFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : Makefile ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

SOURCES=affirmations-bot.cpp jabberbotsession.cpp

include $(SOURCES:.cpp=.d)

clean:
	rm -f *.o *.d affirmations-bot

