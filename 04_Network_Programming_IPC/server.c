#define _POSIX_C_SOURCE 200809L

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define SERVER_PORT 8080
#define BUFFER_SIZE 512
#define MAX_CLIENTS 10
#define SERVER_LOG "server.log"

typedef struct {
    char username[32];
    char password[32];
} Account;

typedef struct {
    int socketFd;
    char address[INET_ADDRSTRLEN];
    int port;
} ClientInfo;

/* Demonstration accounts for coursework testing. */
Account accounts[] = {
    {"saurav", "student123"},
    {"ankit", "member123"},
    {"guest", "guest123"}
};

int activeClients = 0;
int listeningSocket = -1;
volatile sig_atomic_t serverRunning = 1;

pthread_mutex_t clientCountMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t logMutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * Sends all characters in a response.
 * This handles cases where send() transmits only part of the text.
 */
int sendText(int socketFd, const char *text)
{
    size_t totalSent = 0;
    size_t textLength = strlen(text);

    while (totalSent < textLength) {
        ssize_t sent = send(
            socketFd,
            text + totalSent,
            textLength - totalSent,
            0
        );

        if (sent <= 0) {
            return 0;
        }

        totalSent += (size_t) sent;
    }

    return 1;
}

/*
 * Reads one newline-terminated protocol message.
 *
 * Return values:
 *  1  = message received
 *  0  = client disconnected
 * -1  = socket error
 * -2  = message exceeded the buffer size
 */
int receiveLine(int socketFd, char *buffer, int size)
{
    int index = 0;
    int tooLong = 0;
    char character;

    while (1) {
        ssize_t received = recv(
            socketFd,
            &character,
            1,
            0
        );

        if (received == 0) {
            return 0;
        }

        if (received < 0) {
            if (errno == EINTR) {
                continue;
            }

            return -1;
        }

        if (character == '\n') {
            break;
        }

        if (character == '\r') {
            continue;
        }

        if (index < size - 1) {
            buffer[index] = character;
            index++;
        } else {
            tooLong = 1;
        }
    }

    buffer[index] = '\0';

    if (tooLong) {
        return -2;
    }

    return 1;
}

/*
 * Removes spaces from the beginning and end of a message.
 */
char *trimText(char *text)
{
    char *end;

    while (isspace((unsigned char) *text)) {
        text++;
    }

    if (*text == '\0') {
        return text;
    }

    end = text + strlen(text) - 1;

    while (end > text && isspace((unsigned char) *end)) {
        end--;
    }

    end[1] = '\0';

    return text;
}

/*
 * Checks the demonstration username and password.
 */
int validLogin(const char *username, const char *password)
{
    int accountCount =
        sizeof(accounts) / sizeof(accounts[0]);

    for (int index = 0; index < accountCount; index++) {
        if (
            strcmp(accounts[index].username, username) == 0 &&
            strcmp(accounts[index].password, password) == 0
        ) {
            return 1;
        }
    }

    return 0;
}

/*
 * Writes a timestamped record to server.log.
 */
void writeLog(
    const char *address,
    const char *username,
    const char *action,
    const char *result
)
{
    FILE *logFile;
    time_t currentTime;
    struct tm timeDetails;
    char timestamp[32];

    currentTime = time(NULL);
    localtime_r(&currentTime, &timeDetails);

    strftime(
        timestamp,
        sizeof(timestamp),
        "%Y-%m-%d %H:%M:%S",
        &timeDetails
    );

    pthread_mutex_lock(&logMutex);

    logFile = fopen(SERVER_LOG, "a");

    if (logFile != NULL) {
        fprintf(
            logFile,
            "[%s] client=%s user=%s action=%s result=%s\n",
            timestamp,
            address,
            username,
            action,
            result
        );

        fclose(logFile);
        chmod(SERVER_LOG, 0600);
    }

    pthread_mutex_unlock(&logMutex);
}

/*
 * Returns the current number of connected clients safely.
 */
int getActiveClientCount(void)
{
    int count;

    pthread_mutex_lock(&clientCountMutex);
    count = activeClients;
    pthread_mutex_unlock(&clientCountMutex);

    return count;
}

/*
 * Increases the client count if the limit has not been reached.
 */
int addClient(void)
{
    int allowed = 0;

    pthread_mutex_lock(&clientCountMutex);

    if (activeClients < MAX_CLIENTS) {
        activeClients++;
        allowed = 1;
    }

    pthread_mutex_unlock(&clientCountMutex);

    return allowed;
}

/*
 * Decreases the client count after a connection closes.
 */
void removeClient(void)
{
    pthread_mutex_lock(&clientCountMutex);

    if (activeClients > 0) {
        activeClients--;
    }

    pthread_mutex_unlock(&clientCountMutex);
}

/*
 * Sends the command list to a client.
 */
void sendHelp(int socketFd)
{
    sendText(
        socketFd,
        "INFO Commands: LOGIN <user> <password>, "
        "MSG <text>, STATUS, TIME, HELP, QUIT\n"
    );
}

/*
 * Handles one connected client.
 * Every client runs this function in its own thread.
 */
void *handleClient(void *argument)
{
    ClientInfo *client = (ClientInfo *) argument;

    int authenticated = 0;
    int connectionOpen = 1;

    char buffer[BUFFER_SIZE];
    char username[32] = "unauthenticated";
    char response[BUFFER_SIZE];

    printf(
        "Client connected: %s:%d | Active clients: %d\n",
        client->address,
        client->port,
        getActiveClientCount()
    );

    writeLog(
        client->address,
        username,
        "CONNECT",
        "SUCCESS"
    );

    sendText(
        client->socketFd,
        "INFO Connected to the Task 4 server. Type HELP for commands.\n"
    );

    while (connectionOpen) {
        int receiveResult = receiveLine(
            client->socketFd,
            buffer,
            sizeof(buffer)
        );

        if (receiveResult == 0) {
            printf(
                "Client disconnected unexpectedly: %s:%d\n",
                client->address,
                client->port
            );

            writeLog(
                client->address,
                username,
                "DISCONNECT",
                "UNEXPECTED"
            );

            break;
        }

        if (receiveResult == -1) {
            writeLog(
                client->address,
                username,
                "RECEIVE",
                "FAILURE"
            );

            break;
        }

        if (receiveResult == -2) {
            sendText(
                client->socketFd,
                "ERROR Message too long\n"
            );

            writeLog(
                client->address,
                username,
                "VALIDATION",
                "MESSAGE_TOO_LONG"
            );

            continue;
        }

        char *command = trimText(buffer);

        if (command[0] == '\0') {
            sendText(
                client->socketFd,
                "ERROR Empty command\n"
            );

            writeLog(
                client->address,
                username,
                "VALIDATION",
                "EMPTY_COMMAND"
            );

            continue;
        }

        if (strncmp(command, "LOGIN ", 6) == 0) {
            char enteredUsername[32];
            char enteredPassword[32];
            char extraCharacter;

            if (authenticated) {
                sendText(
                    client->socketFd,
                    "ERROR Already authenticated\n"
                );

                continue;
            }

            int valuesRead = sscanf(
                command + 6,
                "%31s %31s %c",
                enteredUsername,
                enteredPassword,
                &extraCharacter
            );

            if (valuesRead != 2) {
                sendText(
                    client->socketFd,
                    "ERROR Usage: LOGIN <username> <password>\n"
                );

                writeLog(
                    client->address,
                    username,
                    "LOGIN",
                    "INVALID_FORMAT"
                );

                continue;
            }

            if (validLogin(enteredUsername, enteredPassword)) {
                authenticated = 1;

                snprintf(
                    username,
                    sizeof(username),
                    "%s",
                    enteredUsername
                );

                sendText(
                    client->socketFd,
                    "OK LOGIN successful\n"
                );

                writeLog(
                    client->address,
                    username,
                    "LOGIN",
                    "SUCCESS"
                );
            } else {
                sendText(
                    client->socketFd,
                    "ERROR Invalid credentials\n"
                );

                writeLog(
                    client->address,
                    enteredUsername,
                    "LOGIN",
                    "DENIED"
                );
            }
        } else if (strncmp(command, "MSG ", 4) == 0) {
            char *message = trimText(command + 4);

            if (!authenticated) {
                sendText(
                    client->socketFd,
                    "ERROR Authentication required\n"
                );

                writeLog(
                    client->address,
                    username,
                    "MSG",
                    "DENIED"
                );

                continue;
            }

            if (message[0] == '\0') {
                sendText(
                    client->socketFd,
                    "ERROR Message cannot be empty\n"
                );

                continue;
            }

            printf(
                "Message from %s: %s\n",
                username,
                message
            );

            sendText(
                client->socketFd,
                "OK MESSAGE received\n"
            );

            writeLog(
                client->address,
                username,
                "MSG",
                "SUCCESS"
            );
        } else if (strcmp(command, "STATUS") == 0) {
            if (!authenticated) {
                sendText(
                    client->socketFd,
                    "ERROR Authentication required\n"
                );

                continue;
            }

            snprintf(
                response,
                sizeof(response),
                "INFO user=%s authenticated=yes active_clients=%d\n",
                username,
                getActiveClientCount()
            );

            sendText(
                client->socketFd,
                response
            );

            writeLog(
                client->address,
                username,
                "STATUS",
                "SUCCESS"
            );
        } else if (strcmp(command, "TIME") == 0) {
            time_t now;
            struct tm timeDetails;
            char timeText[64];

            if (!authenticated) {
                sendText(
                    client->socketFd,
                    "ERROR Authentication required\n"
                );

                continue;
            }

            now = time(NULL);
            localtime_r(&now, &timeDetails);

            strftime(
                timeText,
                sizeof(timeText),
                "%Y-%m-%d %H:%M:%S",
                &timeDetails
            );

            snprintf(
                response,
                sizeof(response),
                "INFO Server time: %s\n",
                timeText
            );

            sendText(
                client->socketFd,
                response
            );

            writeLog(
                client->address,
                username,
                "TIME",
                "SUCCESS"
            );
        } else if (strcmp(command, "HELP") == 0) {
            sendHelp(client->socketFd);
        } else if (strcmp(command, "QUIT") == 0) {
            sendText(
                client->socketFd,
                "BYE Connection closed\n"
            );

            writeLog(
                client->address,
                username,
                "QUIT",
                "SUCCESS"
            );

            connectionOpen = 0;
        } else {
            sendText(
                client->socketFd,
                "ERROR Unknown command. Type HELP.\n"
            );

            writeLog(
                client->address,
                username,
                "COMMAND",
                "UNKNOWN"
            );
        }
    }

    close(client->socketFd);
    removeClient();

    printf(
        "Client connection closed: %s:%d | Active clients: %d\n",
        client->address,
        client->port,
        getActiveClientCount()
    );

    free(client);
    return NULL;
}

/*
 * Allows Ctrl+C to close the listening socket.
 */
void stopServer(int signalNumber)
{
    (void) signalNumber;

    serverRunning = 0;

    if (listeningSocket != -1) {
        close(listeningSocket);
        listeningSocket = -1;
    }
}

int main(void)
{
    struct sockaddr_in serverAddress;
    int reuseAddress = 1;

    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, stopServer);

    listeningSocket = socket(
        AF_INET,
        SOCK_STREAM,
        0
    );

    if (listeningSocket == -1) {
        perror("Could not create server socket");
        return EXIT_FAILURE;
    }

    if (
        setsockopt(
            listeningSocket,
            SOL_SOCKET,
            SO_REUSEADDR,
            &reuseAddress,
            sizeof(reuseAddress)
        ) == -1
    ) {
        perror("Could not configure server socket");
        close(listeningSocket);
        return EXIT_FAILURE;
    }

    memset(
        &serverAddress,
        0,
        sizeof(serverAddress)
    );

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(SERVER_PORT);
    serverAddress.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (
        bind(
            listeningSocket,
            (struct sockaddr *) &serverAddress,
            sizeof(serverAddress)
        ) == -1
    ) {
        perror("Could not bind server socket");
        close(listeningSocket);
        return EXIT_FAILURE;
    }

    if (listen(listeningSocket, MAX_CLIENTS) == -1) {
        perror("Could not listen for clients");
        close(listeningSocket);
        return EXIT_FAILURE;
    }

    printf("========================================\n");
    printf("TASK 4 MULTI-CLIENT TCP SERVER\n");
    printf("========================================\n");
    printf("Listening on 127.0.0.1:%d\n", SERVER_PORT);
    printf("Press Ctrl+C to stop the server.\n\n");

    while (serverRunning) {
        struct sockaddr_in clientAddress;
        socklen_t addressLength = sizeof(clientAddress);

        int clientSocket = accept(
            listeningSocket,
            (struct sockaddr *) &clientAddress,
            &addressLength
        );

        if (clientSocket == -1) {
            if (!serverRunning) {
                break;
            }

            perror("Could not accept client");
            continue;
        }

        if (!addClient()) {
            sendText(
                clientSocket,
                "ERROR Server client limit reached\n"
            );

            close(clientSocket);
            continue;
        }

        ClientInfo *client =
            malloc(sizeof(ClientInfo));

        if (client == NULL) {
            sendText(
                clientSocket,
                "ERROR Server memory failure\n"
            );

            close(clientSocket);
            removeClient();
            continue;
        }

        client->socketFd = clientSocket;
        client->port = ntohs(clientAddress.sin_port);

        if (
            inet_ntop(
                AF_INET,
                &clientAddress.sin_addr,
                client->address,
                sizeof(client->address)
            ) == NULL
        ) {
            snprintf(
                client->address,
                sizeof(client->address),
                "unknown"
            );
        }

        pthread_t clientThread;

        if (
            pthread_create(
                &clientThread,
                NULL,
                handleClient,
                client
            ) != 0
        ) {
            printf("Could not create client thread.\n");

            close(clientSocket);
            free(client);
            removeClient();
            continue;
        }

        pthread_detach(clientThread);
    }

    printf("\nServer stopped safely.\n");

    return EXIT_SUCCESS;
}
