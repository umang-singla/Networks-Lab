#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h> 

// function to validate the user
int validate(char *user, char *user_file){
    FILE *fp;
    fp = fopen(user_file, "r");
    char temp[25];
    // reading the file line by line
    while (fscanf(fp,"%s", temp)!=EOF)
    {
        // if the user is found return 1
        if(strcmp(user, temp) == 0){
            fclose(fp);
            return 1;
        }
    }
    
    return 0;
}

// function to receive a line from the socket
char* read_line(int socketid){
    // allocate memory for the string
    char *res = (char *)malloc(sizeof(char));
    int len = 0, size = 0; // len is the length of the string, size is the size of the allocated memory
    // while the string is not null terminated recieve data from the socket
    while (len == 0 || (len > 0 && res[len-1] != '\0'))
    {
        // temp array to recieve data from the socket
        char temp[50];
        // recieve data from the socket
        int len2 = recv(socketid, temp, 50, 0);
        int i;
        // copy the data from temp to res
        for(i=0;i<len2;i++){
            if(len  + i == size){
                if(size == 0) size = 1;
                else size *= 2;
                res = (char *)realloc(res, size);
            }
            res[len+i] = temp[i];
        }
        len = len+len2;      
    }
    
    // return the string
    return res;
}

// function to send a line to the socket
void send_line(char *line, int socketid){
    char buffer[50];
    int len = strlen(line);
    // send the data in chunks of 50 bytes
    for(int i=0;i<len;i++){
        buffer[i%50] = line[i];
        if(i%50 == 49) send(socketid, buffer, 50, 0); // sending data when the buffer is full
    }
    // sending the remaining data
    buffer[len%50] = '\0';
    while (len%50 != 0)
    {
        buffer[len%50] = '\0';
        len++;
    }
    send(socketid, buffer, strlen(buffer) + 1, 0);   

}



int main(){
    int socketid;
    struct sockaddr_in server_addr, client_addr;

    int i;
    char buffer[50000]; // buffer to store the date and time

    // creating a socket
    if ((socketid = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Cannot create socket\n");
		exit(0);
	}

    // providing the server address
    server_addr.sin_family		= AF_INET;
	server_addr.sin_addr.s_addr	= INADDR_ANY;
	server_addr.sin_port		= htons(20000);

    // binding the socket to the server address
    if (bind(socketid, (struct sockaddr *) &server_addr,sizeof(server_addr)) < 0) {
		perror("Unable to bind local address\n");
		exit(0);
	}

    // listening for connections
    listen(socketid, 5);
    printf("Listening for requests...\n");

    char user_file[100];
    getcwd(user_file, sizeof(user_file));
    strcat(user_file, "/users.txt");

    // Iterative server that handles one client at a time
    for(;;){
        int clilen = sizeof(client_addr);
        // accepting a connection
        int new_socketid = accept(socketid, (struct sockaddr *) &client_addr, &clilen);

        if (new_socketid < 0) {
			perror("Accept error\n");
			exit(0);
		}

        if(fork() == 0){
            strcpy(buffer, "LOGIN:");
            send_line(buffer, new_socketid);

            char *user = read_line(new_socketid);

            // if the user is valid send FOUND to the client
            if(validate(user, user_file)){
                strcpy(buffer, "FOUND");
                send_line(buffer, new_socketid);
                // executing the recieved bash command on the server until exit recieved
                while(1){
                    // reading the command from the client
                    char *res = read_line(new_socketid);
                    // fetching the type of command
                    char *cmd = strtok(res, " ");
                    if(strcmp(cmd, "pwd") == 0){
                        // sending the current working directory to the client
                        getcwd(buffer, sizeof(buffer));
                        send_line(buffer, new_socketid);
                    }else if(strcmp(cmd, "cd") == 0){
                        // changing the current working directory
                        char *path = strtok(NULL, "\0");
                        int err;
                        // if no path is provided change to home directory
                        if(path == NULL || strcmp(path, "~") == 0) err = chdir(getenv("HOME"));
                        else err = chdir(path);
                        getcwd(buffer, sizeof(buffer));
                        // sending the current working directory to the client
                        if(err == 0) send_line(buffer, new_socketid);
                        else{
                            strcpy(buffer, "####");
                            send_line(buffer, new_socketid);
                        }
                    }else if(strcmp(cmd, "dir") == 0){
                        struct dirent *de;  // Pointer for directory entry 
        
                        // opendir() returns a pointer of DIR type.  
                        char *path = strtok(NULL, "\0");
                        DIR *dr;
                        if(path == NULL) dr = opendir("."); 
                        else if(strcmp(path, "~") == 0) dr = opendir(getenv("HOME"));
                        else dr = opendir(path);
                    
                        if (dr == NULL)  // opendir returns NULL if couldn't open directory 
                        { 
                            strcpy(buffer, "####");
                            send_line(buffer, new_socketid);
                        }else{

                            memset(buffer, 0, sizeof(buffer));
                            char temp[300];
                            // sending the list of files in the directory
                            while ((de = readdir(dr)) != NULL){
                                sprintf(temp, "%s\n", de->d_name);
                                strcat(buffer, temp);
                            }

                            closedir(dr); 
                            send_line(buffer, new_socketid);
                        }
                    }else if(strcmp(cmd, "exit") == 0){
                        // exiting the server
                        strcpy(buffer, "exit");
                        send_line(buffer, new_socketid);
                        break;
                    }else{
                        // sending invalid command to the client
                        strcpy(buffer, "$$$$");
                        send_line(buffer, new_socketid);
                    }
                }  
            }else{
                strcpy(buffer, "NOT-FOUND");
                send_line(buffer, new_socketid);
            }

            free(user);

            // closing the connection with the client
            close(new_socketid);
            // exiting the process
            exit(0);
        }

        // closing the connection with the client
		close(new_socketid);
    }

    return 0;
}