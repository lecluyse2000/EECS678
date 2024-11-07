#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <strings.h>
#include <errno.h>
#include <sys/wait.h>

#define BSIZE 256

#define BASH_EXEC  "/bin/bash"
#define FIND_EXEC  "/bin/find"
#define XARGS_EXEC "/usr/bin/xargs"
#define GREP_EXEC  "/bin/grep"
#define SORT_EXEC  "/bin/sort"
#define HEAD_EXEC  "/usr/bin/head"

int main(int argc, char *argv[])
{
  int status;
  pid_t pid_1, pid_2, pid_3, pid_4;
  
  int p1[2], p2[2], p3[2];

  // Checking if input parameters DIR, STR, and NUM_FILES are correct
  if (argc != 4) {
    printf("usage: finder DIR STR NUM_FILES\n");
    exit(0);
  } else {
    printf("I'm meant to be here\n"); // don't delete this print statement
  }

  // STEP 1: Initialize pipes p1, p2, and p3
  if (pipe(p1) == -1 || pipe(p2) == -1 || pipe(p3) == -1) {
    perror("Error opening the pipes!\n");
    exit(EXIT_FAILURE);
  }

  pid_1 = fork();
  if (pid_1 == 0) {
    dup2(p1[1], STDOUT_FILENO);
    close(p1[0]);
    close(p1[1]);
    close(p2[0]);
    close(p2[1]);
    close(p3[0]);
    close(p3[1]);

    // STEP 3: Execute find command
    execl(FIND_EXEC, FIND_EXEC, argv[1], "-name", "*.h", (char *)0);
    perror("execl error for find");
    exit(EXIT_FAILURE);
  }

  // Second Child: xargs grep -c $2
  pid_2 = fork();
  if (pid_2 == 0) {
    // STEP 4: Redirect stdin from p1 and stdout to p2, close unused pipe ends
    dup2(p1[0], STDIN_FILENO);
    dup2(p2[1], STDOUT_FILENO);
    close(p1[0]);
    close(p1[1]);
    close(p2[0]);
    close(p2[1]);
    close(p3[0]);
    close(p3[1]);

    // STEP 5: Execute xargs grep command
    execl(XARGS_EXEC, XARGS_EXEC, GREP_EXEC, "-c", argv[2], (char *)0);
    perror("execl error for xargs grep");
    exit(EXIT_FAILURE);
  }

  // Third Child: sort -t : +1.0 -2.0 --numeric --reverse
  pid_3 = fork();
  if (pid_3 == 0) {
    // STEP 6: Redirect stdin from p2 and stdout to p3, close unused pipe ends
    dup2(p2[0], STDIN_FILENO);
    dup2(p3[1], STDOUT_FILENO);
    close(p1[0]);
    close(p1[1]);
    close(p2[0]);
    close(p2[1]);
    close(p3[0]);
    close(p3[1]);

    // STEP 7: Execute sort command
    execl(SORT_EXEC, SORT_EXEC, "-t", ":", "-k", "2,2n", "--reverse", (char *)0);
    perror("execl error for sort");
    exit(EXIT_FAILURE);
  }

  // Fourth Child: head --lines=$3
  pid_4 = fork();
  if (pid_4 == 0) {
    // STEP 8: Redirect stdin from p3, close unused pipe ends
    dup2(p3[0], STDIN_FILENO);
    close(p1[0]);
    close(p1[1]);
    close(p2[0]);
    close(p2[1]);
    close(p3[0]);
    close(p3[1]);

    // STEP 9: Execute head command
    execl(HEAD_EXEC, HEAD_EXEC, "--lines", argv[3], (char *)0);
    perror("execl error for head");
    exit(EXIT_FAILURE);
  }

  // Close all pipes in parent
  close(p1[0]);
  close(p1[1]);
  close(p2[0]);
  close(p2[1]);
  close(p3[0]);
  close(p3[1]);

  // Wait for all child processes to finish
  if ((waitpid(pid_1, &status, 0)) == -1) {
    fprintf(stderr, "Process 1 encountered an error. ERROR%d", errno);
    return EXIT_FAILURE;
  }
  if ((waitpid(pid_2, &status, 0)) == -1) {
    fprintf(stderr, "Process 2 encountered an error. ERROR%d", errno);
    return EXIT_FAILURE;
  }
  if ((waitpid(pid_3, &status, 0)) == -1) {
    fprintf(stderr, "Process 3 encountered an error. ERROR%d", errno);
    return EXIT_FAILURE;
  }
  if ((waitpid(pid_4, &status, 0)) == -1) {
    fprintf(stderr, "Process 4 encountered an error. ERROR%d", errno);
    return EXIT_FAILURE;
  }

  return 0;
}

