#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <utmp.h>
#include <sys/socket.h>

#define READ 0
#define WRITE 1
#define PATH "client.txt"
#define PATH2 "server.txt"

int main()
{
     char *client_server = PATH;
     mknod(client_server, S_IFIFO | 0666, 0);

     char *server_client = PATH2;
     mknod(server_client, S_IFIFO | 0666, 0);

     int fd;
     int fd2;

     fd = open(client_server, O_RDONLY);
     fd2 = open(server_client, O_WRONLY);

     char buff[100];
     int result = read(fd, buff, 300);
     if (result == -1)
     {
          perror("eroare la citirea din fifo!\n");
     }
     else
     {
          buff[result] = '\0';
     }

     int logged = 0;

     while (result != -1)
     {
          // citesc ce mi s-a scris in fifo
          printf("{debug}s-au citit din FIFO %d bytes, mesajul %s \n", result, buff);

          // PRIMUL TASK
          // LOGIN : USERNAME
          if (strstr(buff, "login") != 0)
          {
               // i want to log in
               char username[256];
               int j = 0;

               strcpy(username, buff + 8);

               printf("{debug}username:%s strlen: %ld\n", username, strlen(username));

               int pipe1[2];
               pipe(pipe1);
               pid_t pid = fork();

               if (pid < 0)
               {
                    perror("Eroare la fork()!");
                    exit(1);
               }

               if (pid == 0)
               { // i m in child
                    close(pipe1[READ]);

                    int fd3 = open("usernames.txt", O_RDONLY);

                    if (fd3 < 0)
                    {
                         perror("Nu pot accesa fisierul!\n");
                         exit(1);
                    }

                    char usernames_file[256];
                    int usernames_file_len = read(fd3, usernames_file, 256);
                    
                    usernames_file[usernames_file_len] = '\0';
                    
                    char *p = strtok(usernames_file, " ");
                    int found = 0;
                    while (p && found == 0)
                    {
                         if (strcmp(p, username) == 0)
                         {
                              found = 1;
                         }
                         printf("{debug}p:%s\n", p);
                         p = strtok(NULL, " ");
                    }
                    printf("%d\n", found);
                    
                    close(fd3);

                    if (found == 1)
                    {
                         write(pipe1[WRITE], "USER LOGGED IN", strlen("USER LOGGED IN"));
                    }
                    else
                    {
                         write(pipe1[WRITE], "USER NOT LOGGED IN", strlen("USER NOT LOGGED IN"));
                    }
                    exit(0);
               }
               else
               {
                    // i m in the parent
                    char response[256];
                    int response_len = read(pipe1[READ], response, 256);
                    response[response_len] = '\0';
                    printf("{debug}i m the parent and i have the response: %s\n", response);

                    if (strcmp(response, "USER LOGGED IN") == 0)
                    {
                         logged = 1;
                    }

                    printf("{debug}}ogged: %d\n", logged);

                    write(fd2, response, response_len);
                    wait(NULL);
               }
          }

          // AL DOILEA+TREILEA+PATRULEA TASK doar daca logged = 1
          if (logged == 1)
          {
               // GET-LOGGED-USERS
               if (strcmp(buff, "get-logged-users") == 0)
               {
                    printf("{debug}get-logged-users\n");
                    int pipe2[2];
                    pipe(pipe2);

                    pid_t pid2 = fork();

                    if (pid2 < 0)
                    {
                         perror("Eroare la fork()!\n");
                         exit(1);
                    }

                    if (pid2 == 0)
                    {
                         // i m in child
                         close(pipe2[READ]);
                         printf("{debug}i m in child no 2!\n");
                         struct utmp *data;
                         char aux[50];
                         char aux2[50];
                         char data_to_give_parent[1000];
                         data_to_give_parent[0] = '\0';
                         int i;
                         char sec[100];
                         data = getutent();
                         // getutent() populates the data structure with information
                         while (data != NULL)
                         {
                              strncpy(aux, data->ut_user, 32);
                              // aux[32] = '\0';
                              strncpy(aux2, data->ut_host, 32);
                              // aux2[32] = '\0';
                              strcat(data_to_give_parent, aux);
                              strcat(data_to_give_parent, " ");
                              strcat(data_to_give_parent, aux2);
                              strcat(data_to_give_parent, " ");
                              i = data->ut_tv.tv_sec;
                              sprintf(sec, "%d", i);
                              strcat(data_to_give_parent, sec);
                              strcat(data_to_give_parent, "\n");
                              data = getutent();
                         }
                         write(pipe2[WRITE], data_to_give_parent, strlen(data_to_give_parent));
                         exit(0);
                    }
                    else
                    {
                         // i m in parent
                         close(pipe2[WRITE]);

                         char response_utmp[1000];
                         int response_utmp_len = read(pipe2[READ], response_utmp, 1000);
                         response_utmp[response_utmp_len] = '\0';

                         write(fd2, response_utmp, response_utmp_len);

                         wait(NULL);
                    }
               }
               else
               {
                    if (strstr(buff, "get-proc-info") != 0)
                    {
                         printf("{debug}i want the informations!\n");

                         char my_pid[100];
                         strcpy(my_pid, buff + 16);
                         printf("{debug}my pid is: %s\n", my_pid);

                         char file_with_info[256];
                         file_with_info[0] = '\0';

                         strcat(file_with_info, "/proc/");
                         strcat(file_with_info, my_pid);
                         strcat(file_with_info, "/status");
                         printf("{debug}my file with info is: %s\n", file_with_info);

                         char info_for_parent[4096];
                         info_for_parent[0] = '\0';

                         int socket[2];
                         // child writes      parent reads

                         static const int parentsocket = 0;
                         static const int childsocket = 1;

                         // call socketpair
                         socketpair(PF_LOCAL, SOCK_STREAM, 0, socket);

                         pid_t pid3;
                         pid3 = fork();
                         
                         if (pid3 < 0)
                         {
                              perror("Eroare la fork()!\n");
                              exit(1);
                         }

                         if (pid3 == 0)
                         {
                              // i m in the child
                              close(socket[parentsocket]);

                              FILE *fd4 = fopen(file_with_info, "r");
                              if (fd4 == NULL)
                              {
                                   perror("Nu pot deschide fisierul!\n");
                                   exit(1);
                              }

                              char *line = NULL;
                              size_t len = 0;

                              while (getline(&line, &len, fd4) != -1)
                              {
                                   if (strstr(line, "Name") != 0)
                                   {
                                        strcat(info_for_parent, line);
                                        strcat(info_for_parent, "\n");
                                   }

                                   if (strstr(line, "State") != 0)
                                   {
                                        strcat(info_for_parent, line);
                                        strcat(info_for_parent, "\n");
                                   }

                                   if(strstr(line, "PPid") != 0){
                                        strcat(info_for_parent, line);
                                        strcat(info_for_parent, "\n");
                                   }

                                   if(strstr(line, "Uid") != 0){
                                        strcat(info_for_parent, line);
                                        strcat(info_for_parent, "\n");
                                   }

                                   if(strstr(line, "VmSize") != 0){
                                        strcat(info_for_parent, line);
                                        strcat(info_for_parent, "\n");
                                   }

                              }

                              printf("{debug} %s\n", info_for_parent);

                              //send the data to parent 
                              write(socket[childsocket], info_for_parent, strlen(info_for_parent));

                              exit(0);
                         }
                         else
                         {
                              // i m the parent
                              close(socket[childsocket]);

                              char read_data[4096];
                              int read_data_len = read(socket[parentsocket], read_data, 4096);
                              read_data[read_data_len] = '\0';

                              write(fd2, read_data, read_data_len);

                              wait(NULL);
                         }
                    }
                    else
                    {
                         if (strcmp(buff, "logout") == 0)
                         {
                              logged = 0;
                              printf("{debug}Nu ai acces la date!\n");
                              int res = write(fd2, "Delogat cu succes!\n", strlen("Delogat cu succes!\n"));
                              if (res == -1)
                              {
                                   perror("problema la scriere in fifo2\n");
                              }
                         }
                    }
               }
          }
          else
          {
               if (logged == 0)
               {
                    printf("{debug}Nu ai acces la date!\n");
                    int res = write(fd2, "Nu ai acces la date!\n", strlen("Nu ai acces la date!\n"));
                    if (res == -1)
                    {
                         perror("problema la scriere in fifo2\n");
                    }
               }
          }

          if (strstr(buff, "quit") != 0)
          {
               printf("{debug} Clientul a iesit fortat!\n");
               int res = write(fd2, "Clientul a iesit fortat!\n", strlen("Clientul a iesit fortat!\n"));
               if (res == -1)
               {
                    perror("problema la scriere in fifo2\n");
               }
               exit(0);
          }

          // vreau sa citesc iar
          // char buff[100];
          int result = read(fd, buff, 300);
          if (result == -1)
          {
               perror("eroare la citirea din fifo!\n");
          }
          else
          {
               buff[result] = '\0';
          }
     }
     close(fd);
     close(fd2);
}
