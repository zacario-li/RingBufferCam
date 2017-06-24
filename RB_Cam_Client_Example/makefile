var = release
src = *.cpp *.hpp
CC = g++
INC = 
ifeq ($(var),debug)
DEBUG = -g
else
DEBUG = 
endif

LDFLAGS = -lpthread -lrt -L./ -lRB
CPPFLAG = -std=c++11
rb-cam-client:$(src)
	$(CC) -o $@ $(src) $(DEBUG) $(CPPFLAG) $(LDFLAGS) $(shell pkg-config --cflags --libs opencv boost)
	echo "done"
	
.PHONY:clean test
clean:
	rm rb-cam-client
test:
	echo "hello"
