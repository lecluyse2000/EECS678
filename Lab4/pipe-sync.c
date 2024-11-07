#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

int main() {
    int pipe_fd1[2];
    int pipe_fd2[2];
    char buf[1];

    if (pipe(pipe_fd1) == -1 || pipe(pipe_fd2) == -1) {
        perror("pipe");
        return 1;
    }

    pid_t ret = fork();
    if (ret == 0) {
        printf("Child line 1\n");
        write(pipe_fd1[1], "1", 1);
        read(pipe_fd2[0], buf, 1);
        printf("Child line 2\n");
        write(pipe_fd1[1], "2", 1);
    } else {
        read(pipe_fd1[0], buf, 1);
        printf("Parent line 1\n");
        write(pipe_fd2[1], "1", 1);
        read(pipe_fd1[0], buf, 1);
        printf("Parent line 2\n");
        wait(NULL);
    }

    return 0;
}

