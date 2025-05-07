#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>

pid_t monitor_pid = 0;
int monitor_exiting = 0;

void handle_sigchld(int sig) {
    int status;
    pid_t pid = waitpid(monitor_pid, &status, WNOHANG);
    if (pid == monitor_pid) {
        printf("[Monitor terminated with status %d]\n", status);
        monitor_pid = 0;
    }
}

void send_command_to_monitor(const char *cmd) {
    if (monitor_pid == 0) {
        printf("[Error] No monitor running.\n");
        return;
    }
    FILE *f = fopen("command.txt", "w");
    if (!f) {
        perror("fopen");
        return;
    }
    fprintf(f, "%s\n", cmd);
    fclose(f);
    kill(monitor_pid, SIGUSR1);
    printf("[Command sent to monitor] %s\n", cmd);
}

int main() {
    char input[256];
    struct sigaction sa;
    sa.sa_handler = handle_sigchld;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sa, NULL);

    printf("Welcome to Treasure Hub\n");
    while (1) {
        printf("> ");
        fflush(stdout);
        if (!fgets(input, sizeof(input), stdin)) break;

        input[strcspn(input, "\n")] = 0; // Remove newline

        if (strcmp(input, "start_monitor") == 0) {
            if (monitor_pid != 0) {
                printf("[Monitor is already running]\n");
                continue;
            }
            monitor_pid = fork();
            if (monitor_pid < 0) {
                perror("fork");
            } else if (monitor_pid == 0) {
                execl("./monitor", "monitor", NULL);
                perror("execl");
                exit(1);
            } else {
                printf("[Monitor started - PID %d]\n", monitor_pid);
            }
        } else if (strcmp(input, "stop_monitor") == 0) {
            if (monitor_pid == 0) {
                printf("[No monitor is running]\n");
                continue;
            }
            monitor_exiting = 1;
            kill(monitor_pid, SIGTERM);
        } else if (strcmp(input, "exit") == 0) {
            if (monitor_pid != 0) {
                printf("[Cannot exit: monitor is still running]\n");
                continue;
            }
            break;
        } else if (strncmp(input, "list_treasures", 15) == 0) {
            // Example: list_treasures Hunt001
            char hunt_id[128];
            if (sscanf(input, "list_treasures %127s", hunt_id) == 1) {
                char cmd[256];
                snprintf(cmd, sizeof(cmd), "./treasure_manager --list %s", hunt_id);
                send_command_to_monitor(cmd);
            } else {
                printf("Usage: list_treasures <hunt_id>\n");
            }
        } else {
            if (monitor_exiting && monitor_pid != 0) {
                printf("[Monitor is shutting down, please wait...]\n");
                continue;
            }
            printf("[Unknown or unavailable command: '%s']\n", input);
        }
    }

    printf("Goodbye.\n");
    return 0;
}
