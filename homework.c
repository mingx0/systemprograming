#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
    FILE *fp;
    int c;
    int lines = 0, words = 0, chars = 0;
    int in_word = 0;

    int opt_l = 0, opt_w = 0, opt_c = 0;

    if (argc == 3) {
        if (!strcmp(argv[1], "-l"))
            opt_l = 1;
        else if (!strcmp(argv[1], "-w"))
            opt_w = 1;
        else if (!strcmp(argv[1], "-c"))
            opt_c = 1;
        else {
            printf("옵션 오류\n");
            return 1;
        }
        fp = fopen(argv[2],"r");
    }
    else if (argc == 2) {
        fp = fopen(argv[1], "r");
    }
    else {
        fp = stdin;
    }
    if (fp == NULL) {
        printf("파일 열기 오류\n");
        return 1;
    }

    while ((c=getc(fp)) != EOF) {

        chars++;

        if (c == '\n')
            lines++;
        
        if (c == ' ' || c == '\n' || c == '\t') {
            in_word = 0;
        }
        else if (in_word == 0) {
            in_word = 1;
            words++;
        }
    }

    if (opt_l)
        printf("줄: %d\n", lines);
    else if (opt_w)
        printf("단어: %d\n", words);
    else if (opt_c)
        printf("문자: %d\n", chars);
    else
        printf("줄: %d 단어: %d 문자: %d\n", lines, words, chars);

    fclose(fp);
    return 0;

}