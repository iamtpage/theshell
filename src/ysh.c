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
            //strncat(my_argv[index], "\0", 1);
            
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
        
        //printf("arg is %c\n", *arg);
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
    for(index = 0; envp[index] != NULL; index++) 
    {
		//cast our array index of environmental variables to a character pointer
		//with the proper memory allocated to hold the envp[index] + 1
        my_envp[index] = (char *) malloc(sizeof(char) * (strlen(envp[index]) + 2));
        
        strcat(my_envp[index], "$");
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
        if(tmp == NULL || strstr(tmp_envp[count], "MOZ") != NULL) 
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

	//set tmp to just past the = of PATH=/usr/bin
    while(*tmp != '=')
    {    
        tmp++;
    }
    
    //increment again, because tmp is at the =, we want to be after it
    tmp++;


	//while the character isn't a string terminator
    while(*tmp != '\0') 
    {
		//if the character is a :
		//the $PATH variable separates entries by a :
		//usr/bin:/usr/sbin etc
        if(*tmp == ':') 
        {
			//add a / to the spot in ret
			//to make it /bin/foo instead of bin/foo
            strncat(ret, "/", 1);
            
            //set the proper size for the path
            search_path[index] = (char *) malloc(sizeof(char) * (strlen(ret) + 1));
            
            //copy it
            strncat(search_path[index], ret, strlen(ret));
            
            //terminate the string
            strncat(search_path[index], "\0", 1);
            
            //increment
            index++;
            
            //reset ret back to terminators
            memset(ret, '\0', 100);
        } 
        else 
        {
			//copy it
            strncat(ret, tmp, 1);
        }
        //increment again to get to the next one
        tmp++;
    }
}
//takes the command typed (echo) and adds it file path
//so 'echo' becomes '/usr/bin/echo'
int attach_path(char *cmd)
{
	//ret again
    char ret[100];
    
    //counter
    int index;
    int fd;
    
    //set ret to \0's
    memset(ret, '\0', 100);
    
    //search all the search paths for the command given
    for(index = 0; search_path[index] != NULL; index++) 
    {
		//takes ret and adds the first search path entry to it
		//e.g /usr/bin
        strcpy(ret, search_path[index]);
        
        //appends the command given to the search path
        //e.g /usr/bin now becomes /usr/bin/echo
        strncat(ret, cmd, strlen(cmd));
        
        //check to see if /usr/bin/echo is a valid file
        //by trying to open the file in a read-only mode
        if((fd = open(ret, O_RDONLY)) > 0) 
        {
			//if it is, copy it to cmd
            strncpy(cmd, ret, strlen(ret));
            
            //don't leave the file open
            close(fd);
            return 0;
        }
        //else the file doesn't exist, which means it isn't a valid
        //command
    }
    //since we are just copying it to cmd, we don't need to return anything
    return 0;
}

//actually execute the program
void call_execve(char *cmd)
{
    int i; //return value for execve
    int counter = 0;
    int subcounter = 0;
    char *tmp;
    
    //print the command for debug
    printf("cmd is %s\n", cmd);
    
    while(my_argv[counter] != NULL)
    {
		if(strchr(my_argv[counter], '$') != NULL)
		{
			printf("Environmental Variable is: %s\n",my_argv[counter]);
	
			tmp = my_argv[counter];
			
			while(my_envp[subcounter] != NULL)
			{
				if(strspn(my_envp[subcounter],tmp) > 3)
				{
					printf("Found System Variable: %s\n", my_envp[subcounter]);
					
				}
				subcounter++;
			}
		}
		counter++;
	}
    
    //if we aren't a child
    if(fork() == 0) 
    {
		//try to execute the command
        i = execve(cmd, my_argv, my_envp);
        
        //print the error code
        printf("errno is %d\n", errno);
        
        //if the execve didn't execute properly
        if(i < 0) 
        {
			//command not found
            printf("%s: %s\n", cmd, "command not found");
            exit(1);        
        }
    } 
    else 
    {
		//we are a child, so wait
        wait(NULL);
    }
}


//frees memory allocated so memory leaks don't happen
void free_argv()
{
	//counter
    int index;
    
    //increment through my_argv array and set it to \0, then NULL
    for(index = 0; my_argv[index] != NULL; index++) 
    {
        memset(my_argv[index], '\0', strlen(my_argv[index]) + 1);
        my_argv[index] = NULL;
        
        //release it into memory now that it is clean
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

	//copy environmental variables given envp[]
    copy_envp(envp);

	//get the $PATH variable
    get_path_string(my_envp, path_str);
    
    //format the $PATH variable so we can use it
    //to search for commands   
    insert_path_str_to_search(path_str);

    //if we aren't a child
    if(fork() == 0) 
    {
        //execve("/usr/bin/clear", argv, my_envp);
        exit(1);
    } 
    
    //we are a child
    else 
    {
		//wait
        wait(NULL);
    }
    
    //print out the prompt
    printf("[MY_SHELL ] ");
    fflush(stdout);

	//c is where we store the input
    while(c != EOF) 
    {
		//get the input and send it through the switch case
        c = getchar();
        switch(c) 
        {
			//new line
            case '\n': if(tmp[0] == '\0') //if its the very beginning
						{
							//print the prompt
							printf("[MY_SHELL ] ");
						}
        
						else 
						{
							//fill the array with stuff from tmp
							fill_argv(tmp);
							
							//copy the command from my_argv to cmd
							strncpy(cmd, my_argv[0], strlen(my_argv[0]));
							
							//add a terminator
							strncat(cmd, "\0", 1);
							
							//if we specify a relative path vs an absolute
							//e.g echo vs /usr/bin/echo
							if(index(cmd, '/') == NULL) 
							{
								//attach the full file path to the cmd
								if(attach_path(cmd) == 0)
								{
									//then execute it if it returns sucessfully
									call_execve(cmd);
								}
                            
								else 
								{
									//else we can't find it
									printf("%s: command not found\n", cmd);
								}
							} 
                       
							//else check the current directory for the command
							else 
							{
								//if we found it in the directory
								if((fd = open(cmd, O_RDONLY)) > 0) 
								{
									//close the file we opened
									close(fd);
									
									//execute the file
									call_execve(cmd);
								} 
                           
								//else it isn't in the current directory either
								else 
								{
									//not found
									printf("%s: command not found\n", cmd);
								}
							}
							
							//free up the memory
							free_argv();
							
							//print prompt
							printf("[MY_SHELL ] ");
							
							//set the cmd to nulls for the next command
							memset(cmd, '\0', 100);
						}
                   
						//set tmp to null so we don't have leftovers
						memset(tmp, '\0', 100);
						
						//break
						break;
                   
			default: strncat(tmp, &c, 1); //no return, so keep grabbing the characters
					 break;
        }
    }
    
    //free  all the memory
    free(tmp);
    free(path_str);
    
    for(i = 0; my_envp[i] != NULL; i++)
    {
        free(my_envp[i]);
    }
    
    for(i = 0; i < 10; i++)
    {
        free(search_path[i]);
    }
    
    printf("\n");
    return 0;
}
