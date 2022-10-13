#include<stdio.h> 
#include<stdlib.h> 
#include<unistd.h> 
#include<sys/types.h> 
#include<string.h> 
#include<sys/wait.h> 
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <utmp.h>

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

   fd = open(client_server, O_RDONLY);
   fd2 = open(server_client, O_WRONLY);

   char buff[100];
   int result = read(fd, buff, 300);
   if(result == -1){
        perror("eroare la citirea din fifo!\n");
   }
   else{
        buff[result]='\0';
   }

   int logged = 0;

     while(result!=-1){
     //citesc ce mi s-a scris in fifo
     printf("s-au citit din FIFO %d bytes, mesajul %s \n", result, buff);

     //PRIMUL TASK
     if(strstr(buff,"login")!=0){
          //i want to login in
          char username[256];
          int j = 0;
          // for(int i = 8; i<strlen(buff); i++){
          //      username[j] = buff[i];
          //      j++;
          // }
          strcpy(username, buff+8);
          //printf("j:%d\n", j);
          //imi apar caractere din memorie cand username < 8
          // for (int i = j; i < 8; i++){
          //      username[i]='\0';
          // }
          
          printf("username:%s strlen: %d\n", username, strlen(username));

          int pipe1[2];
          pipe(pipe1);
          pid_t pid = fork();

          if(pid == 0){//i m in child
               close(pipe1[READ]);

               int fd3 = open("usernames.txt", O_RDONLY);
               char usernames_file[256];
               int usernames_file_len = read(fd3, usernames_file, 256);
               //printf("bytes %d\n", usernames_file_len);
               usernames_file[usernames_file_len] = '\0';
               //printf("%s\n", usernames_file);
               char *p = strtok(usernames_file, " ");
               int found = 0;
               while(p && found == 0){
                    if(strcmp(p, username)==0){
                         found = 1;
                    }
                    printf("p:%s\n", p);
                    p = strtok(NULL, " ");
               }
               printf("%d\n", found);
               // printf("i m in child!\n");
               // printf("username from child:%s\n", username);
               close(fd3);

               if(found == 1){
                    write(pipe1[WRITE], "USER LOGGED IN", strlen("USER LOGGED IN"));
               }
               else{
                    write(pipe1[WRITE], "USER NOT LOGGED IN", strlen("USER NOT LOGGED IN"));
               }
               exit(0);
          }
          else{
               //i m in the parent
               char response[256];
               int response_len = read(pipe1[READ], response, 256);
               response[response_len] = '\0';
               printf("i m the parent and i have the response: %s\n", response);

               if(strcmp(response, "USER LOGGED IN")==0){
                    logged = 1;
               }

               printf("logged: %d\n", logged);

               write(fd2, response, response_len);
               wait(NULL);
          }


     }

     //AL DOILEA+TREILEA+PATRULEA TASK doar daca logged = 1
     if(logged == 1){
          if(strcmp(buff, "get-logged-users") == 0){
               printf("get-logged-users\n");
               int pipe2[2];
               pipe(pipe2);

               pid_t pid2 = fork();

               if(pid2 == 0){
                    //i m in child
                    close(pipe2[READ]);
                    printf("i m in child no 2!\n");
                    struct utmp* data;
                    char aux[50];
                    char aux2[50];
                    char data_to_give_parent[1000];
                    int i = 0;
                    data = getutent();
                    //getutent() populates the data structure with information 
                    while(data != NULL )
                    {
                         strncpy(aux, data->ut_user, 32);
                         //aux[32] = '\0';
                         strncpy(aux2, data->ut_host, 32);
                         //aux2[32] = '\0';
                         strcat(data_to_give_parent, aux);
                         strcat(data_to_give_parent, " ");
                         strcat(data_to_give_parent, aux2);
                         strcat(data_to_give_parent, " ");
                         //strcat(data_to_give_parent, data->ut_time+"0");
                         strcat(data_to_give_parent, "\n");
                         data = getutent();
                    }
                    write(pipe2[WRITE], data_to_give_parent, strlen(data_to_give_parent));
                    exit(0);
               }else{
                    //i m in parent
                    close(pipe2[WRITE]);

                    char response_utmp[1000];
                    int response_utmp_len = read(pipe2[READ], response_utmp, 1000);
                    response_utmp[response_utmp_len] = '\0';

                    write(fd2, response_utmp, response_utmp_len);

                    wait(NULL);
               }
          }
     
     

     }
     //scriu eu ceva in fifo de la tastatura
     // char input[100];
     // scanf("%s", input);
     // int res = write(fd2, input, strlen(input));
     // if(res == -1){
     //     perror("problema la scriere in fifo2\n");
     // }
     // else{
     //     printf("s-a scris in fifo2 %d bytes, mesajul: %s\n", result, input);
     // }

     //vreau sa citesc iar 
     //char buff[100];
     int result = read(fd, buff, 300);
     if(result == -1){
          perror("eroare la citirea din fifo!\n");
     }
     else{
          buff[result]='\0';
     }
   }
     close(fd);
     close(fd2);

}
