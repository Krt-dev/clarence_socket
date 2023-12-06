#include <stdio.h>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <stdlib.h>

#define DEFAULT_PORT "27015"
#define DEFAULT_BUFLEN 512



int main(int argc, char **argv) {
    WSADATA wsaData;
    int iResult;

    SOCKET client_socket = INVALID_SOCKET;
    struct addrinfo *result = NULL, *ptr = NULL, hints;
    char sendbuf[DEFAULT_BUFLEN];
    char recvbuf[DEFAULT_BUFLEN];
    int iSendResult;
    int recvbuflen = DEFAULT_BUFLEN;

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

    // Resolve the server address and port
    iResult = getaddrinfo(argv[1], DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Attempt to connect to the first address returned by
    // the call to getaddrinfo
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
        // Create a SOCKET for connecting to server
        client_socket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (client_socket == INVALID_SOCKET) {
            printf("socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }

        // Connect to server
        iResult = connect(client_socket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(client_socket);
            client_socket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (client_socket == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        WSACleanup();
        return 1;
    }

    int tryAgain = 1;

    while (tryAgain) {
        // Receive and display questions, send answers
        for (int i = 0; i < 4; ++i) {
            // Receive and display the question
            iResult = recv(client_socket, recvbuf, sizeof(recvbuf), 0);
            if (iResult > 0) {
                recvbuf[iResult] = '\0';  // Null-terminate the received data
                printf("%s", recvbuf);
            } else {
                printf("recv failed with error: %d\n", WSAGetLastError());
                break;
            }

            // Get user input and send the answer
            printf("Enter your answer (1-5): ");
            fgets(sendbuf, sizeof(sendbuf), stdin);
            iSendResult = send(client_socket, sendbuf, strlen(sendbuf), 0);
            if (iSendResult == SOCKET_ERROR) {
                printf("send failed with error: %d\n", WSAGetLastError());
                break;
            }
        }

        // Receive and display luck determination
        iResult = recv(client_socket, recvbuf, sizeof(recvbuf), 0);
        if (iResult > 0) {
            recvbuf[iResult] = '\0';  // Null-terminate the received data
            printf("%s", recvbuf);
        } else {
            printf("recv failed with error: %d\n", WSAGetLastError());
        }

        // Ask if the user wants to try again
        printf("Do you want to try again? (1 for yes, 0 for no): ");
        fgets(sendbuf, sizeof(sendbuf), stdin);

        // Convert the input to an integer
        tryAgain = atoi(sendbuf);

        // Send the tryAgain status to the server
        iSendResult = send(client_socket, sendbuf, strlen(sendbuf), 0);
        if (iSendResult == SOCKET_ERROR) {
            printf("send failed with error: %d\n", WSAGetLastError());
            break;
        }
    }

    // Close the socket
    closesocket(client_socket);
    WSACleanup();

    return 0;
}
