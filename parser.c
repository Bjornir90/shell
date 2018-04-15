#include <string.h>

int getNextArgument(int index, char * input, char * argument, char separator){
  char iterator = input[index];
  int indexArgument = 0;
  if(iterator == '\0' || strcmp(input, "") == 0){//If the input is empty
    argument = "";
    return -1;
  }
  while(iterator != separator && iterator != '\n'){//Loop to the end of the word
    argument[indexArgument] = iterator;//Build argument string
    indexArgument++;
    index++;
    iterator = input[index];
  }
  argument[indexArgument] = '\0';
  return index;
}
