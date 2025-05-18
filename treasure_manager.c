#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <errno.h>

#define USERNAME_MAX 32
#define CLUE_MAX 128
#define RECORD_FILE "treasures.dat"
#define LOG_FILE "logged_hunt"

typedef struct {
    int treasure_id;
    char username[USERNAME_MAX];
    float latitude;
    float longitude;
    char clue[CLUE_MAX];
    int value;
} Treasure;

// Utility: log operation
void log_operation(const char *hunt_id, const char *msg) {
    char path[256];
    snprintf(path, sizeof(path), "%s/%s", hunt_id, LOG_FILE);
    int fd = open(path, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd >= 0) {
        dprintf(fd, "%s\n", msg);
        close(fd);
    }
}

// Utility: create symlink
void create_symlink(const char *hunt_id) {
    char target[256], linkname[256];
    snprintf(target, sizeof(target), "%s/%s", hunt_id, LOG_FILE);
    snprintf(linkname, sizeof(linkname), "logged_hunt-%s", hunt_id);
    symlink(target, linkname);
}

void add_treasure(const char *hunt_id) {
    mkdir(hunt_id, 0755);

    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/%s", hunt_id, RECORD_FILE);

    Treasure t;
    printf("Enter treasure ID: ");
    scanf("%d", &t.treasure_id);
    printf("Enter username: ");
    scanf("%s", t.username);
    printf("Enter latitude: ");
    scanf("%f", &t.latitude);
    printf("Enter longitude: ");
    scanf("%f", &t.longitude);
    printf("Enter clue: ");
    getchar(); // consume newline
    fgets(t.clue, CLUE_MAX, stdin);
    t.clue[strcspn(t.clue, "\n")] = '\0'; // remove newline
    printf("Enter value: ");
    scanf("%d", &t.value);

    int fd = open(filepath, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd < 0) {
        perror("Error opening treasure file");
        return;
    }

    write(fd, &t, sizeof(Treasure));
    close(fd);

    log_operation(hunt_id, "Added a treasure.");
    create_symlink(hunt_id);
}

void list_treasures(const char *hunt_id) {
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/%s", hunt_id, RECORD_FILE);

    struct stat st;
    if (stat(filepath, &st) == -1) {
        perror("Could not stat treasure file");
        return;
    }

    printf("Hunt: %s\nSize: %ld bytes\nLast modified: %s",
           hunt_id, st.st_size, ctime(&st.st_mtime));

    int fd = open(filepath, O_RDONLY);
    if (fd < 0) {
        perror("Error opening treasure file");
        return;
    }

    Treasure t;
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        printf("ID: %d, User: %s, (%.2f, %.2f), Value: %d, Clue: %s\n",
               t.treasure_id, t.username, t.latitude, t.longitude, t.value, t.clue);
    }

    close(fd);
}

void view_treasure(const char *hunt_id, int id) {
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/%s", hunt_id, RECORD_FILE);

    int fd = open(filepath, O_RDONLY);
    if (fd < 0) {
        perror("Error opening treasure file");
        return;
    }

    Treasure t;
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        if (t.treasure_id == id) {
            printf("Treasure ID: %d\nUser: %s\nCoordinates: (%.2f, %.2f)\nValue: %d\nClue: %s\n",
                   t.treasure_id, t.username, t.latitude, t.longitude, t.value, t.clue);
            close(fd);
            return;
        }
    }

    printf("Treasure with ID %d not found.\n", id);
    close(fd);
}

void remove_treasure(const char *hunt_id, int id) {
    char filepath[256], temp_filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/%s", hunt_id, RECORD_FILE);
    snprintf(temp_filepath, sizeof(temp_filepath), "%s/temp.dat", hunt_id);

    int fd_old = open(filepath, O_RDONLY);
    int fd_new = open(temp_filepath, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (fd_old < 0 || fd_new < 0) {
        perror("Error opening file");
        return;
    }

    Treasure t;
    int found = 0;
    while (read(fd_old, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        if (t.treasure_id == id) {
            found = 1;
            continue;
        }
        write(fd_new, &t, sizeof(Treasure));
    }

    close(fd_old);
    close(fd_new);

    if (found) {
        rename(temp_filepath, filepath);
        log_operation(hunt_id, "Removed a treasure.");
        printf("Treasure removed.\n");
    } else {
        remove(temp_filepath);
        printf("Treasure with ID %d not found.\n", id);
    }
}

void remove_hunt(const char *hunt_id) {
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/%s", hunt_id, RECORD_FILE);
    unlink(filepath);
    snprintf(filepath, sizeof(filepath), "%s/%s", hunt_id, LOG_FILE);
    unlink(filepath);
    rmdir(hunt_id);

    char symlink_name[256];
    snprintf(symlink_name, sizeof(symlink_name), "logged_hunt-%s", hunt_id);
    unlink(symlink_name);

    printf("Hunt %s removed.\n", hunt_id);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s --command hunt_id [id]\n", argv[0]);
        return 1;
    }

    const char *cmd = argv[1];
    const char *hunt_id = argv[2];

    if (strcmp(cmd, "--add") == 0) {
        add_treasure(hunt_id);
    } else if (strcmp(cmd, "--list") == 0) {
        list_treasures(hunt_id);
    } else if (strcmp(cmd, "--view") == 0 && argc == 4) {
        int id = atoi(argv[3]);
        view_treasure(hunt_id, id);
    } else if (strcmp(cmd, "--remove_treasure") == 0 && argc == 4) {
        int id = atoi(argv[3]);
        remove_treasure(hunt_id, id);
    } else if (strcmp(cmd, "--remove_hunt") == 0) {
        remove_hunt(hunt_id);
    } else {
        fprintf(stderr, "Invalid command or parameters.\n");
    }

    return 0;
}
