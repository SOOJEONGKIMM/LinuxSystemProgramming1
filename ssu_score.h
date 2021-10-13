//#ifndef을 사용하는 이유- 헤더의 중복참조를 막기 위해서. 
//코드의 흐름: MAIN_H_이 선언되어있는지 확인한다. 선언 안되었다면 선언한다. 선언되었다면 무시.

#ifndef MAIN_H_
#define MAIN_H_

#ifndef true
#define true 1
#endif
#ifndef false
#define false 0
#endif
#ifndef STDOUT
#define STDOUT 1
#endif
#ifndef STDERR
#define STDERR 2
#endif
#ifndef TEXTFILE
#define TEXTFILE 3
#endif
#ifndef CFILE
#define CFILE 4
#endif
#ifndef OVER
#define OVER 5
#endif
#ifndef WARNING
#define WARNING -0.1
#endif
#ifndef ERROR
#define ERROR 0
#endif

#define FILELEN 64
#define BUFLEN 1024
#define SNUM 100
#define QNUM 100
#define ARGNUM 5

struct ssu_scoreTable {//점수테이블 구조체엔 문제번호와 점수가 들어있음 
	char qname[FILELEN];
	double score;
};

void ssu_score(int argc, char *argv[]);
int check_option(int argc, char *argv[]);
void print_usage();

void score_students();
double score_student(int fd, char *id);
void write_first_row(int fd);

char *get_answer(int fd, char *result);
int score_blank(char *id, char *filename);
double score_program(char *id, char *filename);
double compile_program(char *id, char *filename);
int execute_program(char *id, char *filname);
pid_t inBackground(char *name);
double check_error_warning(char *filename);
int compare_resultfile(char *file1, char *file2);

void do_iOption(char(*ids)[FILELEN]);
void bubbleSort(char number[QNUM]);//오름차순정렬
void do_mOption(char *filename);//***새로추가한함수 
int is_exist(char(*src)[FILELEN], char *target);

int is_thread(char *qname);
void redirection(char *command, int newfd, int oldfd);
int get_file_type(char *filename);
void rmdirs(const char *path);
void to_lower_case(char *c);

void set_scoreTable(char *ansDir);
void read_scoreTable(char *path);
void make_scoreTable(char *ansDir);//채점 기준 점수표 만들기 
void write_scoreTable(char *filename);
void set_idTable(char *stuDir);
int get_create_type();

void sort_idTable(int size);
void sort_scoreTable(int size);
void get_qname_number(char *qname, int *num1, int *num2);

#endif
