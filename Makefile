all: naming_server storage_server client

naming_server: naming_server.c LRU.c hash.c
	gcc $^ -o naming_server

storage_server: storage_server.c
	gcc $< -o storage_server

client: client.c
	gcc $^ -o client

run: all
	@gnome-terminal --tab --title="Naming Server" --command="./naming_server"
	@gnome-terminal --tab --title="Storage Server" --command="./storage_server"
	@gnome-terminal --tab --title="Client" --command="./client"

clean:
	rm -f naming_server storage_server client
