#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <sys/select.h>
#include <bits/sigaction.h>
#include <asm-generic/signal-defs.h>

#define CMD_FILE ".monitor_command"
#define READ_END 0
#define WRITE_END 1

pid_t monitor_pid = -1;
int monitor_running = 0;
int pipefd[2]; // Pipe between monitor and treasure_hub

void handle_sigchld(int sig) {
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        if (pid == monitor_pid) {
            monitor_running = 0;
            printf("\nMonitor terminated with status: %d\n", WEXITSTATUS(status));
        }
    }
}

void send_command(const char *cmd) {
    if (!monitor_running) {
        printf("Error: Monitor is not running.\n");
        return;
    }

    int fd = open(CMD_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("Cannot write command file");
        return;
    }
    write(fd, cmd, strlen(cmd));
    close(fd);

    kill(monitor_pid, SIGUSR1);
}

#include <fcntl.h>
#include <sys/select.h>

void read_monitor_output() {
    char buffer[512];
    ssize_t n;
    fd_set readfds;
    struct timeval tv;

    // Set pipe read end to non-blocking mode
    int flags = fcntl(pipefd[READ_END], F_GETFL, 0);
    fcntl(pipefd[READ_END], F_SETFL, flags | O_NONBLOCK);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(pipefd[READ_END], &readfds);
        tv.tv_sec = 0;
        tv.tv_usec = 200000; // 200 ms timeout

        int ret = select(pipefd[READ_END] + 1, &readfds, NULL, NULL, &tv);

        if (ret > 0 && FD_ISSET(pipefd[READ_END], &readfds)) {
            n = read(pipefd[READ_END], buffer, sizeof(buffer) - 1);
            if (n > 0) {
                buffer[n] = '\0';
                printf("%s", buffer);
                fflush(stdout);
            } else if (n == 0) {
                // EOF on pipe - monitor probably exited
                monitor_running = 0;
                break;
            } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
                perror("read error");
                break;
            }
        } else {
            // No more data available or timeout
            break;
        }
    }

    // Restore blocking mode if desired
    fcntl(pipefd[READ_END], F_SETFL, flags);
}

void calculate_score(const char *hunt_id) {
    if (!monitor_running) {
        printf("Error: Monitor is not running.\n");
        return;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        return;
    }

    if (pid == 0) {
        // Child process: just exec calculate_score program and print output to stdout directly
        execl("./calculate_score", "calculate_score", hunt_id, NULL);
        perror("Failed to exec calculate_score");
        exit(1);
    } else {
        // Parent waits for child
        waitpid(pid, NULL, 0);
    }
}

int main() {
    struct sigaction sa;
    sa.sa_handler = handle_sigchld;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;

    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    char input[256];

    while (1) {
        printf("hub> ");
        fflush(stdout);

        if (!fgets(input, sizeof(input), stdin)) break;

        input[strcspn(input, "\n")] = 0;

        if (strcmp(input, "start_monitor") == 0) {
            if (monitor_running) {
                printf("Monitor already running.\n");
                continue;
            }

            monitor_pid = fork();
            if (monitor_pid == 0) {
                // Child process: duplicate pipe write end to stdout for monitor
                close(pipefd[READ_END]);
                dup2(pipefd[WRITE_END], STDOUT_FILENO);
                close(pipefd[WRITE_END]);

                execl("./monitor", "monitor", NULL);
                perror("Failed to start monitor");
                exit(1);
            } else if (monitor_pid > 0) {
                close(pipefd[WRITE_END]);
                monitor_running = 1;
                printf("Monitor started with PID %d\n", monitor_pid);
            } else {
                perror("fork");
                exit(1);
            }
        } else if (strcmp(input, "stop_monitor") == 0) {
            send_command("stop_monitor");
        } else if (strcmp(input, "list_hunts") == 0) {
            send_command("list_hunts");
            read_monitor_output();
        } else if (strncmp(input, "list_treasures ", 15) == 0) {
            send_command(input);
            read_monitor_output();
        } else if (strncmp(input, "view_treasure ", 14) == 0) {
            send_command(input);
            read_monitor_output();
        } else if (strncmp(input, "calculate_score ", 16) == 0) {
            char *hunt_id = input + 16;
            calculate_score(hunt_id);
        } else if (strcmp(input, "exit") == 0) {
            if (monitor_running) {
                printf("Cannot exit: monitor is still running.\n");
            } else {
                break;
            }
        } else {
            printf("Unknown or invalid command.\n");
        }
    }

    return 0;
}
