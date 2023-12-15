#define _GNU_SOURCE
#include "net.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

#define PORT 28534

void writeHeader(int nfd, char *status, int length)
{
    char header[100];
    sprintf(header,
            "HTTP/1.0 %s\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: %d\r\n"
            "\r\n",
            status, length);
    printf("%s\n", header); // debugging
    write(nfd, header, strlen(header));
    // free(header);
}

void getReply(int nfd, char *filename, int getContent)
{
    struct stat fileStat;
    if (stat(filename, &fileStat) == -1)
    {
        writeHeader(nfd, "404 Not Found", 239); // hardcoding error message (239 is length of )
        if (getContent == 1)
        {
            char error_message[] = "<html>\n<head>\n  <meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"/>\n  <title>Error</title>\n  <LINK href=\"styles.css\" rel=\"stylesheet\" type=\"text/css\">\n</head>\n\n<body>\n\n<p>\n  404 Error. Page not found!\n</p>\n\n</body>\n</html>\n";
            printf("%s\n", error_message);
            write(nfd, error_message, sizeof(error_message) - 1);
        }
        // return;
    }
    else
    {
        int fd = open(filename, O_RDONLY);
        if (fd == -1)
        {
            perror("Error opening file\n");
            return;
        }
        writeHeader(nfd, "200 OK", fileStat.st_size);
        if (getContent == 1)
        {
            char contentMsg[] = "<---- contents here ---->\n";
            write(nfd, contentMsg, sizeof(contentMsg) - 1);

            FILE *network = fdopen(fd, "r");
            char *line = NULL;
            size_t size;
            ssize_t num;
            while ((num = getline(&line, &size, network)) >= 0)
            {
                printf("%s", line);
                write(nfd, line, num);
            }
            printf("\n");
            write(nfd, "\n", 1);
            free(line);

            fclose(network);
        }
        close(fd);
    }
}

void handle_request(int nfd)
{
    FILE *network = fdopen(nfd, "r");
    char *line = NULL;
    size_t size;
    ssize_t num;
    char method[20];
    char filename[20];
    char version[20];

    if (network == NULL)
    {
        perror("fdopen");
        close(nfd);
        return;
    }

    while ((num = getline(&line, &size, network)) >= 0)
    { // proper format: TYPE filename HTTP/version
        if (sscanf(line, "%s %s %s", method, filename, version) != 3 ||
            strncmp(version, "HTTP/", 5) != 0)
        {
            writeHeader(nfd, "400 Bad Request", 239); // hardcoding error message
            // if (getContent == 1){
            char error_message[] = "<html>\n<head>\n  <meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"/>\n  <title>Error</title>\n  <LINK href=\"styles.css\" rel=\"stylesheet\" type=\"text/css\">\n</head>\n\n<body>\n\n<p>\n  400 Error. Bad request!\n</p>\n\n</body>\n</html>\n";
            printf("%s", error_message);
            write(nfd, error_message, sizeof(error_message) - 1);
        }
        // break;
        else
        {
            strcpy(filename, filename + 1);
            if (strcmp(method, "GET") == 0)
            {
                getReply(nfd, filename, 1);
            }
            else if (strcmp(method, "HEAD") == 0)
            {
                getReply(nfd, filename, 0);
            }
            else
            {
                writeHeader(nfd, "400 Bad Request", 239); // hardcoding error message
                // if (getContent == 1){
                char error_message[] = "<html>\n<head>\n  <meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"/>\n  <title>Error</title>\n  <LINK href=\"styles.css\" rel=\"stylesheet\" type=\"text/css\">\n</head>\n\n<body>\n\n<p>\n  400 Error. Bad request!\n</p>\n\n</body>\n</html>\n";
                printf("%s/n", error_message);
                write(nfd, error_message, sizeof(error_message) - 1);
            }
            // break;
        }
    }
    free(line);
    fclose(network);
}

void run_service(int fd)
{
    while (1)
    {
        int nfd = accept_connection(fd);
        if (nfd == -1)
        {
            perror("accept");
            continue;
        }
        printf("Connection established\n");
        pid_t pid = fork();
        if (pid == -1)
        {
            perror("fork");
            continue;
        }
        if (pid == 0)
        {
            handle_request(nfd);
            printf("Connection closed\n");
            exit(0);
        }
        else
        {
            close(nfd);
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        perror("Invalid argument\n");
        exit(1);
    }
    int port = atoi(argv[1]);
    if (port < 1024 || port > 65535)
    {
        perror("Port number must be between 1024 and 65535\n");
        exit(1);
    }

    int fd = create_service(PORT); // hardcoded port for now to match w client

    if (fd == -1)
    {
        perror(0);
        exit(1);
    }

    printf("listening on port: %d\n", PORT);
    run_service(fd);
    close(fd);

    return 0;
}
