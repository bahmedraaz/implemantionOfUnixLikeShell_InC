#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <termios.h>
#include <stdbool.h>
#include <string>
#include <iostream>
#include <sstream>

using namespace std;
#define LIMIT 256
#define TRUE 1

pid_t pid;

void printShellPrompt(){
	printf("> ");
}

// Function to run commands without pipe and redirection
void runCommand(char **args){
	 int err = -1;

	 if((pid=fork())==err){
		 cerr<<"Problem while creating child process\n"<<endl;
		 return;
	 }

	 int status;
	if(pid==0){
		if (execvp(args[0],args)==err){
			cerr << "invalid command: " << endl;
			kill(getpid(),SIGTERM);
			//return err;
		}
	 }

	 waitpid(pid,&status,0);
	 cout << "exit status: " << WEXITSTATUS(status) << endl;
}

// Function to handle redirection
int redirection(char * args[])
{

	int err = -1;
	int fileDescriptor;
	int i=0, j=0;
	int status = 0;
	int input_redirection = 0;
	int output_redirection = 0;
	char* inputFile = NULL;
	char* outputFile = NULL;
	char* redirArgs[256];

	while( args[i] != NULL )
	{
		if ( strcmp(args[i],"<") == 0 )
		{
			if( args[i+1] == NULL ||
				strcmp(args[i+1],"<") == 0 ||
				strcmp(args[i+1],">") == 0 ||
				strcmp(args[i+1],"|") == 0 )
			{
				cerr << "Invalid command" << endl;
				return err;
			}

			inputFile = args[i+1];
			input_redirection++;
			i+=2;

			if(input_redirection > 1)
			{
				cerr << "Invalid command" << endl;
				return err;
			}
		}
		else if ( strcmp(args[i],">") == 0 )
		{
			if( args[i+1] == NULL ||
				strcmp(args[i+1],"<") == 0 ||
				strcmp(args[i+1],">") == 0 ||
				strcmp(args[i+1],"|") == 0 )
			{
				cerr << "Invalid command" << endl;
				return err;
			}

			outputFile = args[i+1];
			output_redirection++;
			i+=2;

			if(output_redirection > 1)
			{
				cerr << "Invalid command" << endl;
				return -1;
			}
		}
		else
		{
			redirArgs[j] = args[i];
			i++;
			j++;
		}
	}

	if( redirArgs[0] == NULL )
	{
		cerr << "Invalid command" << endl;
		return err;
	}
	else
	{
		if( (pid=fork()) == err )
		{
			cerr<<"Problem while creating child process\n"<<endl;
			return err;
		}

		if( pid==0 )
		{
			if ( (input_redirection != 0 ) && (output_redirection != 0 ) )
			{
				fileDescriptor = open(inputFile, O_RDONLY, 0600);
				dup2(fileDescriptor, STDIN_FILENO);
				close(fileDescriptor);
				fileDescriptor = open(outputFile, O_CREAT | O_TRUNC | O_WRONLY, 0600);
				dup2(fileDescriptor, STDOUT_FILENO);
				close(fileDescriptor);
			}
			else if ( output_redirection != 0 )
			{
				fileDescriptor = open(outputFile, O_CREAT | O_TRUNC | O_WRONLY, 0600);
				dup2(fileDescriptor, STDOUT_FILENO);
				close(fileDescriptor);
			}
			else if( input_redirection != 0 )
			{
				fileDescriptor = open(inputFile, O_RDONLY, 0600);
				dup2(fileDescriptor, STDIN_FILENO);
				close(fileDescriptor);
			}

			if (execvp(redirArgs[0],redirArgs)==err)
			{
				cerr << "invalid command: " << endl;
				kill(getpid(),SIGTERM);
			}
		}

		waitpid(pid,&status,0);
		cout << "exit status: " << WEXITSTATUS(status) << endl;
	}

	return status;
}


// function to handle pipe
void executePipe(char * args[]){
	int pipe_odd[2];
	int pipe_even[2];
	int num_cmds = 0;
	char *command[256];
	pid_t pid;
	int end = 0;
	int i = 0;
	int j = 0;
	int k = 0;
	int l = 0;

	while (args[l] != NULL){
		if (strcmp(args[l],"|") == 0){
			num_cmds++;
		}
		l++;
	}
	num_cmds++;

	while (args[j] != NULL && end != 1){
		k = 0;
		while (strcmp(args[j],"|") != 0){
			command[k] = args[j];
			j++;
			if (args[j] == NULL){
				end = 1;
				k++;
				break;
			}
			k++;
		}
		command[k] = NULL;
		j++;

		if (i % 2 != 0){
			pipe(pipe_odd);
		}else{
			pipe(pipe_even);
		}

		pid=fork();

		if(pid==-1){
			if (i != num_cmds - 1){
				if (i % 2 != 0){
					close(pipe_odd[1]);
					close(pipe_odd[0]);
				}else{
					close(pipe_even[1]);
					close(pipe_even[0]);
				}
			}

			cerr<<"Problem while creating child process\n"<<endl;
			return;
		}

		int status;

		if(pid==0){
			if (i == 0){
				close(pipe_even[0]);
				dup2(pipe_even[1], 1);
			}

			else if (i == num_cmds - 1){
				if (num_cmds % 2 != 0){
					close(pipe_even[1]);
					dup2(pipe_odd[0],0);
				}else{
					close(pipe_odd[1]);
					dup2(pipe_even[0],0);
				}

			}else{
				if (i % 2 != 0){
					close(pipe_even[1]);
					dup2(pipe_even[0],0);
					dup2(pipe_odd[1],1);
				}else{
					close(pipe_odd[1]);
					dup2(pipe_even[1],1);
					dup2(pipe_odd[0], 0);
				}
			}

			if (execvp(command[0],command)<0){
				printf("Command not found\n");
				exit(0);
			}
		}


		if (i == 0){
			close(pipe_even[1]);

		}
		else if (i == num_cmds - 1){
			if (num_cmds % 2 != 0){
				close(pipe_odd[0]);
				close(pipe_odd[1]);
			}else{
				close(pipe_even[0]);
				close(pipe_even[1]);
			}
		}else{
			if (i % 2 != 0){
				close(pipe_even[0]);
				close(pipe_odd[1]);
			}else{
				close(pipe_odd[0]);
				close(pipe_even[1]);
			}
		}

		waitpid(pid,&status,0);
		cout << "exit status: " << WEXITSTATUS(status) << endl;

		i++;
	}
}

// this function checks whether the command finally should need to handle pipe, redirection, normal command or it should exit.
// from this function, the commands are distributed to appropriate function to be executed.
int processCommand(char * args[]){
	int i = 0;
	int j = 0;
	char *args_aux[256];

	while ( args[j] != NULL)
	{
		if ( (strcmp(args[j],">") == 0) || (strcmp(args[j],"<") == 0) || (strcmp(args[j],"&") == 0)){
			break;
		}
		args_aux[j] = args[j];
		j++;
	}
	args_aux[j] = NULL;

	if(strcmp(args[0],"exit") == 0) exit(0);
	else{
		while (args[i] != NULL)
		{
			if (strcmp(args[i],"|") == 0)
			{
				executePipe(args);
				return 1;
			}
			else if (strcmp(args[i],"<") == 0 || strcmp(args[i],">") == 0 )
			{
				return redirection(args);
			}

			i++;
		}
		args_aux[i] = NULL;
		runCommand(args_aux);

	}
return 0;
}


// main program starts here
int main(int argc, char *argv[], char ** envp) {
	char *tokens[LIMIT];
	int numTokens;
	string line;

	while(TRUE){
		string token;
		cout << "> ";
		getline(cin, line);
		numTokens = 0;
		istringstream ss(line);
		while(ss >> token){
			tokens[numTokens] = (char*)malloc(token.size()+1);
			strcpy(tokens[numTokens], token.c_str());
			numTokens++;
		}
		tokens[numTokens] = NULL;
		if(numTokens == 0){
			cerr << "Invalid command" << endl;
			continue;
		}

		processCommand(tokens);

	}

	for(int i=0; i<numTokens; i++){
		free(tokens[i]);
	}

	exit(0);
}
