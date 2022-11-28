BOOST_PATH=/usr/local/include/
CPP=clang++
CPPFLAGS=-I$(BOOST_PATH) -Wall -ggdb
#CPPFLAGS=-I$(BOOST_PATH) -Wall -Os
LDFLAGS=-L/usr/local/lib/ -lboost_program_options
LDFLAGS_STATIC=-L/usr/local/lib/


all:
	$(CPP) $(CPPFLAGS) -c ifaddr.cpp 
	$(CPP) $(CPPFLAGS) $(LDFLAGS) -o ip ifaddr.o
	$(CPP) $(CPPFLAGS) $(LDFLAGS_STATIC) -o ip_s ifaddr.o /usr/local/lib/libboost_program_options.a

install:
	install -s ip_s /usr/local/bin/ip

clean:
	rm ip ip_s *.o
