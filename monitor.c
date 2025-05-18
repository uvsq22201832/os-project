#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <time.h>

#define CMD_FILE ".monitor_command"
#define TREASURE_FILE "treasures.dat"

#ifndef DT_DIR
#define DT_DIR 4
#endif

typedef struct {
    int id;
    char username[32];
    double latitude;
    double longitude;
    char clue[128];
    int value;
} Treasure;

volatile sig_atomic_t command_ready = 0;

void sigusr1_handler(int sig) {
    command_ready = 1;
}

void delay_exit() {
    dprintf(STDOUT_FILENO, "Monitor exiting in 3 seconds...\n");
    sleep(3);
}

void print_treasure(const Treasure *t) {
    dprintf(STDOUT_FILENO, "Treasure ID: %d\n", t->id);
    dprintf(STDOUT_FILENO, "User: %s\n", t->username);
    dprintf(STDOUT_FILENO, "Coordinates: %.6f, %.6f\n", t->latitude, t->longitude);
    dprintf(STDOUT_FILENO, "Clue: %s\n", t->clue);
    dprintf(STDOUT_FILENO, "Value: %d\n", t->value);
    dprintf(STDOUT_FILENO, "---------------------\n");
}

void list_hunts() {
    DIR *d = opendir(".");
    if (!d) {
        dprintf(STDOUT_FILENO, "Failed to open current directory: %s\n", strerror(errno));
        return;
    }
    struct dirent *entry;
    int hunt_count = 0;

    dprintf(STDOUT_FILENO, "Listing hunts:\n");
    while ((entry = readdir(d)) != NULL) {
        if (entry->d_type == DT_DIR) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            char treasure_path[512];
            snprintf(treasure_path, sizeof(treasure_path), "%s/%s", entry->d_name, TREASURE_FILE);

            struct stat st;
            if (stat(treasure_path, &st) == 0) {
                off_t size = st.st_size;
                int count = size / sizeof(Treasure);

                dprintf(STDOUT_FILENO, "Hunt: %s - Treasures: %d\n", entry->d_name, count);
                hunt_count++;
            }
        }
    }
    if (hunt_count == 0) {
        dprintf(STDOUT_FILENO, "No hunts found.\n");
    }
    closedir(d);
}

void list_treasures(const char *hunt_id) {
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", hunt_id, TREASURE_FILE);

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        dprintf(STDOUT_FILENO, "Failed to open treasures for hunt '%s': %s\n", hunt_id, strerror(errno));
        return;
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        dprintf(STDOUT_FILENO, "fstat failed: %s\n", strerror(errno));
        close(fd);
        return;
    }

    dprintf(STDOUT_FILENO, "Hunt: %s\n", hunt_id);
    dprintf(STDOUT_FILENO, "Total treasure file size: %ld bytes\n", st.st_size);
    dprintf(STDOUT_FILENO, "Last modification time: %s", ctime(&st.st_mtime));
    dprintf(STDOUT_FILENO, "Treasures:\n");

    Treasure t;
    ssize_t r;
    while ((r = read(fd, &t, sizeof(t))) == sizeof(t)) {
        print_treasure(&t);
    }

    close(fd);
}

void view_treasure(const char *hunt_id, int treasure_id) {
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", hunt_id, TREASURE_FILE);

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        dprintf(STDOUT_FILENO, "Failed to open treasures for hunt '%s': %s\n", hunt_id, strerror(errno));
        return;
    }

    Treasure t;
    ssize_t r;
    int found = 0;
    while ((r = read(fd, &t, sizeof(t))) == sizeof(t)) {
        if (t.id == treasure_id) {
            dprintf(STDOUT_FILENO, "Treasure details:\n");
            print_treasure(&t);
            found = 1;
            break;
        }
    }

    if (!found) {
        dprintf(STDOUT_FILENO, "Treasure with ID %d not found in hunt '%s'.\n", treasure_id, hunt_id);
    }

    close(fd);
}

void process_command(const char *cmd) {
    if (strcmp(cmd, "stop_monitor") == 0) {
        delay_exit();
        exit(0);
    } else if (strcmp(cmd, "list_hunts") == 0) {
        list_hunts();
    } else if (strncmp(cmd, "list_treasures ", 15) == 0) {
        const char *hunt_id = cmd + 15;
        list_treasures(hunt_id);
    } else if (strncmp(cmd, "view_treasure ", 14) == 0) {
        const char *params = cmd + 14;
        char hunt_id[128];
        int treasure_id;

        if (sscanf(params, "%127s %d", hunt_id, &treasure_id) == 2) {
            view_treasure(hunt_id, treasure_id);
        } else {
            dprintf(STDOUT_FILENO, "Invalid view_treasure command format. Use: view_treasure <hunt_id> <treasure_id>\n");
        }
    } else {
        dprintf(STDOUT_FILENO, "Unknown command: %s\n", cmd);
    }
}

int main() {
    struct sigaction sa;
    sa.sa_handler = sigusr1_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGUSR1, &sa, NULL);

    dprintf(STDOUT_FILENO, "Monitor started with PID %d\n", getpid());

    while (1) {
        pause();

        if (command_ready) {
            command_ready = 0;

            int fd = open(CMD_FILE, O_RDONLY);
            if (fd < 0) {
                dprintf(STDOUT_FILENO, "Monitor: Cannot open command file: %s\n", strerror(errno));
                continue;
            }

            char buf[256] = {0};
            ssize_t n = read(fd, buf, sizeof(buf) - 1);
            close(fd);

            if (n > 0) {
                buf[n] = '\0';
                process_command(buf);
            } else {
                dprintf(STDOUT_FILENO, "Monitor: No command read\n");
            }
        }
    }

    return 0;
}
