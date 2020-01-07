#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **getInput();
extern int yylex_destroy(); // to free up bytes from lex.l
char **args;                // moved it to here from the main function
int execvpWorking = 0;      // execvp working flag
int waitWorking = -1;       // wait working flag
int redirectWorking = 0;    // redirect working flag
int sortWorking = 0;        // sort working flag
char home[256];             // home directory for cd
int arrowIndex = 0;         // '<' index
int sortArrowIndex = 0;     // '<' arrow index for sorting
char fileName[256];
char fileName2[256];          // filename2 is for input to output
bool appendToFile = false;    // flag to check if we are appending
int inputToOutputWorking = 0; // input to output working "< >"
int numPipes = 0;
bool pipeIt = false; // flag to check if '|' exists
int pipeWorking =
    0; // flag to check if pipe worked so i dont go in regular command code

/*since execvp doesnt take care of cd
returns 1 if function worked */
int changeDirectoryCommand() {
  int dir = 0;
  int x = 0;
  if (args[1] == NULL && strncmp(args[0], "cd", 3) == 0) {
    chdir(home);
    x = 1;
  } else if (strncmp(args[0], "cd", 3) == 0) {
    dir = chdir(args[1]);
    if (dir < 0) {
      perror("not a file or directory\n"); // error checking
    }
    x = 1;
  } else {
    // do nothing
  }
  return x;
}

/* redirect file function*/
int redirectFile() {
  int x = 0;
  int f, p;
  // error check if arrow index exists
  if (arrowIndex != 0) {
    x = 1;
    if ((p = fork())) {
      waitpid(p, (void *)0, 0);
    } else {
      if (appendToFile == true) {
        f = open(fileName, O_CREAT | O_APPEND | O_WRONLY, 0666);
      } else {
        f = open(fileName, O_CREAT | O_TRUNC | O_WRONLY, 0666);
      }

      // Redirect
      close(1); // close stdout
      dup(f);
      close(f);

      execvp(args[0], args);
    }
  }
  return x;
}

/*sorting function*/
int sortCommand() {
  int x = 0;
  int f, p;
  // error check to check for input character
  if (sortArrowIndex != 0) {
    x = 1;
    if ((p = fork())) {
      waitpid(p, (void *)0, 0);
    } else {
      f = open(fileName, O_RDONLY, 0666);
      close(0); // close stdin
      dup(f);
      close(f);
      execvp(args[0], args);
    }
  }
  return x;
}

/*input to output function that takes care of < >*/
int inputToOutput() {
  int x = 0;
  int f, p, f2;
  // error check to check for input character
  if (sortArrowIndex != 0) {
    x = 1;
    if ((p = fork())) {
      waitpid(p, (void *)0, 0);
    } else {
      if (appendToFile == true) {
        f = open(fileName, O_RDONLY, 0666);
        close(0);
        dup(f);
        close(f);
        f2 = open(fileName2, O_CREAT | O_APPEND | O_WRONLY, 0666);

        close(1);
        dup(f2);
        close(f2);
        execvp(args[0], args);
      } else {
        f = open(fileName, O_RDONLY, 0666);
        close(0);
        dup(f);
        close(f);
        f2 = open(fileName2, O_CREAT | O_TRUNC | O_WRONLY, 0666);

        close(1);
        dup(f2);
        close(f2);
        execvp(args[0], args);
      }
    }
  }
  return x;
}

/*function that takes care of piping*/
int pipeFunction() {
  int pipe_fds[numPipes]
              [2]; // 2-d array with row as the pipe and column as input/output
  int pid = 0;
  int nextCommandIndex = 0;
  int f, k = 0; // f is file descriptor and k is args iterator
  int counterForCommandsFinished = 0; // count for the command you are on
  int endCommand = 0;                 // last command flag check
  char fName[256]; // file name you are writing out to if you are writing to a
                   // file
  bool waitProblem = false; // checking if you don't need to wait in parent when
                            // writing to a file

  for (int j = 0; j < (numPipes); j++) {
    pipe(pipe_fds[j]); // initialize pipe
  }

  while (counterForCommandsFinished <= numPipes) {
    if ((strncmp(args[k], "|", 3)) == 0 || endCommand == 1) {
      if (endCommand == 0) {
        args[k] = NULL;
      }

      if (endCommand == 1) {
        for (int i = nextCommandIndex; args[i] != NULL; i++) {
          if (strncmp(args[i], ">", 3) ==
              0) // checking if you are writing out to a file
          {
            waitProblem = true;
            break;
          }
        }
      }

      pid = fork();
      // switch case provided by TA to implement piping
      switch (pid) {
      case 0:
        if (counterForCommandsFinished !=
            0) // if you are not on your first command
        {

          close(0);
          dup(pipe_fds[counterForCommandsFinished - 1][0]);

          if (endCommand == 1) {
            for (int i = nextCommandIndex; args[i] != NULL; i++) {
              if ((strncmp(args[i], ">", 3)) == 0) {
                args[i] = NULL;
                if (args[i + 1] != NULL) {
                  strcpy(fName, args[i + 1]); // copy filename
                  f = open(fName, O_CREAT | O_TRUNC | O_WRONLY, 0666);
                  close(1); // close stdout and write out to a file
                  dup(f);
                  close(f);
                  // fprintf(stderr, "%s\n", args[nextCommandIndex]);
                  // fflush(stderr);
                  execvp(args[nextCommandIndex], &args[nextCommandIndex]);
                  fflush(stderr);
                  break;
                }
              }
            }
          }
        }

        if (endCommand == 0) // if you are not on your last command
        {
          close(1);
          dup(pipe_fds[counterForCommandsFinished][1]);
          fflush(stderr);
        }

        for (int i = 0; i < numPipes; i++) {
          close(pipe_fds[i][1]); // closing all pipes in child process
          close(pipe_fds[i][0]);
        }

        execvp(args[nextCommandIndex], &args[nextCommandIndex]);
        break;
      case -1:
        perror("pipe didn't fork properly");
        break;
      case 1:
        break;
      }

      if (args[k + 1] != NULL) // checking if there is another command and if so
                               // get next command index (command after the bar)
      {
        nextCommandIndex = k + 1;
      }
      k++;                             // increment args counter
      counterForCommandsFinished += 1; // increment to next command in args
      //}
      /*else
      {
              k++;
      }*/
    } else {
      k++;
    }

    // if you are on your last command set the flag
    if ((counterForCommandsFinished) == (numPipes)) {
      endCommand = 1;
    }
  }

  for (int i = 0; i < numPipes; i++) {
    close(pipe_fds[i][1]); // close all pipes in parent process
    close(pipe_fds[i][0]);
  }
  // fprintf(stderr,"%d\n", pid);
  // fflush(stderr);
  if (waitProblem != true) // do not wait twice if you are writing out to a file
  {
    waitpid(pid, (void *)0, 0);
  }
  // waitpid(pid, (void *)0, 0);
  // fprintf(stderr,"I am really bored\n");
  // fflush(stderr);
  // waitpid(pid, (void *)0, 0);
  return 1;
}

int main() {
  int i;
  // char **args;
  // pid_t parentID;
  getcwd(home, sizeof(home));
  while (1) {
    printf(">>>"); // lets you know that you are in the shell
    args = getInput();

    for (int j = 0; args[j] != NULL; j++) {
      if ((strncmp(args[j], "|", 3)) == 0) {
        numPipes += 1; // counts the bars to let you know how many pipes exist
        pipeIt = true; // sets flag to true if there is a bar
      }
    }
    if (pipeIt == true) {
      pipeWorking = pipeFunction(); // calls pipe function if pipe flag is true
      // continue;
    }

    /*main for loop that checks for all other special characters for input and
     * output*/
    for (i = 0; args[i] != NULL; i++) {
      // printf("Argument %d: %s\n", i, args[i]);
      if (pipeIt == true) {
        break;
      }
      if ((strncmp(args[i], ">", 3)) == 0 && (args[i + 1] != NULL) &&
          (strncmp(args[i + 1], ">", 3)) == 0) // checks if you are appending
      {
        arrowIndex = i;
        args[i] = NULL;
        args[i + 1] = NULL;  // sets both > to null
        appendToFile = true; // sets append flag to true if >> found
        if (args[i + 2] == NULL) {
          perror("specify a file you are appending to\n");
          redirectWorking = 1; // techinally should be 0 but i dont want it to
                               // go to the if statement below
        } else {
          strcpy(fileName, args[i + 2]);
          redirectWorking = redirectFile();
        }

      } else if ((strncmp(args[i], ">", 3)) ==
                 0) // checks if you are just writing to a file
      {

        arrowIndex = i;
        // fileName = args[i+1];
        args[i] = NULL;
        if (args[i + 1] != NULL) {
          if (strncmp(args[i + 1], ">", 3) ==
              0) // checks if you are appending a file
          {
            strcpy(fileName, ">");
            redirectWorking = redirectFile();
          } else {
            strcpy(fileName, args[i + 1]);
            redirectWorking = redirectFile();
          }
        } else {
          perror("specify a file you are writing to\n");
          redirectWorking = 1; // techinally should be 0 but i dont want it to
                               // go to the if statement below
        }
      } else if ((strncmp(args[i], "<", 3)) ==
                 0) // checks if you are inputting a file
      {
        sortArrowIndex = i;
        args[i] = NULL; // sets < character to null so you can call execvp
        if (args[i + 1] != NULL) {
          if (args[i + 2] != NULL) {
            if ((strncmp(args[i + 2], ">", 5)) ==
                0) // checks if you are taking an input to an ouput
            {
              if (args[i + 3] != NULL) {
                if ((strncmp(args[i + 3], ">", 3)) == 0 &&
                    args[i + 4] != NULL) // checks if you are taking an input
                                         // and appending it to an output
                {
                  appendToFile = true;
                  args[i + 2] = NULL;
                  args[i + 3] = NULL;
                  strcpy(fileName, args[i + 1]);
                  strcpy(fileName2, args[i + 4]);
                  inputToOutputWorking = inputToOutput();
                } else if ((strncmp(args[i + 3], ">", 3)) == 0 &&
                           args[i + 4] == NULL) {
                  perror("give file to append to after input");
                } else {
                  args[i + 2] = NULL;
                  // args[i+3] = NULL;
                  strcpy(fileName, args[i + 1]);
                  strcpy(fileName2, args[i + 3]);
                  inputToOutputWorking = inputToOutput();
                }
              } else {
                perror("enter file for redirect for inputToOutput");
              }
            }
          } else {
            strcpy(fileName, args[i + 1]);
            sortWorking = sortCommand(); // regular sorting command if none of
                                         // the above cases ae true
          }
        } else {
          perror("specify a file to input for input\n");
          sortWorking = 1;
        }
      } else {
        continue;
      }
    }

    if (args[0] != NULL && redirectWorking == 0 && sortWorking == 0 &&
        inputToOutputWorking == 0 && pipeWorking == 0 &&
        pipeIt == false) // regular command with no inputs or outputs
    {
      if ((strncmp(args[0], "exit", 5)) == 0) {
        yylex_destroy(); // frees up memory when you exit
        exit(0);
      } else if (changeDirectoryCommand() == 1) // cd not included in execvp
      {
        continue;
      } else {
        int childID = fork();
        if (childID == 0) {
          execvpWorking = execvp(args[0], args);

          if (execvpWorking == -1) {
            perror("invalid command\n");
            exit(0);
          }
        } else if (childID < 0) {
          perror("fork did not work");
        } else {
          waitpid(childID, (void *)0, 0); // wait in parent
        }
      }
    }
    redirectWorking = 0; // reset all flags to 0 or false after one iteration
    sortWorking = 0;
    appendToFile = false;
    pipeIt = false;
    inputToOutputWorking = 0;
    pipeWorking = 0;
    numPipes = 0;
  }
}
