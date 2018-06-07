#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#define STRSIZE 1024

int getNextArgument(int index, char * input, char * argument, char separator){
  char iterator = input[index];
  int indexArgument = 0;
  int evenNumberBrackets = 1;
  if(iterator == '\0' || strcmp(input, "") == 0){//If the input is empty
    argument = "";
    return -1;
  }
  while(iterator != separator && iterator != '\n'){//Loop to the end of the word
    if (iterator == '"'){ //If you find a guard symbol ("), it allows you to have spaces until the next one
        //1st iteration to skip the 1st ".
        index++;
        iterator = input[index];
        //printf("1ere iteration après un \" \n");
        while(iterator != '"' && iterator != '\n'){
          //printf("iterator = %c \n", iterator );
          if (iterator == '\\'){ // if you find an escaping character
            index++; // you skip it
            iterator = input[index];
            argument[indexArgument] = iterator;// and add the following
            indexArgument++;
            index++; //then continue normally
            iterator = input[index];
            if (iterator == '\n'){
              evenNumberBrackets = 0;
            }
          }
          else{
            argument[indexArgument] = iterator;//Build argument string
            indexArgument++;
            index++;
            iterator = input[index];
            if (iterator == '\n'){
              evenNumberBrackets = 0; //if the line end with a \n and not a " it means that there is an odd number of brackets.
            }
          }
        }
        if (evenNumberBrackets){
        //last iteration to skip the last " if you have an even number of brackets.
        index++;
        iterator = input[index];
        //printf("derniere iteration après un \" \n");
      }
    }
    else{
      argument[indexArgument] = iterator;//Build argument string
      indexArgument++;
      index++;
      iterator = input[index];
    }
  }
  argument[indexArgument] = '\0';
  //printf("argument = %s \n", argument);
  return index;
}

int getAllArguments(int index, char * input, char ** result, char separator, int* newLineRemains){
	char lastChar = input[index];
	int numberOfArgs = 0;
	while(lastChar != '\n' && lastChar != '\0' && lastChar != '\\'){//Loop to the end of the string or to the end of a line
		char * argument = malloc(sizeof(char)*STRSIZE);//Allocate new argument, that we will put in result when we it is built
		index = getNextArgument(index, input, argument, separator);
		if(index == -1) return numberOfArgs;
		lastChar = input[++index];
    printf("dernier caractère = %c .\n",lastChar );
    if(lastChar == '\\'){
      printf("\\ détecté \n");
      *newLineRemains = 1;
    }
    else if(lastChar == '\n' || lastChar == '\0'){
      *newLineRemains = 0;
    }
		result = realloc(result, sizeof(char *)*++numberOfArgs);//Dynamically allocate memory to accept any number of arguments
		result[numberOfArgs-1] = argument;
	}
	return numberOfArgs;
}
