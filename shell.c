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
  char *currentPath = calloc(STRSIZE*4, sizeof(char));
  strcat(currentPath, "/home");
  int redirectCode;
  int shouldQuit = 0;
  printf("You can type \"help\" to receive help\n");

  while(!shouldQuit){
    char ** args = calloc(sizeof(char *), 1);
    int numberOfArgs = 1;
    int index = 0;
    char * path = calloc(sizeof(char), STRSIZE);
    char * buffer = calloc(sizeof(char), STRSIZE);
    printf("%s>", currentPath);
    fgets(buffer, STRSIZE, stdin);


    char iterator = *buffer;

    while(iterator != ' ' && iterator != '\n'){
      path[index] = iterator;
      index++;
      iterator = buffer[index];
    }
    args[0] = path;

    //Internal commands
    int internalResult = handleInternals(path, buffer, index, currentPath);
    if(internalResult){//A command has been found and we should quit this iteration
      continue;
    } else if(internalResult == 0){
      shouldQuit = 1;
      free(buffer);
      free(args);
      free(path);
      free(currentPath);
      printf("It was good seeing you, bye bye !\n");
      return 0;
    }

    int shouldRedirect = 0;
    index++;//Move to the character after the space
    while(iterator != '\n' && iterator != '\0'){//Loop to the end of the string
      char * argument = malloc(sizeof(char)*STRSIZE);//Allocate new argument, that we will put in args when we it is built
      index = getNextArgument(index, buffer, argument, ' ');

      if(*argument == '>'){
        shouldRedirect = 1;
        redirectCode = 1;
        break;
      } else if(*argument == '<'){
        shouldRedirect = 1;
        redirectCode = 0;
        break;
      }
      if(*argument == '2' && argument[1] == '>'){
        shouldRedirect = 1;
        redirectCode = 2;
        index++;
        break;
      }

      iterator = buffer[++index];
      numberOfArgs++;
      args = realloc(args, sizeof(char *)*numberOfArgs);//Dynamically allocate memory to accept any number of arguments
      args[numberOfArgs-1] = argument;
    }

    //Needed to redirect std streams
    int file;

    int pid = fork();
    if(pid == 0){//In the child
      //Redirect as needed
      if(shouldRedirect){
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
        }*/
        dup2(file, redirectCode);
      }

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
