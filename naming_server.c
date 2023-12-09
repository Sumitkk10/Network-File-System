#include "hash.h"
#include "LRU.h"

#define NM_PORT 5000
#define CLIENT_PORT 4000

struct storage_server {
    char ip[50];
    int port_nm;
    int port_client;
    int num;
    char path[100000];
};

typedef struct {
    char operation_type[20];
    char details[100];
    char timestamp[30];
    char outcome[50]; 
    int port;
    char ip[50];
} LogEntry;

FILE* history;

char* get_current_timestamp() {
    time_t rawtime;
    struct tm* timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    return asctime(timeinfo);
}

void log_operation(char* operation_type, char* details, char* outcome, int port, char* ip, FILE* log_file) {
    LogEntry entry;
    strcpy(entry.operation_type, operation_type);
    strcpy(entry.details, details);
    strcpy(entry.timestamp, get_current_timestamp());
    strcpy(entry.outcome, outcome);
    strcpy(entry.ip, ip);
    entry.port = port;
    fprintf(log_file, "[%s] [%s] [%s] %s (Port: %d, IP: %s)\n", entry.timestamp, entry.operation_type, entry.outcome, entry.details, entry.port, entry.ip);
    fflush(log_file);  // Ensure the log is immediately written to the file
}
 
struct storage_server* ss_array = NULL;
size_t ss_count = 0;
int* primes;
node** hashtable;
LRUCache* lru;

// Update database after sending command to create or delete a file/directory
void updateSSInfo(int command, char* name, char* path, struct storage_server* ss_info, int ss) {
    // Assuming ss_info->path is a string to store accessible paths
    char temp[1000];
    strcpy(temp, path);
    if(name[0] != '\0')
        strcat(temp, "/");
    strcat(temp, name);
    int value = hash(temp, primes, strlen(temp));
    if (command == 0 || command == 1) {
 
        // Add the new path to the list of accessible paths
 
        strncat(ss_info[ss].path, temp, sizeof(ss_info[ss].path) - strlen(ss_info[ss].path) - 1);
        strncat(ss_info[ss].path, "\n", sizeof(ss_info[ss].path) - strlen(ss_info[ss].path) - 1);
        
        Insert(temp, strlen(temp), value, hashtable, ss);
        // break;
    } 
    else if (command == 2 || command == 3) {
        // Remove the path from the list of accessible paths
        char *start = strstr(ss_info[ss].path, path);
        if (start) {
            char* end = strstr(start, "\n");
            if (end) {
                // Found the path, remove it
                size_t length = strlen(end + 1);
                memmove(start, end + 1, length + 1);
            }
        }
        Delete(temp, value, hashtable);
        insertLRU(lru, temp, -1);
    }
}

int error_code(int ss_index, int client_data){
    char *acknowledgement = (char*)malloc(200);
 
    if (ss_index == -1) {
        strcpy(acknowledgement, "Path is not found in any storage servers");

    } else {
        strcpy(acknowledgement, "Path is found in a storage server");
    }
 
    printf("acknowledgement: %s\n", acknowledgement);
 
    if (ss_index == -1) {
        size_t ack_length = strlen(acknowledgement) + 1;
        if (send(client_data, &ack_length, sizeof(ack_length), 0) == -1) {
            perror("Error sending ack length to client");
            return -1;
        }
        if (send(client_data, acknowledgement, ack_length, 0) == -1) {
            perror("Error sending acknowledgement to client");
            return -1;
        }
        return -1;
    }
    else {
        size_t ack_length = strlen(acknowledgement) + 1;
        if (send(client_data, &ack_length, sizeof(ack_length), 0) == -1) {
            perror("Error sending ack length to client");
            return -1;
        }
 
        if (send(client_data, acknowledgement, ack_length, 0) == -1) {
            perror("Error sending acknowledgement to client");
            return -1;
        }
        return 1;
    }
}
 
void* handleStorageServer() {
 
    struct sockaddr_in nm_addr;
    int nm_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (nm_socket == -1) {
        perror("Socket creation failed");
        exit(1);
    }
 
    nm_addr.sin_family = AF_INET;
    nm_addr.sin_port = htons(NM_PORT);
    nm_addr.sin_addr.s_addr = INADDR_ANY;
 
    if (bind(nm_socket, (struct sockaddr*)&nm_addr, sizeof(nm_addr)) == -1) {
        perror("Error binding to server");
        exit(1);
    }
 
    if (listen(nm_socket, 100) == -1) {
        perror("Listening failed");
        exit(1);
    }
 
    while (1) {
        struct sockaddr_in temp;
        // Accept incoming connection from the storage server
        int ss_data = accept(nm_socket, (struct sockaddr*)&temp, (socklen_t*)&temp);
        if (ss_data == -1) {
            // perror("Accept failed");
            // exit(1);
            continue;
        }
 
        // Allocate memory for the new storage server info
        ss_count++;
        ss_array = realloc(ss_array, ss_count * sizeof(struct storage_server));
 
        size_t structSize;
         if (recv(ss_data, &structSize, sizeof(size_t), 0) == -1) {
            perror("Error receiving data from Storage Server");
            exit(1);
        }
 
        printf("%ld\n", structSize);     
 
        if (recv(ss_data, &ss_array[ss_count - 1], structSize, 0) == -1) {
            perror("Error receiving data from Storage Server");
            exit(1);
        }
 
        printf("Received data from Storage Server\n");
 
        printf("Storage Server %ld:\n", ss_count);
        printf("IP: %s\n", ss_array[ss_count - 1].ip);
        printf("Port for NM Connection: %d\n", ss_array[ss_count - 1].port_nm);
        printf("Port for Client Connection: %d\n", ss_array[ss_count - 1].port_client);
        printf("Number of Accessible Paths: %d\n", ss_array[ss_count - 1].num);
        printf("Accessible Paths:\n %s \n", ss_array[ss_count - 1].path);
        
        log_operation("New Server", "new storage server got initialized", "success", NM_PORT, ss_array[ss_count - 1].ip, history);
 
        char* pp = malloc(100000);
        strcpy(pp, ss_array[ss_count - 1].path);
        char* token = strtok(pp, "\n");
        while(token != NULL){
            int value = hash(token, primes, strlen(token));
            Insert(token, strlen(token), value, hashtable, ss_count - 1);
            token = strtok(NULL, "\n");
        }
 
        char success_message[] = "success";
        if (send(ss_data, success_message, strlen(success_message) + 1, 0) == -1) {
            perror("Error sending ack to SS");
            exit(1);
        }
        close(ss_data);
    }
    close(nm_socket);
}
 
// Function to find the storage server for a given path
int findStorageServer(char* path) {
    // caching first
    int x = find(lru, path);
    if(x == -1){
        int value = hash(path, primes, strlen(path));
        int ind = get(hashtable, path, value);
        insertLRU(lru, path, ind);
        return ind;
    }
    else{
        insertLRU(lru, path, x);
        return x;
    }
}

void* processClient(void* client_data_ptr) {
    int client_data = *((int*)client_data_ptr);
 
    while (1) {
        int x;
        if (recv(client_data, &x, sizeof(x), 0) == -1) {
            continue;
        }
 
        char command[10005];
        // Receive file operation command from the client
        ssize_t bytes_received = recv(client_data, command, sizeof(command), 0);
        if (bytes_received == -1) {
            continue;
        }
        command[bytes_received] = '\0';
 
        printf("operation: %d path: %s\n", x, command);
 
        if (x == 6 || x == 7 || x == 8 ) {
            // If the operation is READ, WRITE, or GET_INFO, send back IP address and port
            int ss_index = findStorageServer(command);
            
            int pp = error_code(ss_index, client_data);
            if(pp == -1){
                if(x == 6)
                    log_operation("ack", "READ", "failure", CLIENT_PORT, "127.0.0.1", history);
                else if(x == 7)
                    log_operation("ack", "WRITE", "failure", CLIENT_PORT, "127.0.0.1", history);
                else if(x == 8)
                    log_operation("ack", "GET_INFO", "failure", CLIENT_PORT, "127.0.0.1", history);
                continue;
            }
            else{
                if(x == 6)
                    log_operation("ack", "READ", "success", CLIENT_PORT, "127.0.0.1", history);
                else if(x == 7)
                    log_operation("ack", "WRITE", "success", CLIENT_PORT, "127.0.0.1", history);
                else if(x == 8)
                    log_operation("ack", "GET_INFO", "success", CLIENT_PORT, "127.0.0.1", history);
            }

            printf("%d\n", ss_index);

            if(x == 6){
                // read
                //printf("oki\n");
                int value = hash(command, primes, strlen(command));
                node* here = hashtable[value]->nxt;
                bool ok = 1;
                while(here != NULL){
                    if(!strcmp(here->a, command)){
                        int p = pthread_rwlock_tryrdlock(&(here->lock));
                        printf("locked\n");
                        if(p != 0)
                            ok = 0;
                        break;
                    }
                    here = here->nxt;
                }
                if(ok){
                    char *acknowledgement = (char*)malloc(200);
                    strcpy(acknowledgement, "Read is possible");
                    size_t ack_length = strlen(acknowledgement) + 1;
                    if (send(client_data, &ack_length, sizeof(ack_length), 0) == -1) {
                        perror("Error sending ack length to client");
                        continue;
                    }
                    if (send(client_data, acknowledgement, ack_length, 0) == -1) {
                        perror("Error sending acknowledgement to client");
                        continue;
                    }
                }
                else{
                    char *acknowledgement = (char*)malloc(200);
                    strcpy(acknowledgement, "Read not possible because some other client is writing");
                    size_t ack_length = strlen(acknowledgement) + 1;
                    if (send(client_data, &ack_length, sizeof(ack_length), 0) == -1) {
                        perror("Error sending ack length to client");
                        continue;
                    }
                    if (send(client_data, acknowledgement, ack_length, 0) == -1) {
                        perror("Error sending acknowledgement to client");
                        continue;
                    }
                    continue;
                }
            }
            else if(x == 7){
                // write
                int value = hash(command, primes, strlen(command));
                node* here = hashtable[value]->nxt;
                bool ok = 1;
                while(here != NULL){
                    if(!strcmp(here->a, command)){
                        int p = pthread_rwlock_trywrlock(&(here->lock));
                        printf("locked\n");
                        if(p != 0)
                            ok = 0;
                        break;
                    }
                    here = here->nxt;
                }
                if(ok){
                    char *acknowledgement = (char*)malloc(200);
                    strcpy(acknowledgement, "Write is possible");
                    size_t ack_length = strlen(acknowledgement) + 1;
                    if (send(client_data, &ack_length, sizeof(ack_length), 0) == -1) {
                        perror("Error sending ack length to client");
                        continue;
                    }
                    if (send(client_data, acknowledgement, ack_length, 0) == -1) {
                        perror("Error sending acknowledgement to client");
                        continue;
                    }
                }
                else{
                    char *acknowledgement = (char*)malloc(200);
                    strcpy(acknowledgement, "Write not possible because some other client is reading");
                    size_t ack_length = strlen(acknowledgement) + 1;
                    if (send(client_data, &ack_length, sizeof(ack_length), 0) == -1) {
                        perror("Error sending ack length to client");
                        continue;
                    }
                    if (send(client_data, acknowledgement, ack_length, 0) == -1) {
                        perror("Error sending acknowledgement to client");
                        continue;
                    }
                    continue;
                }
            }
 
            const char* storage_server_ip = ss_array[ss_index].ip;
            int storage_server_port = ss_array[ss_index].port_client;
 
            size_t ip_length = strlen(storage_server_ip) + 1;
            if (send(client_data, &ip_length, sizeof(ip_length), 0) == -1) {
                perror("Error sending IP length to client");
		        if(x == 6)
                    log_operation("Path back to client", "READ", "failure", CLIENT_PORT, "127.0.0.1", history);
                else if(x == 7)
                    log_operation("Path back to client", "WRITE", "failure", CLIENT_PORT, "127.0.0.1", history);
                else if(x == 8)
                    log_operation("Path back to client", "GET_INFO", "failure", CLIENT_PORT, "127.0.0.1", history);
                continue;
            }
 
            if (send(client_data, storage_server_ip, ip_length, 0) == -1) {
                perror("Error sending IP address to client");
                if(x == 6)
                    log_operation("Path back to client", "READ", "failure", CLIENT_PORT, "127.0.0.1", history);
                else if(x == 7)
                    log_operation("Path back to client", "WRITE", "failure", CLIENT_PORT, "127.0.0.1", history);
                else if(x == 8)
                    log_operation("Path back to client", "GET_INFO", "failure", CLIENT_PORT, "127.0.0.1", history);
                continue;
            }

            if (send(client_data, &storage_server_port, sizeof(storage_server_port), 0) == -1) {
                perror("Error sending port to client");
                if(x == 6)
                    log_operation("Path back to client", "READ", "failure", CLIENT_PORT, "127.0.0.1", history);
                else if(x == 7)
                    log_operation("Path back to client", "WRITE", "failure", CLIENT_PORT, "127.0.0.1", history);
                else if(x == 8)
                    log_operation("Path back to client", "GET_INFO", "failure", CLIENT_PORT, "127.0.0.1", history);
                continue;
            }
            
            if(x == 6)
                log_operation("Path back to client", "READ", "success", CLIENT_PORT, "127.0.0.1", history);
            else if(x == 7)
                log_operation("Path back to client", "WRITE", "success", CLIENT_PORT, "127.0.0.1", history);
            else if(x == 8)
                log_operation("Path back to client", "GET_INFO", "success", CLIENT_PORT, "127.0.0.1", history);

            int xx;
            if(x != 8){
                if (recv(client_data, &xx, sizeof(xx), 0) == -1) {
                    perror("Error receiving IP length from naming server");
                    continue;
                }
            }
            
            if(x == 6){
                int value = hash(command, primes, strlen(command));
                node* here = hashtable[value]->nxt;
                bool ok = 1;
                while(here != NULL){
                    if(!strcmp(here->a, command)){
                        pthread_rwlock_unlock(&(here->lock));
                        printf("unlocked\n");
                        break;
                    }
                    here = here->nxt;
                }
            }
            else if(x == 7){
                // write
                int value = hash(command, primes, strlen(command));
                node* here = hashtable[value]->nxt;
                bool ok = 1;
                while(here != NULL){
                    if(!strcmp(here->a, command)){
                        pthread_rwlock_unlock(&(here->lock));
                        printf("unlocked\n");
                        break;
                    }
                    here = here->nxt;
                }
            }
        }
        else if (x == 0 || x == 1){
            // 0 --> create file
            // 1 --> create dir
            char* cmd = (char*) malloc(200);
            strcpy(cmd, command);
            char* token = strtok(command, "|");
            char path[200];
            strcpy(path, token);
            token = strtok(NULL, "|");
            char *name = (char*) malloc(200);
            strcpy(name, token);
            printf("%s %s\n", path, name);
            int ss_index = findStorageServer(path);
            // insertLRU(lru, cmd, ss_index);
            int pp = error_code(ss_index, client_data);
            if(pp == -1){
                log_operation("ack", "CREATE", "failure", ss_array[ss_index].port_nm, "127.0.0.1", history);
                continue;
            }
            else{
                log_operation("ack", "CREATE", "sucess", ss_array[ss_index].port_nm, "127.0.0.1", history);
            }
            // For other operations, include code to connect to the storage server
            int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
            if (sock_fd == -1) {
            	log_operation("Operation", "CREATE", "failure", ss_array[ss_index].port_nm, "127.0.0.1", history);
                perror("Operation couldn't be completed: Socket creation failed, ERROR in NM --> SS");
                continue;
            }
 
            struct sockaddr_in ss_addr;
            ss_addr.sin_family = AF_INET;
            ss_addr.sin_port = htons(ss_array[ss_index].port_nm);
            ss_addr.sin_addr.s_addr = INADDR_ANY;
 
            if(connect(sock_fd, (struct sockaddr*) &ss_addr, sizeof(ss_addr)) == -1){
            	log_operation("Operation", "CREATE", "failure", ss_array[ss_index].port_nm, "127.0.0.1", history);
                perror("Operation couldn't be completed: Connect failed, ERROR in NM --> SS");
                close(sock_fd);
                continue;
            }
 
            if (send(sock_fd, &x, sizeof(int), 0) == -1) {
            	log_operation("Operation", "CREATE", "failure", ss_array[ss_index].port_nm, "127.0.0.1", history);
                perror("Operation couldn't be completed: send failed, ERROR in NM --> SS");
                close(sock_fd);
                continue;
            }
 
            if(send(sock_fd, cmd, strlen(cmd) + 1, 0) == -1){
            	log_operation("Operation", "CREATE", "failure", ss_array[ss_index].port_nm, "127.0.0.1", history);
                perror("Operation couldn't be completed: send failed, ERROR in NM --> SS");
                close(sock_fd);
                continue;
            }
            // rev ack and send to client
            int y;
            
            if (recv(sock_fd, &y, sizeof(y), 0) == -1) {
            	log_operation("Operation", "CREATE", "failure", ss_array[ss_index].port_nm, "127.0.0.1", history);
                perror("Error receiving data from Storage Server");
                close(sock_fd);
                continue;
            }
            
            if (y == 1){
                updateSSInfo(x, name, path, ss_array, ss_index);
                printf("CREATED\n");
                log_operation("Operation", "CREATE", "success", ss_array[ss_index].port_nm, "127.0.0.1", history);
            }
            else{
            	log_operation("Operation", "CREATE", "failure", ss_array[ss_index].port_nm, "127.0.0.1", history);
                perror("Operation not done\n");
                close(sock_fd);
                continue;
            }
 
            close(sock_fd);
        }
        else if(x == 2 || x == 3){
            int ss_index = findStorageServer(command);
            printf("%d\n", ss_index);
            // insertLRU(lru, command, ss_index);
            int pp = error_code(ss_index, client_data);
            if(pp == -1){
                log_operation("ack", "CREATE", "failure", ss_array[ss_index].port_nm, "127.0.0.1", history);
                continue;
            }
            else{
                log_operation("ack", "CREATE", "sucess", ss_array[ss_index].port_nm, "127.0.0.1", history);
            }
            int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
            if (sock_fd == -1) {
            	log_operation("Operation", "DELETE", "failure", CLIENT_PORT, "127.0.0.1", history);
                perror("Operation couldn't be completed: Socket creation failed, ERROR in NM --> SS");
                continue;
            }
 
            struct sockaddr_in ss_addr;
            ss_addr.sin_family = AF_INET;
            ss_addr.sin_port = htons(ss_array[ss_index].port_nm);
            ss_addr.sin_addr.s_addr = INADDR_ANY;
            if(connect(sock_fd, (struct sockaddr*) &ss_addr, sizeof(ss_addr)) == -1){
            	log_operation("Operation", "DELETE", "failure", ss_array[ss_index].port_nm, "127.0.0.1", history);
                perror("Operation couldn't be completed: Connect failed, ERROR in NM --> SS");
                close(sock_fd);
                continue;
            }
 
            if (send(sock_fd, &x, sizeof(int), 0) == -1) {
            	log_operation("Operation", "DELETE", "failure", ss_array[ss_index].port_nm, "127.0.0.1", history);
                perror("Operation couldn't be completed: send failed, ERROR in NM --> SS");
                close(sock_fd);
                continue;
            }
 
            if(send(sock_fd, command, strlen(command) + 1, 0) == -1){
            	log_operation("Operation", "DELETE", "failure", ss_array[ss_index].port_nm, "127.0.0.1", history);
                perror("Operation couldn't be completed: send failed, ERROR in NM --> SS");
                close(sock_fd);
                continue;
            }
 
            int y;
            
            if (recv(sock_fd, &y, sizeof(y), 0) == -1) {
            	log_operation("Operation", "DELETE", "failure", ss_array[ss_index].port_nm, "127.0.0.1", history);
                perror("Error receiving data from Storage Server");
                close(sock_fd);
                continue;
            }
            
            if (y == 1) {
                // delete from ss;
                char commandd[1];
                commandd[0] = '\0';
                printf("%s\n", command);
                updateSSInfo(x, commandd, command, ss_array, ss_index);
                // insertLRU(lru, command, -1);
                log_operation("Operation", "DELETE", "success", ss_array[ss_index].port_nm, "127.0.0.1", history);
                printf("DELETED\n");
            }
            else{
            	log_operation("Operation", "DELETE", "failure", ss_array[ss_index].port_nm, "127.0.0.1", history);
                perror("Operation not done\n");
                close(sock_fd);
                continue;
            }
 
            close(sock_fd);
        }
        else if(x == 4){
            char source[10005];
            strcpy(source, command);
            char dest[10005];
            memset(dest, 0, sizeof dest);
            ssize_t dest_path = recv(client_data, dest, sizeof(dest), 0);
            if (dest_path == -1) {
                continue;
            }
            dest[bytes_received] = '\0';         
 
            printf("source: %s destination: %s\n", source, dest);
 
            int ss_index_src = findStorageServer(source);
            // insertLRU(lru, source, ss_index_src);
            int pp = error_code(ss_index_src, client_data);
            if(pp == -1){
                log_operation("ack", "COPY", "failure", CLIENT_PORT, "127.0.0.1", history);
                continue;
            }
            else{
                log_operation("ack", "CREATE", "sucess", CLIENT_PORT, "127.0.0.1", history);
            }

            int ss_index_dest = findStorageServer(dest);
            printf("source SS index: %d dest SS index: %d\n", ss_index_src, ss_index_dest);
 
            const char* src_ip = ss_array[ss_index_src].ip;
            int src_port = ss_array[ss_index_src].port_nm;
 
            printf("source ip: %s port: %d\n", src_ip, src_port);

            // Create NM Socket for SOURCE
            int nm_socket = socket(AF_INET, SOCK_STREAM, 0);
            struct sockaddr_in nm_addr;

            if (nm_socket == -1) {
                perror("Naming Server client socket creation failed");
                continue;
            }

            nm_addr.sin_family = AF_INET;
            nm_addr.sin_addr.s_addr = INADDR_ANY;
            nm_addr.sin_port = htons(src_port);

            if (connect(nm_socket, (struct sockaddr*)&nm_addr, sizeof(nm_addr)) == -1) {
                perror("Naming Server client connection failed");
                close(nm_socket);
                continue;
            }

            printf("Source SS + Client Connection Created\n");

            // send operation to SS
            int operation = 4;
            if (send(nm_socket, &operation, sizeof(operation), 0) == -1) {
                perror("Error sending operation to Storage Server");
                close(nm_socket);
                continue;
            }   
            
            // send path to SS
            if (send(nm_socket, source, strlen(source) + 1, 0) == -1) {
                perror("Error sending file path to Storage Server");
                close(nm_socket);
                continue;
            }
 
            // Receive and print file content in chunks
            char buffer[10000];
            size_t chunk_size;

            printf("\nFILE CONTENT:\n");

            while (1) {

                // Receive the chunk size
                if (recv(nm_socket, &chunk_size, sizeof(chunk_size), 0) == -1) {
                    perror("Error receiving chunk size from storage server");
                    break;
                }
                if (chunk_size == 0) {
                    break;
                }

                // Receive the chunk content
                memset(buffer, 0, sizeof(buffer)); // Clear the buffer
                if (recv(nm_socket, buffer, chunk_size, 0) == -1) {
                    perror("Error receiving file content chunk from storage server");
                    break;
                }
                // Print or process the chunk content
                printf("%s", buffer); 

            }

            // connect to DESTINATION socket

            const char* dest_ip = ss_array[ss_index_dest].ip;
            int dest_port = ss_array[ss_index_dest].port_nm;
 
            printf("destination ip: %s port: %d\n", dest_ip, dest_port);

            // Create NM Socket for DESTINATION

            int nm_socket_dest = socket(AF_INET, SOCK_STREAM, 0);
            struct sockaddr_in nm_addr_dest;

            if (nm_socket_dest == -1) {
                perror("Naming Server client socket creation failed");
                continue;
            }

            nm_addr_dest.sin_family = AF_INET;
            nm_addr_dest.sin_addr.s_addr = INADDR_ANY;
            nm_addr_dest.sin_port = htons(dest_port);

            if (connect(nm_socket_dest, (struct sockaddr*)&nm_addr_dest, sizeof(nm_addr_dest)) == -1) {
                perror("Naming Server client connection failed");
                close(nm_socket_dest);
                continue;
            }

            printf("DESTINATION SS + Client Connection Created\n");

            // send operation to SS
            operation = 5;
            if (send(nm_socket_dest, &operation, sizeof(operation), 0) == -1) {
                perror("Error sending operation to Storage Server");
                close(nm_socket_dest);
                continue;
            }   
            printf("operation: %d\n", operation);
            
            // send path to SS
            if (send(nm_socket_dest, dest, strlen(dest) + 1, 0) == -1) {
                perror("Error sending dest file path to Storage Server");
                close(nm_socket_dest);
                continue;
            }
            printf("path: %s\n", dest);

            // send file contents
            if (send(nm_socket_dest, buffer, sizeof(buffer), 0) == -1) {
                perror("Error sending source file contents to Storage Server");
                close(nm_socket_dest);
                continue;
            }
            printf("contents: %s\n", buffer);
        
            char name[1];
            name[0] = '\0';

            // updateSSInfo(x, name, dest, ss_array, ss_index_dest);
            printf("CREATED\n");

            close(nm_socket);
            close(nm_socket_dest);

            char *acknowledgement = (char*)malloc(200);
            strcpy(acknowledgement, "File copy and transfer done successfully");
            size_t ack_length = strlen(acknowledgement) + 1;
            if (send(client_data, &ack_length, sizeof(ack_length), 0) == -1) {
                perror("Error sending ack length to client");
                continue;
            }
            if (send(client_data, acknowledgement, ack_length, 0) == -1) {
                perror("Error sending acknowledgement to client");
                continue;
            }
            close(client_data);
        }
    }
    close(client_data);
    return NULL;
}

void* handleClient(void* arg) {
 
    struct sockaddr_in client_addr;
 
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Client socket creation failed");
        exit(1);
    }
 
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(CLIENT_PORT);
    client_addr.sin_addr.s_addr = INADDR_ANY;
 
    if (bind(client_socket, (struct sockaddr*)&client_addr, sizeof(client_addr)) == -1) {
        perror("Error binding to server");
        exit(1);
    }
 
    if (listen(client_socket, 100) == -1) {
        perror("Listening failed");
        exit(1);
    }
    while (1) {
        struct sockaddr_in temp;
        int client_data = accept(client_socket, (struct sockaddr*)&temp, (socklen_t*)&temp);
        if (client_data == -1) {
            perror("Accept failed");
            continue;
        }
        // Create a thread for each client connection
        pthread_t client_thread;
        pthread_create(&client_thread, NULL, processClient, (void*) &client_data);
        pthread_detach(client_thread);
    }
    close(client_socket);
    return NULL;
}
 
int main() {

	history = fopen("history.txt", "a");
    if (history == NULL) {
        perror("Error opening log file");
        exit(EXIT_FAILURE);
    }
 
    // Initializing the hashmap & LRU
 
    primes = (int*) malloc(sizeof(int) * 200);
    int untill = 0, pre = 1;
    while(untill < 200){
        int x = findnextprime(pre);
        primes[untill] = x;
        pre = x;
        ++untill;
    }
 
    hashtable = CreateHT(100003);
    lru = createLRUCache(20);
 
    // Database for file directory
    pthread_t threads[2];
 
    pthread_create(&threads[0], NULL, handleStorageServer, NULL);
    pthread_create(&threads[1], NULL, handleClient, NULL);
 
    pthread_join(threads[0], NULL);
    pthread_join(threads[1], NULL);
 
    free(ss_array);
    destroyLRUCache(lru);
 
    return 0;
}   
