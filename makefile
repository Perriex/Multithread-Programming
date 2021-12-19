all: main main2

main2 : main2.cpp
	g++ -std=c++11 main2.cpp -o main2 -lpthread

main : main.cpp
	g++ -std=c++11 main.cpp -o main 

clean:
	rm main main2