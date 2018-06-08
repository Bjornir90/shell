#include "internal.h"
#include "parser.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#define STRSIZE 1024

const int READ_END = 0, WRITE_END = 1;
FILE* fileLog;

char* checkPATH(char* commandName)
{ // Check if the command is valid in the PATH
    char* PATHString = getenv("PATH");
    char* pathToCheck = calloc(sizeof(char), STRSIZE);
    int index = getNextArgument(0, PATHString, pathToCheck, ':');
    while(strcmp(pathToCheck, "") != 0 && index != -1) {
	char* pathFinal = calloc(sizeof(char), STRSIZE * 2);
	strcat(pathFinal, pathToCheck);
	strcat(pathFinal, "/");
	strcat(pathFinal, commandName);
	if(access(pathFinal, F_OK) != -1) {
	    return pathFinal;
	}
	index = getNextArgument(++index, PATHString, pathToCheck, ':');
	free(pathFinal);
    }
    return "";
}

int main(int argc, char* argv[]){
	char currentPath[STRSIZE * 4];
	int redirectCode;
	int shouldQuit = 0;
	system("clear");
	printf("You can type \"help\" to receive help\n");
	fileLog = fopen("shell.log", "w");

	while(!shouldQuit) {
		char** args = calloc(sizeof(char*), 1);
		int numberOfArgs = 0;
		int newLineRemains = 1;
		char* buffer = calloc(sizeof(char), STRSIZE);
		getcwd(currentPath, sizeof(currentPath));

		printf("%s", currentPath);
		while(newLineRemains) {
			newLineRemains = 0;
			printf(">");
			fgets(buffer, STRSIZE, stdin);
			if(*buffer == NULL) {
				system("clear");
				printf("Bye bye !\n");
				if(args != NULL) {
					free(args);
				}
				return 0;
			}
			numberOfArgs = getAllArguments(0, buffer, &args, numberOfArgs, ' ', &newLineRemains);

			char*** argumentsForEachCommand = malloc(sizeof(char**));
			char** commands = malloc(sizeof(char*));
			char** argumentsOfTheCommand = malloc(sizeof(char*));
			int numberOfCommands = 1, numberOfArguments = 0;

			commands[0] = args[0];
			argumentsOfTheCommand[0] = malloc(sizeof(char));
			argumentsOfTheCommand[0][0] = 0; // The first cell is the length of the table [maxSize == 256]
			// As a result of this, argumentsOfTheCommand is not really 0-indexed anymore, at least for the arguments
			// themselves

			for(int i = 1; i < numberOfArgs; i++) { // Parse each individual command, for pipes
				if(strcmp(args[i - 1], "|") == 0) { // If previous argument is a pipe, we have a command

					if(strcmp(args[i], "|") == 0) {
					argumentsForEachCommand[numberOfCommands - 1] = argumentsOfTheCommand;
					continue;
					}
					commands = realloc(commands, sizeof(char*) * ++numberOfCommands);
					commands[numberOfCommands - 1] = args[i];
					// printf("Command %d : %s\n", numberOfCommands, args[i]);

					argumentsForEachCommand = realloc(argumentsForEachCommand, sizeof(char**) * numberOfCommands);
					argumentsForEachCommand[numberOfCommands - 2] = argumentsOfTheCommand;

					numberOfArguments = 0;
					argumentsOfTheCommand = malloc(sizeof(char*));
					argumentsOfTheCommand[0] = malloc(sizeof(char));
					argumentsOfTheCommand[0][0] = 0;

					if(i == numberOfArgs - 1)
					argumentsForEachCommand[numberOfCommands - 1] = argumentsOfTheCommand;

				} else if(strcmp(args[i], "|") != 0) { // We have an argument

					numberOfArguments++;
					argumentsOfTheCommand[0][0] = numberOfArguments; // Add 1 to the number of cells
					argumentsOfTheCommand = realloc(argumentsOfTheCommand, sizeof(char*) * (numberOfArguments + 1));
					argumentsOfTheCommand[numberOfArguments] = args[i];

					if(i == numberOfArgs - 1) // We reached the last arg in args
					argumentsForEachCommand[numberOfCommands - 1] = argumentsOfTheCommand;

				} else {

					// printf("Pipe found\n");
				}
			}

			// Needed to redirect std streams
			int fpfd[2], bpfd[2];
			int result;
			if(numberOfCommands > 1) {
				if(pipe(fpfd) == -1)
					fprintf(fileLog, "Pipe error : %s code : %d\n", strerror(errno), errno);
			}

			for(int i = 0; i < numberOfCommands; i++) { // To add a last process if there are more than 1 command
				int result = handleInternals(args, numberOfArgs);
				if(i > 0) { // not for first command as it has no bpfd
					if(i > 1) {
						fprintf(fileLog, "Closing bpfd[READ_END]");
						close(bpfd[READ_END]);
					}
					fprintf(fileLog, " and turning fpfd into bpfd");
					bpfd[READ_END] = fpfd[READ_END];
					bpfd[WRITE_END] = fpfd[WRITE_END];
					if(i < numberOfCommands - 1) { // Not for last command
						fprintf(fileLog, " and piping fpfd");
						if(pipe(fpfd) == -1)
							fprintf(fileLog, "Pipe error : %s code : %d\n", strerror(errno), errno);
					}
					fprintf(fileLog, "\n");
				}
				if(result == 1) {
					continue; // A internal command has been found, and so we go to the next commmand
				} else if(result == 0) {
					system("clear");
					printf("Goodbye !\n");
					if(buffer != NULL)
						free(buffer);
					if(args != NULL) {
						for(int i = 0; i < numberOfArgs; i++) {
							if(args[i] != NULL)
							free(args[i]);
						}
						free(args);
					}
					return 0;
				}
				int pid = fork();
				if(pid == 0) { // We are in the child
					if(numberOfCommands > 1) {
						if(i > 0 && i < numberOfCommands - 1) { // Command in the middle of others
							fprintf(fileLog, "Redirecting both stdin and stdout\n");
							close(bpfd[WRITE_END]);
							close(fpfd[READ_END]);
							if(dup2(bpfd[READ_END], 0) == -1)
							fprintf(fileLog, "dup2 error on bpfd read end : %s code : %d\n", strerror(errno), errno); // stdin to backward pipe read end
							if(dup2(fpfd[WRITE_END], 1) == -1)
							fprintf(fileLog, "dup2 error on fpfd write end : %s code : %d\n", strerror(errno), errno); //stdout to forward pipe write end
							close(bpfd[READ_END]);
							close(fpfd[WRITE_END]);
						} else {
							if(i > 0) { // Last command
								fprintf(fileLog, "Redirecting stdin\n");
								close(bpfd[WRITE_END]);
								if(dup2(bpfd[READ_END], 0) == -1)
									fprintf(fileLog, "dup2 error on bpfd read end : %s code : %d\n", strerror(errno), errno); // stdin to pipe read end
								close(bpfd[READ_END]);
							}
							if(i < numberOfCommands - 1) { // First command
								fprintf(fileLog, "Redirecting stdout\n");
								close(fpfd[READ_END]);
								if(dup2(fpfd[WRITE_END], 1) == -1)
									fprintf(fileLog, "dup2 error on fpfd write end : %s code : %d\n", strerror(errno), errno); // stdout to pipe write end
								close(fpfd[WRITE_END]);
							}
						}
					}

					fprintf(fileLog, "In child : %d\n", getpid());
					fflush(fileLog);

					char** arguments = malloc(sizeof(char*) * (argumentsForEachCommand[i][0][0] + 2));
					// arguments has 2 cells more than the number of arguments (end null and command name at the start),
					// so we malloc with +2
					arguments[0] = commands[i]; // first argument given to the program is it's name
					for(int j = 1; j <= argumentsForEachCommand[i][0][0]; j++) {
						arguments[j] = argumentsForEachCommand[i][j];
					}
					arguments[argumentsForEachCommand[i][0][0] + 1] = (char*)NULL; // As required
					if(access(arguments[0], F_OK) == -1) {
						char* newPath = checkPATH(arguments[0]); // Check if command exists in PATH if it doesn't in the local env
						if(strcmp(newPath, "") != 0) {
							fprintf(fileLog, "Calling execv on %s (in PATH)\n", newPath);
							for(int j = 0; j <= argumentsForEachCommand[i][0][0]; j++) {
								fprintf(fileLog, "\targuments %d : %s\n", j, arguments[j]);
								fflush(fileLog);
							}
							result = execv(newPath, arguments);
						} else {
							errno = ENOENT; // File not found
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

					fclose(fileLog);
					exit(1);
				} else { // In the parent process
					int status;
					fprintf(fileLog, "i : %d\n", i);
					if(i > 0) {
						fprintf(fileLog, "Closing backward pipe file descriptor WRITE_END\n");
						close(bpfd[WRITE_END]);
					}
					fflush(fileLog);
					pid_t return_pid = waitpid(pid, &status, 0);
				}
			}
			if(buffer != NULL)
				free(buffer);
			if(args != NULL) {
				for(int i = 0; i < numberOfArgs; i++) {
					if(args[i] != NULL)
						free(args[i]);
				}
				free(args);
			}
		}
	}

    return 0;
}
