#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <time.h>
#include "parser.h"
#include "internal.h"
#define STRSIZE 1024

FILE *fileLog;

char * checkPATH(char * commandName){//Check if the command is valid in the PATH
	char * PATHString = getenv("PATH");
	char * pathToCheck = calloc(sizeof(char), STRSIZE);
	fprintf(fileLog, "Checking PATH\n");
	int index = getNextArgument(0, PATHString, pathToCheck, ':');
	while(strcmp(pathToCheck, "") != 0 && index != -1){
		char * pathFinal = calloc(sizeof(char), STRSIZE*2);
		strcat(pathFinal, pathToCheck);
		strcat(pathFinal, "/");
		strcat(pathFinal, commandName);
		if(access(pathFinal, F_OK) != -1){
			fprintf(fileLog, "Path : %s\n", pathFinal);
			return pathFinal;
		}
		index = getNextArgument(++index, PATHString, pathToCheck, ':');
		free(pathFinal);
	}
	return "";
}

int main(int argc, char * argv[]){
	//char *currentPath = calloc(STRSIZE*4, sizeof(char));
	char currentPath[STRSIZE*4];
	int redirectCode;
	int shouldQuit = 0;
	printf("You can type \"help\" to receive help\n");

	while(!shouldQuit){
		char ** args = calloc(sizeof(char *), 1);
		int numberOfArgs;
		int index = 0;
		char * buffer = calloc(sizeof(char), STRSIZE);
		getcwd(currentPath, sizeof(currentPath));
		printf("%s>", currentPath);
		fgets(buffer, STRSIZE, stdin);
		if (buffer == NULL){
			printf("Bye bye !\n");
			return 0;
		}
		numberOfArgs = getAllArguments(0, buffer, &args, ' ');
		/*int result = handleInternals(args, numberOfArgs);
		if(result == 1){
			continue; //A command has been found, and so we go to the next commmand
		} else if(result == 0){
			printf("Goodbye !\n");
			for(int i = 0; i<numberOfArgs; i++){
				free(args[i]);
			}
			free(args);
			free(buffer);
			return 0;
		}*/

		char *path = args[0];
		char ***argumentsForEachCommand = malloc(sizeof(char **));
		char **commands = malloc(sizeof(char *));
		char **argumentsOfTheCommand = malloc(sizeof(char *));
		int numberOfCommands = 1, numberOfArguments = 0;

		commands[0] = args[0];
		argumentsOfTheCommand[0] = malloc(sizeof(char));
		argumentsOfTheCommand[0][0] = 0;//The first cell is the length of the table [maxSize == 256]
		//As a result of this, argumentsOfTheCommand is not really 0-indexed anymore, at leat for the arguments themselves

		for(int i = 0; i<numberOfArgs; i++){
			printf("args[%d/%d] = %s\n", i+1, numberOfArgs, args[i]);
		}

		for(int i = 1; i<numberOfArgs; i++){//Parse each individual command, for pipes
			if(strcmp(args[i-1], "|") == 0){//If previous argument is a pipe, we have a command

				if(strcmp(args[i], "|") == 0){
					argumentsForEachCommand[numberOfCommands-1] = argumentsOfTheCommand;
					continue;
				}
				commands = realloc(commands, sizeof(char *)*++numberOfCommands);
				commands[numberOfCommands-1] = args[i];
				printf("Command %d : %s\n", numberOfCommands, args[i]);

				argumentsForEachCommand = realloc(argumentsForEachCommand, sizeof(char **)*numberOfCommands);
				argumentsForEachCommand[numberOfCommands-2] = argumentsOfTheCommand;

				numberOfArguments = 0;
				argumentsOfTheCommand = malloc(sizeof(char *));
				argumentsOfTheCommand[0] = malloc(sizeof(char));
				argumentsOfTheCommand[0][0] = 0;

				if(i == numberOfArgs-1)
					argumentsForEachCommand[numberOfCommands-1] = argumentsOfTheCommand;

			} else if(strcmp(args[i], "|") != 0) {//We have an argument

				numberOfArguments++;
				argumentsOfTheCommand[0][0] = numberOfArguments;//Add 1 to the number of cells
				argumentsOfTheCommand = realloc(argumentsOfTheCommand, sizeof(char *)*numberOfArguments+1);
				argumentsOfTheCommand[numberOfArguments] = args[i];

				if (i == numberOfArgs-1)//We reached the last arg in args
					argumentsForEachCommand[numberOfCommands-1] = argumentsOfTheCommand;

			} else {

				printf("Pipe found\n");

			}
		}
		printf("Number of commands : %d\n", numberOfCommands);
		//NOTE : debug only
		for (int i=0; i<numberOfCommands; i++){
			printf("Command : %s\n", commands[i]);
			for(int j=1; j<=argumentsForEachCommand[i][0][0]; j++){
				printf("\tArgument %d : %s\n", j, argumentsForEachCommand[i][j]);
			}
		}

		//Needed to redirect std streams
		int pfd[2];
		int result;
		if(numberOfCommands > 1){
			if(pipe(pfd) == -1) printf("%s\n", strerror(errno));
		}
		fileLog = fopen("shell.log", "w");
		fprintf(fileLog, "Initialize log %d PID : %d\n", time(NULL), getpid());

		for (int i=0; i<numberOfCommands; i++){
			int pid = fork();
			if(pid == 0){//We are in the child
				if(numberOfCommands > 1){
					if(i>0){
						fprintf(fileLog, "Redirecting stdin\n");
						if(dup2(pfd[0], 0) == -1) fprintf(fileLog, "%s\n", strerror(errno));//stdin to pipe read end
					}
					if(i<numberOfCommands-1){
						fprintf(fileLog, "Redirecting stdout\n");
						if(dup2(pfd[1], 1) == -1) fprintf(fileLog, "%s\n", strerror(errno));//stdout to pipe write end
					}
				}
			
				fprintf(fileLog, "In child : %d\n", getpid());
				fflush(fileLog);
				
				char ** arguments = malloc(sizeof(char *)*(argumentsForEachCommand[i][0][0]+2));
				//arguments has 2 cells more than the number of arguments (end null and command name at the start), so we malloc with +2
				arguments[0] = commands[i];//first argument given to the program is it's name
				for(int j=1; j<=argumentsForEachCommand[i][0][0]; j++){
					arguments[j] = argumentsForEachCommand[i][j];
					fprintf(fileLog, "%d\n", j);
				}
				arguments[argumentsForEachCommand[i][0][0]+1] = (char *) NULL;//As required
				fprintf(fileLog, "Last index %d\n", argumentsForEachCommand[i][0][0]+1);
				fprintf(fileLog, "Arguments table built\n");
				fflush(fileLog);
				if(access(arguments[0], F_OK) == -1){
					char * newPath = checkPATH(arguments[0]);//Check if command exists in PATH if it doesn't in the local env
					if(strcmp(newPath, "") != 0){
						fprintf(fileLog, "Calling execv on %s (in PATH)\n", newPath);
						for(int j = 0; j<=argumentsForEachCommand[i][0][0]; j++){
							fprintf(fileLog, "\targuments %d : %s\n", j, arguments[j]);
							fflush(fileLog);
						}
						result = execv(newPath, arguments);
					} else {
						errno = ENOENT;//File not found
						result = -1;
					}
				} else {
					fprintf(fileLog, "Calling execv on %s\n", arguments[0]);
					fflush(fileLog);
					result = execv(arguments[0], arguments);
				}
				fprintf(fileLog, "execv failed\n");
				fflush(fileLog);

				if(result == -1) {
					fprintf(fileLog, "An error has occured\n");
					fprintf(fileLog, "%s\n", strerror(errno));
					fflush(fileLog);
				}
				
				fprintf(fileLog, "Killing current process (child)\n");
				fflush(fileLog);
				fclose(fileLog);
				exit(1);
			} else if(i == numberOfCommands-1) {
				int status;
				printf("Waiting on child\n");
				waitpid(pid, &status, 0);
			}
		}
		/*int pid = fork();
		if(pid == 0){//In the child
			//Redirect as needed
			/*if(shouldRedirect){
			char * fileName = calloc(sizeof(char), STRSIZE);
			getNextArgument(++index, buffer, fileName, ' ');
			if(strcmp(fileName, "") == 0){//Empty string
			fprintf(stderr, "Empty filename\n");
			continue;
		} else {
		file = open(fileName, O_RDWR|O_CREAT);
		if(file <= 0){
		fprintf(stderr, "Could not open file\n");
		continue;
	}
}
//0 : stdin, 1 : stdout, 2 : stderr
dup2(file, redirectCode);
}*/
		/*
		numberOfArgs++;
		args = realloc(args, sizeof(char *)*numberOfArgs);
		args[numberOfArgs-1] = NULL;//As required in the specifications
		int result;
		if(access(path, F_OK) == -1){
			char * newPath = checkPATH(path);//Check if command exists in PATH if it doesn't in the local env
			if(strcmp(newPath, "") != 0){
				result = execv(newPath, args);
			} else {
				errno = ENOENT;//File not found
				result = -1;
			}
		} else {
			result = execv(path, args);
		}
		if(result == -1) {
			printf("An error has occured\n");
			printf("%s\n", strerror(errno));
		}
		close(file);
		break;//we exit the loop and kill the child
	} else{
		int status;
		waitpid(pid, &status, 0);
	}*/
		free(buffer);
		free(args);
		free(path);
	}

	return 0;
}
