#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
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
  } else if (strcmp(path, "cat") == 0){
    char pathToFile[STRSIZE*4];
    getNextArgument(++index, buffer, pathToFile, ' ');

    if(strcmp(pathToFile, "") == 0){
      fprintf(stderr, "cat : empty filename\n");
      return 1;
    }

    int file = open(pathToFile, O_RDONLY);
    if (file == -1){
      fprintf(stderr, "Could not open file\n %s\n", strerror(errno));
      return 1;
    }

    char buffer[STRSIZE];
    int numberOfBytesRead;
    do {
      numberOfBytesRead = read(file, buffer, STRSIZE-1);
      buffer[numberOfBytesRead] = '\0';
      printf("%s", buffer);
    } while (numberOfBytesRead == STRSIZE-1);

    close(file);
    return 1;
  } else if (strcmp(path, "pid") == 0){
    printf("Process ID : %d\n", getpid());
    return 1;
  } else if (strcmp(path, "ls") == 0){
    char argument[STRSIZE*4];
    char source[STRSIZE*4];
    int listAllFiles = 0;
    int detailsRequired = 0;
    char lastChar = buffer[index];
    while (lastChar != '\0' && lastChar != '\n'){
      index = getNextArgument(++index, buffer, argument, ' ');
      if (index > -1){
        if (*argument != '-'){
          strcpy(source, argument);
        } else {
          if (strcmp(argument, "-a") == 0){
            listAllFiles = 1;
          } else if (strcmp(argument, "-l") == 0){
            detailsRequired = 1;
          }
        }
      } else {
        break;
      }
      lastChar = buffer[index];
    }
    if(strcmp(source, "") == 0 || strcmp(source, ".") == 0){
      getcwd(source, sizeof(source));
    }
    struct stat source_stat;
    stat(source, &source_stat);
    int isDir = S_ISDIR(source_stat.st_mode);
    if(!isDir){
      fprintf(stderr, "ls : input is not a directory\n");
      return 1;
    }
    DIR *dirOrigin = opendir(source);
    struct dirent * dir = malloc(sizeof(struct dirent));
    while ((dir = readdir(dirOrigin)) != NULL) {
      if(*dir->d_name == '.' && !listAllFiles) continue; //We don't print files starting with '.'
      //TODO : manage -l command
      printf("%s\n", dir->d_name);
    }
    return 1;
  }
  return -1;
}
