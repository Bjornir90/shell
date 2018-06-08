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
#include "parser.h"
#include "internal.h"
#define STRSIZE 1024

char * checkPATH(char * commandName){//Check if the command is valid in the PATH
	char * PATHString = getenv("PATH");
	char * pathToCheck = calloc(sizeof(char), STRSIZE);
	int index = getNextArgument(0, PATHString, pathToCheck, ':');
	while(strcmp(pathToCheck, "") != 0 && index != -1){
		char * pathFinal = calloc(sizeof(char), STRSIZE*2);
		strcat(pathFinal, pathToCheck);
		strcat(pathFinal, "/");
		strcat(pathFinal, commandName);
		if(access(pathFinal, F_OK) != -1){
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
		int numberOfArgs = 0;
		int index = 0;

		int newLineRemains = 1; //boolean to see if you need to wait for new lines7

		char * buffer = calloc(sizeof(char), STRSIZE);
		getcwd(currentPath, sizeof(currentPath));
		if (buffer == NULL){ //jamais null car quand fgets renconter un probleme = laisse intact la chaine et renvoie null, mais buffer inchangé
			printf("Bye bye !\n");
			return 0;
		}

			printf("%s>", currentPath);
			while(newLineRemains){
				newLineRemains = 0;
				if( fgets(buffer, STRSIZE, stdin) == NULL) {
							printf("Error while reading standard input. Exiting.\n");
							return 1;
						}
				numberOfArgs = getAllArguments(0, buffer, args, numberOfArgs, ' ', &newLineRemains);
				printf(" number of args : %d\n", numberOfArgs );
				for(int i = 0; i< numberOfArgs; i++){
					printf("dans args : %s\n", args[i] );
				}
				printf("new line remains = %d .\n", newLineRemains );
				index = numberOfArgs;
		}


		int result = handleInternals(args, numberOfArgs);
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
		}

		char *path = args[0];
		char ***argumentsForEachCommand = malloc(sizeof(char **));
		char **commands = malloc(sizeof(char *));
		int numberOfCommands = 1, numberOfArguments = 0;

		commands[0] = args[0];
 //////////////////////// ICI ON COMMENCE A TRAITER LES COMMANDES //////////////////////////
 if(!newLineRemains){
		for(int i = 1; i<numberOfArguments; i++){//Parse each individual command, for pipes
			char **argumentsOfTheCommand = malloc(sizeof(char *));
			if(args[i-1] == "|"){//If previous argument is a pipe, we have a command
				commands = realloc(commands, sizeof(char *)*++numberOfCommands);
				commands[numberOfCommands-1] = args[i];
				if(numberOfArguments>0){
					argumentsForEachCommand = realloc(argumentsForEachCommand, sizeof(char **)*numberOfCommands);
					argumentsForEachCommand[numberOfCommands-1] = argumentsOfTheCommand;
				} else {
					argumentsForEachCommand[numberOfCommands-1] = NULL;
				}
				numberOfArguments = 0;
			} else if(args[i] != "|"){//We have an argument
				argumentsOfTheCommand = realloc(argumentsOfTheCommand, sizeof(char *)*++numberOfArguments);
				argumentsOfTheCommand[numberOfArguments-1] = args[i];
			}
		}

		//Needed to redirect std streams
		int file;

		int pid = fork();
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
		}


		free(buffer);
		free(args);
		free(path);
	}// end of the if(canBeExecuted);
}//end of the while(!shouldQuit);

	return 0;
}
