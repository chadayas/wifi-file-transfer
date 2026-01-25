all: server client

server : src/server.cpp
	g++ -std=c++17 -o build/s src/server.cpp
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
