#define _POSIX_C_SOURCE 200809L

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define SERVER_ADDRESS "127.0.0.1"
#define SERVER_PORT 8080
#define BUFFER_SIZE 512

/*
 * Sends the complete message to the server.
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
 * Receives one newline-terminated response from the server.
 *
 * Return values:
 *  1 = response received
 *  0 = server disconnected
 * -1 = socket error
 */
int receiveLine(int socketFd, char *buffer, int size)
{
    int index = 0;
    char character;

    while (index < size - 1) {
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

        buffer[index] = character;
        index++;
    }

    buffer[index] = '\0';
    return 1;
}

/*
 * Removes extra input when the user types more than the buffer allows.
 */
void clearExtraInput(void)
{
    int character;

    while (
        (character = getchar()) != '\n' &&
        character != EOF
    ) {
        /* Remove remaining characters. */
    }
}

int main(void)
{
    int clientSocket;
    struct sockaddr_in serverAddress;

    char input[BUFFER_SIZE];
    char response[BUFFER_SIZE];

    clientSocket = socket(
        AF_INET,
        SOCK_STREAM,
        0
    );

    if (clientSocket == -1) {
        perror("Could not create client socket");
        return EXIT_FAILURE;
    }

    memset(
        &serverAddress,
        0,
        sizeof(serverAddress)
    );

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(SERVER_PORT);

    if (
        inet_pton(
            AF_INET,
            SERVER_ADDRESS,
            &serverAddress.sin_addr
        ) <= 0
    ) {
        printf("Invalid server address.\n");
        close(clientSocket);
        return EXIT_FAILURE;
    }

    if (
        connect(
            clientSocket,
            (struct sockaddr *) &serverAddress,
            sizeof(serverAddress)
        ) == -1
    ) {
        perror("Could not connect to server");
        close(clientSocket);
        return EXIT_FAILURE;
    }

    printf("========================================\n");
    printf("TASK 4 TCP CLIENT\n");
    printf("========================================\n");
    printf(
        "Connected to %s:%d\n\n",
        SERVER_ADDRESS,
        SERVER_PORT
    );

    int receiveResult = receiveLine(
        clientSocket,
        response,
        sizeof(response)
    );

    if (receiveResult <= 0) {
        printf("Server closed the connection.\n");
        close(clientSocket);
        return EXIT_FAILURE;
    }

    printf("Server: %s\n", response);

    while (1) {
        printf("\nCommand: ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("\nInput ended.\n");
            break;
        }

        if (strchr(input, '\n') == NULL) {
            clearExtraInput();
            printf("Input is too long. Try again.\n");
            continue;
        }

        if (strcmp(input, "\n") == 0) {
            printf("Command cannot be empty.\n");
            continue;
        }

        if (!sendText(clientSocket, input)) {
            printf("Could not send command to server.\n");
            break;
        }

        receiveResult = receiveLine(
            clientSocket,
            response,
            sizeof(response)
        );

        if (receiveResult == 0) {
            printf("Server disconnected.\n");
            break;
        }

        if (receiveResult == -1) {
            printf("Error receiving server response.\n");
            break;
        }

        printf("Server: %s\n", response);

        if (strncmp(response, "BYE", 3) == 0) {
            break;
        }
    }

    close(clientSocket);

    printf("Client closed safely.\n");

    return EXIT_SUCCESS;
}
