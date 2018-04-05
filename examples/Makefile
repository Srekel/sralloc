# INCLUDES = -I..

CFLAGS = -Werror -Wall -Wextra -Wpedantic -std=c99 $(EXTRA_DEFINES)
CPPFLAGS = -Werror -Wall -Wextra -Wpedantic -std=c++0x $(EXTRA_DEFINES)

build_c:
	$(CC) $(CFLAGS) unittest/unittest.c -DNO_IGDEBUG
build_cpp:
	$(CXX) $(CPPFLAGS) unittest/unittest.c external/ig_debugheap/DebugHeap.c

all: build_c
