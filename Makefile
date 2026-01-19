all: server client

server : server.cpp
	g++ -o s server.cpp
client : client.cpp
	g++ -o c client.cpp

mdns : mdns.cpp
	g++ -o m mdns.cpp
c_mdns: tests/c_mdns.cpp
	g++ -o d tests/c_mdns.cpp

run all:
	./m & 
	sleep 1.0
	./d
clean:
	rm -f m s c d

then:
	make run
