#include <stdio.h>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <stdlib.h>

#define DEFAULT_PORT "27015"
#define DEFAULT_BUFLEN 512

void handle_client(SOCKET client_socket);

int main() {
    WSADATA wsaData;
    int iResult;

    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;

    struct addrinfo *result = NULL;
    struct addrinfo hints;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for the server to listen for client connections.
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Setup the TCP listening socket
    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // Accept a client socket
    ClientSocket = accept(ListenSocket, NULL, NULL);
    if (ClientSocket == INVALID_SOCKET) {
        printf("accept failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // No longer need the server socket
    closesocket(ListenSocket);

    int tryAgain = 1;

    while (tryAgain) {
        // Handle the client
        handle_client(ClientSocket);

        // Receive the tryAgain status from the client
        char recvbuf[DEFAULT_BUFLEN];
        iResult = recv(ClientSocket, recvbuf, sizeof(recvbuf), 0);

        if (iResult > 0) {
            recvbuf[iResult] = '\0';  // Null-terminate the received data

            // Convert the received data to an integer
            tryAgain = atoi(recvbuf);
        } else {
            printf("recv failed with error: %d\n", WSAGetLastError());
            break;
        }
    }

    // Shutdown and cleanup
    closesocket(ClientSocket);
    WSACleanup();

    return 0;
}

void handle_client(SOCKET client_socket) {
    char recvbuf[DEFAULT_BUFLEN];
    int iResult;
    int score = 0;

    const char* questions[] = {
        "How was your day? (1-5)\n",
        "How often do you experience what you consider lucky events? (1-5)\n",
        "Have you ever made a decision that turned out to be exceptionally lucky? (1-5)\n",
        "To what extent do you believe luck plays a role in your life? (1-5)\n"
    };

for (int i = 0; i < 4; ++i) {
        send(client_socket, questions[i], strlen(questions[i]), 0);
        iResult = recv(client_socket, recvbuf, sizeof(recvbuf), 0);

        if (iResult > 0) {
            int answer = atoi(recvbuf);
            if (answer >= 1 && answer <= 5) {
                score += answer;
            } else {
                printf("Invalid answer received. Ignoring.\n");
            }
        } else {
            printf("recv failed with error: %d\n", WSAGetLastError());
            break;
        }
    }

    if (score <= 8) {
        send(client_socket, "You have bad luck today.\n", 26, 0);
    } else if (score <= 12) {
        send(client_socket, "You're quite lucky.\n", 21, 0);
    } else {
        send(client_socket, "You're very lucky today.\n", 25, 0);
    }
}
