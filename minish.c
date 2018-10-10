//TODO: BUGLIST->
//1. SegFault when : sort -n < sort.txt | grep 2 | less > hi.txt

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
long long fetchTotalCommands(char *commandWithArg[]);                            // Store | separated command and Retuns total commands
int performCommand(char *commandWithArg[], long long totalCommands);             // Perform the fork on individual comand
int parseCommand(char *commandWithArg, char *command, char *argument[], int *isBackground, int *isInputRedirect, int *isOutputRedirect, char *inputFileName, char *outputFileName);     // Return number of arguments
void signalHandler(int signum);

int pipefd[BINARY_SIZE][2]; // Handle as many number of commands
pid_t childgroupIDs[BINARY_SIZE*BINARY_SIZE];
long long childCount = 0;

int main(int argc, char * argv[])
{
  int ret = 0;
  struct sigaction signalActionNew, signalActionOld;

  signalActionNew.sa_handler = signalHandler;
  sigemptyset (&signalActionNew.sa_mask);
  signalActionNew.sa_flags = SA_RESTART;      // Restart an Interrupted System call, Used when child dies in background, signal generates and getline is called

  sigaction (SIGINT, NULL, &signalActionOld);
  if (signalActionOld.sa_handler != SIG_IGN)
    sigaction (SIGINT, &signalActionNew, NULL);
  sigaction (SIGCHLD, NULL, &signalActionOld);
  if (signalActionOld.sa_handler != SIG_IGN)
    sigaction (SIGCHLD, &signalActionNew, NULL);

  ret = showMinish();

  if(ret != 0)
  {
    printf("Error occured in mini Shell\n");
  }
  return 0;
}

int showMinish(void)
{
  long long totalCommands = 0;
  char *commandWithArg[ARGUMENT_SIZE];
  int status = 0;

  childCount = 0;     // Reset number of child process

  for (int i = 0; i< BINARY_SIZE; i++)
  {
    commandWithArg[i] = (char *)malloc(BINARY_SIZE*sizeof(char));
  }

  printf("minish> ");

  totalCommands = fetchTotalCommands(commandWithArg);

  status = performCommand(commandWithArg, totalCommands);

  for (int i = 0; i< BINARY_SIZE; i++)      // Free the memory
  {
    free(commandWithArg[i]);
  }

  showMinish();
  return 0;
}

long long fetchTotalCommands(char *commandWithArg[])
{
  FILE *fp;
  char *inputString;
  char *tempCommandWithArg = NULL;
  size_t len = 0;
  ssize_t lenOfCommand = 0;
  long long commandCount = 0;

  if ( (fp = fdopen(STDIN_FILENO, "r")) == NULL)
  {
		perror("Error, can't convert STDIN to read new command\n");
		exit(-1);
	}

	if ( (lenOfCommand = getline(&inputString, &len, fp)) == -1)
  {
		perror("error reading line from stdin\n");
		exit(-1);
	}
  *(inputString+lenOfCommand) = '\0';

  inputString = strtok(inputString, "\n");                                      // Trim inputString for \n

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

int performCommand(char *commandWithArg[], long long totalCommands)
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
  char *outputFileName = (char *)malloc(BINARY_SIZE*sizeof(char));
  int commandCount = 0;
  int initialChildFlag = 0; // If 0, child is initial child else not

  if(command == NULL || inputFileName == NULL || outputFileName == NULL)
  {
    perror("Memory not allocated.");
    exit(-1);
  }

  FILE *inputRedirectFile, *outputRedirectFile;

  for(int i=0; i< ARGUMENT_SIZE;i++)
  {
    argument[i] = malloc(ARGUMENT_SIZE*sizeof(char));
  }

  for (int j=0; j < (tmpTotalCmd-1); j++)   // Create required number of pipes
  {
    // printf("Creating Pipe at # %d && tmpTotalCmd=%d\n", j, tmpTotalCmd);
    if (pipe(pipefd[j]) == -1)
    {
      perror("Error Creating a Pipe");
      exit(-1);
    }
  }

  while (tmpTotalCmd > 0)                                                        // Only do this if the total commands is at least 1
  {
    isBackground = 0;
    isInputRedirect = 0;
    isOutputRedirect = 0;

    totalArgument = parseCommand(commandWithArg[totalCommands - tmpTotalCmd], command, argument, &isBackground, &isInputRedirect, &isOutputRedirect, inputFileName, outputFileName);

    // printf("COMMAND# %s isBackground=%d isInputRedirect=%d isOutputRedirect=%d\n", command, isBackground, isInputRedirect, isOutputRedirect);
    if(strcmp(command, "exit") == 0)                                 // Handles exit command in minish
    {
      free(command);
      free(inputFileName);
      free(outputFileName);

      for(long long i=0; i < childCount; i++)
      {
        printf("Sending SIGKILL to: %d\n", childgroupIDs[i]);
        if(killpg(childgroupIDs[i], SIGKILL) == -1)
        {
          perror("Error killing the process group");
        }
      }

      if (killpg(getpid(), SIGKILL) == -1)                          // Send SIGKILL to all the processes in the Process group created by parent Process minish
      {
        perror("Error killing the process group");
      }

      exit(0);
    }

    argument[totalArgument] = '\0';

    if ((pid = fork()) == -1)
    {
      perror("Failed to fork child process");
      return -1;
    }

    if (pid == 0)   // child
    {
      if(isBackground != 1)   // Only for the foregroud processes
      {
        if(setsid() == -1)      // Set pid of the child as it's group ID
        {
          perror("Error Setting Process Group ID for child");
          exit(-1);
        }
      }
      // printf("PGID from Child: %d\n", getpgid(getpid()));

      if(tmpTotalCmd > 0)
      {
        // printf("commandCount=%d && totalCommands=%lld\n", commandCount, totalCommands-1);
        if(commandCount != 0)
        {
          close(pipefd[commandCount-1][1]);
          // printf("Redirect Input from #%d\n", commandCount - 1);
          if (dup2(pipefd[commandCount-1][0], 0) == -1)
          {
            perror("Error Redirecting Input");
            return -1;
          }
          // close(pipefd[commandCount-1][0]);
        }
        else
        {
          if (dup2(fileno(stdin), 0) == -1)
          {
            perror("Error Redirecting Input");
            return -1;
          }
        }

        if (commandCount < (totalCommands-1))
        {
          close(pipefd[commandCount][0]);
          // printf("Redirect Output to #%d\n", commandCount);
          if (dup2(pipefd[commandCount][1], 1) == -1)
          {
            perror("Error Redirecting Output");
            return -1;
          }
          // close(pipefd[commandCount][1]);
        }
        else
        {
          if (dup2(fileno(stdout), 1) == -1)
          {
            perror("Error Redirecting Output");
            return -1;
          }
        }
      }

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
          inputRedirectFile = fopen(inputFileName,"r");
          if(inputRedirectFile == NULL)
          {
            perror("Error Opening file for Input Redirection");
            return -1;
          }

          if (dup2(fileno(inputRedirectFile), 0) == -1)
          {
            perror("Error Redirecting Input");
            return -1;
          }
          fclose(inputRedirectFile);
        }

        if (isOutputRedirect == 1)                                       // Redirect Output
        {
          outputRedirectFile = fopen(outputFileName,"w");

          if (outputRedirectFile == NULL)
          {
            perror("Error Opening file for Output Redirection");
            return -1;
          }

          if (dup2(fileno(outputRedirectFile), 1) == -1)
          {
            perror("Error Redirecting Output");
            return -1;
          }
          fclose(outputRedirectFile);
        }

        if (execvp(command, argument) == -1)
        {
          perror("Execv Failed");
          return -1;
        }
      }
    }
    else      // Parent
    {
      if (commandCount != 0)
      {
        // printf("CLOSING commandCount=%d\n", commandCount-1);
        close(pipefd[commandCount-1][0]);
        close(pipefd[commandCount-1][1]);
      }

      tmpTotalCmd--;
      commandCount++;
      if(isBackground == 0)                                       // If in background, don't wait for child to die and ready to serve next command
      {
        childgroupIDs[childCount++] = pid;      // Add child Group ID in Array for further killing the child through signalpg
        waitpid(pid, wstatus, 0);
      }
    }
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

  // printf("BEFO tmpStr=%s\n", tmpStr);
  while (tmpStr != NULL)
  {
    // printf("YES tmpStr=[%s] count=%d argument[1]=%s argument[2]=%s\n", tmpStr, count, argument[1], argument[2]);
    // argument[count++] = strdup("0");
    strcpy(argument[count++],tmpStr);   //TODO: SegFault when : sort -n < sort.txt | grep 2 | less > hi.txt
    tmpStr = strtok(NULL, " ");
    // printf("tmpStr=%s\n", tmpStr);
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

  // printf("outputFileName=%s\n", outputFileName);

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

  // printf("Total arguments: %d\n",count );
  // for(int k=0; k< count; k++)
  // {
  //   printf("[%s]\n", argument[k]);
  // }

  return count;
}

void signalHandler (int signum)
{
  pid_t pid;
  int *wstatus;

  switch (signum)
  {
    case SIGCHLD:
      // printf("SIGCHLD Recieved\n");
      pid =  waitpid(-1, wstatus, WNOHANG);
    break;

    case SIGINT:
      for(long long i=0; i < childCount; i++)
      {
        // printf("Sending SIGINT to %d\n", childgroupIDs[i]);
        killpg(childgroupIDs[i], SIGINT);
      }
    break;
  }

  return;
}
