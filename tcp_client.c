#if defined(_WIN32)
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <conio.h> // for using khbit() to read terminal input in Windows

#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#endif

#if defined(_WIN32)
#define ISVALIDSOCKET(s) ((s) != INVALID_SOCKET)
#define CLOSESOCKET(s) closesocket(s)
#define GETSOCKETERRNO() (WSAGetLastError())

#else
#define ISVALIDSOCKET(s) ((s) >= 0)
#define CLOSESOCKET(s) close(s)
#define SOCKET int
#define GETSOCKETERRNO() (errno)
#endif

#include <stdio.h>
#include <string.h>

static void InsertPrefix(char *String, size_t Length, char  *Prefix);

int main(int argc, char *argv[]) {

#if defined(_WIN32)
    WSADATA d;
    if (WSAStartup(MAKEWORD(2, 2), &d)) {
        fprintf(stderr, "Failed to initialize.\n");
        return 1;
    }
#endif

    if (argc < 4) {
        fprintf(stderr, "usage: tcp_client hostname port username\n");
        return 1;
    }

    printf("Configuring remote address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo *peer_address;
    if (getaddrinfo(argv[1], argv[2], &hints, &peer_address)) {
        fprintf(stderr, "getaddrinfo() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }


    printf("Remote address is: ");
    char address_buffer[100];
    char service_buffer[100];
    getnameinfo(peer_address->ai_addr, peer_address->ai_addrlen,
            address_buffer, sizeof(address_buffer),
            service_buffer, sizeof(service_buffer),
            NI_NUMERICHOST);
    printf("%s %s\n", address_buffer, service_buffer);


    printf("Creating socket...\n");
    SOCKET socket_peer;
    socket_peer = socket(peer_address->ai_family,
            peer_address->ai_socktype, peer_address->ai_protocol);
    if (!ISVALIDSOCKET(socket_peer)) {
        fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }


    printf("Connecting...\n");
    if (connect(socket_peer,
                peer_address->ai_addr, peer_address->ai_addrlen)) {
        fprintf(stderr, "connect() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }
    freeaddrinfo(peer_address);

    printf("Connected.\n");
    printf("\t\t Welcome to UMP Chats !!! \n\tTo send data, enter text followed by enter.\n\n");

    while(1) {
        fd_set reads;
        FD_ZERO(&reads);
        FD_SET(socket_peer, &reads);
#if !defined(_WIN32)
        FD_SET(0, &reads);
#endif

        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000;

        if (select(socket_peer+1, &reads, 0, 0, &timeout) < 0) {
            fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
            return 1;
        }

        if (FD_ISSET(socket_peer, &reads)) {
            char read[4096];
            int bytes_received = recv(socket_peer, read, 4096, 0);
            if (bytes_received < 1) {
                printf("Connection closed by peer.\n");
                break;
            }
            read[bytes_received] = 0;
            printf("  >> %.*s", bytes_received, read);
        }

#if defined(_WIN32)
        if(_kbhit()) {
        printf("%s : ", argv[3]);           
#else

        if(FD_ISSET(0, &reads)) {
#endif

            
            char read[4096];

            if (!fgets(read, 4096, stdin)) break;

            InsertPrefix(read, sizeof read,argv[3]);  

#if !defined(_WIN32)
            printf("\033[F");
            printf("%s", read);
#endif

            int bytes_sent = send(socket_peer, read, strlen(read), 0);

        }
    } //end while(1)

    printf("Closing socket...\n");
    CLOSESOCKET(socket_peer);

#if defined(_WIN32)
    WSACleanup();
#endif

    printf("Finished.\n");
    return 0;
}

static void InsertPrefix(char *String, size_t Length, char  *Prefix)
{
    //  Find out how many characters is needeed.
    int CharactersNeeded = snprintf(NULL, 0, "%s : ", Prefix);

    //  Find the current string length.
    size_t Current = strlen(String);

    //  Move the string to make room for the prefix.
    memmove(String + CharactersNeeded, String, Current + 1);

    /*  Remember the first character, because snprintf will overwrite it with a
        null character.
    */
    char Temporary = String[0];

    //  Write the prefix, including a terminating null character.
    snprintf(String, CharactersNeeded + 1, "%s : ", Prefix);

    //  Restore the first character of the original string.
    String[CharactersNeeded] = Temporary;
}