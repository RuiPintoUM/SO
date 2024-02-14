#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#define BUFSIZE 10000

// recebe info do fifo escreve no STDOUT
typedef struct package {
    pid_t pid;
    int estado;
    long int time;
    char* process;
    struct package *next;
} *Package;

Package add_pack_to_list(Package list_package,Package pack)
{
    if (list_package == NULL)
    {
        return pack;
    }
    else
    {
        Package aux = list_package;

        while(aux->next != NULL)
            aux = aux->next;
        aux->next = pack;    
    }
    return list_package;
}

Package get_pack_from_list(Package list_package,int pid)
{
    Package aux = list_package;

    while(aux != NULL)
    {
        if(aux->pid == pid)
            return aux;
        aux = aux->next;    
    }
    return NULL;
}

long calculate_time(long time_start,long time_end)
{
    long time = time_end - time_start;

    return time;
}

int main()
{
    mkfifo("communication_fifo_TM", 0666);
    mkfifo("communication_fifo_MT", 0666);

    int fd = open("communication_fifo_TM", O_RDONLY);
    int fd2 = open("communication_fifo_MT", O_WRONLY);

    //char bufferaux[10000];
    int flag,size_of_process;
    Package list_pack = NULL;
    struct timespec timespec;

    while(1)
    {
        read(fd,&flag,sizeof(int));

        if(flag == 1)
        {
            struct package *pack = malloc(sizeof(struct package));
            pack->process = NULL;
            pack->next = NULL;
            pack->estado = 0;

            read(fd,&pack->pid,sizeof(int));

            read(fd,&size_of_process,sizeof(int));
            char name[size_of_process];
        
            read(fd,name,sizeof(char) * size_of_process);
            pack->process = strdup(name);

            read(fd,&pack->time,sizeof(long));

            list_pack = add_pack_to_list(list_pack,pack);
        }
        if(flag == 2)
        {
            
        }
        if(flag == 3)
        {
            int pid_numb;
            long time_end;
            
            read(fd,&pid_numb,sizeof(int));

            read(fd,&time_end,sizeof(long));

            //printf("pid: %d time: %ld\n",pid_numb,time_end);

            Package pack = get_pack_from_list(list_pack,pid_numb);

            if(pack != NULL)
            {
                //printf("time start: %ld time end: %ld\n",pack->time,time_end);
                long time = calculate_time(pack->time,time_end);
                //printf("time: %ld\n",time);
                pack->estado = 1;

                write(fd2,&time,sizeof(long));
            }
            else
            {
                perror("not found");
            }
        }
        if(flag == 4)
        {
            char buffer[BUFSIZE];
            int MAX_LINE_SIZE = 90,buffer_size = 0;


            if (list_pack == NULL)
            {
                perror("none package found on list");
            }

            else
            {
                Package aux = list_pack;

                while(aux->next != NULL)
                {
                    if(aux->estado == 0)
                    {
                        long aux_time_now = 0,aux_time_calculated = 0;
                        char line[MAX_LINE_SIZE];

                        clock_gettime(CLOCK_MONOTONIC, &timespec);
                        aux_time_now = timespec.tv_nsec / 1000000 + timespec.tv_sec * 1000;

                        aux_time_calculated = calculate_time(aux->time,aux_time_now);

                        snprintf(line, MAX_LINE_SIZE, "last pid: %d processo: %s tempo: %ld\n",
                            aux->pid, aux->process, aux_time_calculated);

                        if (buffer_size + strlen(line) < BUFSIZE) 
                        {
                            strcat(buffer, line);
                            buffer_size += strlen(line);
                        } 
                        else 
                        {
                            perror("Buffer overflow");
                        }
                    }
                    aux = aux->next;
                }
                
                // Ultimo processo da lista
                char line[MAX_LINE_SIZE];
                long aux_time_now = 0,aux_time_calculated = 0;

                clock_gettime(CLOCK_MONOTONIC, &timespec);
                aux_time_now = timespec.tv_nsec / 1000000 + timespec.tv_sec * 1000;
                aux_time_calculated = calculate_time(aux->time,aux_time_now);
                
                snprintf(line, MAX_LINE_SIZE, "last pid: %d processo: %s tempo: %ld\n",
                    aux->pid, aux->process, aux_time_calculated);

                if (buffer_size + strlen(line) < BUFSIZE) 
                {
                    strcat(buffer, line);
                    buffer_size += strlen(line)+ 1;
                } 
                else 
                {
                    perror("Buffer overflow");
                }

                write(fd2,&buffer_size,sizeof(int));
                write(fd2,buffer,buffer_size);
            }
        }
        flag = 0;
    }
        close(fd);
        return 0;


    return 0;
}
