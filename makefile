FILES = LFS.o
NAME = project5
CC = g++
CFLAG = -Wall -Wextra -DDEBUG -g -pedantic -std=c++14
OPTION = $(CFLAG) -c

all: $(NAME)


$(NAME): $(FILES)
	$(CC) $(CFLAG) $(FILES) -o $(NAME)


LFS.o: LFS.cpp
	$(CC) $(OPTION) LFS.cpp -o LFS.o

# Main.o: Main.cpp
# 	$(CC) $(OPTION) Main.cpp -o Main.o


clean:
	rm *.o $(NAME)

run:
	./$(NAME)
