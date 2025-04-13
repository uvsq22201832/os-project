#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <errno.h>

#define TREASURE_FILE "treasures.dat"
#define LOG_FILE "logged_hunt"
#define MAX_USERNAME 32
#define MAX_CLUE 128
#define MAX_ID 16

// Structure fixe pour un trésor
typedef struct {
    char id[MAX_ID];
    char username[MAX_USERNAME];
    float latitude;
    float longitude;
    char clue[MAX_CLUE];
    int value;
} Treasure;

// Créer un dossier si nécessaire
void create_directory(const char *path) {
    mkdir(path, 0777);
}

// Chemin complet vers le fichier trésors
void get_treasure_file_path(char *buffer, const char *hunt_id) {
    sprintf(buffer, "%s/%s", hunt_id, TREASURE_FILE);
}

// Chemin complet vers le fichier de log
void get_log_file_path(char *buffer, const char *hunt_id) {
    sprintf(buffer, "%s/%s", hunt_id, LOG_FILE);
}

// Créer lien symbolique pour le log
void create_symlink_log(const char *hunt_id) {
    char target[256];
    char linkname[256];
    get_log_file_path(target, hunt_id);
    sprintf(linkname, "logged_hunt-%s", hunt_id);
    symlink(target, linkname); // ignore erreur si existe
}

// Log action utilisateur
void log_action(const char *hunt_id, const char *action) {
    char path[256];
    get_log_file_path(path, hunt_id);
    int fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd >= 0) {
        dprintf(fd, "%s\n", action);
        close(fd);
    }
}

// Ajouter un trésor
void add_treasure(const char *hunt_id) {
    create_directory(hunt_id);
    char path[256];
    get_treasure_file_path(path, hunt_id);
    int fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) { perror("open"); return; }

    Treasure t;
    printf("ID: "); scanf("%15s", t.id);
    printf("Username: "); scanf("%31s", t.username);
    printf("Latitude: "); scanf("%f", &t.latitude);
    printf("Longitude: "); scanf("%f", &t.longitude);
    printf("Clue: "); getchar(); fgets(t.clue, MAX_CLUE, stdin);
    t.clue[strcspn(t.clue, "\n")] = 0; // remove newline
    printf("Value: "); scanf("%d", &t.value);

    write(fd, &t, sizeof(Treasure));
    close(fd);

    log_action(hunt_id, "ADD TREASURE" );
    create_symlink_log(hunt_id);
}

// Lister tous les trésors d'une chasse
void list_treasures(const char *hunt_id) {
    char path[256];
    get_treasure_file_path(path, hunt_id);
    int fd = open(path, O_RDONLY);
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

// Voir un trésor spécifique
void view_treasure(const char *hunt_id, const char *id) {
    char path[256];
    get_treasure_file_path(path, hunt_id);
    int fd = open(path, O_RDONLY);
    if (fd < 0) { perror("open"); return; }
    Treasure t;
    int found = 0;
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        if (strcmp(t.id, id) == 0) {
            printf("ID: %s\nUser: %s\nCoords: %.4f, %.4f\nClue: %s\nValue: %d\n",
                   t.id, t.username, t.latitude, t.longitude, t.clue, t.value);
            found = 1;
            break;
        }
    }
    if (!found) printf("Treasure not found.\n");
    close(fd);
    log_action(hunt_id, "VIEW TREASURE");
}

// Supprimer un trésor spécifique
void remove_treasure(const char *hunt_id, const char *id) {
    char path[256];
    get_treasure_file_path(path, hunt_id);
    int fd = open(path, O_RDONLY);
    if (fd < 0) { perror("open"); return; }

    char tmp_path[256];
    sprintf(tmp_path, "%s/tmp.dat", hunt_id);
    int tmp_fd = open(tmp_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
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

// Supprimer une chasse entière
void remove_hunt(const char *hunt_id) {
    char cmd[256];
    sprintf(cmd, "rm -rf %s", hunt_id);
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
