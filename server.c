#include "server.h"
#include<netinet/in.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<pthread.h>
#include<dirent.h>

char root[128];
// add mutex
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

int str_search(char *query, char *str)
{
    int i,j,flag;
    int count = 0;
    for(i=0; str[i]!='\0'; i++)
    {
        flag = 0;
        if(str[i] == query[0])
        {
            for(j=0; query[j]!='\0'; j++)
            {
                if(str[i+j] == query[j])
                {
                    flag = 1;
                }
                else
                {
                    flag = 0;
                    break;
                }
            }
        }
        if(flag == 1)
        {
            count++;
        }
    }
    return count;
}

char output[1024] = "";
int check_not_found;
int dir_recursive(char *path, char *query)
{
    char glue = '/';
    int i;
    // try to open dir
    DIR *dp = opendir(path);
    char aa[1024];


    if(!dp)
    {

        int co = 0;
        // read file
        FILE *fp = fopen(path, "r");
        while(fscanf(fp, "%[^\n]\n",aa) != EOF)
        {
            co += str_search(query, aa);

        }
        char bb[64];
        if(co != 0)
        {
            strcat(output, "File: ");
            strcat(output, path);
            strcat(output, ", Count: ");
            sprintf(bb, "%d", co);
            strcat(output, bb);
            strcat(output, "\n");
            check_not_found = 1;
        }
        fclose(fp);


        return 1;
    }

    struct dirent *filename;
    while((filename = readdir(dp)))
    {
        if(!strcmp(filename->d_name,"..") || !strcmp(filename->d_name,"."))
        {
            continue;
        }

        int pathLenth=strlen(path)+strlen(filename->d_name)+2;

        //char *pathStr = (char*)malloc(sizeof(char) * pathLenth);
        char pathStr[pathLenth];

        strcpy(pathStr, path);

        int i=strlen(pathStr);
        if(pathStr[i-1]!=glue)
        {
            pathStr[i]=glue;
            pathStr[i+1]='\0';
        }

        strcat(pathStr, filename->d_name);

        // recursive
        dir_recursive(pathStr, query);
    }
    closedir(dp);

    return 1;
}

void *search(void *ptr)
{
    char *query;
    query = (char *)ptr;
    char glue = '/';
    int i;
    for(i=0; i<1024; i++)
    {
        output[i] = '\0';
    }
    // LOCK HERE
    pthread_mutex_lock(&mutex1);

    strcat(output, "String: \"");
    strcat(output, query);
    strcat(output, "\"\n");
    check_not_found = 0;
    dir_recursive(root, query);
    if(check_not_found == 0)
    {
        strcat(output, "Not found\n");
    }


    // UNLOCK HERE
    pthread_mutex_unlock(&mutex1);

    // return
    pthread_exit((void *)output);
}

int main(int argc, char *argv[])
{
    int port;
    int thread_num = 0;


    // ROOT
    if(strcmp(argv[1], "-r") == 0)
    {
        strncpy(root, argv[2], strlen(argv[2]));
    }
    else if(strcmp(argv[3], "-r") == 0)
    {
        strncpy(root, argv[4], strlen(argv[4]));
    }
    else if(strcmp(argv[5], "-r") == 0)
    {
        strncpy(root, argv[6], strlen(argv[6]));
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
    else if(strcmp(argv[5], "-p") == 0)
    {
        port = atoi(argv[6]);
    }

    // THREAD_NUMBER
    if(strcmp(argv[1], "-n") == 0)
    {
        thread_num = atoi(argv[2]);
    }
    else if(strcmp(argv[3], "-n") == 0)
    {
        thread_num = atoi(argv[4]);
    }
    else if(strcmp(argv[5], "-n") == 0)
    {
        thread_num = atoi(argv[6]);
    }

    // creat thread
    pthread_t thread_pool[thread_num];
    void *ret;
    ///////////

    // set socket's address info
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htons(INADDR_ANY);
    server_addr.sin_port = htons(port);

    // creat a stream socket
    int server_socket = socket(PF_INET, SOCK_STREAM, 0);
    if(server_socket < 0)
    {
        printf("creat socket fail\n");
        exit(1);
    }

    if(bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)))
    {
        printf("server bind port: %d failed\n",port);
        exit(1);
    }

    if(listen(server_socket, 20))
    {
        printf("server listen failed\n");
        exit(1);
    }

    ////////////////////////

    while(1)
    {
        // creat client socket
        struct sockaddr_in client_addr;
        socklen_t length = sizeof(client_addr);
        // waiting client
        int new_server_socket = accept(server_socket, (struct sockaddr*)&client_addr, &length);
        if(new_server_socket < 0)
        {
            printf("server accept fail\n");
            break;
        }

        char buffer[4096];
        int index = 0;
        int is_first = 0;
        bzero(buffer, sizeof(buffer));
        if(recv(new_server_socket, buffer, 4096, 0) > 0)
            printf("%s\n",buffer);

        while(recv(new_server_socket, buffer, 4096, 0) > 0)
        {


            pthread_create(&(thread_pool[index]), NULL, search, (void*)buffer);
            pthread_join(thread_pool[index], &ret);

            strcpy(buffer, (char*)ret);

            send(new_server_socket, buffer, 4096, 0);

            if(index == thread_num-1)
            {
                index = 0;
            }
            else
            {
                index++;
            }
        }


        close(new_server_socket);
    }

    close(server_socket);
    return 0;
}
