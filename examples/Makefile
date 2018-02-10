# INCLUDES = -I..

CFLAGS = -Werror -Wall -Wextra -Wpedantic -std=c99
CPPFLAGS = -Werror -Wall -Wextra -Wpedantic -std=c++0x

build_c:
	$(CC) $(CFLAGS) unittest/unittest.c
build_cpp:
	$(CXX) $(CPPFLAGS) unittest/unittest.c

all: build_c
