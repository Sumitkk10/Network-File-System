# Network File System - team_025

![](https://blog.eduonix.com/wp-content/uploads/2018/04/HDFS-Architecture.png)

### Introduction:

Welcome to our team project focused on implementing a Network File System (NFS). In this endeavor, we aim to create a robust and efficient system that allows users to seamlessly interact with files and folders over a network. The NFS architecture consists of three major components: Clients, Naming Server, and Storage Servers.

### Components:

**Clients:**
Clients are the end-users or systems that initiate file-related operations. They play a pivotal role in interacting with the NFS, performing actions such as reading, writing, deleting, and more. Clients act as the primary interface, making file system operations accessible to users.

**Naming Server:**
The Naming Server acts as a central hub in the NFS architecture. It facilitates communication between clients and storage servers, providing essential information about the location of files or folders. Essentially, it functions as a directory service, ensuring efficient and accurate file access within the network.

**Storage Servers:**
Storage Servers form the foundation of the NFS, managing the physical storage and retrieval of files and folders. They are responsible for data persistence and distribution across the network, ensuring secure and efficient file storage.

### File Operations:

Within the NFS ecosystem, clients have access to a suite of essential file operations:

**Writing a File/Folder:** Actively create and update the content of files and folders within the NFS, ensuring a dynamic repository.
The syntax for writing is:
``` 
    WRITE
    <file_path>
    <data>
```

**Reading a File:** Retrieve the contents of files stored within the NFS, granting clients access to the information they seek.
The syntax for writing is:
``` 
    READ
    <file_path>
```

**Deleting a File/Folder:** Remove files and folders from the network file system when no longer needed, contributing to efficient space management.
```
    DELETE FILE
    <file_path>
```
or
```
    DELETE DIR
    <dir_path>
```

**Creating a File/Folder:** Generate new files and folders, facilitating the expansion and organization of the file system.
```
    CREATE FILE
    <file_path>
    <file_name>
```
or
```
    CREATE DIR
    <dir_path>
    <dir_name>
```


**Getting additional information:** Access supplementary information about specific files, including details such as file size, access rights, timestamps, and other metadata.

To run this NFS, run the following in three different terminals:

```gcc naming_server.c LRU.c hash.c -o a```

```gcc <absolute path of storage server.c> -o b```

```gcc client.c -o c```