#include <dirent.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "parser.h"
#define STRSIZE 1024

int handleInternals(char *path, char *buffer, int index, char *currentPath){
  const char exitCommand[] = "exit";
  const char helpCommand[] = "help";
  if(strcmp(exitCommand, path) == 0){//if the user request to quit
    return 0; //we exit the loop after freeing memory
  } else if (strcmp(helpCommand, path) == 0){//If the user request help
    printf("You can quit by typing \"%s\"\n You can redirect output and input using > and < respectively at the end of your command\n", exitCommand);
    return 1;//Quit this iteration
  } else if (strcmp("cd", path) == 0){//User request to change directory
    char *newDirectory = calloc(STRSIZE*4, sizeof(char));
    getNextArgument(++index, buffer, newDirectory, ' ');
    if(strcmp(newDirectory, "") == 0){
      fprintf(stderr, "Empty directory\n");
      return 1;
    }
    if(*newDirectory == '/'){//Absolute path
      DIR* dir = opendir(newDirectory);
      if (dir)  strcpy(currentPath, newDirectory); //Directory exists
      else if (errno == ENOENT) fprintf(stderr, "%s : directory does not exists\n", newDirectory);
    } else if(*newDirectory == '.' && newDirectory[1] == '.'){//Go up one node in the file tree

      //Find last '/' in path
      int lastIndexOfOccurence = 0;
      int i;
      for(i=0; i<STRSIZE*4; i++){
        if(currentPath[i] == '/'){
          lastIndexOfOccurence = i;
        } else if(currentPath[i] == '\0'){
          break;
        }
      }

      currentPath[lastIndexOfOccurence] = '\0';//Remove the end of the path

    } else if(*newDirectory == '~'){//Go back to home directory
      strcpy(currentPath, "/home");
    } else {
      char *temp = malloc(sizeof(char)*STRSIZE*4);
      strcpy(temp, currentPath);//Backup the current path in case the directory does not exists
      strcat(currentPath, "/");
      strcat(currentPath, newDirectory);
      DIR* dir = opendir(currentPath);
      if (errno == ENOENT){//If directory does not exists, reverts to previous position
        fprintf(stderr, "%s : directory does not exists\n", currentPath);
        strcpy(currentPath, temp);
      }
      free(temp);
    }
    return 1;
  }
}
