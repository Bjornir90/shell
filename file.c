#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>


int copyFile(char* source, char* dest)
{
  int sizeBuf = 2048;
  int fOrigin = open(source, O_RDONLY);
  int fDest = open(dest, O_WRONLY|O_CREAT);
  struct stat st;
  ssize_t res;
  char * buf = malloc(sizeBuf*sizeof(char));
  ssize_t nbrWrite;

  stat(source, &st);
  chmod(dest, st.st_mode);
  do {
    res = read(fOrigin, buf, sizeBuf);
    if(res>0){
      nbrWrite = write(fDest, buf, res);
      printf("Bloc lu %d et copie %d\n", res, nbrWrite);
      if(nbrWrite == -1){
        printf("Error : %s", strerror(errno));
      }
    }
  } while(res > 0);
  close(fOrigin);
  close(fDest);
  free(buf);

  return 0;
}

int copyFolder(char* source, char* dest)
{
  struct stat source_stat;
  stat(source, &source_stat);
  int isDir = S_ISDIR(source_stat.st_mode);
  DIR *dirOrigin = opendir(source);
  struct dirent * dir = malloc(sizeof(struct dirent));
  char * pathOrigin = calloc(5000, sizeof(char));
  char * pathDest = calloc(5000, sizeof(char));
  pathOrigin = strcpy(pathOrigin, source);
  pathDest = strcpy(pathDest, dest);
  while ((dir = readdir(dirOrigin)) != NULL) {
    pathOrigin = strcpy(pathOrigin, source);
    pathOrigin = strcat(pathOrigin, "/");
    pathOrigin = strcat(pathOrigin, dir->d_name);
    pathDest = strcpy(pathDest, dest);
    pathDest = strcat(pathDest, "/");
    pathDest = strcat(pathDest, dir->d_name);
    struct stat origin_stat;
    stat(pathOrigin, &origin_stat);
    int isDirOrigin = S_ISDIR(origin_stat.st_mode);
    if(isDirOrigin){//if it is a sub_directory, we recursively call the function
      mkdir(pathDest, 0777);
      return copyFolder(pathOrigin, pathDest);
    }else if( strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0){//If it's a file and not the .. or ., we copy
      printf("Origine : %s Destination : %s\n", pathOrigin, pathDest);
      copyFile(pathOrigin, pathDest);
    }
  }
  return 0;
}

//return number of files found; list contains dirent of each file
int getAllFiles(char * path, struct dirent ** list){
  DIR* workingDir = opendir(path);
  struct dirent * dir = malloc(sizeof(struct dirent));
  int numberOfFiles = 0;

  while ((dir = readdir(workingDir)) != NULL){
    if(strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0){
      printf("%d:%s:%p allocating block of %d\n", numberOfFiles, dir->d_name, dir, sizeof(struct dirent *)*++numberOfFiles);
      list = realloc(list, sizeof(struct dirent *)*++numberOfFiles);
      list[numberOfFiles-1] = dir;
      printf("%s:%p\n", list[numberOfFiles-1]->d_name, list[numberOfFiles-1]);
      dir = malloc(sizeof(struct dirent));//We allocate a new one each time to not overwrite the preceding dir
    }
  }
  //free(dir);//We allocate one dir too much
  printf("Listed %d files\n", numberOfFiles);
  return numberOfFiles;
}
