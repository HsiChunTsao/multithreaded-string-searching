#include "client.h"
#include<netinet/in.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<stdio.h>
#include<stdlib.h>
#include<dirent.h>
#include<string.h>
#include<pthread.h>

char *strsep(char **stringp, const char *delim)
{
    char *begin, *end;
    begin = *stringp;
    if(begin == NULL)
        return NULL;
    end = begin + strcspn(begin, delim);
    if(*end)
    {
        *end++ = '\0';
        *stringp = end;
    }
    else
    {
        *stringp = NULL;
    }
    return begin;
}

char *strdup(const char *s)
{
    size_t len = strlen(s) + 1;
    void *new = malloc(len);

    if(new == NULL)
        return NULL;

    return (char *)memcpy(new, s, len);
}

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
int client_socket;


int check(char *str)
{
    int i;
    char delim[] = "\"";
    char *dupstr = strdup(str);
    char *sepstr = dupstr;
    char *substr = NULL;
    int str_count = 0;
    int count = 0;

    if(str[0]!='Q' || str[1]!='u' || str[2]!='e' || str[3]!='r' || str[4]!='y' || str[5]!=' ' || str[6]!='\"')
    {
        return 0;
    }

    for(i=0; str[i]!='\0'; i++)
    {
        if(str[i] == '\"')	count++;
    }
    if(count%2 != 0)
    {
        return 0;
    }

    substr = strsep(&sepstr, delim);

    do
    {
        if(strcmp(substr, " ") != 0)
        {
            str_count++;
        }
        substr = strsep(&sepstr, delim);
    }
    while(substr);
    free(dupstr);
    if(count/2 != (str_count-2))
    {
        return 0;
    }

    return 1;
}

void *send_req(void *substr)
{
    pthread_mutex_lock(&mutex1);
    if(send(client_socket, substr, 4096, 0) < 0)
    {
        printf("send fail\n");
    }
    pthread_mutex_unlock(&mutex1);

    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    char localhost[64];
    int port;
    int i,ch;
    char delim[] = "\"";
    pthread_t thread_pool[5];
    void *ret;

    // LOCALHOST
    if(strcmp(argv[1], "-h") == 0)
    {
        strncpy(localhost, argv[2], strlen(argv[2]));
    }
    else if(strcmp(argv[3], "-h") == 0)
    {
        strncpy(localhost, argv[4], strlen(argv[4]));
    }

    // PORT
    if(strcmp(argv[1], "-p") == 0)
    {
        port = atoi(argv[2]);
    }
    else if(strcmp(argv[3], "-p") == 0)
    {
        port = atoi(argv[4]);
    }



    while(1)
    {
        // set socket's address info
        struct sockaddr_in client_addr;
        bzero(&client_addr, sizeof(client_addr));
        client_addr.sin_family = AF_INET;
        client_addr.sin_addr.s_addr = htons(INADDR_ANY);
        client_addr.sin_port = htons(0);

        // creat a stream socket
        client_socket = socket(AF_INET, SOCK_STREAM, 0);
        if(client_socket < 0)
        {
            printf("creat socket failed\n");
            exit(1);
        }

        // bind
        if(bind(client_socket, (struct sockaddr*)&client_addr, sizeof(client_addr)))
        {
            printf("client bind port failed\n");
            exit(1);
        }

        struct sockaddr_in server_addr;
        bzero (&server_addr, sizeof(server_addr));
        server_addr.sin_family = AF_INET;

        if(inet_aton(localhost, &server_addr.sin_addr) == 0)
        {
            printf("server IP address error\n");
            exit(1);
        }
        server_addr.sin_port = htons(port);
        socklen_t server_addr_length = sizeof(server_addr);

        if(connect(client_socket, (struct sockaddr*)&server_addr, server_addr_length) < 0)
        {
            printf("cannot connect to %s\n", localhost);
            exit(1);
        }

        char buffer[4096];
        gets(buffer);

        /////////////////////
        ch = check(buffer);
        if(ch == 0)
        {
            printf("The strings format is not correct\n");
        }
        else
        {
            send(client_socket, buffer, 4096, 0);
            char *dupstr = strdup(buffer);
            char *sepstr = dupstr;
            char *substr = NULL;
            int count = 0;
            int flag = 0;
            int send_count = 0;
            int index = 0;
            substr = strsep(&sepstr, delim);

            do
            {
                if(strcmp(substr, " ") != 0)
                {
                    if(flag != 0)
                    {
                        if(strcmp(substr, "") != 0)
                        {

                            /*if(send(client_socket, substr, 4096, 0) < 0) {
                            	printf("send fail\n");
                            }*/
                            pthread_create(&(thread_pool[index]), NULL, send_req, (void*)substr);
                            pthread_join(thread_pool[index], &ret);
                            if(index == 4)
                            {
                                index = 0;
                            }
                            else
                            {
                                index++;
                            }
                            send_count++;
                        }
                    }
                    flag++;
                }
                substr = strsep(&sepstr, delim);
            }
            while(substr);
            free(dupstr);

            for(i=0; i<send_count; i++)
            {
                recv(client_socket, buffer, 4096, 0);
                printf("%s",buffer);
            }
        }
        close(client_socket);

    }
    return 0;
}
