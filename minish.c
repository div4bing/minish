#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdbool.h>

#define BINARY_SIZE 1024                     // 1024 * 1024
#define ARGUMENT_SIZE 1024                   // 1024 * 1024

int showMinish(void);
long long fetchTotalCommands(char **commandWithArg);                            // Store | separated command and Retuns total commands
int performCommand(char **commandWithArg, long long totalCommands);             // Perform the fork on individual comand
int parseCommand(char *commandWithArg, char *command, char *argument[]);     // Return number of arguments

int main(int argc, char * argv[])
{
  int ret = 0;

  ret = showMinish();

  if(ret != 0)
  {
    printf("Error occured in mini Shell\n");
  }
  return 0;
}

int showMinish(void)
{
  // pid_t pid;
  long long totalCommands = 0;
  char **commandWithArg = (char **)malloc(BINARY_SIZE*sizeof(char));
  int status = 0;

  for (int i = 0; i< BINARY_SIZE; i++)
  {
    commandWithArg[i] = (char *)malloc(BINARY_SIZE*sizeof(char));
  }

  printf("minish> ");

  totalCommands = fetchTotalCommands(commandWithArg);

  status = performCommand(commandWithArg, totalCommands);

  showMinish();
  free(commandWithArg);
  return 0;
}

long long fetchTotalCommands(char **commandWithArg)
{
  char *inputString = NULL;
  char *tempCommandWithArg = NULL;
  size_t len = 0;
  ssize_t lenOfCommand = 0;
  long long commandCount = 0;

  lenOfCommand = getline(&inputString, &len, stdin);
  *(inputString+lenOfCommand) = '\0';
  inputString = strtok(inputString, "\n");                                      // Trim inputString for \n
  printf("Retrieved line is: [%s]\n", inputString);

  tempCommandWithArg = strtok(inputString, "|");

  while (tempCommandWithArg != NULL)
  {
    strcpy(commandWithArg[commandCount++],tempCommandWithArg);
    tempCommandWithArg = strtok(NULL, "|");
  }

  commandWithArg[commandCount] = NULL;     // In case if we want to detect how many command we have based till we find a NULL

  free(inputString);
  free(tempCommandWithArg);
  return commandCount;
}

int performCommand(char **commandWithArg, long long totalCommands)
{
  char *command = (char *)malloc(BINARY_SIZE*sizeof(char));
  char *argument[ARGUMENT_SIZE] = {'\0'};
  int totalArgument = 0;
  int tmpTotalCmd = totalCommands;
  pid_t pid;
  int *wstatus;

  for(int i=0; i< ARGUMENT_SIZE;i++)
  {
    argument[i] = malloc(ARGUMENT_SIZE*sizeof(char));
  }

  while (tmpTotalCmd > 0)                                                        // Only do this if the total commands is at least 1
  {
    // printf("Index#%lld\n", (totalCommands - tmpTotalCmd));
    totalArgument = parseCommand(commandWithArg[totalCommands - tmpTotalCmd], command, argument);
    tmpTotalCmd--;

    argument[totalArgument] = '\0';
//###########################################################
    if ((pid = fork()) == -1)
    {
      perror("Failed to fork child process");
      return -1;
    }

    if (pid == 0)   // child
    {
      if(strcmp(command, "cd") == 0)                                         // Handles CD command
      {
          if(chdir(argument[0]) == -1)
          {
            perror("CD failed");
          }
      }
      else if (execvp(command, argument) == -1)
      {
        perror("Execv Failed");
        return -1;
      }
    }

    wait(wstatus);
//###########################################################
  }

  free(command);
  return 0;
}

int parseCommand(char* commandWithArg, char *command, char *argument[])
{
  char * tmpStr = strtok(commandWithArg, " ");
  if (tmpStr != NULL)                             // Fetch the command first
  {
    strcpy(command,tmpStr);
    strcpy(argument[0],tmpStr);                   // First argument should be the command itself
  }
  else
  {
    return 0;   // We don't have command so argument does not matter
  }

  int count = 1;
  tmpStr = strtok(NULL, " ");                     // Start fetching arguments
  while (tmpStr != NULL)
  {
    strcpy(argument[count++],tmpStr);
    tmpStr = strtok(NULL, " ");
  }

  argument[count] = '\0';


  // printf("Command: [%s]\n", command);
  // for (int i=0; i < count; i++)
  // {
  //   printf("Argument#%d: [%s]\n",i, argument[i]);
  // }
  return count;
}
