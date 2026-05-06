all: server client
server: server.cpp crypto.h protocol.h crypto.cpp protocol.cpp
	g++ server.cpp protocol.cpp crypto.cpp -o server
	
client: client.cpp crypto.h protocol.h crypto.cpp protocol.cpp
	g++ client.cpp protocol.cpp crypto.cpp -o client