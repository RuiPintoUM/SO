#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <string.h>
#include <time.h>

#define BUFSIZE 10000

int exec_command(char*arg)
{
    char* exec_args[10];
    int i = 0;
    char* string;
    char* command = strdup(arg);
    string = strtok(command," ");

    while(string != NULL)
    {
        exec_args[i] = string;
        i++;
        string = strtok(NULL, " ");
    }
    exec_args[i] = NULL;


    int reg = execvp(exec_args[0],exec_args);

    if (reg<0)
    {
        perror("erro");
    }
        return reg;
}

void exec_multiple_command(char* arg)
{
    char *command = strtok(arg, "|");
    char **commands = malloc(sizeof(char *) * 2200);

    int number_of_commands = 0;

    while(command != NULL)
    {
        if(*command == ' ')
        {
            command++;
        }
        commands[number_of_commands] = command;
        number_of_commands++;
        command =strtok(NULL, "|");
    }

    int pipes[number_of_commands-1][2];
    
    for(int i = 0;i<number_of_commands;i++){
        
        if(i==0){
            pipe(pipes[0]);
            int fres = fork();
            if(fres == 0){
                close(pipes[i][0]);
                dup2(pipes[i][1],1);
                close(pipes[i][1]);
                exec_command(commands[i]);
                _exit(0);
            }
            else{

                close(pipes[i][1]);
            }
        }
        else if(i == number_of_commands-1){
            int fres = fork();
            if(fres==0){
                dup2(pipes[i-1][0],0);
                close(pipes[i-1][0]);
                exec_command(commands[i]);
                _exit(0);
            }
            else{
                close(pipes[i-1][0]);
            }
        }
        else{
            pipe(pipes[i]);
            int fres = fork();
            if(fres==0){
                dup2(pipes[i-1][0],0);
                close(pipes[i-1][0]);
                
                close(pipes[i][0]);
                dup2(pipes[i][1],1);
                close(pipes[i][1]);
                exec_command(commands[i]);

            }
            else{
                //printf("%s\n",commands[i]);
                close(pipes[i-1][0]);
                close(pipes[i][1]);
            }
        }
        // trocar wait(NULL) 
        for(int i = 0;i<number_of_commands;i++){
            wait(NULL);
        }
    }

    printf("Pipeline finished\n");
}

int main(int argc, char *argv[]) {
    long time_start = 0, time_end = 0,time_total = 0;
    //int pd[2];
    int fd = open("communication_fifo_TM", O_WRONLY);
    int fd2 = open("communication_fifo_MT", O_RDONLY);

    pid_t pid;
    int status;
    int pid_numb;
    int size_of_buffer;
    char buffer[BUFSIZE];

    struct timespec time;
    int flag, size_of_process;
    

    if(strcmp(argv[1],"execute") == 0)
    {
        if(strcmp(argv[2],"-u") == 0)
        {   
            //pide(pd);
            flag = 1;
            pid = fork();
            if(pid < 0) {
                perror("fork");
                return EXIT_FAILURE;
            }

            if(pid == 0) 
            {   

                clock_gettime(CLOCK_MONOTONIC, &time);

                time_start = time.tv_nsec / 1000000 + time.tv_sec * 1000;


                pid_numb = getpid();
                
                printf("PID do programa: %d\n", pid_numb);

                char* process =  strdup(argv[3]);
                char* token = strtok(process," ");
                size_of_process = sizeof(token) + 1;

                // indicação da flag
                write(fd,&flag,sizeof(int));
                
                write (fd,&pid_numb,sizeof(int));
                
                //indicação do número de bytes do processo
                write(fd,&size_of_process,sizeof(int));
                write(fd,token, sizeof(char) * size_of_process);

                write(fd,&time_start,sizeof(long));
                close(fd);
                
                exec_command(argv[3]);

                perror("exec");
                return EXIT_FAILURE;
            } 
            else 
            {   

                pid_numb = wait(&status);
                flag = 3;

                if(WIFEXITED(status))
                {
                    clock_gettime(CLOCK_MONOTONIC, &time);

                    time_end = time.tv_nsec / 1000000 + time.tv_sec * 1000;

                    write(fd,&flag,sizeof(int));

                    //printf("pid: %d\n",pid_numb);
                    
                    write(fd,&pid_numb,sizeof(int));
                    
                    write(fd,&time_end,sizeof(long));

                    read(fd2,&time_total,sizeof(long));

                    printf("tempo de execução: %ld\n",time_total);
                }
                else {
                    perror("Erro ao executar o programa");
                }
            }
        }

        if(strcmp(argv[2],"-p") == 0)
        {
            flag = 2;

            clock_gettime(CLOCK_MONOTONIC, &time);

            time_start = time.tv_nsec / 1000000 + time.tv_sec * 1000;

            pid_numb = getpid();
                
            printf("PID do programa: %d\n", pid_numb);

            char* process =  strdup(argv[3]);
            char* token = strtok(process," ");
            size_of_process = sizeof(token) + 1;

            // indicação da flag
            write(fd,&flag,sizeof(int));
            
            write (fd,&pid_numb,sizeof(int));
                
            //indicação do número de bytes do processo
            write(fd,&size_of_process,sizeof(int));
            write(fd,token, sizeof(char) * size_of_process);

            write(fd,&time_start,sizeof(long));
            close(fd);

            exec_multiple_command(argv[3]);

            perror("exec fail");
        }
    }
    if(strcmp(argv[1],"status") == 0)
    {
        flag = 4;

        write(fd,&flag,sizeof(int));

        read(fd2,&size_of_buffer,sizeof(int));
        read(fd2,buffer,size_of_buffer);

        printf("%s",buffer);
    }
    return 0;
}
