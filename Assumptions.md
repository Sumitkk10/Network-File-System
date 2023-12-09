# Assumptions:

1. IP addresses of all storage servers remains same and is same as Naming server's IP.

2. All file /paths are unique.

3. Number of accessible paths in a storage server will be less than 100000.

4. For the file count.txt (which stores number of SS till now), update the line in storage_server.c with the absoulte path of connt.txt.

5. File paths never contain comma.

6. Deleting a File / Directory
    Input constraints: 
    - Deletion of a file/directory in the root directory of an SS that existed before initialization will not require "./" 
    - Deletion of a file/directory in the root directory of an SS that existed after initialization will require "./"

7. Size of LRU Cache is kept as 20

8. Copy/Read file operation will be performed on very small size text files, so that data can be sent in one go.

9. Write will be done only in file.

10. To delete directory, everything inside should be deleted.
