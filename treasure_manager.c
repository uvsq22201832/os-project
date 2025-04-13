#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <errno.h>

#ifdef _WIN32
#include <io.h>
#define mkdir(path, mode) mkdir(path)
#define SYMLINK_SUPPORTED 0
#else
#define SYMLINK_SUPPORTED 1
#endif

#define TREASURE_FILE "treasures.dat"
#define LOG_FILE "logged_hunt"
#define MAX_USERNAME 32
#define MAX_CLUE 128
#define MAX_ID 16

// Fixed-size structure for a treasure
typedef struct {
    char id[MAX_ID];
    char username[MAX_USERNAME];
    float latitude;
    float longitude;
    char clue[MAX_CLUE];
    int value;
} Treasure;

// Create a directory if needed
void create_directory(const char *path) {
    mkdir(path, 0777);
}

// Full path to the treasure data file
void get_treasure_file_path(char *buffer, const char *hunt_id) {
    sprintf(buffer, "%s/%s", hunt_id, TREASURE_FILE);
}

// Full path to the log file
void get_log_file_path(char *buffer, const char *hunt_id) {
    sprintf(buffer, "%s/%s", hunt_id, LOG_FILE);
}

// Create a symbolic link to the log (not supported on Windows MinGW)
void create_symlink_log(const char *hunt_id) {
#if SYMLINK_SUPPORTED
    char target[256];
    char linkname[256];
    get_log_file_path(target, hunt_id);
    sprintf(linkname, "logged_hunt-%s", hunt_id);
    symlink(target, linkname);
#else
    // Inform that symlink is not supported on Windows
    printf("[INFO] Symlink not supported on this platform.\n");
#endif
}

// Log a user action
void log_action(const char *hunt_id, const char *action) {
    char path[256];
    get_log_file_path(path, hunt_id);
    FILE *f = fopen(path, "a");
    if (f) {
        fprintf(f, "%s\n", action);
        fclose(f);
    }
}

// Add a treasure
void add_treasure(const char *hunt_id) {
    create_directory(hunt_id);
    char path[256];
    get_treasure_file_path(path, hunt_id);
    int fd = open(path, O_WRONLY | O_CREAT | O_APPEND | O_BINARY, 0644);
    if (fd < 0) { perror("open"); return; }

    Treasure t;
    printf("ID: "); scanf("%15s", t.id);
    printf("Username: "); scanf("%31s", t.username);
    printf("Latitude: "); scanf("%f", &t.latitude);
    printf("Longitude: "); scanf("%f", &t.longitude);
    printf("Clue (a hint to find the treasure): "); getchar(); fgets(t.clue, MAX_CLUE, stdin);
    t.clue[strcspn(t.clue, "\n")] = 0; // remove newline
    printf("Value (in points): "); scanf("%d", &t.value);

    write(fd, &t, sizeof(Treasure));
    close(fd);

    log_action(hunt_id, "ADD TREASURE" );
    create_symlink_log(hunt_id);
}

// List all treasures of a hunt
void list_treasures(const char *hunt_id) {
    char path[256];
    get_treasure_file_path(path, hunt_id);
    int fd = open(path, O_RDONLY | O_BINARY);
    if (fd < 0) { perror("open"); return; }

    struct stat st;
    if (stat(path, &st) == -1) { perror("stat"); return; }
    printf("Hunt: %s\nFile size: %ld bytes\nLast modified: %s\n",
           hunt_id, st.st_size, ctime(&st.st_mtime));

    Treasure t;
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        printf("[%s] %s (%.4f, %.4f), %d pts\n",
               t.id, t.username, t.latitude, t.longitude, t.value);
    }
    close(fd);
    log_action(hunt_id, "LIST TREASURES");
}

// View a specific treasure
void view_treasure(const char *hunt_id, const char *id) {
    char path[256];
    get_treasure_file_path(path, hunt_id);
    int fd = open(path, O_RDONLY | O_BINARY);
    if (fd < 0) { perror("open"); return; }
    Treasure t;
    int found = 0;
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        if (strcmp(t.id, id) == 0) {
            printf("ID: %s\nUser: %s\nCoordinates: %.4f, %.4f\nClue: %s\nValue: %d\n",
                   t.id, t.username, t.latitude, t.longitude, t.clue, t.value);
            found = 1;
            break;
        }
    }
    if (!found) printf("Treasure not found.\n");
    close(fd);
    log_action(hunt_id, "VIEW TREASURE");
}

// Remove a specific treasure
void remove_treasure(const char *hunt_id, const char *id) {
    char path[256];
    get_treasure_file_path(path, hunt_id);
    int fd = open(path, O_RDONLY | O_BINARY);
    if (fd < 0) { perror("open"); return; }

    char tmp_path[256];
    sprintf(tmp_path, "%s/tmp.dat", hunt_id);
    int tmp_fd = open(tmp_path, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0644);
    Treasure t;
    int removed = 0;

    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        if (strcmp(t.id, id) != 0) {
            write(tmp_fd, &t, sizeof(Treasure));
        } else {
            removed = 1;
        }
    }
    close(fd);
    close(tmp_fd);
    rename(tmp_path, path);
    if (removed) printf("Treasure removed.\n");
    else printf("Treasure not found.\n");
    log_action(hunt_id, "REMOVE TREASURE");
}

// Remove an entire hunt
void remove_hunt(const char *hunt_id) {
    char cmd[256];
#ifdef _WIN32
    sprintf(cmd, "rmdir /s /q %s", hunt_id);
#else
    sprintf(cmd, "rm -rf %s", hunt_id);
#endif
    system(cmd);
    char linkname[256];
    sprintf(linkname, "logged_hunt-%s", hunt_id);
    unlink(linkname);
    printf("Hunt '%s' removed.\n", hunt_id);
}

// Main dispatcher
int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s --add|--list|--view|--remove_treasure|--remove <hunt_id> [<treasure_id>]\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "--add") == 0) {
        add_treasure(argv[2]);
    } else if (strcmp(argv[1], "--list") == 0) {
        list_treasures(argv[2]);
    } else if (strcmp(argv[1], "--view") == 0 && argc >= 4) {
        view_treasure(argv[2], argv[3]);
    } else if (strcmp(argv[1], "--remove_treasure") == 0 && argc >= 4) {
        remove_treasure(argv[2], argv[3]);
    } else if (strcmp(argv[1], "--remove") == 0) {
        remove_hunt(argv[2]);
    } else {
        printf("Invalid command.\n");
        return 1;
    }
    return 0;
}
