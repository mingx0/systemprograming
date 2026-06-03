#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAXLINE 100

int readLine(int fd, char *str);

int main()
{
    int fd;
    char str[MAXLINE];

    unlink("myPipe");

    /* mknod를 이용해 이름 있는 파이프 생성 */
    if (mknod("myPipe", S_IFIFO | 0660, 0) == -1) {
        perror("mknod");
    }

    fd = open("myPipe", O_RDONLY);

    while (readLine(fd, str))
        printf("%s\n", str);

    close(fd);

    return 0;
}

int readLine(int fd, char *str)
{
    int n;

    do {
        n = read(fd, str, 1);
    } while (n > 0 && *str++ != '\0');

    return (n > 0);
}
