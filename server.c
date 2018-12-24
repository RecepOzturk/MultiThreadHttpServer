#include <stdio.h>
#include <string.h>    
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h> 
#include <unistd.h>    
#include <pthread.h>   
#include <semaphore.h>
#include  <unistd.h>
#include <fcntl.h>

#define path "/home/cme2002/Desktop/b/server/"  
#define PORT 8080

sem_t lock;  //mutex semaphore for processing each connection from each thread

void *connection_handler(void *socket_desc)
{
    //Get the socket descriptor
   
    sem_wait(&lock);

    int sock = *((int*)socket_desc); //socket descriptor
    int temp, recv_size;
    char recv_buf[512];
    char *message, *error, pathh[100];
    char file_name[100];

    strcpy(pathh, path);  //path which was defined above for index.html, copy a char variable

    temp = (recv (sock, recv_buf, sizeof(recv_buf), 0));  //Take off request 
    if (temp < 0){     
        perror("Receive Error");
        //exit(errno);
    }
    temp = temp/sizeof(char);
    recv_buf[temp-2] = '\0';


    if (strncmp(recv_buf, "GET", 3)==0 || strncmp(recv_buf, "HEAD", 4) == 0)  //if request includes then process the req.
    {
        puts(recv_buf);  //

        recv_size= strlen(recv_buf);
        int i;
        int j = 0;   
        for (i=4; i<recv_size ;i++, j++) {  //request parsing to take file that was demanded by client
            if ( (recv_buf[i] == '\0') || (recv_buf[i] == '\n') || (recv_buf[i] == ' ') ){
                break;
            }
            else if ( recv_buf[i] == '/') {                    
                --j;
            }
            else {
                file_name[j] = recv_buf[i];             
                
            }
        }   
        file_name[j] = '\0';   //file name that was demanded by client

        strcat(pathh, file_name);      //concatenate file name and path (defined)                  
        
        if (strstr(pathh, "html")) 
        {
            char source[1024];
            FILE *fp = fopen(pathh, "r");  //try to read .html file
            if (fp != NULL) {              // if there is exist file demanded the read
                size_t newLen = fread(source, sizeof(char), 1024, fp);
                if ( ferror( fp ) != 0 ) {
                    fputs("Error reading file", stderr);
                } else {
                    source[newLen++] = '\0';
                }

                fclose(fp);
                send(sock, source, strlen(source),0 ); //send file to client 
            }
            else{  // if there isn't exist file then error message 404 type header
                message = "HTTP/1.0 404 not found \r\n content-type: text" ;
                send(sock, message, strlen(message),0 );
            }
        }
        else if(strstr(pathh, "jpg") || strstr(pathh, "jpeg")){  //try to read image file
            int img_file_descriptor;
            img_file_descriptor = open(pathh, O_RDONLY);
            if (img_file_descriptor!=-1)
            {
                sendfile(sock, img_file_descriptor, NULL, (unsigned long)131072);
            }
            else{
                message = "HTTP/1.0 404 not found \r\n content-type: text" ;
                send(sock, message, strlen(message),0 );
            }
        }
        else if(strstr(pathh, "favicon")) { //if there is favicon request then do anything
            ;;
        }
        else { // if request includes except html or jpg the send error not acceptable 
                message = "HTTP/1.0 406 not acceptable\r\n content-type: text";
                send(sock, message, strlen(message),0 );
        } 
    } 
    else {  //except HEAD or GET
        error = "HTTP/1.0 405 Method not allowed\r\n content-type: text";
        if(send(sock,error, strlen(error),0) < 0)
        {
            puts("Send failed"); 
        }
    }

    
    free(socket_desc); //Free the socket pointer   
    close(sock); 

    sem_post(&lock);
    pthread_exit(NULL);
    return 0;
    
}
int main(int argc, char *argv[])
{
    int socket_desc, new_socket, c, *new_sock;
    struct sockaddr_in server, client;
    char *message;
    sem_init(&lock,0,1);
    
    memset(&server, 0, sizeof(server));

    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        puts("Could not create socket");
        return 1;
    }
     
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);
     
    if(bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        puts("Binding failed");
        return 1;
    }
     
    listen(socket_desc, 10);
     
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);
    
    while(new_socket = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c) )
    {
        pthread_t sniffer_thread;
        new_sock = malloc(sizeof(*new_sock));
        *new_sock = new_socket;
         
        if(pthread_create(&sniffer_thread, NULL, connection_handler, (void*)new_sock) < 0)
        {
            puts("Could not create thread");
        }
    }
     
    return 0;
}
