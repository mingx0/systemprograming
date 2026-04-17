#include <stdio.h>
#define MAXLINE 80
int main(int argc, char *argv[])
{
    FILE *fp;
    int line = 0;
    int number = 0;
    char buffer[MAXLINE];
    if (argc == 3) {
        number = 1;
        fp = fopen(argv[2], "r");
    }
    else if (argc == 2) {
        fp = fopen(argv[1], "r");
    }
    else {
        fprintf(stderr, "사용법: cat2 [-n] 파일이름\n");
        return 1;
    }
    if (fp == NULL) {
        fprintf(stderr, "파일 열기 오류\n");
        return 2;
    }
    while (fgets(buffer, MAXLINE, fp) != NULL) {
        if (number == 1) {
        line++;
        printf("%3d %s", line, buffer);            
        }
        else {
            printf("%s", buffer);
        }
    }
    fclose(fp);
    return 0;
}

