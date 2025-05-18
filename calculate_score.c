#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

#define TREASURE_FILE "treasures.dat"
#define USERNAME_MAX 32

typedef struct {
    int id;
    char username[32];
    double latitude;
    double longitude;
    char clue[128];
    int value;
} Treasure;


typedef struct UserScore {
    char username[USERNAME_MAX];
    int score;
    struct UserScore *next;
} UserScore;

void add_score(UserScore **head, const char *username, int value) {
    UserScore *curr = *head;
    while (curr) {
        if (strcmp(curr->username, username) == 0) {
            curr->score += value;
            return;
        }
        curr = curr->next;
    }

    UserScore *new_node = malloc(sizeof(UserScore));
    if (!new_node) {
        perror("malloc");
        exit(1);
    }
    strncpy(new_node->username, username, USERNAME_MAX);
    new_node->score = value;
    new_node->next = *head;
    *head = new_node;
}

void free_scores(UserScore *head) {
    while (head) {
        UserScore *tmp = head;
        head = head->next;
        free(tmp);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <hunt_id>\n", argv[0]);
        return 1;
    }

    char path[512];
    snprintf(path, sizeof(path), "%s/%s", argv[1], TREASURE_FILE);

    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "Failed to open treasures file '%s': %s\n", path, strerror(errno));
        return 1;
    }

    UserScore *scores = NULL;
    Treasure t;

    while (fread(&t, sizeof(Treasure), 1, f) == 1) {
        add_score(&scores, t.username, t.value);
    }
    fclose(f);

    if (!scores) {
        printf("No treasures found in hunt '%s'.\n", argv[1]);
        return 0;
    }

    printf("Scores for hunt '%s':\n", argv[1]);
    UserScore *curr = scores;
    while (curr) {
        printf("User: %s, Score: %d\n", curr->username, curr->score);
        curr = curr->next;
    }

    free_scores(scores);
    return 0;
}
