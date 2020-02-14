/*
 
 Name: Bijay Raj Raut
 ID:   1001562222
 
 */
// The MIT License (MIT)
//
// Copyright (c) 2016, 2017 Trevor Bakker
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
// so we need to define what delimits our tokens.
// In this case  white space
// will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

#define MAX_NUM_ARGUMENTS 10     // Mav shell supports ten arguments

#define MAX_PID_HISTORY 15     //Mav shell only supports 15 pid history

//save the list of command in a linked list
//so that can be retrived using history command
typedef struct LinkedList
{
    char history[MAX_COMMAND_SIZE];
    struct LinkedList *next_ptr;
}list;

//Adds history command to the linklist head
//created in main to keep track of the history of the commands
//takes in the address of the headnode and the entered command
void AddNode(list **LLH,char *cmd);

//prints the linked list by traversing through it
//when history command is entered in the shell
void PrintNode(list *LLH);

//upon command !num, the function will traverse
//the linked list to retrieve the nth
//node and copy the command to the cmd array
void retrievenode(list *LLH,int num,char cmd[]);

//freeing up the nodes afte the usage
//receives the pointer of the head of the LINKED LIST
void freenodes(list **LLH);

//adding the handle to the shell
static void handle_signal (int sig )
{
    //ignoring the signals by doing nothing
}

int main(int argc, char **argv)
{
    char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE );
    list *LLH = NULL;
    //count the number of nodes and number of pids
    int counter=0,pid_count=0,suspendedpid=0;
    
    int pids[MAX_PID_HISTORY]; //array to save pids of the processes
    
    //sigaction struct for signal handling
    struct sigaction act;
    
    //Zero out the sigaction struct
    memset(&act,'\0',sizeof(act));
    
    //Set the handler to use the function handle_signal()
    act.sa_handler = &handle_signal;
    
    //Install the handler fot ctrl-c SIGINT and ctrl-z SIGTSTP
    //and check the return value.
    if(sigaction(SIGINT,&act,NULL)<0)
    {
        perror ("sigaction: ");
        return 1;
    }
    if(sigaction(SIGTSTP,&act,NULL)<0)
    {
        perror ("sigaction: ");
        return 1;
    }
    while( 1 )
    {
        // Print out the msh prompt
        printf ("msh> ");
        // Read the command from the commandline.  The
        // maximum command that will be read is MAX_COMMAND_SIZE
        // This while command will wait here until the user
        // inputs something since fgets returns NULL when there
        // is no input
        while( !fgets (cmd_str, MAX_COMMAND_SIZE, stdin) );
        //save all the input commands
        //Since being a linked list space is not that big of a problem
        //it saves way more than 15 commands and all of them can be
        //recalled using ! command
        if(cmd_str[0]!='\n')
            AddNode(&LLH,cmd_str);
        //if !n is entered then retrieve the requested history
        if((cmd_str[0]=='!')&&(LLH!=NULL))
        {
            //num saves the integer following ! command
            int num=0;
            
            //array to retrive the command from the linked list
            char cmd[MAX_COMMAND_SIZE];
            
            //retrieve the desired history command index
            //%*c will ignore the !
            sscanf(cmd_str,"%*c%d%*c",&num);
            
            //check if the requested number is within the range
            if(counter<=num)
            {
                printf("Command not in history.\n");
            }
            else
            {
                //pass the head of the linked list, index and empty array
                retrievenode(LLH,num,cmd);
                //replace the command from the history to perform the task
                strcpy(cmd_str,cmd);
            }
        }
        //skip the rest of the process if cmd entered is a \n,!,' '
        if(cmd_str[0]!='\n'&&cmd_str[0]!='!'&&cmd_str[0]!=' ')
        {
            //exit on "exit" and "quit" with signal 0
            if(strcmp(cmd_str,"exit\n")==0||strcmp(cmd_str,"quit\n")==0)
            {
                exit(0);
            }
            //Calls the print function if history is entered in command
            //DISCLAIMER: Number of history displayed and executed is
            //not limited to 15
            if(strcmp(cmd_str,"history\n")==0)
            {
                PrintNode(LLH);
            }
            else
            {
                //calling kill on the saved pid when bg is called
                //which will resume the SIGTSTP process in the background
                if(strcmp(cmd_str,"bg\n")==0)
                {
                    kill(suspendedpid,SIGCONT);
                }
                else
                {
                    //Parse input
                    char *token[MAX_NUM_ARGUMENTS];
                    int token_count = 0;
                    
                    // Pointer to point to the token
                    // parsed by strsep
                    char *arg_ptr;
                    char *working_str  = strdup( cmd_str );
                    
                    // we are going to move the working_str pointer so
                    // keep track of its original value so we can deallocate
                    // the correct amount at the end
                    char *working_root = working_str;
                    
                    // Tokenize the input stringswith whitespace used as the delimiter
                    while (((arg_ptr = strsep(&working_str,WHITESPACE))!= NULL) &&
                           (token_count<MAX_NUM_ARGUMENTS))
                    {
                        token[token_count] = strndup( arg_ptr, MAX_COMMAND_SIZE );
                        if( strlen( token[token_count] ) == 0 )
                        {
                            token[token_count] = NULL;
                        }
                        token_count++;
                    }
                    /*
                     if(strcmp(token[0],"exit")==0||strcmp(token[0],"quit")==0)
                     {
                     exit(0);
                     }
                     */
                    //custom changing directory command to handle cd commands in msh
                    if(strcmp(token[0],"cd")==0)
                    {
                        chdir(token[1]);
                    }
                    //listing the pid values upon "listpids" command
                    //Print the array using the loop
                    else if(strcmp(token[0],"listpids")==0)
                    {
                        int j;
                        for(j=0;j<pid_count&&j<MAX_PID_HISTORY;j++)
                        {
                            printf("%2d: %d\n",j,pids[j]);
                        }
                        //Avoiding Array Out of Bound Problem
                        if(j==MAX_PID_HISTORY)
                            printf("**Unable to display more than %d**\n",MAX_PID_HISTORY);
                    }
                    //Create a special case for echo
                    //DISCLAIMER: echo foo ; echo bar ; echo baz
                    //works with space before and after the command
                    else if(strcmp(token[0],"echo")==0)
                    {
                        int i=0,j=1,semi_counter=0;
                        
                        //using the command line and counting the number of ;
                        //while loop iterates through the cmd_str string
                        //until it hits the null and if there is a ; command
                        //in it, the semi_counter variable gets incremented
                        while(cmd_str[i]!='\0')
                        {
                            if(cmd_str[i]==';')
                                semi_counter++;
                            i++;
                        
                        }
                        //fork() and execl() in the loop will echo the commands
                        //in the indexing pattern that token creates with the
                        //forementioned echo command
                        for(i=0;i<=semi_counter;i++)
                        {
                            int status;
                            pid_t pid=fork();
                            
                            //error checking for array out of bound problem
                            if(pid_count<MAX_PID_HISTORY)
                            {
                                pids[pid_count++]=pid;
                            }
                            if(pid==0)
                            {
                                //execl() will go in the /bin/echo directory
                                //execute the echo command with token[0]
                                //then token[j] will have the following commands
                                //which gets executed one at a time
                                execl("/bin/echo",token[0],token[j],NULL);
                                exit(1);
                            }
                            waitpid(pid,&status,0);
                            //printf("ARRAY: %s\n",token[j]);
                            j=j+3;
                        }
                    }
                    else
                    {
                        //char *path[]={"/usr/local/bin","/usr/bin","/bin"};
                        pid_t pid=fork();
                        //error checking to not to exced max # of pids
                        if(pid_count<MAX_PID_HISTORY)
                        	pids[pid_count++]=pid;
                
                        int status;
                        
                        //handles the error caused by failed fork(returns -1 on failure)
                        if(pid==-1)
                        {
                            perror("fork() failed\n");
                            exit(1);
                        }
                        if(pid==0)
                        {
                            //execvp takes in the user input as a first parameter
                            //and takes in all the other parameters,
                            //searches all the possible paths to execute the command
                            //if statement is used to handle the errors that gets
                            //trigerred by invalid commands and parameters
                            if(execvp(token[0],token)<0)
                            {
                                //errno 2 : ENOENT refers to no such file or directory
                                //execvp fails with ENOENT errno so if the shell catches such
                                //replace it with custom print message
                                if(ENOENT)
                                {
                                    printf("%s: Command not found.\n",token[0]);
                                }
                                //handels default errors caused by invalid parameters
                                else
                                {
                                    perror(token[0]);
                                }
                                exit(1);
                            }
                        }
                        //if the child process get interupted by a signal the waitpid returns
                        // a negative value and we can save the pid that got terminated without
                        //completing the process in a variable so that we can wake it up
                        //in the background when 'bg' is entered
                        if(waitpid(pid,&status,0)<0)
                        {
                            suspendedpid=pid;
                            //printf("Signal Caught with pid %d",suspendedpid);
                        }
                    }
                    free( working_root );
                }
            }
        }
        //free(cmd_str);
        //incrementing the history counter after each iteration of the while loop
        counter++;
    }
    freenodes(&LLH);
    return 0;
}
void AddNode(list **LLH,char *cmd)
{
    list *newnode,*temp;
    newnode = malloc(sizeof(list));
    strcpy(newnode->history,cmd);
    newnode->next_ptr=NULL;
    if(*LLH==NULL)
    {
        *LLH=newnode;
    }
    else
    {
        temp=*LLH;
        while(temp->next_ptr!=NULL)
            temp=temp->next_ptr;
        temp->next_ptr=newnode;
    }
}
void PrintNode(list *LLH)
{
    if(LLH!=NULL)
    {
        int i=0;
        while(LLH!=NULL)
        {
            printf("%2d: %s",i++,LLH->history);
            LLH=LLH->next_ptr;
        }
    }
}
void retrievenode(list *LLH,int num,char cmd[])
{
    if(LLH!=NULL)
    {
        int i=0;
        //traverse the linked list to the desired number
        //and make sures it doesnot fo beyond the last member
        while(LLH!=NULL&&i!=num)
        {
            LLH=LLH->next_ptr;
            i++;
        }
        if(LLH!=NULL)
        {
            //copying the command to the empty array
            strcpy(cmd,LLH->history);
            /*
            //if the command is not the newline character
            //remove the newline character from the end
            if(LLH->history[0]!='\n')
            {
                cmd[strlen(cmd)-1]='\0';
            }
             */
        }
    }
}
//freeing up the Linked List Head
//traversing the linked list copying its next ptr
//to a temp and freeing up the head and then setting the
//new head to be temp untill LLH reaches the null
void freenodes(list **LLH)
{
    list *tempnode;
    while(*LLH!=NULL)
    {
        tempnode=(*LLH)->next_ptr;
        free(*LLH);
        *LLH=tempnode;
    }
}
