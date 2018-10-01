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
int parseCommand(char *commandWithArg, char *command, char *argument[], int *isBackground, int *isInputRedirect, int *isOutputRedirect, char *inputFileName, char *outputFileName);     // Return number of arguments

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
  // printf("Retrieved line is: [%s]\n", inputString);

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
  int isBackground = 0;
  int isInputRedirect = 0;
  int isOutputRedirect = 0;
  char *inputFileName = (char *)malloc(BINARY_SIZE*sizeof(char));
  char *outputFileName = (char *)malloc(BINARY_SIZE*sizeof(char));;

  FILE *inputRedirectFile, *outputRedirectFile;

  for(int i=0; i< ARGUMENT_SIZE;i++)
  {
    argument[i] = malloc(ARGUMENT_SIZE*sizeof(char));
  }

  while (tmpTotalCmd > 0)                                                        // Only do this if the total commands is at least 1
  {
    isBackground = 0;
    isInputRedirect = 0;
    isOutputRedirect = 0;

    // printf("Index#%lld\n", (totalCommands - tmpTotalCmd));
    totalArgument = parseCommand(commandWithArg[totalCommands - tmpTotalCmd], command, argument, &isBackground, &isInputRedirect, &isOutputRedirect, inputFileName, outputFileName);

    if(strcmp(command, "exit") == 0)                                 // Handles exit command in minish
    {
      free(command);
      free(inputFileName);
      free(outputFileName);
      exit(0);
    }

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
      // Check for regular commands "exit" and "cd"
      if(strcmp(command, "cd") == 0)                                         // Handles CD command
      {
        if(chdir(argument[1]) == -1)
        {
          perror("CD failed");
        }
      }
      else
      {
        if(isBackground == 1)
        {
          printf("Process %d in background mode\n", getpid());
        }

        if (isInputRedirect == 1)                                             // Redirecy Input
        {
          printf("INPUT FILE: %s\n", inputFileName);
          inputRedirectFile = fopen(inputFileName,"r");
          dup2(fileno(inputRedirectFile), 0);
          fclose(inputRedirectFile);
        }

        if (isOutputRedirect == 1)                                       // Redirect Output
        {
          printf("OUTPUT FILE: %s\n", outputFileName);
          outputRedirectFile = fopen(outputFileName,"w");
          dup2(fileno(outputRedirectFile), 1);
          fclose(outputRedirectFile);
        }

        if (execvp(command, argument) == -1)
        {
          perror("Execv Failed");
          return -1;
        }
      }
    }
    else
    {
      if(isBackground == 0)                                       // If in background, don't wait for child to die and ready to serve next command
      {
        // wait(wstatus);
        waitpid(pid, wstatus, 0);
      }
      else
      {
        waitpid(-1, wstatus, WNOHANG);
      }
    }
//###########################################################
  }

  free(command);
  free(inputFileName);
  free(outputFileName);
  return 0;
}

int parseCommand(char* commandWithArg, char *command, char *argument[], int *isBackground, int *isInputRedirect, int *isOutputRedirect, char *inputFileName, char *outputFileName)
{
  char * tmpStr = strtok(commandWithArg, " ");
  if (tmpStr != NULL)                             // Fetch the command first
  {
    strcpy(command,tmpStr);
    strcpy(argument[0],tmpStr);                   // First argument should be the command itself

    if (command[strlen(command) - 1] == '&')
    {
      *isBackground = 1;
    }
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

  printf("BEFORE Command: [%s]\n", command);
  for (int i=0; i < count; i++)
  {
    printf("Argument#%d: [%s]\n",i, argument[i]);
  }

  //Check if we have a redirection of input/output
  for(int i=0; i < count; i++)
  {
    if (*argument[i] == '<')                   // Check for input redirection
    {
      *isInputRedirect = 1;
      strcpy(inputFileName,argument[i+1]);
      for (int j=i+1; j < count; j++)          // Skip '<' and copy rest as argument
      {
        strcpy(argument[i],argument[j]);
      }
    }

    if (*argument[i] == '>')                   // Check for outpit redirection
    {
      *isOutputRedirect = 1;
      strcpy(outputFileName,argument[i+1]);
      argument[i] = '\0';
      count = i;
    }
  }

  if(*isInputRedirect == 1)     // We removed one argument as form of '<'
  {
    count = count-1;
  }

  argument[count] = '\0';

  if (*argument[count-1] == '&')                   // Check for the background process
  {
    *isBackground = 1;
    argument[count-1] = '\0';                     // Remove & from the argument list
  }

  printf("AFTER Command: [%s]\n", command);
  for (int i=0; i < count; i++)
  {
    printf("Argument#%d: [%s]\n",i, argument[i]);
  }
  return count;
}
