var = release
src = *.cpp *.hpp
CC = g++
INC = -I/usr/local/include # -I/home/lzj/tools/boost164/include
ifeq ($(var),debug)
DEBUG = -g
else
DEBUG =
endif
BUILD_LIB = -fPIC -shared -DSHD_LIBS

LDFLAGS = -lpthread -lrt # -L/home/lzj/tools/boost164/lib
CPPFLAG = -std=c++11

all::RB-Cam libRB.so

RB-Cam:$(src)
	$(CC) -o $@ $(src) $(DEBUG) $(CPPFLAG) $(LDFLAGS) $(shell pkg-config --cflags --libs opencv boost)
	echo "done"

libRB.so:$(src)
	$(CC) $(src) $(BUILD_LIB) -o $@ $(DEBUG) $(CPPFLAG) $(LDFLAGS) $(shell pkg-config --cflags --libs opencv boost)
	echo "make lib done"

.PHONY:clean test
clean:
	rm RB-Cam libRB.so
test:
	echo "hello"
