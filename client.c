#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT 9000
#define BUF_SIZE 1024

int send_all(int fd, const void *buf, size_t len)
{
    const char *p = (const char *)buf;
    while (len > 0)
    {
        ssize_t n = send(fd, p, len, 0);
        if (n <= 0) return -1;
        p += n;
        len -= n;
    }
    return 0;
}

int recv_line(int fd, char *buf, int size)
{
    int i = 0;
    char ch;
    while (i < size - 1)
    {
        ssize_t n = recv(fd, &ch, 1, 0);
        if (n <= 0) return 0;
        if (ch == '\n') break;
        buf[i++] = ch;
    }
    buf[i] = '\0';
    return 1;
}

int recv_bytes(int fd, char *buf, int len)
{
    int total = 0;
    while (total < len)
    {
        ssize_t n = recv(fd, buf + total, len - total, 0);
        if (n <= 0) return 0;
        total += n;
    }
    return 1;
}

int main(int argc, char *argv[])
{
    int sock_fd;
    struct sockaddr_in server_addr;
    char *server_ip = "127.0.0.1";
    char line[BUF_SIZE];
    char input[BUF_SIZE];

    if (argc >= 2)
        server_ip = argv[1];

    sock_fd = socket(AF_INET, SOCK_STREAM, 0);   // Internet socket
    if (sock_fd == -1)
    {
        perror("socket");
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(server_ip);

    if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("connect");
        close(sock_fd);
        return 1;
    }

    while (1)
    {
        if (!recv_line(sock_fd, line, sizeof(line))) break;

        if (strncmp(line, "TEXT ", 5) == 0)
        {
            int len = atoi(line + 5);
            char *msg = (char *)malloc(len + 1);
            if (msg == NULL) break;
            if (!recv_bytes(sock_fd, msg, len))
            {
                free(msg);
                break;
            }
            msg[len] = '\0';
            printf("%s", msg);
            fflush(stdout);
            free(msg);
        }
        else if (strncmp(line, "INPUT ", 6) == 0)
        {
            int len = atoi(line + 6);
            char *prompt = (char *)malloc(len + 1);
            if (prompt == NULL) break;
            if (!recv_bytes(sock_fd, prompt, len))
            {
                free(prompt);
                break;
            }
            prompt[len] = '\0';
            printf("%s", prompt);
            fflush(stdout);
            free(prompt);

            if (fgets(input, sizeof(input), stdin) == NULL) break;
            send_all(sock_fd, input, strlen(input));
        }
        else if (strcmp(line, "END") == 0)
        {
            continue;
        }
        else if (strcmp(line, "EXIT") == 0)
        {
            break;
        }
    }

    close(sock_fd);
    return 0;
}