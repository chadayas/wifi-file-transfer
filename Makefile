all: server client

server : src/server.cpp
	g++ src/server.cpp src/mdns.cpp -std=c++17 -pthread -o build/s 
client : tests/client.cpp
	g++ -std=c++17 -o build/c client.cpp

mdns : src/mdns.cpp
	g++ -std=c++17 -o build/m src/mdns.cpp
c_mdns: tests/c_mdns.cpp
	g++ -std=c++17 -o build/d tests/c_mdns.cpp

run all:
	build/m & 
	sleep 1.0
	build/d
clean:
	rm -f m s c d

then:
	make run
