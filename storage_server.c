#include "headers.h"
 
#define NM_IP "127.0.0.1"
#define NM_PORT 5000
int cnt;
char current_directory[500];
 
typedef struct SS {
    char ip[50];
    int nm_port;
    int client_port;
    int num; // denotes the number of paths in this storage server
    char path[100000];
} SS;

char* perms(const char* filepath) {
    struct stat file_stat;

    if (stat(filepath, &file_stat) != 0) {
        perror("Error getting file information");
        exit(EXIT_FAILURE);
    }

    static char permissions[11];

    snprintf(permissions, sizeof(permissions),
             "%s%s%s%s%s%s%s%s%s%s",
             (S_ISDIR(file_stat.st_mode)) ? "d" : "-",
             (file_stat.st_mode & S_IRUSR) ? "r" : "-",
             (file_stat.st_mode & S_IWUSR) ? "w" : "-",
             (file_stat.st_mode & S_IXUSR) ? "x" : "-",
             (file_stat.st_mode & S_IRGRP) ? "r" : "-",
             (file_stat.st_mode & S_IWGRP) ? "w" : "-",
             (file_stat.st_mode & S_IXGRP) ? "x" : "-",
             (file_stat.st_mode & S_IROTH) ? "r" : "-",
             (file_stat.st_mode & S_IWOTH) ? "w" : "-",
             (file_stat.st_mode & S_IXOTH) ? "x" : "-");

    return permissions;
}
 
// gpt prompt: C code to get all files in a given directory, recursively of all folders.
// line 24-61
 
void traverse_directory(const char *dirname, const char *base, SS *here, int *path_count) {
    DIR *dir;
    struct dirent *ent;
    struct stat st;
 
    if ((dir = opendir(dirname)) != NULL) {
        while ((ent = readdir(dir)) != NULL && *path_count < 1024) {
            if (ent->d_name[0] == '.') {
                continue;
            }
            char path[500];
            snprintf(path, sizeof(path), "%s/%s", dirname, ent->d_name);
 
            if (stat(path, &st) == 0) {
                char relative_path[1024];
                
                if (strcmp(base, ".") == 0) {
                    // If the base is ".", just use the file/folder name
                    snprintf(relative_path, sizeof(relative_path), "%s", ent->d_name);
                } else {
                    // Otherwise, construct the relative path
                    snprintf(relative_path, sizeof(relative_path), "%s/%s", base, ent->d_name);
                }
 
                if (S_ISDIR(st.st_mode)) {
                    // Recursive call for subdirectories
                    if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {
                        traverse_directory(path, relative_path, here, path_count);
                    }
                }
 
                // Store the relative path
 
                strcat(here->path, relative_path);
                strcat(here->path, "\n");
                (*path_count)++;
            }
        }
        closedir(dir);
    } else {
        perror("Error reading directory");
    }
}
 
void fill_values(SS* here){
    FILE *fp = fopen("/home/sumit_kk10/Desktop/Clg Studies/SEM_3/OSN/final-project-025/count.txt", "r");
    fscanf(fp, "%d", &cnt);
    fclose(fp);
    fp = fopen("/home/sumit_kk10/Desktop/Clg Studies/SEM_3/OSN/final-project-025/count.txt", "w"); // REPLACE THIS WITH THE ABSOULTE PATH OF THE FILE COUNT
    fprintf(fp, "%d", cnt + 2);
    fclose(fp);
    strcpy(here->ip, NM_IP);
    here->nm_port = NM_PORT + cnt;
    here->client_port = NM_PORT + cnt + 1;
    
    if (getcwd(current_directory, sizeof(current_directory)) != NULL) {
        int path_count = 0;
        strcat(here->path, "\n");
        strcat(here->path, ".");
        strcat(here->path, "\n");
        traverse_directory(current_directory, ".", here, &path_count);
        here->num = path_count;
        printf("%d\n", path_count);
        printf("%s\n", here->path);
    } else
        perror("Error getting current directory");
    
}
 
void connect_with_nm(SS* here){
    struct sockaddr_in add;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock == -1){
        printf("Error creating a socket\n");
        return;
    }
    add.sin_family = AF_INET;
    add.sin_port = htons(NM_PORT);
    add.sin_addr.s_addr = INADDR_ANY;
 
    if(connect(sock, (struct sockaddr*) &add, sizeof(add)) == -1) {
        printf("Error connecting to naming server\n");
        close(sock);
        return;
    }
 
    size_t herre = sizeof(SS);
 
    printf("%ld\n", herre);
 
    if (send(sock, &herre, sizeof(size_t), 0) == -1) {
        perror("Error sending struct size");
        close(sock);
        return;
    }
 
    if(send(sock, here, herre, 0) == -1){
        printf("Error in sending SS data to NM\n");
        close(sock);
        return;
    }
 
    char ack[100];
    recv(sock, ack, 100, 0);
    if(!strcmp(ack, "success"))
        printf("Storage server initialized\n");
    else
        printf("Some error in initializing storage server\n");
    close(sock);
}
 
void* handle_nm(void* arg){
    SS* ss = (SS*) arg;
    struct sockaddr_in add;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock == -1){
        printf("Error creating a socket\n");
        return NULL;
    }
 
    add.sin_family = AF_INET;
    add.sin_port = htons(ss->nm_port);
    add.sin_addr.s_addr = INADDR_ANY;
 
    if (bind(sock, (struct sockaddr*)&add, sizeof(add)) == -1) {
        perror("Error binding to naming server");
        exit(1);
    }
 
    if (listen(sock, 100) == -1) {
        perror("Listening failed");
        exit(1);
    }

    // stores file contents to copy
    char buffer[100000];
    char source[1000];
 
    while(1){
        
        // now we need to recieve a request from naming server.
        struct sockaddr_in temp;
        // Accept incoming connection from the storage server
        size_t pp = sizeof(temp);
        int ss_data = accept(sock, (struct sockaddr*)&temp, (socklen_t*) &pp);
        if (ss_data == -1) {
            // perror("Operation couldn't get completed: Accept failed");
            // exit(1);
            continue;
        }
 
        int x;
        if(recv(ss_data, &x, sizeof(x), 0) > 0){
            // 0 -> create a file
            // 1 -> create a directory
            // 2 -> delete a file 
            // 3 -> delete a directory
            // 4 -> copy a file
            // 5 -> copy a directory
            // 6 -> read 
            // 7 -> write
            // 8 -> get info 
            printf("%d\n", x);
            if(x <= 1){
 
                char mess[200];
 
                if(recv(ss_data, mess, 200, 0) > 0){
                    printf("%s\n", mess);
                    char* token = strtok(mess, "|");
                    char path[200];
                    strcpy(path, token);
                    char name[100];
                    token = strtok(NULL, "|");
                    strcpy(name, token);
                    
                    char fullpath[5000];
                    snprintf(fullpath, sizeof(fullpath), "%s/%s/%s", current_directory, path, name);
 
                    if(!x){
                        FILE *file = fopen(fullpath, "w");
                        if(file == NULL){
                            perror("Error creating file");
                            continue;
                        }
                        fclose(file);
                        printf("Empty file created: %s\n", fullpath);
                    }
                    else{
                        if(mkdir(fullpath, 0777) == 0) 
                            printf("Empty directory created: %s\n", fullpath);
                        else{
                            perror("Error creating directory\n");
                            continue;
                        }
                    }
                    int y = 1;
                    if (send(ss_data, &y, sizeof(y), 0) == -1) {
                        perror("Error sending ack to NM");
                        continue;
                    }
                }
                else{
                    perror("Operation couldn't get completed: Accept failed\n");
                    continue;
                }
            }
            else if(x <= 3){
                char mess[200];
                if(recv(ss_data, mess, 200, 0) > 0){
                    char path[5000];
                    snprintf(path, sizeof(path), "%s/%s", current_directory, mess);
                    if(x == 2){
                        if(remove(path) == 0)
                            printf("File deleted: %s\n", path);
                        else{
                            perror("Error deleting file\n");
                            continue;
                        }
                    }
                    else{
                        if(rmdir(path) == 0)
                            printf("Directory deleted: %s\n", path);
                        else{
                            perror("Error deleting directory\n");
                            continue;
                        }
                    }
                    int y = 1;
                    if (send(ss_data, &y, sizeof(y), 0) == -1) {
                        perror("Error sending ack to NM");
                        continue;
                    }
                }
            }        
            else if(x == 4) {      
                if(recv(ss_data, source, 200, 0) > 0){
                    char fullpath[5000];
                    snprintf(fullpath, sizeof(fullpath), "%s/%s", current_directory, source);    
                    printf("fullpath: %s\n", fullpath);

                    FILE* file = fopen(fullpath, "r");
                    if (file == NULL) {
                        perror("Error opening file for reading");
                        continue;
                    }

                    size_t read_size;

                    // Read and send the file content in chunks
                    while ((read_size = fread(buffer, 1, sizeof(buffer), file)) > 0) {
                        // Send the chunk size
                        if (send(ss_data, &read_size, sizeof(read_size), 0) == -1) {
                            perror("Error sending chunk size to client");
                            fclose(file);
                            break;
                        }
                        printf("chunks: %zu\n", read_size);

                        // Send the buffer content
                        if (send(ss_data, buffer, read_size, 0) == -1) {
                            perror("Error sending file content chunk to client");
                            fclose(file);
                            break;
                        }
                        printf("file content: %s", buffer);
                        // memset(buffer, 0, sizeof(buffer)); // Clear the buffer
                    }

                    // Signal end of file by sending a zero-sized chunk
                    read_size = 0;
                    if (send(ss_data, &read_size, sizeof(read_size), 0) == -1) {
                        perror("Error sending end of file to client");
                    }
                    fclose(file);
                }
            }
            else if (x == 5) {
                char path[1000];
                if (recv(ss_data, path, 200, 0) > 0) {
                    // destination path
                    char fullpath[5000];
                    // Assuming 'current_directory' contains the destination path
                    snprintf(fullpath, sizeof(fullpath), "%s/%s/%s", current_directory, path, source);
                    printf("fullpath: %s\n", fullpath);

                    FILE *file = fopen(fullpath, "w");
                    if (file == NULL) {
                        perror("Error creating file");
                        continue;
                    }
                    fseek(file, 0, SEEK_SET);
                    printf("Empty file created: %s\n", fullpath);

                    // Receive the new file contents
                    char new_content[10000];
                    memset(new_content, 0, sizeof(new_content));
                    size_t received_size;

                    // Receive and write the file content in chunks
                    while ((received_size = recv(ss_data, new_content, sizeof(new_content), 0)) > 0) {
                        // if(new_content == NULL) break;
                        fprintf(file, "%s", new_content);
                        memset(new_content, 0, sizeof(new_content));
                    } 

                    if (received_size < 0) {
                        perror("Error receiving data");
                    }

                    fflush(file);
                    fclose(file);
                    printf("File content replaced successfully.\n");
                }
            }
        }
    }
    close(sock);
    return NULL;
}

 // concurrency for client connections
void* processClient(void* client_data_ptr) {
    int client_data = *((int*)client_data_ptr);
    
    while(1){
        // printf("Client socket connection accepted\n");
 
        int operation;
        char path[1024];
        
        if(recv(client_data, &operation, sizeof(operation), 0) > 0){
            printf("operation: %d\n", operation);
            // 6 --> read a file
            // 7 --> write a file
            // 8 --> get file size and permissions
 
            // Receive the file path
            if (recv(client_data, path, sizeof(path), 0) == -1) {
                perror("Error receiving file path from client");
                continue;
            }
            printf("path: %s\n", path);
            char fullpath[10000];
            snprintf(fullpath, sizeof(fullpath), "%s/%s", current_directory, path);
            printf("fullpath: %s\n", fullpath);
 
            if (operation == 6) {
                printf("OPERATION: READ\n");
                // read a file
                FILE* file = fopen(fullpath, "r");
                if (file == NULL) {
                    perror("Error opening file for reading");
                    continue;
                }
 
                char buffer[10000];
                size_t read_size;

                // Read and send the file content in chunks
                while ((read_size = fread(buffer, 1, sizeof(buffer), file)) > 0) {
                // Send the chunk size
                    if (send(client_data, &read_size, sizeof(read_size), 0) == -1) {
                        perror("Error sending chunk size to client");
                        fclose(file);
                        break;
                    }

                    // Send the buffer content
                    if (send(client_data, buffer, read_size, 0) == -1) {
                        perror("Error sending file content chunk to client");
                        fclose(file);
                        break;
                    }

                    memset(buffer, 0, sizeof(buffer)); // Clear the buffer
                }
                printf("%s\n", buffer);

                // Signal end of file by sending a zero-sized chunk
                read_size = 0;
                if (send(client_data, &read_size, sizeof(read_size), 0) == -1) {
                    perror("Error sending end of file to client");
                }

                fclose(file);
            }
            else if (operation == 7){
                // write a file
 
                // Receive the new file contents
                char new_content[10000];
                memset(new_content, 0, sizeof (new_content));
                size_t received_size;
 
                FILE* file = fopen(fullpath, "w");
                if (file == NULL) {
                    perror("Error opening file for writing");
                    continue;
                }
                fseek(file, 0, SEEK_SET);
                
                // Receive and write the file content in chunks
                if(recv(client_data, new_content, sizeof(new_content), 0) == 1){
                    fflush(file);
                    fclose(file);
                    perror("Error in reciving the data");
                    continue;
                }
 
                fprintf(file, "%s", new_content);
                printf("\n%s", new_content);
                
                fflush(file);
                fclose(file);
                printf("File content replaced successfully.\n");
            }
            else if (operation == 8){
                // get file size
                printf("GET INFO OPERATION\n");
 
                FILE* file = fopen(fullpath, "r");
                if (file == NULL) {
                    printf("File not found");
                    exit(1);
                }
 
                fseek(file, 0L, SEEK_END);
                long int file_size = ftell(file);
                fclose(file);
                if (send(client_data, &file_size, sizeof(file_size) + 1, 0) == -1) {
                    perror("Error sending file size to Storage Server");
                    continue;
                }

                const char* permission_str = perms(fullpath);
                size_t ack_length = strlen(permission_str) + 1;
                if (send(client_data, &ack_length, sizeof(ack_length), 0) == -1) {
                    perror("Error sending ack length to client");
                    continue;
                }
                printf("%s\n", permission_str);
                if (send(client_data, permission_str, ack_length, 0) == -1) {
                    perror("Error sending acknowledgement to client");
                    continue;
                }
                printf("%s\n", permission_str);
                // Get file permissions
                // struct stat file_stat;
                // if (stat(fullpath, &file_stat) == -1) {
                //     perror("Error getting file information");
                //     continue;
                // }
                
                // uint16_t pp = (uint16_t) file_stat.st_mode;
                
                // // Send file permissions as an integer to Storage Server
                // printf("permissions: %u", pp);
                // if (send(client_data, &pp, sizeof(pp), 0) == -1) {
                //     perror("Error sending file permissions to Storage Server");
                //     continue;
                // }
                
 
                char stop_packet[] = "STOP";
                if (send(client_data, stop_packet, strlen(stop_packet) + 1, 0) == -1) {
                    perror("Did not send STOP packet to client\n");
                    continue;
                }
            } 

        }
    }
    close(client_data);
    return NULL;
}
 
 void* handle_client(void* arg){
    SS* ss = (SS*) arg;
    struct sockaddr_in add;
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(client_socket == -1){
        printf("Error creating a socket\n");
        return NULL;
    }
    add.sin_family = AF_INET;
    add.sin_port = htons(ss->client_port);
    add.sin_addr.s_addr = INADDR_ANY;
 
    if (bind(client_socket, (struct sockaddr*)&add, sizeof(add)) == -1) {
        perror("Error binding to client");
        exit(1);
    }
 
    if (listen(client_socket, 100) == -1) {
        perror("Listening failed");
        exit(1);
    }
    while(1){
        struct sockaddr_in temp;
        // Accept incoming connection from the storage server
        int client_data = accept(client_socket, (struct sockaddr*)&temp, (socklen_t*)&temp);
        if (client_data == -1) {
            perror("Accept failed");
            continue;
        }
 
        pthread_t client_thread;
        pthread_create(&client_thread, NULL, (void*)processClient, (void*)&client_data);
        pthread_detach(client_thread);
    }
    close(client_socket);
    return NULL;
}
 
int main(){
    SS ss;
    fill_values(&ss);
    connect_with_nm(&ss);
    // we need to make two threads
    // one which will listen from ss.nm_port
    // one which will listen from ss.client_port
    pthread_t threads[2];
 
    pthread_create(&threads[0], NULL, handle_nm, (void*) &ss);
    pthread_create(&threads[1], NULL, handle_client, (void*) &ss);
 
    pthread_join(threads[0], NULL);
    pthread_join(threads[1], NULL);
 
    return 0;
}
