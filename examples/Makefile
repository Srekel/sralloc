# INCLUDES = -I..

COMPILEFLAGS = -Werror -Wall -Wextra -Wpedantic

clang_c:
	$(CC) $(COMPILEFLAGS) unittest/unittest.c
clang_cpp:
	$(CXX) $(COMPILEFLAGS) unittest/unittest.c

all: clang_c clang_cpp