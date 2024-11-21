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

int main(int argc, char *argv[]) {
  int status;
  pid_t pid_1, pid_2, pid_3, pid_4;

  int p1[2], p2[2], p3[2];

  // Checking if input parameters DIR, STR are correct
  if (argc != 4) {
    printf("usage: finder DIR STR NUM_FILES\n");
    exit(0);
  } else {
    printf("I'm meant to be here\n");  // don't delete this print statement
  }

  // Initialize pipes p1, p2, and p3
  pipe(p1);
  pipe(p2);
  pipe(p3);

  pid_1 = fork();
  if (pid_1 == 0) {
    /* First Child: find */
    // Redirect standard output to p1's write end
    dup2(p1[1], STDOUT_FILENO);
    close(p1[0]);  // Close unused pipe ends
    close(p1[1]);
    close(p2[0]); close(p2[1]);
    close(p3[0]); close(p3[1]);

    // Execute find command
    execl(FIND_EXEC, FIND_EXEC, argv[1], "-name", "*.h", (char *)0);
    exit(0);
  }

  pid_2 = fork();
  if (pid_2 == 0) {
    /* Second Child: xargs grep -c */
    // Redirect standard input from p1's read end and output to p2's write end
    dup2(p1[0], STDIN_FILENO);
    dup2(p2[1], STDOUT_FILENO);
    close(p1[0]); close(p1[1]);
    close(p2[0]); close(p2[1]);
    close(p3[0]); close(p3[1]);

    // Execute xargs grep -c
    execl(XARGS_EXEC, XARGS_EXEC, GREP_EXEC, "-c", argv[2], (char *)0);
    exit(0);
  }

  pid_3 = fork();
  if (pid_3 == 0) {
    /* Third Child: sort */
    // Redirect standard input from p2's read end and output to p3's write end
    dup2(p2[0], STDIN_FILENO);
    dup2(p3[1], STDOUT_FILENO);
    close(p1[0]); close(p1[1]);
    close(p2[0]); close(p2[1]);
    close(p3[0]); close(p3[1]);

    // Execute sort
    execl(SORT_EXEC, SORT_EXEC, "-t", ":", "-k2,2n", "--numeric", "--reverse", (char *)0);
    exit(0);
  }

  pid_4 = fork();
  if (pid_4 == 0) {
    /* Fourth Child: head */
    // Redirect standard input from p3's read end
    dup2(p3[0], STDIN_FILENO);
    close(p1[0]); close(p1[1]);
    close(p2[0]); close(p2[1]);
    close(p3[0]); close(p3[1]);

    // Execute head
    execl(HEAD_EXEC, HEAD_EXEC, "--lines", argv[3], (char *)0);
    exit(0);
  }

  // Parent closes all pipes
  close(p1[0]); close(p1[1]);
  close(p2[0]); close(p2[1]);
  close(p3[0]); close(p3[1]);

  // Wait for all children to finish
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
