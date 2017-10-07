# Copyright 2010 Jonathan R. Guthrie

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

# http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

CC=g++
CXXFLAGS=-Wall -g
LDFLAGS=-ldb_cxx -L/usr/local/lib -lgloox -lboost_thread -lboost_system

affirmations-bot: affirmations-bot.o jabberbot.o

%.d: %.cpp
	@set -e; rm -f $@; \
	$(CC) -MM $(CPPFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : Makefile ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

SOURCES=affirmations-bot.cpp jabberbot.cpp

include $(SOURCES:.cpp=.d)

clean:
	rm -f *.o *.d affirmations-bot

