NAME = codexion
CC = cc 
FLAGS = -Wall -Wextra -Werror -pthread

SRC = coders/main.c \
	coders/init.c \
	coders/compiling.c \
	coders/coders.c \
	coders/dongles.c \
	coders/resources.c \
	coders/sync.c \
	coders/simulation.c

OBJ = $(SRC:.c=.o)

all: $(NAME)

$(NAME): $(OBJ)
	$(CC) $(FLAGS) $(OBJ) -o $(NAME)

%.o:%.c coders/codexion.h
	$(CC) $(FLAGS) -c $< -o $@

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME)

re:
	$(MAKE) fclean all

.PHONY: clean fclean re all