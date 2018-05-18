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
	getcwd(currentPath, sizeof(currentPath));
	int redirectCode;
	int shouldQuit = 0;
	printf("You can type \"help\" to receive help\n");

	while(!shouldQuit){
		char ** args = calloc(sizeof(char *), 1);
		int numberOfArgs;
		int index = 0;
		char * buffer = calloc(sizeof(char), STRSIZE);
		printf("%s>", currentPath);
		fgets(buffer, STRSIZE, stdin);
		if (buffer == NULL){
			printf("Bye bye !\n");
			return 0;
		}

		numberOfArgs = getAllArguments(0, buffer, args, " ");
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
		char * path = args[0];

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
/*if(redirectCode){//We want to change the output
dup2(file, 1);
} else {//We want to select an input
dup2(file, 0);
}
dup2(file, redirectCode);
}
*/
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
}

return 0;
}
