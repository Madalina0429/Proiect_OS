#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>

#define MAX_STRING 512
#define MAX_CLUE 1024
#define MAX_TREASURES 100
#define MAX_LOG_DETAILS 1024 // Increased buffer size for log details

// Structure to hold treasure information
typedef struct
{
    int id;
    char username[MAX_STRING];
    double latitude;
    double longitude;
    char clue[MAX_CLUE];
    int value;
} Treasure;

// Structure to hold hunt information
typedef struct
{
    char hunt_id[MAX_STRING];
    Treasure treasures[MAX_TREASURES];
    int treasure_count;
} Hunt;

// Function declarations
void add_treasure(const char *hunt_id);
void list_treasures(const char *hunt_id);
void view_treasure(const char *hunt_id, int treasure_id);
void create_hunt_directory(const char *hunt_id);
char *get_treasure_file_path(const char *hunt_id);
void save_treasures(const char *hunt_id, Hunt *hunt);
Hunt *load_treasures(const char *hunt_id);
void log_operation(const char *hunt_id, const char *operation, const char *details);
void merge_hunt_logs();

// Function to merge hunt logs into a single file
void merge_hunt_logs()
{
    FILE *output_file = fopen("hunt_log.txt", "a"); // Open in append mode
    if (output_file == NULL)
    {
        perror("Error opening hunt_log.txt");
        return;
    }

    DIR *hunt_dir = opendir("hunt");
    if (hunt_dir == NULL)
    {
        perror("Error opening hunt directory");
        fclose(output_file);
        return;
    }

    struct dirent *entry;
    struct stat st;
    char log_path[MAX_STRING];
    char buffer[1024];

    while ((entry = readdir(hunt_dir)) != NULL)
    {
        if (snprintf(log_path, sizeof(log_path), "hunt/%s", entry->d_name) >= sizeof(log_path))
        {
            fprintf(stderr, "Path truncated: %s\n", entry->d_name);
            continue;
        }

        if (stat(log_path, &st) == 0 && S_ISDIR(st.st_mode)) // Ensure it's a directory
        {
            if (snprintf(log_path, sizeof(log_path), "hunt/%s/logged_hunt.txt", entry->d_name) >= sizeof(log_path))
            {
                fprintf(stderr, "Path truncated: %s\n", entry->d_name);
                continue;
            }

            FILE *log_file = fopen(log_path, "r");
            if (log_file != NULL)
            {
                printf("Appending log from: %s\n", log_path); // Debugging line
                fprintf(output_file, "=== Log for Hunt: %s ===\n", entry->d_name);
                while (fgets(buffer, sizeof(buffer), log_file) != NULL)
                {
                    fputs(buffer, output_file);
                }
                fprintf(output_file, "\n");
                fclose(log_file);
            }
        }
    }

    closedir(hunt_dir);
    fclose(output_file);
    printf("Hunt logs merged successfully into hunt_log.txt\n");
}

// Function to log operations
void log_operation(const char *hunt_id, const char *operation, const char *details)
{
    char log_path[MAX_STRING];
    if (snprintf(log_path, sizeof(log_path), "hunt/hunt%s/logged_hunt.txt", hunt_id) >= sizeof(log_path))
    {
        fprintf(stderr, "Log path truncated for hunt_id: %s\n", hunt_id);
        return;
    }

    FILE *log_file = fopen(log_path, "a");
    if (log_file == NULL)
    {
        perror("Error opening log file");
        return;
    }

    time_t now;
    time(&now);
    char timestamp[26];
    if (ctime_r(&now, timestamp) == NULL)
    {
        perror("Error generating timestamp");
        fclose(log_file);
        return;
    }
    timestamp[24] = '\0'; // Remove newline

    fprintf(log_file, "[%s] %s: %s\n", timestamp, operation, details);
    fclose(log_file);
    merge_hunt_logs();
}

// Function to create hunt subdirectory if it doesn't exist
void create_hunt_directory(const char *hunt_id)
{
    char dir_path[MAX_STRING];
    if (snprintf(dir_path, sizeof(dir_path), "hunt/hunt%s", hunt_id) >= sizeof(dir_path))
    {
        fprintf(stderr, "Directory path truncated for hunt_id: %s\n", hunt_id);
        exit(EXIT_FAILURE);
    }

    if (mkdir(dir_path, 0755) != 0 && errno != EEXIST)
    {
        perror("Error creating hunt directory");
        exit(EXIT_FAILURE);
    }
}

// Function to get the full path to the treasure file
char *get_treasure_file_path(const char *hunt_id)
{
    static char path[MAX_STRING];
    if (snprintf(path, sizeof(path), "hunt/hunt%s/treasures.dat", hunt_id) >= sizeof(path))
    {
        fprintf(stderr, "Treasure file path truncated for hunt_id: %s\n", hunt_id);
        exit(EXIT_FAILURE);
    }
    return path;
}

// Function to save treasures to file
void save_treasures(const char *hunt_id, Hunt *hunt)
{
    char *file_path = get_treasure_file_path(hunt_id);
    FILE *file = fopen(file_path, "wb");

    if (file == NULL)
    {
        perror("Error opening treasure file for writing");
        exit(EXIT_FAILURE);
    }

    fwrite(&hunt->treasure_count, sizeof(int), 1, file);
    fwrite(hunt->treasures, sizeof(Treasure), hunt->treasure_count, file);

    fclose(file);
}

// Function to load treasures from file
Hunt *load_treasures(const char *hunt_id)
{
    static Hunt hunt;
    strncpy(hunt.hunt_id, hunt_id, MAX_STRING - 1);
    hunt.treasure_count = 0;

    char *file_path = get_treasure_file_path(hunt_id);
    FILE *file = fopen(file_path, "rb");

    if (file == NULL)
    {
        return &hunt; // Return empty hunt if file doesn't exist
    }

    fread(&hunt.treasure_count, sizeof(int), 1, file);
    fread(hunt.treasures, sizeof(Treasure), hunt.treasure_count, file);

    fclose(file);
    return &hunt;
}

// Function to add a new treasure
void add_treasure(const char *hunt_id)
{
    create_hunt_directory(hunt_id);
    Hunt *hunt = load_treasures(hunt_id);

    if (hunt->treasure_count >= MAX_TREASURES)
    {
        printf("Error: Maximum number of treasures reached\n");
        log_operation(hunt_id, "ADD", "Failed: Maximum number of treasures reached");
        return;
    }

    Treasure *new_treasure = &hunt->treasures[hunt->treasure_count];
    new_treasure->id = hunt->treasure_count + 1;

    printf("Enter username: ");
    scanf("%s", new_treasure->username);

    printf("Enter latitude: ");
    scanf("%lf", &new_treasure->latitude);

    printf("Enter longitude: ");
    scanf("%lf", &new_treasure->longitude);

    printf("Enter clue: ");
    getchar(); // Clear buffer
    fgets(new_treasure->clue, MAX_CLUE, stdin);
    new_treasure->clue[strcspn(new_treasure->clue, "\n")] = 0; // Remove newline

    printf("Enter value: ");
    scanf("%d", &new_treasure->value);

    hunt->treasure_count++;
    save_treasures(hunt_id, hunt);

    char log_details[MAX_LOG_DETAILS];
    int written = snprintf(log_details, sizeof(log_details),
                           "Added treasure ID: %d, Username: %s, Value: %d",
                           new_treasure->id, new_treasure->username, new_treasure->value);

    if (written >= sizeof(log_details))
    {
        // Truncate the username if needed
        char truncated_username[MAX_STRING];
        strncpy(truncated_username, new_treasure->username, sizeof(truncated_username) - 1);
        truncated_username[sizeof(truncated_username) - 1] = '\0';

        snprintf(log_details, sizeof(log_details),
                 "Added treasure ID: %d, Username: %s, Value: %d",
                 new_treasure->id, truncated_username, new_treasure->value);
    }

    log_operation(hunt_id, "ADD", log_details);
    printf("Treasure added successfully with ID: %d\n", new_treasure->id);
}

// Function to list all treasures from a hunt
void list_treasures(const char *hunt_id)
{
    Hunt *hunt = load_treasures(hunt_id);

    if (hunt->treasure_count == 0)
    {
        printf("No treasures found in hunt: %s\n", hunt_id);
        log_operation(hunt_id, "LIST", "No treasures found");
        return;
    }

    char *file_path = get_treasure_file_path(hunt_id);
    struct stat st;
    if (stat(file_path, &st) == 0)
    {
        printf("Hunt: %s\n", hunt_id);
        printf("File size: %ld bytes\n", st.st_size);
        printf("Last modified: %s", ctime(&st.st_mtime));
        printf("\nTreasures:\n");
    }

    for (int i = 0; i < hunt->treasure_count; i++)
    {
        Treasure *t = &hunt->treasures[i];
        printf("\nID: %d\n", t->id);
        printf("Username: %s\n", t->username);
        printf("Location: %.6f, %.6f\n", t->latitude, t->longitude);
        printf("Clue: %s\n", t->clue);
        printf("Value: %d\n", t->value);
    }

    char log_details[MAX_LOG_DETAILS];
    snprintf(log_details, sizeof(log_details), "Listed %d treasures", hunt->treasure_count);
    log_operation(hunt_id, "LIST", log_details);
}

// Function to view a specific treasure
void view_treasure(const char *hunt_id, int treasure_id)
{
    Hunt *hunt = load_treasures(hunt_id);

    for (int i = 0; i < hunt->treasure_count; i++)
    {
        if (hunt->treasures[i].id == treasure_id)
        {
            Treasure *t = &hunt->treasures[i];
            printf("\nTreasure Details:\n");
            printf("ID: %d\n", t->id);
            printf("Username: %s\n", t->username);
            printf("Location: %.6f, %.6f\n", t->latitude, t->longitude);
            printf("Clue: %s\n", t->clue);
            printf("Value: %d\n", t->value);

            char log_details[MAX_LOG_DETAILS];
            int written = snprintf(log_details, sizeof(log_details),
                                   "Viewed treasure ID: %d, Username: %s",
                                   t->id, t->username);

            if (written >= sizeof(log_details))
            {
                // Truncate the username if needed
                char truncated_username[MAX_STRING];
                strncpy(truncated_username, t->username, sizeof(truncated_username) - 1);
                truncated_username[sizeof(truncated_username) - 1] = '\0';

                snprintf(log_details, sizeof(log_details),
                         "Viewed treasure ID: %d, Username: %s",
                         t->id, truncated_username);
            }

            log_operation(hunt_id, "VIEW", log_details);
            return;
        }
    }

    printf("Treasure with ID %d not found in hunt %s\n", treasure_id, hunt_id);
    char log_details[MAX_LOG_DETAILS];
    snprintf(log_details, sizeof(log_details), "Failed to view treasure ID: %d (not found)", treasure_id);
    log_operation(hunt_id, "VIEW", log_details);
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("Usage: %s <command> <hunt_id> [treasure_id]\n", argv[0]);
        printf("Commands:\n");
        printf("  add <hunt_id> - Add a new treasure\n");
        printf("  list <hunt_id> - List all treasures\n");
        printf("  view <hunt_id> <treasure_id> - View specific treasure\n");
        return 1;
    }

    char *command = argv[1];
    char *hunt_id = argv[2];
    int treasure_id = (argc > 3) ? atoi(argv[3]) : 0;

    if (strcmp(command, "add") == 0)
    {
        add_treasure(hunt_id);
    }
    else if (strcmp(command, "list") == 0)
    {
        list_treasures(hunt_id);
    }
    else if (strcmp(command, "view") == 0)
    {
        view_treasure(hunt_id, treasure_id);
    }
    else
    {
        printf("Unknown command: %s\n", command);
        return 1;
    }

    return 0;
}