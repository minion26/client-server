#include<stdio.h>    
#include<stdlib.h> 
#include<unistd.h> 
#include<sys/types.h> 
#include<string.h> 
#include<sys/wait.h> 
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>

#define READ 0
#define WRITE 1 
#define PATH "client.txt"
#define PATH2 "server.txt"

int main(){
   char * client_server = PATH;
   mknod(client_server, S_IFIFO|0666,0);

   char * server_client = PATH2;
   mknod(server_client, S_IFIFO|0666, 0);

   int fd;
   int fd2;

   fd = open(client_server, O_WRONLY);
   fd2 = open(server_client, O_RDONLY);

   char input[256];
   
   input[0] = '\0';
   int input_len = read(0, input, 256);
   input[input_len-1] = '\0';
  
   while(strcmp(input,"quit")!=0){
      //scriu in fifo ce mi s-a dat de la tastatura
      
      int result = write(fd, input, strlen(input));
      if(result == -1){
        perror("problema la scriere in fifo\n");
      }
      // else{
      //   printf("s-a scris in fifo %d bytes, mesajul %s\n", result, input);
      // }

      //citesc din fifo ce mi s-a scris
      char buff[100];
      int res = read(fd2, buff, 300);
      if(res == -1){
        perror("eroare din citirea din fifo2\n");
      }
      else{
        buff[res] = '\0';
        printf("raspuns: %s\n", buff);
      }

      //vreau sa scriu iar in fifo 
      int input_len = read(0, input, 256);
      input[input_len-1] = '\0';
   }

   close(fd);
   close(fd2);
}
