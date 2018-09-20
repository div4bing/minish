#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdbool.h>

#define COMMAND_SIZE 1048576
#define BINARY_SIZE 1024
#define ARGUMENT_SIZE 1024

int startShell(void);
int parseInput(char * binary, char * args[ARGUMENT_SIZE]);  // Returns Numnber of Arguments

int main(int argc, char * argv[])
{
  int ret = 0;
  ret = startShell();

  if(ret != 0)
  {
    printf("Error occured in mini Shell\n");
  }
  return 0;
}

int startShell(void)
{
  pid_t pid;
  int * waitStatus;
  char * minishBinary;
  char * minishArg[ARGUMENT_SIZE];
  int argCount = 0;

  minishBinary = malloc(BINARY_SIZE*sizeof(char));
  for (int i=0; i<ARGUMENT_SIZE; i++)
  {
    minishArg[i] = (char *)malloc(ARGUMENT_SIZE * sizeof(int));
  }

  printf("minish> ");
  argCount = parseInput(minishBinary, minishArg);

  printf("Binary: %s\n", minishBinary);
  for (int i=0; i< argCount; i++)
  {
    printf("Argument%d: %s\n",i, minishArg[i]);
  }
  minishArg[argCount] = NULL;

  //###########################################################
  if ((pid = fork()) == -1)
  {
    perror("Failed to fork child process");
    return -1;
  }

  if (pid == 0)   // child
  {
    if (execvp(minishBinary, minishArg) == -1)
    {
      perror("Execv Failed");
      return -1;
    }
  }

  wait(waitStatus);

  startShell();

  free(minishBinary);
  return 0;
}

int parseInput(char * binary, char * args[ARGUMENT_SIZE])
{
  unsigned char stdPtr = '\0';
  int count = 0;
  int a1,a2;
  int argCount = 0;

// Get Binary
  stdPtr = (char)getchar();
  count = 0;

  while (stdPtr != ' ' && stdPtr != '\n')              // Scan till new line
  {
    binary[count++] = stdPtr;
    stdPtr = (char)getchar();
    if (stdPtr == '\n')
    {
      break;                        // End of input
    }
  }

  binary[count] = '\0';
  strcpy(args[0], binary);          // Copy first argument as binary itself

  if (stdPtr == '\n')
  {
    return 1;                        // End of input
  }

// Get arguments
  stdPtr = (char)getchar();
  a1 = 1;           // Our Frist arg is binary name itself
  a2 = 0;

  while (stdPtr != '\n')
  {
    while (stdPtr != ' ' && stdPtr != '\n')
    {
      args[a1][a2++] = stdPtr;
      stdPtr = (char)getchar();
    }
    args[a1++][a2] = '\0';
    if (stdPtr == '\n')
    {
      break;                        // End of input
    }
    a2 = 0;
    stdPtr = (char)getchar();
  }
  argCount = a1;

  return argCount;
}
