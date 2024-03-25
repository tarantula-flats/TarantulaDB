#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>

#define FILE_NAME "database.dat"
#define PORT 2345
#define BUFFER_SIZE 256


#define DEFAULT_FILE_NAME "tarantuladb-config.json"
#define ENV_VAR_NAME "TARANTULADB_DATA_FILE"

typedef struct {
    char file_path[256];
} ServerConfig;

void load_config_from_file(const char *filename, ServerConfig *config) {
    
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open configuration file");
        return;
    }

    // todo replace with JSON parsing logic
    strncpy(config->file_path, "database.dat", sizeof(config->file_path) - 1);

    
    fclose(file);
}

void configure_server(ServerConfig *config) {
    // Initialize with default values
    strncpy(config->file_path, DEFAULT_FILE_NAME, sizeof(config->file_path) - 1);

    // Load configuration from file
    load_config_from_file("server_config.json", config);

    // Check for environment variable override
    char *env_file_path = getenv(ENV_VAR_NAME);
    if (env_file_path != NULL) {
        strncpy(config->file_path, env_file_path, sizeof(config->file_path) - 1);
    }
}

typedef struct {
    int id;
    char name[20];
} Record;

void error(const char *msg) {
    perror(msg);
    exit(1);
}

void insertRecord(int id, const char *name) {
    Record record;
    FILE *file = fopen(FILE_NAME, "ab+");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }
    record.id = id;
    strncpy(record.name, name, sizeof(record.name));
    fwrite(&record, sizeof(Record), 1, file);
    fclose(file);
}

void fetchAllRecords(int client_sockfd) {
    Record record;
    FILE *file = fopen(FILE_NAME, "rb");
    char buffer[BUFFER_SIZE];
    if (file == NULL) {
        perror("Error opening file");
        return;
    }
    while (fread(&record, sizeof(Record), 1, file)) {
        snprintf(buffer, BUFFER_SIZE, "ID: %d, Name: %s\n", record.id, record.name);
        write(client_sockfd, buffer, strlen(buffer));
    }
    fclose(file);
}

void fetchById(int client_sockfd, int id) {
    Record record;
    FILE *file = fopen(FILE_NAME, "rb");
    char buffer[BUFFER_SIZE];
    if (file == NULL) {
        perror("Error opening file");
        return;
    }
    int found = 0;
    while (fread(&record, sizeof(Record), 1, file)) {
        if (record.id == id) {
            snprintf(buffer, BUFFER_SIZE, "ID: %d, Name: %s\n", record.id, record.name);
            write(client_sockfd, buffer, strlen(buffer));
            found = 1;
            break;
        }
    }
    if (!found) {
        snprintf(buffer, BUFFER_SIZE, "No record found with ID: %d\n", id);
        write(client_sockfd, buffer, strlen(buffer));
    }
    fclose(file);
}

void deleteById(int client_sockfd, int id) {
    Record record;
    FILE *file = fopen(FILE_NAME, "rb");
    FILE *tempFile = fopen("temp.dat", "wb");
    char buffer[BUFFER_SIZE];
    if (file == NULL || tempFile == NULL) {
        perror("Error opening file");
        return;
    }
    int found = 0;
    while (fread(&record, sizeof(Record), 1, file)) {
        if (record.id != id) {
            fwrite(&record, sizeof(Record), 1, tempFile);
        } else {
            found = 1;
        }
    }
    fclose(file);
    fclose(tempFile);

    // Rename and delete files as needed
    if (found) {
        remove(FILE_NAME); // Remove the original file
        rename("temp.dat", FILE_NAME); // Rename the temp file to original file name
        snprintf(buffer, BUFFER_SIZE, "Record with ID: %d deleted successfully.\n", id);
    } else {
        remove("temp.dat"); // No need to replace original, delete temp file
        snprintf(buffer, BUFFER_SIZE, "No record found with ID: %d to delete.\n", id);
    }
    write(client_sockfd, buffer, strlen(buffer));
}


//for fork of process
void handleClient(int sock) {
    char buffer[BUFFER_SIZE];
    int n;

    bzero(buffer, BUFFER_SIZE);
    n = read(sock, buffer, BUFFER_SIZE - 1);
    if (n < 0) error("ERROR reading from socket");

    printf("Received command: %s\n", buffer);

    if (strncmp(buffer, "INSERT", 6) == 0) {
        int id;
        char name[20];
        sscanf(buffer, "INSERT %d %s", &id, name);
        insertRecord(id, name);
        write(sock, "Record inserted\n", 17);
    } else if (strncmp(buffer, "FETCH_ALL", 9) == 0) {
        fetchAllRecords(sock);
    } else {
        write(sock, "Unknown command\n", 16);
    }

    close(sock);
}


void handleClient2(int sock) {
    char buffer[BUFFER_SIZE];
    int n, id;
    char command[20];  // Assuming commands are short words

    bzero(buffer, BUFFER_SIZE);
    n = read(sock, buffer, BUFFER_SIZE - 1);
    if (n < 0) error("ERROR reading from socket");

    printf("Received command: %s\n", buffer);

    // Parse the command from the buffer
    sscanf(buffer, "%s %d", command, &id);

    if (strncmp(command, "INSERT", 6) == 0) {
        char name[20];
        // Re-parse to get the name for the insert command as it wasn't fetched before
        sscanf(buffer, "INSERT %d %s", &id, name);
        insertRecord(id, name);
        write(sock, "Record inserted\n", 17);
    } else if (strncmp(command, "FETCH_ALL", 9) == 0) {
        fetchAllRecords(sock);
    } else if (strncmp(command, "FETCH_BY_ID", 11) == 0) {
        fetchById(sock, id);
    } else if (strncmp(command, "DELETE_BY_ID", 12) == 0) {
        deleteById(sock, id);
    } else {
        snprintf(buffer, BUFFER_SIZE, "Unknown command\n");
        write(sock, buffer, strlen(buffer));
    }

    close(sock);
}


int main() {
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    pid_t pid;

    // Handle SIGCHLD to avoid zombie processes
    signal(SIGCHLD, SIG_IGN);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    bzero((char *)&serv_addr, sizeof(serv_addr));
    portno = PORT;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

    while (1) {
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0)
            error("ERROR on accept");

        pid = fork();
        if (pid < 0) {
            error("ERROR on fork");
        }

        if (pid == 0) {  // This is the child process
            close(sockfd);
            handleClient2(newsockfd);
            exit(0);
        } else {  // This is the parent process
            close(newsockfd);
        }
    }

    close(sockfd);
    return 0;
}
