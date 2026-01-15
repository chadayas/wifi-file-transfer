all: server client

server : server.cpp
	g++ -o s server.cpp
client : client.cpp
	g++ -o c client.cpp

mdns : mdns.cpp
	g++ -o m mdns.cpp
run all:
	./s &
	sleep 0.5
	./c
clean:
	rm -f m s c

then:
	make run
