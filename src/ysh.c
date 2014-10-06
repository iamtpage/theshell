#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>

extern int errno;

//this is a test
typedef void (*sighandler_t)(int);

//arguments and environmental variables
static char *my_argv[100], *my_envp[100];

//path to search for programs
static char *search_path[10];

void handle_signal(int signo)
{
	//prints out MY_SHELL
    printf("\n[MY_SHELL ] ");
    
    //not really necessary, but good practice
    fflush(stdout);
}

void fill_argv(char *tmp_argv)
{
	//pointer to list of arguments
    char *arg = tmp_argv;
    
    //counter
    int index = 0;
    
    char ret[100];
    
    //set ret to 0 for good measure
    memset(ret, '\0', sizeof(ret));
    
    //while we don't have a string terminator
    while(*arg != '\0') 
    { //take the tmp_argv and examine it character by character
        
        //looks like we only support 10 arguments
        if(index == 10)
        {
            break;
		}
		
		//we found a space
        if(*arg == ' ') 
        {
			//if the index spot in my_argv isn't being used
            if(my_argv[index] == NULL)
            {
				//allocate enough memory for the index to store the argument
                my_argv[index] = (char *)malloc(sizeof(char) * strlen(ret) + 1);
			}
            else 
            {
				//set the index contents to a bunch of 0's
                memset(my_argv[index], '\0', strlen(my_argv[index]));
            }
            
            //now that we have allocated enough space
            //copy the ret to the spot in my_argv
            strncpy(my_argv[index], ret, strlen(ret));
            
            //add a string terminator for formatting purposes
            strncat(my_argv[index], "\0", 1);
            
            //set memset back to a bunch of 0's
            memset(ret, '\0', 100);
            
            //rinse and repeat
            index++;
            
        } 
        else 
        {
			//not a space
			//so just copy it
            strncat(ret, arg, 1);
        }
        
        //increment to the next argument
        arg++;
        
        /*printf("arg is %c\n", *arg);*/
    }
    
    //set the arg index to the size it needs to be
    my_argv[index] = (char *)malloc(sizeof(char) * strlen(ret) + 1);
    
    //copy ret to my_argv[index]
    strncpy(my_argv[index], ret, strlen(ret));
    
    //add a terminator at the end
    strncat(my_argv[index], "\0", 1);
}

//copy the environmental variables for use
void copy_envp(char **envp)
{
	//counter
    int index = 0;
    
    //increment through until envp[index] = NULL
    for(index = 0;envp[index] != NULL; index++) 
    {
		//cast our array index of environmental variables to a character pointer
		//with the proper memory allocated to hold the envp[index] + 1
        my_envp[index] = (char *) malloc(sizeof(char) * (strlen(envp[index]) + 1));
        
        //copy from envp to my_envp
        memcpy(my_envp[index], envp[index], strlen(envp[index]));
    }
}

//get the $PATH variable so we know where to look
//when we are trying to execute commands
void get_path_string(char **tmp_envp, char *bin_path)
{
	//counter
    int count = 0;
    char *tmp;
    while(1) 
    {
		//increment through the environmental variables
		
		//look for the $PATH one
        tmp = strstr(tmp_envp[count], "PATH");
        
        //keep incrementing until we find it
        if(tmp == NULL) 
        {
            count++;
        } 
        
        //we found it, so no need to keep looping
        else 
        {
            break;
        }
    }
		//copy the value of the $PATH variable to bin_path
        strncpy(bin_path, tmp, strlen(tmp));
}

void insert_path_str_to_search(char *path_str) 
{
    //counter
    int index=0;
    
    //temporary pointer so we don't overwrite *path_str
    char *tmp = path_str;
    
    //array
    char ret[100];

	//as long as *tmp isn't =, increment
    while(*tmp != '=')
    {    
        tmp++;
    }
    
    //increment again?
    tmp++;

	//while the character isn't a string terminator
    while(*tmp != '\0') 
    {
		//if the character is a :
        if(*tmp == ':') 
        {
			//add a / to the spot in the ret array
            strncat(ret, "/", 1);
            
            search_path[index] = 
			(char *) malloc(sizeof(char) * (strlen(ret) + 1));
            strncat(search_path[index], ret, strlen(ret));
            strncat(search_path[index], "\0", 1);
            
            index++;
            memset(ret, '0', sizeof(ret));
        } 
        else 
        {
            strncat(ret, tmp, 1);
        }
        
        tmp++;
    }
}

int attach_path(char *cmd)
{
    char ret[100];
    int index;
    int fd;
    memset(ret, '0', sizeof(ret));
    
    for(index=0;search_path[index]!=NULL;index++) 
    {
        strcpy(ret, search_path[index]);
        strncat(ret, cmd, strlen(cmd));
        if((fd = open(ret, O_RDONLY)) > 0) 
        {
            strncpy(cmd, ret, strlen(ret));
            close(fd);
            return 0;
        }
    }
    return 0;
}

void call_execve(char *cmd)
{
    int i;
    printf("cmd is %s\n", cmd);
    if(fork() == 0) 
    {
        i = execve(cmd, my_argv, my_envp);
        printf("errno is %d\n", errno);
        if(i < 0) 
        {
            printf("%s: %s\n", cmd, "command not found");
            exit(1);        
        }
    } 
    else 
    {
        wait(NULL);
    }
}

void free_argv()
{
    int index;
    for(index=0;my_argv[index]!=NULL;index++) 
    {
        memset(my_argv[index], '0', strlen(my_argv[index]));
        my_argv[index] = NULL;
        free(my_argv[index]);
    }
}

int main(int argc, char *argv[], char *envp[])
{
    char c;
    int i, fd;
    char *tmp = (char *)malloc(sizeof(char) * 100);
    char *path_str = (char *)malloc(sizeof(char) * 256);
    char *cmd = (char *)malloc(sizeof(char) * 100);
    
    signal(SIGINT, SIG_IGN);
    signal(SIGINT, handle_signal);

    copy_envp(envp);

    get_path_string(my_envp, path_str);   
    insert_path_str_to_search(path_str);

    if(fork() == 0) 
    {
        execve("/usr/bin/clear", argv, my_envp);
        exit(1);
    } 
    else 
    {
        wait(NULL);
    }
    
    printf("[MY_SHELL ] ");
    fflush(stdout);

    while(c != EOF) 
    {
        c = getchar();
        switch(c) 
        {
            case '\n': if(tmp[0] == '\0') {
                       printf("[MY_SHELL ] ");
        } 
        else 
        {
                       fill_argv(tmp);
                       strncpy(cmd, my_argv[0], strlen(my_argv[0]));
                       strncat(cmd, "\0", 1);
                       if(index(cmd, '/') == NULL) 
                       {
                           if(attach_path(cmd) == 0) 
                           {
                               call_execve(cmd);
                           } 
                           else 
                           {
                               printf("%s: command not found\n", cmd);
                           }
                       } 
                       
                       else 
                       {
                           if((fd = open(cmd, O_RDONLY)) > 0) 
                           {
                               close(fd);
                               call_execve(cmd);
                           } 
                           
                           else 
                           {
                               printf("%s: command not found\n", cmd);
                           }
                       }
                       
                       free_argv();
                       printf("[MY_SHELL ] ");
                       memset(cmd, '0', 100);
                   }
                   
                   memset(tmp, '0', 100);
                   break;
                   
				 default: strncat(tmp, &c, 1);
                 break;
        }
    }
    
    free(tmp);
    free(path_str);
    
    for(i=0;my_envp[i]!=NULL;i++)
    {
        free(my_envp[i]);
    }
    
    for(i=0;i<10;i++)
    {
        free(search_path[i]);
    }
    
    printf("\n");
    return 0;
}
