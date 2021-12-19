all: main main2

% : %.c 
	gcc -Wall -g -ggdb -pthread -lpthread -std=c++11 -o $@ $^

clean:
	rm main main2