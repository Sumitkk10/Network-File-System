#include "headers.h"

#define NM_IP "127.0.0.1"  // Replace with the actual IP address of your Naming Server
#define NM_PORT 4000

int error_code(int client_socket_nm, int timeout_seconds) {
    // ERROR CODE:
    char *acknowledgement = (char*)malloc(200);

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(client_socket_nm, &read_fds);

    struct timeval timeout;
    timeout.tv_sec = timeout_seconds;
    timeout.tv_usec = 0;

    int ready = select(client_socket_nm + 1, &read_fds, NULL, NULL, &timeout);

    if (ready == -1) {
        perror("Error in select");
        return -1;
    } else if (ready == 0) {
        // Timeout
        printf("Timeout: Acknowledgement not received within %d seconds.\n", timeout_seconds);
        return -1;
    }

    // Continue with the existing logic for receiving and processing the acknowledgement
    size_t ack_length;
    if (recv(client_socket_nm, &ack_length, sizeof(ack_length), 0) == -1) {
        perror("Error receiving ack length from naming server");
        return -1;
    }

    if (recv(client_socket_nm, acknowledgement, ack_length, 0) == -1) {
        perror("Error receiving acknowledgement from naming server");
        return -1;
    }

    if (strcmp(acknowledgement, "Path is not found in any storage servers") == 0) {
        printf("\nAcknowledgement: %s\n", acknowledgement);
        return -1;
    }

    printf("\nAcknowledgement: %s\n", acknowledgement);
    return 1;
}

// Function to send requests to the naming server
void* handleNM() {

    struct sockaddr_in nm_addr, temp;
    int client_socket_nm = socket(AF_INET, SOCK_STREAM, 0);
    pthread_t nm_thread, ss_thread;

    if (client_socket_nm == -1) {
        perror("Naming Server client socket creation failed");
        exit(1);
    }

    nm_addr.sin_family = AF_INET;
    nm_addr.sin_addr.s_addr = INADDR_ANY;
    nm_addr.sin_port = htons(NM_PORT);

    if (connect(client_socket_nm, (struct sockaddr*)&nm_addr, sizeof(nm_addr)) == -1) {
        perror("Naming Server client connection failed");
        exit(1);
    }

    while (1) {
        printf("Enter file operation command (READ, WRITE, GET_INFO, CREATE FILE, CREATE DIR, DELETE FILE, DELETE DIR, COPY FILE, COPY FOLDER): ");
        char command[200];
        fgets(command, sizeof(command), stdin);
        printf("%s", command);
        char storage_server_ip[50];
        int storage_server_port;

        if (strcmp(command, "READ\n") == 0) {
            printf("Enter file path: ");
            char path[1024];
            scanf("%s", path);
            getchar();
            int operation = 6;
             // Send operation and file path to the naming server
            if (send(client_socket_nm, &operation, sizeof(operation), 0) == -1) {
                perror("Error sending operation to naming server");
                continue;
            }
 
            if (send(client_socket_nm, path, sizeof(path), 0) == -1) {
                perror("Error sending file path to naming server");
                continue;
            }

            int pp = error_code(client_socket_nm, 3);
            if(pp == -1) continue;

            size_t len;
            if (recv(client_socket_nm, &len, sizeof(len), 0) == -1) {
                perror("Error receiving IP length from naming server");
                continue;
            }

            char ack[50];

            // Receive the IP address
            if (recv(client_socket_nm, ack, len, 0) == -1) {
                perror("Error receiving IP address from naming server");
                continue;
            }

            if(!strcmp(ack, "Read not possible because some other client is writing")){
                printf("ERROR: Read not possible because some other client is writing\n");
                continue;
            }
            else
                printf("Read is possible\n");
            
            size_t ip_length;
            if (recv(client_socket_nm, &ip_length, sizeof(ip_length), 0) == -1) {
                perror("Error receiving IP length from naming server");
                continue;
            }

            // Receive the IP address
            if (recv(client_socket_nm, storage_server_ip, ip_length, 0) == -1) {
                perror("Error receiving IP address from naming server");
                continue;
            }

            // Null-terminate the received string
            storage_server_ip[ip_length - 1] = '\0';

            // Receive the port from the naming server
            if (recv(client_socket_nm, &storage_server_port, sizeof(storage_server_port), 0) == -1) {
                perror("Error receiving port from naming server");
                continue;
            }
            printf("Received IP: %s, Port: %d from naming server\n", storage_server_ip, storage_server_port);

            // connect to storage server

            int client_socket_ss = socket(AF_INET, SOCK_STREAM, 0);
            struct sockaddr_in ss_addr;

            if (client_socket_ss == -1) {
                perror("Storage Server client socket creation failed");
                continue;
            }

            ss_addr.sin_family = AF_INET;
            ss_addr.sin_addr.s_addr = inet_addr(storage_server_ip);
            ss_addr.sin_port = htons(storage_server_port);

            if (connect(client_socket_ss, (struct sockaddr*)&ss_addr, sizeof(ss_addr)) == -1) {
                perror("Storage Server client connection failed");
                close(client_socket_ss);
                continue;
            }
            printf("Storage Server connection accepted\n");

            // send operation to SS
            if (send(client_socket_ss, &operation, sizeof(operation), 0) == -1) {
                perror("Error sending operation to Storage Server");
                close(client_socket_ss);
                continue;
            }
            
            // send path to SS
            if (send(client_socket_ss, path, sizeof(path), 0) == -1) {
                perror("Error sending file path to Storage Server");
                close(client_socket_ss);
                continue;
            }

            // Receive and print file content in chunks
            size_t chunk_size;
            printf("\nFILE CONTENT:\n");
            while (1) {
            // Receive the chunk size
                if (recv(client_socket_ss, &chunk_size, sizeof(chunk_size), 0) == -1) {
                    perror("Error receiving chunk size from storage server");
                    break;
                }

                if (chunk_size == 0) {
                    break;
                }

                // Receive the chunk content
                char buffer[10000];
                memset(buffer, 0, sizeof(buffer)); // Clear the buffer
                if (recv(client_socket_ss, buffer, chunk_size, 0) == -1) {
                    perror("Error receiving file content chunk from storage server");
                    break;
                }

                // Print or process the chunk content
                printf("%s", buffer);
            }
            printf("\n");
            int xx = 1;
            if (send(client_socket_nm, &xx, sizeof(xx), 0) == -1) {
                perror("Error sending ack length to client");
                continue;
            }

            close(client_socket_ss);
        } else if (strcmp(command, "WRITE\n") == 0) {
            printf("Enter file path: ");
            char path[1024];
            scanf("%s", path);
            getchar();
            int operation = 7;
            // Send operation and file path to the naming server 
            if (send(client_socket_nm, &operation, sizeof(operation), 0) == -1) {
                perror("Error sending operation to naming server");
                continue;
            }
 
            if (send(client_socket_nm, path, sizeof(path), 0) == -1) {
                perror("Error sending file path to naming server");
                continue;
            }

            int pp = error_code(client_socket_nm, 3);
            if(pp == -1) continue;

            size_t len;
            if (recv(client_socket_nm, &len, sizeof(len), 0) == -1) {
                perror("Error receiving IP length from naming server");
                continue;
            }

            char ack[50];

            // Receive the IP address
            if (recv(client_socket_nm, ack, len, 0) == -1) {
                perror("Error receiving IP address from naming server");
                continue;
            }

            if(!strcmp(ack, "Write not possible because some other client is reading")){
                printf("ERROR: Write not possible because some other client is reading\n");
                continue;
            }
            else
                printf("Write is possible\n");

            size_t ip_length;
            if (recv(client_socket_nm, &ip_length, sizeof(ip_length), 0) == -1) {
                perror("Error receiving IP length from naming server");
                continue;
            }

            // Receive the IP address
            if (recv(client_socket_nm, storage_server_ip, ip_length, 0) == -1) {
                perror("Error receiving IP address from naming server");
                continue;
            }

            // Null-terminate the received string
            storage_server_ip[ip_length - 1] = '\0';

            // Receive the port from the naming server
            if (recv(client_socket_nm, &storage_server_port, sizeof(storage_server_port), 0) == -1) {
                perror("Error receiving port from naming server");
                continue;
            }
            printf("Received IP: %s, Port: %d from naming server\n", storage_server_ip, storage_server_port);
            

            // connect to storage server

            int client_socket_ss = socket(AF_INET, SOCK_STREAM, 0);
            struct sockaddr_in ss_addr;

            if (client_socket_ss == -1) {
                perror("Storage Server client socket creation failed");
                continue;
            }

            ss_addr.sin_family = AF_INET;
            ss_addr.sin_addr.s_addr = inet_addr(storage_server_ip);
            ss_addr.sin_port = htons(storage_server_port);

            if (connect(client_socket_ss, (struct sockaddr*)&ss_addr, sizeof(ss_addr)) == -1) {
                perror("Storage Server client connection failed");
                close(client_socket_ss);
                continue;
            }
            printf("Storage Server connection accepted\n");

            // send operation to SS
            if (send(client_socket_ss, &operation, sizeof(operation), 0) == -1) {
                perror("Error sending operation to Storage Server");
                continue;
            }
            // send path to SS
            if (send(client_socket_ss, path, sizeof(path), 0) == -1) {
                perror("Error sending file path to Storage Server");
                continue;
            }

            // Prompt the user for the new data to be written to the file
            printf("Enter new data:\n");
            char new_data[10000];
            fgets(new_data, sizeof(new_data), stdin);

            // send new content to SS
            if (send(client_socket_ss, new_data, strlen(new_data), 0) == -1) {
                perror("Error sending new file content to Storage Server");
                continue;
            }

            int xx = 1;
            if (send(client_socket_nm, &xx, sizeof(xx), 0) == -1) {
                perror("Error sending ack length to client");
                continue;
            }

            close(client_socket_ss);
        } else if (strcmp(command, "GET_INFO\n") == 0) {
            printf("Enter file path: ");
            char path[1024];
            scanf("%s", path);
            getchar();
            int operation = 8;
             // Send operation and file path to the naming server
            if (send(client_socket_nm, &operation, sizeof(operation), 0) == -1) {
                perror("Error sending operation to naming server");
                continue;
            }
 
            if (send(client_socket_nm, path, sizeof(path), 0) == -1) {
                perror("Error sending file path to naming server");
                continue;
            }

            int pp = error_code(client_socket_nm, 3);
            if(pp == -1) continue;
            size_t ip_length;
            if (recv(client_socket_nm, &ip_length, sizeof(ip_length), 0) == -1) {
                perror("Error receiving IP length from naming server");
                continue;
            }

            // Receive the IP address
            if (recv(client_socket_nm, storage_server_ip, ip_length, 0) == -1) {
                perror("Error receiving IP address from naming server");
                continue;
            }

            // Null-terminate the received string
            storage_server_ip[ip_length - 1] = '\0';

            // Receive the port from the naming server
            if (recv(client_socket_nm, &storage_server_port, sizeof(storage_server_port), 0) == -1) {
                perror("Error receiving port from naming server");
                continue;
            }
            printf("Received IP: %s, Port: %d from naming server\n", storage_server_ip, storage_server_port);
            
            // connect to storage server

            int client_socket_ss = socket(AF_INET, SOCK_STREAM, 0);
            struct sockaddr_in ss_addr;

            if (client_socket_ss == -1) {
                perror("Storage Server client socket creation failed");
                continue;
            }

            ss_addr.sin_family = AF_INET;
            ss_addr.sin_addr.s_addr = inet_addr(storage_server_ip);
            ss_addr.sin_port = htons(storage_server_port);

            if (connect(client_socket_ss, (struct sockaddr*)&ss_addr, sizeof(ss_addr)) == -1) {
                perror("Storage Server client connection failed");
                close(client_socket_ss);
                continue;
            }
            printf("Storage Server connection accepted\n");

            // send operation to SS
            if (send(client_socket_ss, &operation, sizeof(operation), 0) == -1) {
                perror("Error sending operation to Storage Server");
                continue;
            }
            // send path to SS
            if (send(client_socket_ss, path, sizeof(path), 0) == -1) {
                perror("Error sending file path to Storage Server");
                continue;
            }

            int size;

            // Receive size of file
            if (recv(client_socket_ss, &size, sizeof(size), 0) == -1) {
                perror("Error receiving file size from Storage Server");
                close(client_socket_ss);
                continue;
            }

            size_t ack_length;

            if (recv(client_socket_ss, &ack_length, sizeof(ack_length), 0) == -1) {
                perror("Error receiving acknowledgment length from SS");
                continue;
            }

            char buffer[10000];
            memset(buffer, 0, sizeof(buffer));

            // Receive the acknowledgment
            if (recv(client_socket_ss, buffer, ack_length, 0) == -1) {
                perror("Error receiving acknowledgment from SS");
                continue;
            }

            printf("File Size: %d, File Permissions: %s\n", size, buffer);

            char stop_packet[10];
            if (recv(client_socket_ss, stop_packet, strlen(stop_packet) + 1, 0) == -1) {
                perror("stop packet not received");
                continue;
            }

            close(client_socket_ss);
        } else if (strcmp(command, "CREATE FILE\n") == 0) {
            printf("Enter file path: ");
            char path[1024];
            scanf("%s", path);
            printf("Enter file name: ");
            char name[1024];
            scanf("%s", name);
            getchar(); 
            strcat(path, "|");
            strcat(path, name);
 
            printf("%s\n", path);
 
            int operation = 0;
 
            if (send(client_socket_nm, &operation, sizeof(operation), 0) == -1) {
                perror("Error sending file operation request to the Naming Server");
                continue;
            }
 
            if (send(client_socket_nm, path, strlen(path), 0) == -1) {
                perror("Error sending file operation request to the Naming Server");
                continue;
            }

            int pp = error_code(client_socket_nm, 3);
            if(pp == -1) continue;
            // pthread_create(&ss_thread, NULL, handleSS, &client_socket_ss);
            // Wait for the Storage Server thread to finish
            // pthread_join(ss_thread, NULL);
        } else if (strcmp(command, "CREATE DIR\n") == 0) {
            printf("Enter file path: ");
            char path[1024];
            scanf("%s", path);
            getchar();
            printf("Enter file name: ");
            char name[1024];
            scanf("%s", name);
            
            strcat(path, "|");
            strcat(path, name);
            getchar(); 
 
            printf("%s\n", path);
            int operation = 1;
 
            if (send(client_socket_nm, &operation, sizeof(operation), 0) == -1) {
                perror("Error sending file operation request to the Naming Server");
                continue;
            }
 
            if (send(client_socket_nm, path, strlen(path), 0) == -1) {
                perror("Error sending file operation request to the Naming Server");
                continue;
            }

            int pp = error_code(client_socket_nm, 3);
            if(pp == -1)
                continue;
 
            // sendFileOperation(client_socket_nm, 1, path);
 
            // pthread_create(&ss_thread, NULL, handleSS, &client_socket_ss);
 
            // Wait for the Storage Server thread to finish
            // pthread_join(ss_thread, NULL);
 
        } else if(strcmp(command, "DELETE FILE\n") == 0){
            printf("Enter file path: ");
            char path[1024];
            scanf("%s", path);
            
            getchar(); 
 
            int operation = 2;
 
            if (send(client_socket_nm, &operation, sizeof(operation), 0) == -1) {
                perror("Error sending file operation request to the Naming Server");
                continue;
            }
 
            if (send(client_socket_nm, path, strlen(path), 0) == -1) {
                perror("Error sending file operation request to the Naming Server");
                continue;
            }
            int pp = error_code(client_socket_nm, 3);
            if(pp == -1)
                continue;
            // pthread_create(&ss_thread, NULL, handleSS, &client_socket_ss);
 
            // Wait for the Storage Server thread to finish
            // pthread_join(ss_thread, NULL);
        } else if(strcmp(command, "DELETE DIR\n") == 0){
            printf("Enter file path: ");
            char path[1024];
            scanf("%s", path);
            
            getchar(); 
 
            int operation = 3;
 
            if (send(client_socket_nm, &operation, sizeof(operation), 0) == -1) {
                perror("Error sending file operation request to the Naming Server");
                continue;
            }
 
            if (send(client_socket_nm, path, strlen(path), 0) == -1) {
                perror("Error sending file operation request to the Naming Server");
                continue;
            }
            int pp = error_code(client_socket_nm, 3);
            if(pp == -1)
                continue;
            // pthread_create(&ss_thread, NULL, handleSS, &client_socket_ss);
 
            // Wait for the Storage Server thread to finish
            // pthread_join(ss_thread, NULL);
        } else if (strcmp(command, "COPY FILE\n") == 0) {
            int operation = 4;
             // Send operation and file path to the naming server
            if (send(client_socket_nm, &operation, sizeof(operation), 0) == -1) {
                perror("Error sending operation to naming server");
                continue;
            }
            printf("Enter source file path: ");
            char sourcePath[1024];
            scanf("%s", sourcePath);
            getchar();
            if (send(client_socket_nm, sourcePath, sizeof(sourcePath), 0) == -1) {
                perror("Error sending source file path to naming server");
                continue;
            }
            
            printf("Enter destination file path: ");
            char destinationPath[1024];
            scanf("%s", destinationPath);
            getchar();
            if (send(client_socket_nm, destinationPath, sizeof(destinationPath), 0) == -1) {
                perror("Error sending destination file path to naming server");
                continue;
            }

            printf("source path: %s\n", sourcePath);
            printf("destination path: %s\n", destinationPath);
            int pp = error_code(client_socket_nm, 3);
            if(pp == -1) continue;
            
            // ack message recieved
            char *acknowledgement = (char*)malloc(200);
            size_t ack_length;
            if (recv(client_socket_nm, &ack_length, sizeof(ack_length), 0) == -1) {
                perror("Error receiving ack length from naming server");
                continue;
            }

            if (recv(client_socket_nm, acknowledgement, ack_length, 0) == -1) {
                perror("Error receiving acknowledgement from naming server");
                continue;
            }
            printf("%s\n", acknowledgement);
        } else if(strcmp(command, "COPY FOLDER\n") == 0){
            // not implemented;
        }else {
            printf("Invalid command. Please try again.\n");
        }
    }
    close(client_socket_nm);
    pthread_exit(NULL);
}
 
int main() {
    // int client_socket_ss = socket(AF_INET, SOCK_STREAM, 0);
 
    // struct sockaddr_in nm_addr, ss_addr;
 
    // Create threads to handle connections
    pthread_t nm_thread, ss_thread;
    pthread_create(&nm_thread, NULL, handleNM, NULL);
    // pthread_create(&ss_thread, NULL, handleSS, &client_socket_ss);
 
    // Wait for threads to finish
    pthread_join(nm_thread, NULL);
    // pthread_join(ss_thread, NULL);
 
    return 0;
}
