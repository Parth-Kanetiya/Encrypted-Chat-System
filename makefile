all: server client
server: server.cpp crypto.h protocol.h crypto.cpp protocol.cpp
	g++ server.cpp protocol.cpp crypto.cpp -o server
# 	g++ server.cpp protocol.cpp crypto.cpp -o server -std=c++17 -pthread
# 	g++ -o server server.cpp crypto.h protocol.h
	
client: client.cpp crypto.h protocol.h crypto.cpp protocol.cpp
	g++ client.cpp protocol.cpp crypto.cpp -o client
# 	g++ client.cpp protocol.cpp crypto.cpp -o client -std=c++17 -pthread