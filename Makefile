clientserver:
	gcc ChatClient.c -o client
	gcc ChatServer.c -o server
 clean:
	-rm -f .*o 
