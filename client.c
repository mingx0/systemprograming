/*
 * client.c
 * ------------------------------------------------------------
 * 학생 정보 관리 프로그램의 클라이언트 코드입니다.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

// 서버와 같은 포트 번호를 사용해야 접속할 수 있습니다.
#define PORT 9000
// 서버와 주고받을 문자열 버퍼 크기
#define BUF_SIZE 1024

/*
 * send_all()
 * 사용자가 입력한 내용을 서버로 보낼 때 사용합니다.
 * send()가 일부만 보낼 수 있으므로 모든 바이트가 전송될 때까지 반복합니다.
 */
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

/*
 * recv_line()
 * 서버가 보낸 명령 헤더(TEXT 길이, INPUT 길이, END, EXIT)를
 * 한 줄 단위로 읽습니다.
 */
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

/*
 * recv_bytes()
 * 서버가 알려준 길이만큼 본문 데이터를 정확히 읽습니다.
 */
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

/*
 * main()
 * 서버 IP로 접속한 뒤, 서버가 보내는 TEXT/INPUT/END/EXIT 명령에 따라 동작합니다.
 * 클라이언트는 DB 파일을 직접 열지 않습니다.
 */
int main(int argc, char *argv[])
{
    int sock_fd;
    struct sockaddr_in server_addr;
    char *server_ip = "127.0.0.1";
    char line[BUF_SIZE];
    char input[BUF_SIZE];

    // 실행 시 IP를 입력하지 않으면 기본값으로 127.0.0.1에 접속합니다.
    // 예: ./client 127.0.0.1
    if (argc >= 2)
        server_ip = argv[1];

    // AF_INET + SOCK_STREAM이므로 인터넷 소켓(TCP/IP) 방식입니다.
    // pipe는 사용하지 않습니다.
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

    // 서버의 IP와 PORT로 접속을 시도합니다.
    if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("connect");
        close(sock_fd);
        return 1;
    }

    // 서버가 보내는 명령을 계속 받아 처리합니다.
    while (1)
    {
        if (!recv_line(sock_fd, line, sizeof(line))) break;

        // TEXT: 서버가 보낸 출력용 문자열입니다. 화면에 출력만 합니다.
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
        // INPUT: 서버가 입력을 요청한 경우입니다.
        // 프롬프트를 출력하고 사용자 입력을 서버로 전송합니다.
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
        // END: 메뉴 기능 하나가 끝났다는 의미입니다. 연결은 유지합니다.
        else if (strcmp(line, "END") == 0)
        {
            continue;
        }
        // EXIT: 서버가 종료를 요청한 경우 클라이언트를 종료합니다.
        else if (strcmp(line, "EXIT") == 0)
        {
            break;
        }
    }

    close(sock_fd);
    return 0;
}
