#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include "student.h"

#define PORT 9000
#define BACKLOG 10
#define BUF_SIZE 1024

char *FILE_NAME;

void service_client(int client_fd);
void do_menu(int client_fd, int menu);

void input_student(int client_fd);
void print_all_students(int client_fd);
void search_student(int client_fd);
void update_student(int client_fd);
void delete_student(int client_fd);

int lock_record(FILE *fp, short lock_type);
int unlock_record(FILE *fp);
int is_valid_phone(char *str);
int is_valid_birth(char *str);
int input_string_client(int fd, char *str, int size);
int input_int_client(int fd, int *num);
int send_text(int fd, const char *text);
int send_input(int fd, const char *prompt);

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

int send_text(int fd, const char *text)
{
    char header[64];
    int len = (int)strlen(text);
    snprintf(header, sizeof(header), "TEXT %d\n", len);
    if (send_all(fd, header, strlen(header)) == -1) return -1;
    return send_all(fd, text, len);
}

int send_input(int fd, const char *prompt)
{
    char header[64];
    int len = (int)strlen(prompt);
    snprintf(header, sizeof(header), "INPUT %d\n", len);
    if (send_all(fd, header, strlen(header)) == -1) return -1;
    return send_all(fd, prompt, len);
}

int send_end(int fd)
{
    return send_all(fd, "END\n", 4);
}

int send_exit_msg(int fd)
{
    return send_all(fd, "EXIT\n", 5);
}

int recv_line(int fd, char *buf, int size)
{
    int i = 0;
    char ch;
    int full = 0;

    while (1)
    {
        ssize_t n = recv(fd, &ch, 1, 0);
        if (n <= 0) return 0;
        if (ch == '\n') break;

        if (i < size - 1)
            buf[i++] = ch;
        else
            full = 1;  // 원래 input_string처럼 길이 초과 처리
    }

    buf[i] = '\0';
    if (full) return -1;
    return 1;
}

int main(int argc, char *argv[])
{
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;

    if (argc < 2)
    {
        printf("사용법 : %s 파일이름\n", argv[0]);
        return 1;
    }

    FILE_NAME = argv[1];

    server_fd = socket(AF_INET, SOCK_STREAM, 0);   // Internet socket
    if (server_fd == -1)
    {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, BACKLOG) == -1)
    {
        perror("listen");
        close(server_fd);
        return 1;
    }

    printf("서버 실행 중... port %d\n", PORT);
    printf("DB 파일은 서버에서만 접근합니다: %s\n", FILE_NAME);

    while (1)
    {
        client_len = sizeof(client_addr);
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd == -1) continue;

        pid_t pid = fork();
        if (pid < 0)
        {
            close(client_fd);
            continue;
        }
        else if (pid == 0)
        {
            close(server_fd);
            printf("클라이언트 접속: %s, pid=%d\n", inet_ntoa(client_addr.sin_addr), getpid());
            service_client(client_fd);
            close(client_fd);
            exit(0);
        }
        else
        {
            close(client_fd);
            while (waitpid(-1, NULL, WNOHANG) > 0);
        }
    }

    close(server_fd);
    return 0;
}

void service_client(int client_fd)
{
    int menu;

    while (1)
    {
        send_text(client_fd,
            "\n < 학생 파일 관리 프로그램 > \n"
            "1. 학생 정보 입력\n"
            "2. 전체 학생 정보 리스트로 보기\n"
            "3. 학생 정보 검색 (이름으로 검색)\n"
            "4. 학생 정보 갱신\n"
            "5. 학생 정보 삭제\n"
            "0. 프로그램 종료\n");
        send_input(client_fd, "입력 : ");

        if (!input_int_client(client_fd, &menu))
        {
            send_text(client_fd, "잘못입력하셨습니다.\n");
            send_text(client_fd, "다시 입력해주세요.\n");
            send_end(client_fd);
            continue;
        }

        if (menu == 0)
        {
            send_text(client_fd, "프로그램을 종료합니다!\n");
            send_exit_msg(client_fd);
            break;
        }

        if (menu >= 1 && menu <= 5)
        {
            pid_t pid = fork();
            if (pid < 0)
            {
                send_end(client_fd);
                continue;
            }
            else if (pid == 0)
            {
                do_menu(client_fd, menu);
                send_end(client_fd);
                exit(0);
            }
            else
            {
                waitpid(pid, NULL, 0);
            }
        }
        else
        {
            send_text(client_fd, "잘못 입력하셨습니다.\n");
            send_text(client_fd, "다시 입력해주세요.\n");
            send_end(client_fd);
        }
    }
}

void do_menu(int client_fd, int menu)
{
    switch (menu)
    {
        case 1:
            input_student(client_fd);
            break;
        case 2:
            print_all_students(client_fd);
            break;
        case 3:
            search_student(client_fd);
            break;
        case 4:
            update_student(client_fd);
            break;
        case 5:
            delete_student(client_fd);
            break;
    }
}

int lock_record(FILE *fp, short lock_type)
{
    struct flock lock;
    lock.l_type = lock_type;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    return fcntl(fileno(fp), F_SETLKW, &lock);
}

int unlock_record(FILE *fp)
{
    struct flock lock;
    lock.l_type = F_UNLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    return fcntl(fileno(fp), F_SETLK, &lock);
}

int input_string_client(int fd, char *str, int size)
{
    int ret = recv_line(fd, str, size);
    if (ret != 1) return 0;
    return 1;
}

int input_int_client(int fd, int *num)
{
    char buf[BUF_SIZE];
    char *end;
    long value;

    int ret = recv_line(fd, buf, sizeof(buf));
    if (ret != 1) return 0;

    errno = 0;
    value = strtol(buf, &end, 10);
    if (errno != 0 || end == buf || *end != '\0')
        return 0;

    *num = (int)value;
    return 1;
}

int is_valid_phone(char *str)
{
    int i;
    if (strlen(str) == 0)
        return 0;

    for (i = 0; str[i] != '\0'; i++)
    {
        if (!((str[i] >= '0' && str[i] <= '9') || str[i] == '+' || str[i] == '-'))
            return 0;
    }

    return 1;
}

int is_valid_birth(char *str)
{
    int i;
    if (strlen(str) == 0)
        return 0;

    for (i = 0; str[i] != '\0'; i++)
    {
        if (!((str[i] >= '0' && str[i] <= '9') || str[i] == '-'))
            return 0;
    }

    return 1;
}

void input_student(int client_fd)
{
    FILE *fp;
    STUDENT s;

    fp = fopen(FILE_NAME, "ab");
    if (fp == NULL)
    {
        send_text(client_fd, "파일을 열 수 없습니다.\n");
        return;
    }

    if (lock_record(fp, F_WRLCK) == -1)
    {
        fclose(fp);
        return;
    }

    send_text(client_fd, "학생 정보를 입력해주세요!\n");

    while (1)
    {
        send_input(client_fd, "ID : ");
        if (input_int_client(client_fd, &s.id))
            break;
        send_text(client_fd, "잘못입력하셨습니다.\n");
        send_text(client_fd, "다시 입력해주세요.\n");
    }

    while (1)
    {
        send_input(client_fd, "이름 : ");
        if (input_string_client(client_fd, s.name, sizeof(s.name)))
            break;
        send_text(client_fd, "잘못입력하셨습니다.\n");
        send_text(client_fd, "다시 입력해주세요.\n");
    }

    while (1)
    {
        send_input(client_fd, "주소 : ");
        if (input_string_client(client_fd, s.address, sizeof(s.address)))
            break;
        send_text(client_fd, "잘못입력하셨습니다.\n");
        send_text(client_fd, "다시 입력해주세요.\n");
    }

    while (1)
    {
        send_input(client_fd, "전화번호 : ");
        if (input_string_client(client_fd, s.number, sizeof(s.number)) && is_valid_phone(s.number))
            break;
        send_text(client_fd, "잘못입력하셨습니다.\n");
        send_text(client_fd, "다시 입력해주세요.\n");
    }

    while (1)
    {
        send_input(client_fd, "생년월일 : ");
        if (input_string_client(client_fd, s.birth, sizeof(s.birth)) && is_valid_birth(s.birth))
            break;
        send_text(client_fd, "잘못입력하셨습니다.\n");
        send_text(client_fd, "다시 입력해주세요.\n");
    }

    fwrite(&s, sizeof(STUDENT), 1, fp);
    unlock_record(fp);
    fclose(fp);

    send_text(client_fd, "저장 완료!\n");
}

void print_all_students(int client_fd)
{
    FILE *fp;
    STUDENT s;
    int count = 0;
    char msg[BUF_SIZE];

    fp = fopen(FILE_NAME, "rb");
    if (fp == NULL)
    {
        send_text(client_fd, "데이터가 존재하지 않습니다!\n");
        return;
    }

    if (lock_record(fp, F_RDLCK) == -1)
    {
        fclose(fp);
        return;
    }

    send_text(client_fd, "모든 학생 정보!\n");

    while (fread(&s, sizeof(STUDENT), 1, fp) == 1)
    {
        snprintf(msg, sizeof(msg), "\n[%d]\n", ++count);
        send_text(client_fd, msg);
        snprintf(msg, sizeof(msg), "ID : %d\n", s.id);
        send_text(client_fd, msg);
        snprintf(msg, sizeof(msg), "이름 : %s\n", s.name);
        send_text(client_fd, msg);
        snprintf(msg, sizeof(msg), "주소 : %s\n", s.address);
        send_text(client_fd, msg);
        snprintf(msg, sizeof(msg), "전화번호 : %s\n", s.number);
        send_text(client_fd, msg);
        snprintf(msg, sizeof(msg), "생년월일 :%s\n", s.birth);
        send_text(client_fd, msg);
    }

    if (count == 0)
        send_text(client_fd, "데이터 없음\n");

    unlock_record(fp);
    fclose(fp);
}

void search_student(int client_fd)
{
    FILE *fp;
    STUDENT s;
    char name[21];
    int found = 0;
    char msg[BUF_SIZE];

    fp = fopen(FILE_NAME, "rb");
    if (fp == NULL)
    {
        send_text(client_fd, "데이터 없음 \n");
        return;
    }

    if (lock_record(fp, F_RDLCK) == -1)
    {
        fclose(fp);
        return;
    }

    while (1)
    {
        send_input(client_fd, "이름을 입력해주세요!\n");
        if (input_string_client(client_fd, name, sizeof(name)))
            break;
        send_text(client_fd, "잘못입력하셨습니다.\n");
        send_text(client_fd, "다시 입력해주세요.\n");
    }

    while (fread(&s, sizeof(STUDENT), 1, fp) == 1)
    {
        if (strcmp(s.name, name) == 0)
        {
            send_text(client_fd, "검색하신 분의 정보입니다!\n");
            snprintf(msg, sizeof(msg), "ID : %d\n", s.id);
            send_text(client_fd, msg);
            snprintf(msg, sizeof(msg), "이름 : %s\n", s.name);
            send_text(client_fd, msg);
            snprintf(msg, sizeof(msg), "주소 : %s\n", s.address);
            send_text(client_fd, msg);
            snprintf(msg, sizeof(msg), "전화번호 : %s\n", s.number);
            send_text(client_fd, msg);
            snprintf(msg, sizeof(msg), "생년월일 : %s\n", s.birth);
            send_text(client_fd, msg);
            found = 1;
            break;
        }
    }

    if (!found)
        send_text(client_fd, "검색하신 이름을 찾을 수 없습니다!\n");

    unlock_record(fp);
    fclose(fp);
}

void update_student(int client_fd)
{
    FILE *fp;
    STUDENT s;
    char name[21];
    int found = 0;
    int menu;
    char msg[BUF_SIZE];

    fp = fopen(FILE_NAME, "rb+");
    if (fp == NULL)
    {
        send_text(client_fd, "데이터가 존재하지 않습니다!\n");
        return;
    }

    if (lock_record(fp, F_WRLCK) == -1)
    {
        fclose(fp);
        return;
    }

    while (1)
    {
        send_input(client_fd, "\n수정 할 이름을 입력해주세요! \n");
        if (input_string_client(client_fd, name, sizeof(name)))
            break;
        send_text(client_fd, "잘못입력하셨습니다.\n");
        send_text(client_fd, "다시 입력해주세요.\n");
    }

    while (fread(&s, sizeof(STUDENT), 1, fp) == 1)
    {
        if (strcmp(s.name, name) == 0)
        {
            found = 1;
            while (1)
            {
                send_text(client_fd, "\n < 학생 정보 > \n");
                snprintf(msg, sizeof(msg), "1. ID : %d\n", s.id);
                send_text(client_fd, msg);
                snprintf(msg, sizeof(msg), "2. 이름 : %s\n", s.name);
                send_text(client_fd, msg);
                snprintf(msg, sizeof(msg), "3. 주소 : %s\n", s.address);
                send_text(client_fd, msg);
                snprintf(msg, sizeof(msg), "4. 전화번호 : %s\n", s.number);
                send_text(client_fd, msg);
                snprintf(msg, sizeof(msg), "5. 생년월일 : %s\n", s.birth);
                send_text(client_fd, msg);
                send_text(client_fd, "0. 수정 완료! (종료)\n");
                send_input(client_fd, "수정하실 항목을 선택하세요 : ");

                if (!input_int_client(client_fd, &menu))
                {
                    send_text(client_fd, "잘못입력하셨습니다!\n");
                    send_text(client_fd, "다시 작성해주세요!\n");
                    continue;
                }

                switch (menu)
                {
                    case 1:
                        while (1)
                        {
                            send_input(client_fd, "수정할 ID : ");
                            if (input_int_client(client_fd, &s.id))
                                break;
                            send_text(client_fd, "잘못입력하셨습니다!\n");
                            send_text(client_fd, "다시 작성해주세요!\n");
                        }
                        break;

                    case 2:
                        while (1)
                        {
                            send_input(client_fd, "수정할 이름 : ");
                            if (input_string_client(client_fd, s.name, sizeof(s.name)))
                                break;
                            send_text(client_fd, "잘못입력하셨습니다!\n");
                            send_text(client_fd, "다시 작성해주세요!\n");
                        }
                        break;

                    case 3:
                        while (1)
                        {
                            send_input(client_fd, "수정할 주소 : ");
                            if (input_string_client(client_fd, s.address, sizeof(s.address)))
                                break;
                            send_text(client_fd, "잘못입력하셨습니다!\n");
                            send_text(client_fd, "다시 작성해주세요!\n");
                        }
                        break;

                    case 4:
                        while (1)
                        {
                            send_input(client_fd, "수정할 전화번호 : ");
                            if (input_string_client(client_fd, s.number, sizeof(s.number)) && is_valid_phone(s.number))
                                break;
                            send_text(client_fd, "잘못입력하셨습니다!\n");
                            send_text(client_fd, "다시 작성해주세요!\n");
                        }
                        break;

                    case 5:
                        while (1)
                        {
                            send_input(client_fd, "수정할 생년월일 : ");
                            if (input_string_client(client_fd, s.birth, sizeof(s.birth)) && is_valid_birth(s.birth))
                                break;
                            send_text(client_fd, "잘못입력하셨습니다!\n");
                            send_text(client_fd, "다시 작성해주세요!\n");
                        }
                        break;

                    case 0:
                        fseek(fp, -(long)sizeof(STUDENT), SEEK_CUR);
                        fwrite(&s, sizeof(STUDENT), 1, fp);
                        send_text(client_fd, "수정이 완료되었습니다!\n");
                        unlock_record(fp);
                        fclose(fp);
                        return;

                    default:
                        send_text(client_fd, "잘못입력하셨습니다!\n");
                        send_text(client_fd, "다시 작성해주세요!\n");
                }
            }
        }
    }

    if (!found)
    {
        send_text(client_fd, "해당되는 이름이 존재하지 않습니다!\n");
        send_text(client_fd, "다시 작성해주세요!");
    }

    unlock_record(fp);
    fclose(fp);
}

void delete_student(int client_fd)
{
    FILE *fp, *temp;
    STUDENT s;
    char name[21];
    int found = 0;
    char confirm[10];

    fp = fopen(FILE_NAME, "rb+");
    temp = fopen("temp.db", "wb");

    if (fp == NULL || temp == NULL)
    {
        send_text(client_fd, "파일에 오류가 생겼습니다!");
        if (fp != NULL)
            fclose(fp);
        if (temp != NULL)
            fclose(temp);
        return;
    }

    if (lock_record(fp, F_WRLCK) == -1)
    {
        fclose(fp);
        fclose(temp);
        return;
    }

    while (1)
    {
        send_input(client_fd, "\n삭제할 이름을 입력해주세요!\n");
        if (input_string_client(client_fd, name, sizeof(name)))
            break;
        send_text(client_fd, "잘못입력하셨습니다.\n");
        send_text(client_fd, "다시 입력해주세요.\n");
    }

    send_input(client_fd, "정말 삭제 하시겠습니까? (Y/N)");
    if (!input_string_client(client_fd, confirm, sizeof(confirm)))
        confirm[0] = 'N';

    if (confirm[0] != 'y' && confirm[0] != 'Y')
    {
        send_text(client_fd, "삭제가 취소되었습니다.\n");
        unlock_record(fp);
        fclose(fp);
        fclose(temp);
        remove("temp.db");
        return;
    }

    while (fread(&s, sizeof(STUDENT), 1, fp) == 1)
    {
        if (strcmp(s.name, name) == 0)
        {
            found = 1;
            continue;
        }
        fwrite(&s, sizeof(STUDENT), 1, temp);
    }

    fclose(temp);
    unlock_record(fp);
    fclose(fp);

    if (found)
    {
        remove(FILE_NAME);
        rename("temp.db", FILE_NAME);
        send_text(client_fd, "정상적으로 삭제되었습니다!\n");
    }
    else
    {
        remove("temp.db");
        send_text(client_fd, "존재하지 않습니다!\n");
    }
}