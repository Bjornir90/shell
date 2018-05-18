#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#define STRSIZE 1024

int getNextArgument(int index, char * input, char * argument, char separator){
	char iterator = input[index];
	int indexArgument = 0;
	if(iterator == '\0' || strcmp(input, "") == 0){//If the input is empty
		argument = "";
		return -1;
	}
	while(iterator != separator && iterator != '\n'){//Loop to the end of the word
		argument[indexArgument] = iterator;//Build argument string
		printf(" %c ", iterator);
		indexArgument++;
		index++;
		iterator = input[index];
	}
	argument[indexArgument] = '\0';
	return index;
}

int getAllArguments(int index, char * input, char ** result, char separator){
	char lastChar = input[index];
	int numberOfArgs = 0;
	while(lastChar != '\n' && lastChar != '\0'){//Loop to the end of the string
		char * argument = malloc(sizeof(char)*STRSIZE);//Allocate new argument, that we will put in result when we it is built
		index = getNextArgument(index, input, argument, separator);
		if(index == -1) return numberOfArgs;
		lastChar = input[++index];
		result = realloc(result, sizeof(char *)*++numberOfArgs);//Dynamically allocate memory to accept any number of arguments
		result[numberOfArgs-1] = argument;
	}
	return numberOfArgs;
}
