NAME := ircserv
CXX := c++
CXXFLAGS := -Wall -Wextra -Werror -std=c++98 -pedantic
LDFLAGS := 
SRC := main.cpp Server.cpp Client.cpp Channel.cpp Commands.cpp
OBJ := $(SRC:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
