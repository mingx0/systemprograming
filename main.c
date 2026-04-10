#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "student.h"

//메뉴 기능 함수 선언부 (switch-case에서 사용하는 함수)
void input_student(void);
void print_all_students(void);
void search_student(void);
void update_student(void);
void delete_student(void);
void clear_input_buffer(void);

char *FILE_NAME;

//파일
int main(int argc, char *argv[])
{
    int menu;

    if (argc < 2)
    {
        printf("사용법 : %s 파일이름\n", argv[0]);
        return 1;
    }
    FILE_NAME = argv[1];

    while (1)  //0입력 전까지 무한 반복.
    {
        //메뉴
        printf("\n < 학생 파일 관리 프로그램 > \n");
        printf("1. 학생 정보 입력\n");
        printf("2. 전체 학생 정보 리스트로 보기\n");
        printf("3. 학생 정보 검색 (이름으로 검색)\n");
        printf("4. 학생 정보 갱신\n");
        printf("5. 학생 정보 삭제\n");
        printf("0. 프로그램 종료\n");
        printf("입력 : ");

        if (scanf("%d", &menu) != 1)
        {
            printf("잘못입력하셨습니다.\n");
            printf("다시 입력해주세요.\n");
            clear_input_buffer();
            continue;
        }

        clear_input_buffer();
        
        switch (menu)
        {   
            //학생 정보 입력
            case 1:
                input_student();
                break;
            //전체 학생 정보 리스트로 보기
            case 2:
                print_all_students();
                break;
            //학생 정보 검색
            case 3:
                search_student();
                break;
            //학생 정보 갱신
            case 4:
                update_student();
                break;
            //학생 정보 삭제
            case 5:
                delete_student();
                break;
            //종료
            case 0:
                printf("프로그램을 종료합니다!\n");
                return 0;
            default:
                printf("잘못 입력하셨습니다.\n");
                printf("다시 입력해주세요.\n");
        }
    }
    return 0;
}

//버퍼 지우기 (버퍼에 남아있는 '\n'을 없애기 위해서!)
void clear_input_buffer(void)
{
    int ch;
    while ((ch = getchar()) != '\n'&& ch != EOF);
}

//학생 정보 입력
void input_student(void)
{
    FILE*fp;
    STUDENT s;  //struct ??

    fp = fopen(FILE_NAME, "ab");
    if (fp == NULL)
    {
        printf("파일을 열 수 없습니다.\n");
        return;
    }

    printf("학생 정보를 입력해주세요!\n");

    printf("ID : ");
    scanf("%d",&s.id);
    getchar();

    printf("이름 : ");
    fgets(s.name, sizeof(s.name),stdin);
    s.name[strcspn(s.name,"\n")] = '\0';

    printf("주소 : ");
    fgets(s.address,sizeof(s.address),stdin);
    s.address[strcspn(s.address,"\n")] = '\0';

    printf("전화번호 : ");
    fgets(s.number, sizeof(s.number),stdin);
    s.number[strcspn(s.number,"\n")] = '\0';

    printf("생년월일 : ");
    fgets(s.birth, sizeof(s.birth),stdin);
    s.birth[strcspn(s.birth,"\n")] = '\0';

    fwrite(&s, sizeof(STUDENT),1,fp);
    fclose(fp);

    printf("저장 완료!\n");
}

//모든 학생 정보 리스트로 보기
void print_all_students(void)
{
    FILE *fp;
    STUDENT s;
    int count = 0;

    fp = fopen(FILE_NAME, "rb");
    if (fp == NULL)
    {
        printf("데이터가 존재하지 않습니다!\n");
        return;
    }

    printf("모든 학생 정보!\n");

    while (fread(&s, sizeof(STUDENT),1,fp)==1)
    {
        printf("\n[%d]\n",++count);
        printf("ID : %d\n",s.id);
        printf("이름 : %s\n", s.name);
        printf("주소 : %s\n",s.address);
        printf("전화번호 : %s\n",s.number);
        printf("생년월일 :%s\n",s.birth);
    }

    if (count == 0)
        printf("데이터 없음\n");
    fclose(fp);
}

//학생 정보 검색 (이름)
void search_student(void)
{
    FILE *fp;
    STUDENT s;
    char name[21];
    int found = 0;

    fp = fopen(FILE_NAME,"rb");
    if (fp == NULL)
    {
        printf("데이터 없음 \n");
        return;
    }
    printf("이름을 입력해주세요!\n");
    fgets(name, sizeof(name),stdin);
    name[strcspn(name,"\n")] = '\0';

    while (fread(&s,sizeof(STUDENT),1,fp)==1)
    {
        if (strcmp(s.name, name) == 0)
        {
            printf("검색하신 분의 정보입니다!\n");
            printf("ID : %d\n",s.id);
            printf("이름 : %s\n",s.name);
            printf("주소 : %s\n", s.address);
            printf("전화번호 : %s\n", s.number);
            printf("생년월일 : %s\n",s.birth);
            found = 1;
            break;
        }
    }
    if(!found)
        printf("검색하신 이름을 찾을 수 없습니다!\n");
    fclose(fp);
}

//학생 정보 갱신
void update_student(void)
{
    FILE *fp;
    STUDENT s;
    char name[21];
    int found=0;
    int menu;

    fp = fopen(FILE_NAME, "rb+");
    if (fp == NULL)
    {
        printf("데이터가 존재하지 않습니다!\n");
        return;
    }

    printf("\n수정 할 이름을 입력해주세요! \n");
    fgets(name, sizeof(name),stdin);
    name[strcspn(name,"\n")] = '\0';

    while (fread(&s, sizeof(STUDENT),1,fp)==1)
    {
        if (strcmp(s.name, name) == 0)
        {
            found = 1;

            while(1)
            {
                printf("\n < 학생 정보 > \n");
                printf("1. ID : %d\n", s.id);
                printf("2. 이름 : %s\n", s.name);
                printf("3. 주소 : %s\n", s.address);
                printf("4. 전화번호 : %s\n", s.number);
                printf("5. 생년월일 : %s\n", s.birth);
                printf("0. 수정 완료! (종료)\n");
                printf("수정하실 항목을 선택하세요 : ");

                scanf("%d",&menu);
                getchar();

                switch (menu)
                {
                    case 1:
                        printf("수정할 ID : ");
                        scanf("%d",&s.id);
                        getchar();
                        break;
                    case 2:
                        printf("수정할 이름 : ");
                        fgets(s.name, sizeof(s.name), stdin);
                        s.name[strcspn(s.name,"\n")] = '\0';
                        break;
                    case 3:
                        printf("수정할 주소 : ");
                        fgets(s.address, sizeof(s.address), stdin);
                        s.address[strcspn(s.address,"\n")] = '\0';
                        break;
                    case 4:
                        printf("수정할 전화번호 : ");
                        fgets(s.number, sizeof(s.number), stdin);
                        s.number[strcspn(s.number,"\n")] = '\0';
                        break;
                    case 5:
                        printf("수정할 생년월일 : ");
                        fgets(s.birth, sizeof(s.birth), stdin);
                        s.birth[strcspn(s.birth,"\n")] = '\0';
                        break;
                    case 0:
                        fseek(fp, -(long)sizeof(STUDENT),SEEK_CUR);
                        fwrite(&s, sizeof(STUDENT),1,fp);
                        printf("수정이 완료되었습니다!\n");
                        fclose(fp);
                        return;
                    default:
                        printf("잘못입력하셨습니다!\n");
                        printf("다시 작성해주세요!\n");
                }
            }
        }
    }
    if (!found)
    {
        printf("해당되는 이름이 존재하지 않습니다!\n");
        printf("다시 작성해주세요!");
    }
    fclose(fp);
}

//학생 정보 삭제
void delete_student(void)
{
    FILE *fp, *temp;
    STUDENT s;
    char name[21];
    int found = 0;
    char confirm;

    fp = fopen(FILE_NAME, "rb");
    temp = fopen("temp.db","wb");

    if(fp == NULL || temp == NULL)
    {
        printf("파일에 오류가 생겼습니다!");
        return;
    }

    printf("\n삭제할 이름을 입력해주세요!\n");
    fgets(name, sizeof(name),stdin);
    name[strcspn(name, "\n")] = '\0';

    //정말 삭제 할건지 다시 물어보기
    printf("정말 삭제 하시겠습니까? (Y/N)");
    scanf(" %c", &confirm);
    getchar();

    if (confirm != 'y' && confirm != 'Y')
    {
        printf("삭제가 취소되었습니다.\n");
        return;
    }

    while (fread(&s, sizeof(STUDENT),1,fp)==1)
    {
        if (strcmp(s.name, name) == 0)
        {
            found = 1;
            continue;
        }
        fwrite(&s, sizeof(STUDENT),1,temp);
    }
    fclose(fp);
    fclose(temp);

    remove(FILE_NAME);
    rename("temp.db",FILE_NAME);

    if (found)
        printf("정상적으로 삭제되었습니다!\n");
    else
        printf("존재하지 않습니다!\n");
}