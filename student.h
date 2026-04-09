typedef struct {
    int id;
    char name[21]; //'\0' 때문에 최대 글자 수 + 1
    char address[51]; // '\0' 때문에 최대 글자 수 + 1
    char number[21];
    char birth[21];
}STUDENT;