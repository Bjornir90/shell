#include <dirent.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "parser.h"
#define STRSIZE 1024

int handleInternals(char *path, char *buffer, int index){
  const char exitCommand[] = "exit";
  const char helpCommand[] = "help";
  char currentPath[STRSIZE*4];

  getcwd(currentPath, sizeof(currentPath));

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
      if (chdir(newDirectory) == -1){
        fprintf(stderr, "%s : error accessing directory\n", newDirectory);
      }
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

      currentPath[lastIndexOfOccurence] = '\0'; //Remove the end of the path
      chdir(currentPath);
    } else if(*newDirectory == '~'){//Go back to home directory
      chdir("/home");
    } else {//Relative path
      if(strlen(currentPath) > 1){//We are not in the root directory
        strcat(currentPath, "/");
      }
      strcat(currentPath, newDirectory);
      if (chdir(currentPath) == -1){
        fprintf(stderr, "%s : error accessing directory\n", currentPath);
      }
    }
    return 1;
  }
  return -1;
}
