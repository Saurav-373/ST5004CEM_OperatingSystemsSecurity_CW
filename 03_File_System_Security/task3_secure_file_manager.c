#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#define STORAGE_FOLDER "secure_storage"
#define METADATA_FILE "file_metadata.txt"
#define AUDIT_FILE "audit.log"

#define MAX_FILES 30
#define MAX_NAME 64
#define MAX_TEXT 512

typedef struct {
    char username[20];
    char password[20];
    char group[20];
    int isAdmin;
} User;

typedef struct {
    char filename[MAX_NAME];
    char owner[20];
    char group[20];
    char permissions[10];
} FileInfo;

/*
 * Demonstration accounts.
 * Saurav and Ankit belong to the same group so that
 * group permissions can be tested.
 */
User users[] = {
    {"admin", "admin123", "admins", 1},
    {"saurav", "student123", "students", 0},
    {"ankit", "member123", "students", 0},
    {"guest", "guest123", "guests", 0}
};

FileInfo files[MAX_FILES];
int fileCount = 0;
User *currentUser = NULL;

/*
 * Reads a complete line of text and removes the newline.
 */
void getText(const char *message, char *buffer, int size)
{
    int character;

    printf("%s", message);
    fflush(stdout);

    if (fgets(buffer, size, stdin) == NULL) {
        buffer[0] = '\0';
        return;
    }

    if (strchr(buffer, '\n') != NULL) {
        buffer[strcspn(buffer, "\n")] = '\0';
    } else {
        while ((character = getchar()) != '\n' && character != EOF) {
            /* Clear extra input. */
        }
    }
}

/*
 * Reads a menu number safely.
 */
int getNumber(const char *message)
{
    char input[32];
    char *end;
    long value;

    getText(message, input, sizeof(input));
    value = strtol(input, &end, 10);

    while (isspace((unsigned char) *end)) {
        end++;
    }

    if (input[0] == '\0' || *end != '\0') {
        return -1;
    }

    return (int) value;
}

/*
 * Creates the folder used by the program.
 */
int setupStorage(void)
{
    if (mkdir(STORAGE_FOLDER, 0700) == -1 && errno != EEXIST) {
        perror("Could not create secure storage");
        return 0;
    }

    chmod(STORAGE_FOLDER, 0700);
    return 1;
}

/*
 * Records important successful and failed actions.
 */
void writeAudit(
    const char *action,
    const char *filename,
    const char *result
)
{
    FILE *logFile;
    time_t now;
    struct tm *timeInfo;
    char timestamp[32];

    const char *username =
        currentUser == NULL ? "unknown" : currentUser->username;

    const char *safeFilename =
        filename == NULL || filename[0] == '\0' ? "-" : filename;

    now = time(NULL);
    timeInfo = localtime(&now);

    if (timeInfo == NULL) {
        strcpy(timestamp, "unknown-time");
    } else {
        strftime(
            timestamp,
            sizeof(timestamp),
            "%Y-%m-%d %H:%M:%S",
            timeInfo
        );
    }

    logFile = fopen(AUDIT_FILE, "a");

    if (logFile == NULL) {
        return;
    }

    fprintf(
        logFile,
        "[%s] user=%s action=%s file=%s result=%s\n",
        timestamp,
        username,
        action,
        safeFilename,
        result
    );

    fclose(logFile);

    /*
     * Only the Linux account running the program can access
     * the audit log directly.
     */
    chmod(AUDIT_FILE, 0600);
}

/*
 * Loads saved file ownership and permission information.
 */
void loadMetadata(void)
{
    FILE *metadata;
    char line[200];

    metadata = fopen(METADATA_FILE, "r");

    if (metadata == NULL) {
        return;
    }

    fileCount = 0;

    while (
        fileCount < MAX_FILES &&
        fgets(line, sizeof(line), metadata) != NULL
    ) {
        if (sscanf(
                line,
                "%63[^|]|%19[^|]|%19[^|]|%9s",
                files[fileCount].filename,
                files[fileCount].owner,
                files[fileCount].group,
                files[fileCount].permissions
            ) == 4) {
            fileCount++;
        }
    }

    fclose(metadata);
}

/*
 * Saves file ownership and permission information.
 */
void saveMetadata(void)
{
    FILE *metadata;
    int index;

    metadata = fopen(METADATA_FILE, "w");

    if (metadata == NULL) {
        perror("Could not save metadata");
        return;
    }

    for (index = 0; index < fileCount; index++) {
        fprintf(
            metadata,
            "%s|%s|%s|%s\n",
            files[index].filename,
            files[index].owner,
            files[index].group,
            files[index].permissions
        );
    }

    fclose(metadata);
    chmod(METADATA_FILE, 0600);
}

/*
 * Allows only simple filenames.
 * Paths such as ../secret.txt are rejected.
 */
int validFilename(const char *filename)
{
    int index;

    if (
        filename[0] == '\0' ||
        !isalnum((unsigned char) filename[0])
    ) {
        return 0;
    }

    if (strstr(filename, "..") != NULL) {
        return 0;
    }

    for (index = 0; filename[index] != '\0'; index++) {
        char character = filename[index];

        if (
            !isalnum((unsigned char) character) &&
            character != '.' &&
            character != '_' &&
            character != '-'
        ) {
            return 0;
        }
    }

    return 1;
}

/*
 * Checks a nine-character permission value.
 * Example: rwxr-----
 */
int validPermissions(const char *permissions)
{
    int index;
    char expected[] = "rwx";

    if (strlen(permissions) != 9) {
        return 0;
    }

    for (index = 0; index < 9; index++) {
        if (
            permissions[index] != '-' &&
            permissions[index] != expected[index % 3]
        ) {
            return 0;
        }
    }

    return 1;
}

/*
 * Returns the position of a file in the metadata array.
 */
int findFile(const char *filename)
{
    int index;

    for (index = 0; index < fileCount; index++) {
        if (strcmp(files[index].filename, filename) == 0) {
            return index;
        }
    }

    return -1;
}

/*
 * Builds a path inside secure_storage.
 */
void makePath(const char *filename, char *path)
{
    snprintf(
        path,
        150,
        "%s/%s",
        STORAGE_FOLDER,
        filename
    );
}

/*
 * Checks owner, group or others permissions.
 */
int hasPermission(FileInfo *file, char permission)
{
    int start;
    int position;

    /*
     * The administrator can access every managed file.
     */
    if (currentUser->isAdmin) {
        return 1;
    }

    if (strcmp(file->owner, currentUser->username) == 0) {
        start = 0;
    } else if (strcmp(file->group, currentUser->group) == 0) {
        start = 3;
    } else {
        start = 6;
    }

    if (permission == 'r') {
        position = 0;
    } else if (permission == 'w') {
        position = 1;
    } else {
        position = 2;
    }

    return file->permissions[start + position] == permission;
}

/*
 * Allows three login attempts.
 */
int login(void)
{
    char username[20];
    char password[20];
    int attempt;
    int index;
    int userCount = sizeof(users) / sizeof(users[0]);

    printf("========================================\n");
    printf("SECURE FILE MANAGEMENT SYSTEM\n");
    printf("========================================\n");

    for (attempt = 1; attempt <= 3; attempt++) {
        printf("\nLogin attempt %d of 3\n", attempt);

        getText(
            "Username: ",
            username,
            sizeof(username)
        );

        getText(
            "Password: ",
            password,
            sizeof(password)
        );

        for (index = 0; index < userCount; index++) {
            if (
                strcmp(users[index].username, username) == 0 &&
                strcmp(users[index].password, password) == 0
            ) {
                currentUser = &users[index];

                printf(
                    "Login successful. Welcome, %s.\n",
                    username
                );

                writeAudit(
                    "LOGIN",
                    NULL,
                    "SUCCESS"
                );

                return 1;
            }
        }

        printf("Invalid username or password.\n");
    }

    printf("Too many failed login attempts.\n");
    return 0;
}

/*
 * Creates an empty managed file.
 */
void createFile(void)
{
    char filename[MAX_NAME];
    char path[150];
    FILE *file;

    getText(
        "Enter a new filename: ",
        filename,
        sizeof(filename)
    );

    if (!validFilename(filename)) {
        printf(
            "Invalid filename. Do not use paths or special characters.\n"
        );

        writeAudit(
            "CREATE",
            filename,
            "INVALID_NAME"
        );

        return;
    }

    if (findFile(filename) != -1) {
        printf("A file with that name already exists.\n");

        writeAudit(
            "CREATE",
            filename,
            "ALREADY_EXISTS"
        );

        return;
    }

    if (fileCount >= MAX_FILES) {
        printf("The file limit has been reached.\n");
        return;
    }

    makePath(filename, path);

    file = fopen(path, "w");

    if (file == NULL) {
        perror("Could not create file");

        writeAudit(
            "CREATE",
            filename,
            "FAILURE"
        );

        return;
    }

    fclose(file);
    chmod(path, 0600);

    strcpy(
        files[fileCount].filename,
        filename
    );

    strcpy(
        files[fileCount].owner,
        currentUser->username
    );

    strcpy(
        files[fileCount].group,
        currentUser->group
    );

    /*
     * Owner: read and write
     * Group: read
     * Others: no access
     */
    strcpy(
        files[fileCount].permissions,
        "rw-r-----"
    );

    fileCount++;
    saveMetadata();

    printf(
        "File created with permissions rw-r-----.\n"
    );

    writeAudit(
        "CREATE",
        filename,
        "SUCCESS"
    );
}

/*
 * Reads a file after checking read permission.
 */
void readFile(void)
{
    char filename[MAX_NAME];
    char path[150];
    char line[MAX_TEXT];
    FILE *file;
    int fileIndex;
    int empty = 1;

    getText(
        "Enter the filename to read: ",
        filename,
        sizeof(filename)
    );

    fileIndex = findFile(filename);

    if (fileIndex == -1) {
        printf("File not found.\n");

        writeAudit(
            "READ",
            filename,
            "NOT_FOUND"
        );

        return;
    }

    if (!hasPermission(&files[fileIndex], 'r')) {
        printf(
            "Access denied: Read permission is required.\n"
        );

        writeAudit(
            "READ",
            filename,
            "DENIED"
        );

        return;
    }

    makePath(filename, path);

    file = fopen(path, "r");

    if (file == NULL) {
        perror("Could not open file");

        writeAudit(
            "READ",
            filename,
            "FAILURE"
        );

        return;
    }

    printf("\n----- File contents -----\n");

    while (fgets(line, sizeof(line), file) != NULL) {
        printf("%s", line);
        empty = 0;
    }

    if (empty) {
        printf("(file is empty)\n");
    }

    printf("-------------------------\n");

    fclose(file);

    writeAudit(
        "READ",
        filename,
        "SUCCESS"
    );
}

/*
 * Adds text to an existing file.
 */
void writeFile(void)
{
    char filename[MAX_NAME];
    char path[150];
    char text[MAX_TEXT];
    FILE *file;
    int fileIndex;

    getText(
        "Enter the filename to write to: ",
        filename,
        sizeof(filename)
    );

    fileIndex = findFile(filename);

    if (fileIndex == -1) {
        printf("File not found.\n");

        writeAudit(
            "WRITE",
            filename,
            "NOT_FOUND"
        );

        return;
    }

    if (!hasPermission(&files[fileIndex], 'w')) {
        printf(
            "Access denied: Write permission is required.\n"
        );

        writeAudit(
            "WRITE",
            filename,
            "DENIED"
        );

        return;
    }

    getText(
        "Enter text to append: ",
        text,
        sizeof(text)
    );

    makePath(filename, path);

    file = fopen(path, "a");

    if (file == NULL) {
        perror("Could not open file");

        writeAudit(
            "WRITE",
            filename,
            "FAILURE"
        );

        return;
    }

    fprintf(file, "%s\n", text);
    fclose(file);

    printf("Text added successfully.\n");

    writeAudit(
        "WRITE",
        filename,
        "SUCCESS"
    );
}

/*
 * Only the owner or administrator can delete a file.
 */
void deleteFile(void)
{
    char filename[MAX_NAME];
    char path[150];
    int fileIndex;
    int index;

    getText(
        "Enter the filename to delete: ",
        filename,
        sizeof(filename)
    );

    fileIndex = findFile(filename);

    if (fileIndex == -1) {
        printf("File not found.\n");

        writeAudit(
            "DELETE",
            filename,
            "NOT_FOUND"
        );

        return;
    }

    if (
        !currentUser->isAdmin &&
        strcmp(
            files[fileIndex].owner,
            currentUser->username
        ) != 0
    ) {
        printf(
            "Access denied: Only the owner or administrator can delete this file.\n"
        );

        writeAudit(
            "DELETE",
            filename,
            "DENIED"
        );

        return;
    }

    makePath(filename, path);

    if (remove(path) != 0) {
        perror("Could not delete file");

        writeAudit(
            "DELETE",
            filename,
            "FAILURE"
        );

        return;
    }

    for (
        index = fileIndex;
        index < fileCount - 1;
        index++
    ) {
        files[index] = files[index + 1];
    }

    fileCount--;
    saveMetadata();

    printf("File deleted successfully.\n");

    writeAudit(
        "DELETE",
        filename,
        "SUCCESS"
    );
}

/*
 * Allows the owner or administrator to change permissions.
 */
void changePermissions(void)
{
    char filename[MAX_NAME];
    char permissions[20];
    int fileIndex;

    getText(
        "Enter the filename: ",
        filename,
        sizeof(filename)
    );

    fileIndex = findFile(filename);

    if (fileIndex == -1) {
        printf("File not found.\n");
        return;
    }

    if (
        !currentUser->isAdmin &&
        strcmp(
            files[fileIndex].owner,
            currentUser->username
        ) != 0
    ) {
        printf(
            "Access denied: Only the owner or administrator can change permissions.\n"
        );

        writeAudit(
            "CHMOD",
            filename,
            "DENIED"
        );

        return;
    }

    printf(
        "Current permissions: %s\n",
        files[fileIndex].permissions
    );

    getText(
        "Enter new permissions, for example rwxr-----: ",
        permissions,
        sizeof(permissions)
    );

    if (!validPermissions(permissions)) {
        printf(
            "Invalid permission format. Use nine characters such as rwxr-----\n"
        );

        writeAudit(
            "CHMOD",
            filename,
            "INVALID_PERMISSION"
        );

        return;
    }

    memcpy(
        files[fileIndex].permissions,
        permissions,
        10
    );

    saveMetadata();

    printf("Permissions changed successfully.\n");

    writeAudit(
        "CHMOD",
        filename,
        "SUCCESS"
    );
}

/*
 * Checks execute permission without running the file.
 */
void checkExecutePermission(void)
{
    char filename[MAX_NAME];
    int fileIndex;

    getText(
        "Enter the filename to check: ",
        filename,
        sizeof(filename)
    );

    fileIndex = findFile(filename);

    if (fileIndex == -1) {
        printf("File not found.\n");
        return;
    }

    if (hasPermission(&files[fileIndex], 'x')) {
        printf("Execute permission is granted.\n");

        writeAudit(
            "EXECUTE_CHECK",
            filename,
            "ALLOWED"
        );
    } else {
        printf("Execute permission is denied.\n");

        writeAudit(
            "EXECUTE_CHECK",
            filename,
            "DENIED"
        );
    }
}

/*
 * XOR uses the same operation for encryption and decryption.
 * The same key must be used for both.
 *
 * This is a teaching demonstration and is not suitable
 * for protecting real sensitive information.
 */
void xorFile(const char *action)
{
    char filename[MAX_NAME];
    char keyText[20];
    char path[150];
    char temporaryPath[170];

    FILE *input;
    FILE *output;

    int fileIndex;
    int character;
    unsigned char key;

    getText(
        "Enter the filename: ",
        filename,
        sizeof(filename)
    );

    fileIndex = findFile(filename);

    if (fileIndex == -1) {
        printf("File not found.\n");

        writeAudit(
            action,
            filename,
            "NOT_FOUND"
        );

        return;
    }

    if (!hasPermission(&files[fileIndex], 'w')) {
        printf(
            "Access denied: Write permission is required.\n"
        );

        writeAudit(
            action,
            filename,
            "DENIED"
        );

        return;
    }

    getText(
        "Enter a one-character key: ",
        keyText,
        sizeof(keyText)
    );

    if (strlen(keyText) != 1) {
        printf(
            "The key must contain exactly one character.\n"
        );

        return;
    }

    key = (unsigned char) keyText[0];

    makePath(filename, path);

    snprintf(
        temporaryPath,
        sizeof(temporaryPath),
        "%s.tmp",
        path
    );

    input = fopen(path, "rb");
    output = fopen(temporaryPath, "wb");

    if (input == NULL || output == NULL) {
        printf("Could not process the file.\n");

        if (input != NULL) {
            fclose(input);
        }

        if (output != NULL) {
            fclose(output);
        }

        remove(temporaryPath);

        writeAudit(
            action,
            filename,
            "FAILURE"
        );

        return;
    }

    while ((character = fgetc(input)) != EOF) {
        fputc(
            (unsigned char) character ^ key,
            output
        );
    }

    fclose(input);
    fclose(output);

    if (rename(temporaryPath, path) != 0) {
        printf(
            "Could not replace the original file.\n"
        );

        remove(temporaryPath);

        writeAudit(
            action,
            filename,
            "FAILURE"
        );

        return;
    }

    printf(
        "File %s successfully.\n",
        strcmp(action, "ENCRYPT") == 0
            ? "encrypted"
            : "decrypted"
    );

    writeAudit(
        action,
        filename,
        "SUCCESS"
    );
}

/*
 * Only the administrator can display the audit log.
 */
void viewAuditLog(void)
{
    FILE *logFile;
    char line[MAX_TEXT];

    if (!currentUser->isAdmin) {
        printf(
            "Access denied: Only the administrator can view the audit log.\n"
        );

        writeAudit(
            "VIEW_AUDIT",
            AUDIT_FILE,
            "DENIED"
        );

        return;
    }

    logFile = fopen(AUDIT_FILE, "r");

    if (logFile == NULL) {
        printf("The audit log is empty.\n");
        return;
    }

    printf(
        "\n=============== AUDIT LOG ===============\n"
    );

    while (fgets(line, sizeof(line), logFile) != NULL) {
        printf("%s", line);
    }

    printf(
        "=========================================\n"
    );

    fclose(logFile);

    writeAudit(
        "VIEW_AUDIT",
        AUDIT_FILE,
        "SUCCESS"
    );
}

void showMenu(void)
{
    printf("\n========================================\n");

    printf(
        "Logged in as: %s (%s)\n",
        currentUser->username,
        currentUser->group
    );

    printf("========================================\n");
    printf("1. Create file\n");
    printf("2. Read file\n");
    printf("3. Write to file\n");
    printf("4. Delete file\n");
    printf("5. Change permissions\n");
    printf("6. Check execute permission\n");
    printf("7. Encrypt file\n");
    printf("8. Decrypt file\n");
    printf("9. View audit log (administrator only)\n");
    printf("0. Exit\n");
}

int main(void)
{
    int choice;

    if (!setupStorage()) {
        return EXIT_FAILURE;
    }

    loadMetadata();

    if (!login()) {
        return EXIT_FAILURE;
    }

    do {
        showMenu();

        choice = getNumber(
            "Choose an option: "
        );

        switch (choice) {
            case 1:
                createFile();
                break;

            case 2:
                readFile();
                break;

            case 3:
                writeFile();
                break;

            case 4:
                deleteFile();
                break;

            case 5:
                changePermissions();
                break;

            case 6:
                checkExecutePermission();
                break;

            case 7:
                xorFile("ENCRYPT");
                break;

            case 8:
                xorFile("DECRYPT");
                break;

            case 9:
                viewAuditLog();
                break;

            case 0:
                printf("Exiting securely.\n");

                writeAudit(
                    "LOGOUT",
                    NULL,
                    "SUCCESS"
                );

                break;

            default:
                printf(
                    "Please choose a valid menu option.\n"
                );
        }

    } while (choice != 0);

    return EXIT_SUCCESS;
}
