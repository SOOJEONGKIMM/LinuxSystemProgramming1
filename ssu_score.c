#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "ssu_score.h"
#include "blank.h"
//다른 파일의 코드에서 사용 위해 extern전역변수 사용 
//점수테이블을 위한 배열을 구조체로 선언. 헤더에 QNLM 문제배열을 100으로 define함.
extern struct ssu_scoreTable score_table[QNUM];
//학번테이블을 위한 char타입배열 선언. 헤더에 SNLM 학생배열을 100으로 define함.
extern char id_table[SNUM][10];

//extern전역변수선언문을 한번은 반드시 extern없이 정의해주어야한다. 
struct ssu_scoreTable score_table[QNUM];
char id_table[SNUM][10];

//BUFLEN을 1024로 define하였음. 
char stuDir[BUFLEN];
char ansDir[BUFLEN];
char errorDir[BUFLEN];
char threadFiles[ARGNUM][FILELEN];
char iIDs[ARGNUM][FILELEN];//cIDs를 iIDs로 수정 

int eOption = false;
int tOption = false;
//int pOption = false;
int iOption = false;//c옵션을 i옵션으로 수정 
int mOption = false;

void ssu_score(int argc, char *argv[])
{
	char saved_path[BUFLEN];
	int i;

	//-h옵션: 사용법 출력
	for (i = 0; i < argc; i++) {
		if (!strcmp(argv[i], "-h")) {
			print_usage();
			return;
		}
	}

	//BUFLEN만큼 saved_path포인터(시작주소)부터 0으로 채운다 
	memset(saved_path, 0, BUFLEN);
	if (argc >= 3 && strcmp(argv[1], "-i") != 0) {
		strcpy(stuDir, argv[1]);
		strcpy(ansDir, argv[2]);
	}

	if (!check_option(argc, argv))
		exit(1);

	//i옵션인 경우 do_iOption()실행 
	if (!eOption && !tOption && iOption && !mOption) {
		do_iOption(iIDs);
		return;
	}
	//m옵션인 경우 do_mOption()실행 
	/*if (!eOption && !tOption && !iOption && mOption) {
		do_mOption("score_table.csv");
		printf("dsfsdf");
		return;
	}*/


	getcwd(saved_path, BUFLEN);

	if (chdir(stuDir) < 0) {
		fprintf(stderr, "%s doesn't exist\n", stuDir);
		return;
	}
	getcwd(stuDir, BUFLEN);

	chdir(saved_path);
	if (chdir(ansDir) < 0) {
		fprintf(stderr, "%s doesn't exist\n", ansDir);
		return;
	}
	getcwd(ansDir, BUFLEN);

	chdir(saved_path);

	set_scoreTable(ansDir);//점수테이블 세팅함수 호출
	set_idTable(stuDir);//학번테이블 세팅함수 호출 

	/*if (mOption)
		do_mOption("score_table.csv");
		printf("here");
		*/
		//채점 시작...
	printf("grading student's test papers..\n");
	score_students();

	if (iOption)
		do_iOption(iIDs);

	return;
}
//옵션 체크(옵션 아규먼트 개수, 옵션 아규먼트 배열) 
int check_option(int argc, char *argv[])
{
	int i, j;
	int a;

	//getopt()는 옵션을 처리하는 함수 
	//더 이상 실행할 옵션이 없어지면 -1을 리턴한다 
	while ((a = getopt(argc, argv, "e:thim")) != -1)
	{
		switch (a) {
		case 'e':
			//-e [DIRNAME] 옵션: DIRNAME/학번/문제번호_error.txt에 에러메시지가 출력 
			eOption = true;
			//optarg가 errorDir로 복사됨 
			strcpy(errorDir, optarg);

			//int access(const char *filename, int how)원형 
			//access()함수: filename파일에 대한 접근방법을 how인자로 확인 
			//F_OK는 파일의 exsistence test 플래그(how인자)
			////errorDir파일이 존재하는지 확인하는 access()
			if (access(errorDir, F_OK) < 0)//존재 안한다면
				//0755퍼미션은 octal value로 symbolic value로는 -rwxr-xr-x이다.
				//errorDir에 대한 접근권한허용 설정
				mkdir(errorDir, 0755);//0755퍼미션으로 생성
			else {//존재한다면
				rmdirs(errorDir);//존재파일을 지우고
				mkdir(errorDir, 0755);//새로 생성 
			}
			break;

			//-t[QNAMES]옵션: QNAME을 문제번호로 하는 문제는 컴파일시 -lpthread 옵션 추가 
		case 't':
			tOption = true;
			//The variable optind is the index of the next element to be processed in argv
			i = optind;			j = 0;

			//argc가 i보다 크고 argv배열 음수가 아니다 
			while (i < argc && argv[i][0] != '-') {
				//arg초과한 j는 예외처리 
				//기타-가변인자 최대 개수를 초과한 경우의 출력. 
				if (j >= ARGNUM)
					printf("Maximum Number of Argument Exceeded.  :: %s\n", argv[i]);
				//아니면 스레드파일배열에 문자열 복사 
				else
					strcpy(threadFiles[j], argv[i]);
				i++;
				j++;
			}
			break;
			/*case 'p'://***p옵션 삭제
				pOption = true;
				break;
				*/
				//기존c[STUDENTIDS]옵션: 채점결과 파일이 있는 경우 해당 학생들의 점수 출력 
				// i[STUDENTIDS]옵션: 채점결과 파일이 있는 경우 해당 학생들의 틀린 문제파일 출력
					//=> <STUDENTDIR> <TRUESETDIR>가 없어도 사용 가능 
		case 'i'://case'c'를 'i'로 수정해서 활용  
			iOption = true;
			i = optind;
			j = 0;
			//채점결과파일이 있는 경우
			while (i < argc && argv[i][0] != '-') {
				//초과된 매개변수 처리한다 
				if (j >= ARGNUM)
					printf("Maximum Number of Argument Exceeded.  :: %s\n", argv[i]);
				//iIDs배열로 문자열복사 
				else
					strcpy(iIDs[j], argv[i]);
				i++;
				j++;
			}
			break;
		case 'm'://채점 전에 옵션실행이기 때문에 ssu_score()에서 바로 인자를 받도록 함.
			//-m옵션: 채점 전에 원하는 문제의 점수를 수정 
			mOption = true;
			break;
		case '?':
			//알 수 없는 opt들 
			printf("Unknown option %c\n", optopt);
			return false;
		}
	}

	return true;
}


void do_iOption(char(*ids)[FILELEN])//학생들(학번)넣어서 틀린문제번호 출력해내는 옵션 
{
	FILE *fp;
	char tmp[BUFLEN];
	char qname[FILELEN][FILELEN];
	char inputId[ARGNUM][BUFLEN];
	//char realStu[BUFLEN];
	int i = 0, j = 0;//j는 모든문제를 담는 배열용.
	char *p, *q, *z, *saved;
	int idx;//idx는 틀린문제만 담는 배열용.
	struct ssu_scoreTable score_record[QNUM];
	int matched;
	char realStu[SNUM][SNUM] = { "20200001","20200002","20200003","20200004","20200005","20200006","20200007",
	"20200008","20200009","20200010" };

	//파일데이터가 없는 경우 예외처리 //학생들성적테이블 open 
	if ((fp = fopen("score.csv", "r")) == NULL) {
		fprintf(stderr, "file open error for score.csv\n");
		return;
	}
	fscanf(fp, "%s\n", tmp);//문제번호들(첫줄(\n전까지))데이터 받아냄. 
	//printf("FIRST %s ", tmp);//debug
	z = strtok(tmp, ",");//처음에 ", 1-1.txt, 1-2.txt..."할때 맨처음 ','토큰처리.
	strcpy(qname[i++], z);
	while ((z = strtok(NULL, ",")) != NULL) {//그 다음 ,토큰을 읽기위해서 
		//업데이트된tmp를 넣기 위해 strtok(tmp,",")가 아닌 strtok(NULL,",")를해준다.
		strcpy(qname[i++], z);//1-1.txt, 1-2.txt...순서대로 qname배열에 들어감. 
	}


	//주어진 학번 외의 학번 입력시 예외처리

	for (i = 0; i < ARGNUM; i++) {
		strcpy(inputId[i], ids[i]);
	}
	for (i = 0; i < ARGNUM; i++) {
		matched = false;
		for (j = 0; j < 10; j++) {

			if ((strcmp(realStu[j], ids[i])) == 0) {
				matched = true;
				continue;
			}

		}

		if (matched == false && ids[i] != NULL) {
			if (strstr(*realStu, ids[i]) == NULL)
				printf("%s doesn't exist \n", ids[i]);
		}
	}
	i = 0;
	while (fscanf(fp, "%s\n", tmp) != EOF) {
	
		idx = j = 0;//그 다음 사람으로 넘어가면 초기화 
		p = strtok(tmp, ",");//학번(','토큰으로구분) 하나씩 받음. 

		
		//입력값으로 받은 ids에 해당하는 채점표 확인
		//입력값이 아닌 ids는 확인할 필요 없음. 
		if (!is_exist(ids, tmp)) {
			//학번매칭확인후, 아니면 continue..그 다음 사람 확인 
			continue;
		}

	

		//is_exist로 입력값임을확인,매칭이 된 학번이면 출력
		printf("%s's wrong answer :\n", tmp);//20162969's wrong answer: 
	
		//그 다음부턴 점수들을 쭉 ','토큰으로 나누어 q에 계속 받아냄. 
		while ((q = strtok(NULL, ",")) != NULL) {
			//q를 saved에 할당한다. 
			
			saved = q;
			//printf("%s\n", q);
			if (!strcmp(saved, "0")) {//틀린 점수 0
				//printf("RRR%s ", saved);debug
				strcpy(score_record[idx].qname, qname[j]);
				idx++;

				
			}
			j++;//다음점수 넘어갈때마다 카운트. 총j는 총문제개수.
			//총idx는 틀린문제의개수 
		}
		//오름차순정렬 
		bubbleSort(score_record[idx].qname);

		for (int k = 0; k < idx; k++) {
			if (k == idx - 1) {
				printf("%s\n", score_record[k].qname);
				break;
			}
			printf("%s, ", score_record[k].qname);
		}


	}

	fclose(fp);
}
//오름차순 정렬 
void bubbleSort(char number[QNUM]) {
	int temp;
	int num = QNUM;
	for (int i = 0; i < num; i++) {
		for (int j = 0; j < num - i - 1; j++) {
			if (number[j] > number[j + 1]) {
				temp = number[j];
				number[j] = number[j + 1];
				number[j + 1] = temp;
			}
		}
	}
	return;
}
void do_mOption(char *filename)// - m옵션: 채점 전에 원하는 문제의 점수를 수정
{
	//FILE *fp;
	int fd;
	char modQ[QNUM];
	double NewSco[SNUM], curS[SNUM];;
	char qname[BUFLEN];//문제번호 확장자명을 널처리하므로 전역변수는 건드리지않음. 
	char exname[BUFLEN];
	char *newname;//.txt .c 확장자명 널처리하느라 임시로 담는 배열 
	int matched;
	int i = 0, j = 0;//j는 모든문제를 담는 배열용.
	int num = sizeof(score_table) / sizeof(score_table[0]);

	//파일데이터가 없는 경우 예외처리 //학생들성적테이블 open 
	if ((fd = creat(filename, 0666)) < 0) {
		fprintf(stderr, "open error for %s\n", filename);
		return;
	}
	newname = malloc(sizeof(char)*BUFLEN);
	while (1) {
		matched = false;
		printf("Input question's number to modify >> ");
		scanf("%s", modQ);//ex)"3-1" input
		if (!strcmp(modQ, "no")) {//"no"라고 입력시 배점수정입력으로 write_scoretable로 넘어감
			break;
		}

		for (i = 0; i < num; i++) {
			newname = 0;
			strcpy(qname, score_table[i].qname);

			//.txt나 .c 확장자명은 널 처리
			newname = strtok(qname, ".");
			if (newname != NULL)
				strcpy(qname, newname);

			if (strcmp(qname, modQ) == 0) {//score_table.csv에 존재하는 문제번호라면 
				printf("Current score: ");//해당 배점 출력
				printf("%.2f\n", score_table[i].score);//ex)0.2
				printf("New score: ");
				scanf("%lf", NewSco);//ex)0.1
				//printf("NewSco: %lf\n", *NewSco);//debug
				score_table[i].score = *NewSco;
				matched = true;
				break;
			}
			//printf("DEBUG %s ", qname);
		}

		//score_table.csv에 존재하지 않는 문제번호라면 예외처리 
		if (matched == false) {
			printf("This Question doesn't exist! Try again.\n");
		}

		if (!strcmp(modQ, "no")) {//"no"라고 입력시 배점수정입력으로 write_scoretable로 넘어감
			break;
		}
	}

	write_scoreTable(filename);
	//free(newname);
	//newname = NULL;
}
//학번이 여러개일 때 각각 읽었던 첫토큰과 일치하는지 확인.
int is_exist(char(*src)[FILELEN], char *target)
{
	int i = 0;

	while (1)
	{
		//가변인자개수 초과 예외처리 
		if (i >= ARGNUM)
			return false;
		//src비어있다면 -1리턴
		else if (!strcmp(src[i], ""))
			return false;
		else if (!strcmp(src[i++], target))
			return true;
	}
	return false;
}

void set_scoreTable(char *ansDir)
{
	char filename[FILELEN];

	//filename포인터배열에 csv파일이름 보내줌
	sprintf(filename, "%s", "score_table.csv");
	//F_OK는 파일의 exsistence test 플래그(how인자)
	if (access(filename, F_OK) == 0)
		//파일이 존재한다면 read
		read_scoreTable(filename);
	else {
		//파일이 존재하지않는다면 만들고 write
		make_scoreTable(ansDir);

		write_scoreTable(filename);

	}
	if (mOption) {
		do_mOption(filename);
	}

}

void read_scoreTable(char *path)//filename 인자로 받음 
{
	FILE *fp;
	char qname[FILELEN];
	char score[BUFLEN];
	int idx = 0;
	//파일을 r읽기 권한으로 open(예외처리)
	if ((fp = fopen(path, "r")) == NULL) {
		fprintf(stderr, "file open error for %s\n", path);
		return;
	}
	//파일 읽는다(EOF까지) 
	//qname과 score 데이터를 파일로부터 읽는다  
	//%[^,]는 개행문자 ' ', '\t','\n'등을 스킵하고 input한다. 
	while (fscanf(fp, "%[^,],%s\n", qname, score) != EOF) {
		strcpy(score_table[idx].qname, qname);
		//atof()는 str을 double타입으로 바꿔준다. 
		score_table[idx++].score = atof(score);
	}

	fclose(fp);
}


void make_scoreTable(char *ansDir)
{
	int type, num;
	double score, bscore, pscore;
	struct dirent *dirp;//디렉토리entry 포인터 구조체
	DIR *dp;
	char tmp[BUFLEN];
	int idx = 0;
	int i;

	//num=문제별로 점수배점/모든문제 동일배점 중 type 리턴
	num = get_create_type();

	if (num == 1)//전체 다 input한경우 
	{
		//빈칸문제  
		printf("Input value of blank question : ");
		scanf("%lf", &bscore);
		//프로그램문제 
		printf("Input value of program question : ");
		scanf("%lf", &pscore);
	}
	////ansDir스트림의 포인터를 리턴. 그 스트림은 디렉토리 첫번째 요소의 위치 가리킴 
	if ((dp = opendir(ansDir)) == NULL) {
		fprintf(stderr, "open dir error for %s\n", ansDir);
		return;
	}

	//***기존코드는 서브디렉토리로 들어가지만 그러지않아도되므로 코드 수정함***
	while ((dirp = readdir(dp)) != NULL)
	{
		//struct dirent *readdir(DIR *dirp);원형
		//readdir()함수는 dirent구조체의 포인터를 리턴. 
		//dirent구조체는 dirp가 가리키는 디렉토리스트림의 다음요소의 위치 가리킴. 
			//"."은 나 자신 디렉토리
		if (!strcmp(dirp->d_name, ".") && !strcmp(dirp->d_name, ".."))//***".."삭제해야하는지 냅둬야하는지 봐야함***
			continue;

		if ((type = get_file_type(dirp->d_name)) < 0)//추가된 코드 
			continue;

		strcpy(score_table[idx++].qname, dirp->d_name);
		//ansDir/d_name 파일경로가 아니라 지워줌
		//sprintf(tmp, "%s/%s", ansDir, dirp->d_name);
		/***
		//child dp는 필요없으므로 주석 처리.
				if((c_dp = opendir(tmp)) == NULL){
					fprintf(stderr, "open dir error for %s\n", tmp);
					return;
				}

				while((c_dirp = readdir(c_dp)) != NULL)
				{
					if(!strcmp(c_dirp->d_name, ".") || !strcmp(c_dirp->d_name, ".."))
						continue;

					if((type = get_file_type(c_dirp->d_name)) < 0)
						continue;

					strcpy(score_table[idx++].qname, c_dirp->d_name);
				}

				closedir(c_dp);
		*/
	}
	closedir(dp);
	sort_scoreTable(idx);

	for (i = 0; i < idx; i++)
	{
		//qname의 확장자가 .txt인지 .c인지 리턴 
		type = get_file_type(score_table[i].qname);

		//1.(빈칸과 프로그램문제 각각) 전체 다 같은 배점인 경우 
		if (num == 1)
		{
			if (type == TEXTFILE)
				score = bscore;
			else if (type == CFILE)
				score = pscore;
		}
		//2.문제별로 배점을 다르게 매긴 경우   
		else if (num == 2)
		{
			printf("Input of %s: ", score_table[i].qname);
			scanf("%lf", &score);//각 문제별 배점을 표준입력. 
		}

		score_table[i].score = score;
	}
}

//채점 후, 테이블에 문제별 점수 입력.-m옵션으로 수정할때도 호출.
void write_scoreTable(char *filename)
{
	int fd;
	char tmp[BUFLEN];
	int i;
	int num = sizeof(score_table) / sizeof(score_table[0]);

	if ((fd = creat(filename, 0666)) < 0) {
		fprintf(stderr, "creat error for %s\n", filename);
		return;
	}

	for (i = 0; i < num; i++)
	{
		if (score_table[i].score == 0)//0점 배점은 아무것도 입력X.
			break;
		//맞은 문제는 입력했던배점(점수)를 각각 자기자리에 출력.
		sprintf(tmp, "%s,%.2f\n", score_table[i].qname, score_table[i].score);
		write(fd, tmp, strlen(tmp));
	}

	close(fd);
}

//테이블 첫열에 학번들 순서대로 써주는 함수(sort_idTable()과 세트)
void set_idTable(char *stuDir)//학생답안 인자로(학번별폴더로 있기때문에)
{
	struct stat statbuf;
	struct dirent *dirp;
	DIR *dp;
	char tmp[BUFLEN];
	int num = 0;
	//학생답안디렉토리 오픈 
	if ((dp = opendir(stuDir)) == NULL) {
		fprintf(stderr, "opendir error for %s\n", stuDir);
		exit(1);
	}
	//폴더
	while ((dirp = readdir(dp)) != NULL) {//디렉토리entry 포인터를 dirp에게 리턴함 
		if (!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, ".."))
			continue;
		//tmp포인터배열에 부모폴더(STD_DIR)와 자식폴더(학번별폴더들) string(char elements)를 보냄
		sprintf(tmp, "%s/%s", stuDir, dirp->d_name);
		//tmp 정보를 읽음 
		stat(tmp, &statbuf);

		if (S_ISDIR(statbuf.st_mode))//디렉토리 파일인지 판별
			strcpy(id_table[num++], dirp->d_name);//자식폴더이름들(학번별폴더들)str을 id_table[]에 모두 복사
		else
			continue;
	}

	sort_idTable(num);//학번 정렬하는 함수 호출
}
//set_idTable()과 세트
void sort_idTable(int size)
{
	int i, j;
	char tmp[10];

	//for문 돌면서 학번 순서대로 테이블에 문자열 복사가 이루어짐 
	for (i = 0; i < size - 1; i++) {
		for (j = 0; j < size - 1 - i; j++) {
			if (strcmp(id_table[j], id_table[j + 1]) > 0) {
				strcpy(tmp, id_table[j]);
				strcpy(id_table[j], id_table[j + 1]);
				strcpy(id_table[j + 1], tmp);
			}
		}
	}
}

//점수테이블 정렬 
void sort_scoreTable(int size)//idx를 인자로 받음.
{
	int i, j;
	struct ssu_scoreTable tmp;//점수테이블구조체를 가져옴
	int num1_1, num1_2;
	int num2_1, num2_2;

	for (i = 0; i < size - 1; i++) {
		for (j = 0; j < size - 1 - i; j++) {

			get_qname_number(score_table[j].qname, &num1_1, &num1_2);
			get_qname_number(score_table[j + 1].qname, &num2_1, &num2_2);


			if ((num1_1 > num2_1) || ((num1_1 == num2_1) && (num1_2 > num2_2))) {

				memcpy(&tmp, &score_table[j], sizeof(score_table[0]));
				memcpy(&score_table[j], &score_table[j + 1], sizeof(score_table[0]));
				memcpy(&score_table[j + 1], &tmp, sizeof(score_table[0]));
			}
		}
	}
}
//문제번호 ex) 1-1,1-2 
void get_qname_number(char *qname, int *num1, int *num2)
{
	char *p;
	char dup[FILELEN];

	//char *strncpy(char *dest,const char*src,size_t n) src to dest까지 n char를 복사 
	//dup배열에 qname을 복사 
	strncpy(dup, qname, strlen(qname));
	//num1에 '1-2'을 '1'과 '2'로 나눔 
	*num1 = atoi(strtok(dup, "-."));

	//"-"로 문자열 나누기 _빈칸문제 
	p = strtok(NULL, "-.");
	if (p == NULL)
		*num2 = 0;
	else
		*num2 = atoi(p);
}


//num=(빈칸+프로그램문제점수)/모든문제점수 중 type리턴 
int get_create_type()
{
	int num;

	while (1)
	{
		printf("score_table.csv file doesn't exist in TREUDIR!\n");
		printf("1. input blank question and program question's score. ex) 0.5 1\n");
		printf("2. input all question's score. ex) Input value of 1-1: 0.1\n");
		printf("select type >> ");
		scanf("%d", &num);

		if (num != 1 && num != 2)//예외처리 
			printf("not correct number!\n");
		else
			break;
	}

	return num;
}
//double score_student(int fd, char *id)와 세트 
void score_students()
{
	double score = 0;
	int num;
	int fd;
	char tmp[BUFLEN];
	int size = sizeof(id_table) / sizeof(id_table[0]);

	//점수채점테이블 생성
	if ((fd = creat("score.csv", 0666)) < 0) {
		fprintf(stderr, "creat error for score.csv");
		return;
	}
	write_first_row(fd);//score.csv첫줄(열)에 문제번호이름 순서대로 write

	for (num = 0; num < size; num++)
	{
		//아무것도 적혀있지않다면 
		if (!strcmp(id_table[num], ""))
			break;
		//학번테이블부터 학번데이터들을 가져옴
		sprintf(tmp, "%s,", id_table[num]);
		write(fd, tmp, strlen(tmp));
		//학번별 점수채점데이터 가져와서 추가 
		score += score_student(fd, id_table[num]);
	}

	//평균계산해서 출력 
	printf("Total average : %.2f\n", score / num);

	close(fd);
}
//void score_students()와 세트 
double score_student(int fd, char *id)
{
	int type;
	double result;
	double score = 0;
	int i;
	char tmp[BUFLEN];
	int size = sizeof(score_table) / sizeof(score_table[0]);

	for (i = 0; i < size; i++)
	{
		if (score_table[i].score == 0)
			break;
		//학생폴더에서 학번과 문제번호 데이터들 가져옴 
		sprintf(tmp, "%s/%s/%s", stuDir, id, score_table[i].qname);
		//파일데이터가 있는지 
		if (access(tmp, F_OK) < 0)
			result = false;
		else
		{	//.txt인지 .c인지 
			if ((type = get_file_type(score_table[i].qname)) < 0)
				continue;
			//.txt
			if (type == TEXTFILE)
				result = score_blank(id, score_table[i].qname);
			//.c
			else if (type == CFILE)
				result = score_program(id, score_table[i].qname);
		}

		if (result == false)
			write(fd, "0,", 2);//오답 처리. 
		else {
			if (result == true) {//정답 처리.
				score += score_table[i].score;
				//문제별 점수배점 데이터 가져와서 점수입력 
				sprintf(tmp, "%.2f,", score_table[i].score);
			}
			else if (result < 0) {//감점 요소가 있다면 
				score = score + score_table[i].score + result;//+result로 감점처리 
				sprintf(tmp, "%.2f,", score_table[i].score + result);
			}
			write(fd, tmp, strlen(tmp));
		}
	}


	printf("%s is finished.. score : %.2f\n", id, score);


	sprintf(tmp, "%.2f\n", score);
	write(fd, tmp, strlen(tmp));

	return score;
}

//score.csv첫줄(열)에 문제번호이름 순서대로 write
void write_first_row(int fd)
{
	int i;
	char tmp[BUFLEN];//점수 
	int size = sizeof(score_table) / sizeof(score_table[0]);

	//","쓰기 
	write(fd, ",", 1);

	for (i = 0; i < size; i++) {
		if (score_table[i].score == 0)
			break;
		//문제번호 출력 
		sprintf(tmp, "%s,", score_table[i].qname);
		//문제별 점수 출력 
		write(fd, tmp, strlen(tmp));
	}
	//합산점수 출력 
	write(fd, "sum\n", 4);
}
//빈칸문제 답안비교 
char *get_answer(int fd, char *result)//fd_std, s_answer//fd_ans, a_answer 인자로 받음 
{
	char c;
	int idx = 0;

	memset(result, 0, BUFLEN);
	while (read(fd, &c, 1) > 0)
	{
		if (c == ':')
			break;

		result[idx++] = c;//답안//학생제출답_읽은 결과 저장 
	}
	//채점완료 
	if (result[strlen(result) - 1] == '\n')
		result[strlen(result) - 1] = '\0';

	return result;
}
//빈칸문제 채점 
int score_blank(char *id, char *filename)
{
	char tokens[TOKEN_CNT][MINLEN];
	node *std_root = NULL, *ans_root = NULL;//blank.c에 쓰임.
	int idx, start;
	char tmp[BUFLEN];
	char s_answer[BUFLEN], a_answer[BUFLEN];
	char qname[FILELEN];
	int fd_std, fd_ans;
	int result = true;
	int has_semicolon = false;

	memset(qname, 0, sizeof(qname));
	memcpy(qname, filename, strlen(filename) - strlen(strrchr(filename, '.')));

	//학생답안폴더로부터 학번과 답안파일데이터들 가져옴 
	sprintf(tmp, "%s/%s/%s", stuDir, id, filename);
	//학생fd 읽기전용으로 오픈 
	fd_std = open(tmp, O_RDONLY);
	//읽은 답안 정보 복사 
	strcpy(s_answer, get_answer(fd_std, s_answer));

	//답안에 내용이 없다면 
	if (!strcmp(s_answer, "")) {
		close(fd_std);
		return false;
	}
	//답안에 "()"가 있다면 학생답안 닫음  
	if (!check_brackets(s_answer)) {
		close(fd_std);
		return false;
	}
	//개행문자들 때문에 오답처리되지않도록 처리해줌. 
	strcpy(s_answer, ltrim(rtrim(s_answer)));

	//마지막에 세미콜론이 있다면 
	if (s_answer[strlen(s_answer) - 1] == ';') {
		has_semicolon = true;
		s_answer[strlen(s_answer) - 1] = '\0';//세미콜론 없애줌.
	}

	//만약 make_tokens에서 -1을 리턴하면 학생답안fd를 닫고 -1리턴.
	if (!make_tokens(s_answer, tokens)) {//blank.c에 있는 make_tokens()호출 
		close(fd_std);
		return false;
	}

	idx = 0;
	//blank.c에 있는 make_tree()호출.
	//리턴값은 그 트리의 루트 노드.
	std_root = make_tree(std_root, tokens, &idx, 0);
	//tmp에 정답답안데이터 가져옴. 
	sprintf(tmp, "%s/%s", ansDir, filename);
	//정답fd 읽기전용으로 오픈 
	fd_ans = open(tmp, O_RDONLY);

	while (1)
	{
		ans_root = NULL;
		result = true;

		for (idx = 0; idx < TOKEN_CNT; idx++)
			memset(tokens[idx], 0, sizeof(tokens[idx]));

		//답안내용 백업 
		strcpy(a_answer, get_answer(fd_ans, a_answer));

		//답안내용이없다면 
		if (!strcmp(a_answer, ""))
			break;
		//개행문자때문에 오답처리되지않도록 처리.
		strcpy(a_answer, ltrim(rtrim(a_answer)));
		//마지막에 세미콜론 있는지 확인 
		if (has_semicolon == false) {
			if (a_answer[strlen(a_answer) - 1] == ';')
				continue;
		}
		//마지막에 세미콜론 없는지 확인 
		else if (has_semicolon == true)
		{
			if (a_answer[strlen(a_answer) - 1] != ';')
				continue;
			else
				a_answer[strlen(a_answer) - 1] = '\0';//세미콜론 없앰
		}
		//blank.c의 make_tokens()호출. 
		if (!make_tokens(a_answer, tokens))
			continue;

		idx = 0;
		ans_root = make_tree(ans_root, tokens, &idx, 0);

		compare_tree(std_root, ans_root, &result);

		if (result == true) {
			close(fd_std);
			close(fd_ans);
			//std_root나 ans_root가 정보가 있다면 그가 가리키는 포인터들을 끊어낸다. 
			if (std_root != NULL)
				free_node(std_root);
			if (ans_root != NULL)
				free_node(ans_root);
			return true;

		}
	}

	close(fd_std);
	close(fd_ans);

	if (std_root != NULL)
		free_node(std_root);
	if (ans_root != NULL)
		free_node(ans_root);

	return false;
}
//프로그램 문제 채점 
double score_program(char *id, char *filename)
{
	double compile;
	int result;
	//컴파일함수로부터 컴파일정보 가져옴.
	compile = compile_program(id, filename);
	//컴파일 에러는 -1리턴.
	if (compile == ERROR || compile == false)
		return false;
	//표준출력채점 정보 가져옴.
	result = execute_program(id, filename);

	if (!result)
		return false;

	if (compile < 0)
		return compile;

	return true;
}
//스레드파일인지 아닌지 
int is_thread(char *qname)
{
	int i;
	int size = sizeof(threadFiles) / sizeof(threadFiles[0]);

	for (i = 0; i < size; i++) {
		//스레드파일과 문제번호가 같다면 1리턴. 
		if (!strcmp(threadFiles[i], qname))
			return true;
	}
	return false;
}
//프로그램문제를 위한 함수.
double compile_program(char *id, char *filename)
{
	int fd;
	char tmp_f[BUFLEN], tmp_e[BUFLEN];
	char command[BUFLEN];
	char qname[FILELEN];
	int isthread;
	off_t size;
	double result;

	memset(qname, 0, sizeof(qname));
	//파일이름을 문제번호로 가져옴. 
	memcpy(qname, filename, strlen(filename) - strlen(strrchr(filename, '.')));

	isthread = is_thread(qname);//스레드파일인지 아닌지. 
	//파일과 실행파일 모두 가져옴. 
	sprintf(tmp_f, "%s/%s", ansDir, filename);//***기존코드 ansDir, qname, filename
	sprintf(tmp_e, "%s.exe", ansDir);//***기존코드ansDir, qname, qname

	//-t[QNAMES]옵션:QNAME을 문제번호로 하는 문제는 컴파일 시 -lpthread옵션 추가. 
	if (tOption && isthread)
		sprintf(command, "gcc -o %s %s -lpthread", tmp_e, tmp_f);
	else
		sprintf(command, "gcc -o %s %s", tmp_e, tmp_f);

	//-e[DIRNAME]옵션:DIRNAME/학번/문제번호_error.txt에 에러 메시지가 출력. 
	sprintf(tmp_e, "%s/%s_error.txt", ansDir, qname);//***재수정함
	fd = creat(tmp_e, 0666);

	//void redirection(char *command, int new, int old)
	redirection(command, fd, STDERR);
	//fd의 사이즈 가져옴. 
	size = lseek(fd, 0, SEEK_END);
	close(fd);
	//에러메시지출력파일포인터와의 파일링크 해제. 
	unlink(tmp_e);

	if (size > 0)
		return false;
	//학생답안파일과 스레드파일 정보들을 가져옴.
	sprintf(tmp_f, "%s/%s/%s", stuDir, id, filename);
	sprintf(tmp_e, "%s/%s/%s.stdexe", stuDir, id, qname);

	//-t[QNAMES]옵션:QNAME을 문제번호로 하는 문제는 컴파일 시 -lpthread옵션 추가. 
	if (tOption && isthread)
		sprintf(command, "gcc -o %s %s -lpthread", tmp_e, tmp_f);
	else
		sprintf(command, "gcc -o %s %s", tmp_e, tmp_f);

	sprintf(tmp_f, "%s/%s/%s_error.txt", stuDir, id, qname);
	fd = creat(tmp_f, 0666);

	redirection(command, fd, STDERR);
	size = lseek(fd, 0, SEEK_END);
	close(fd);

	if (size > 0) {
		//-e[DIRNAME]옵션: DIRNAME/학번/문제번호_error..txt에 에러 메시지 출력
		if (eOption)
		{
			sprintf(tmp_e, "%s/%s", errorDir, id);
			if (access(tmp_e, F_OK) < 0)
				mkdir(tmp_e, 0755);

			sprintf(tmp_e, "%s/%s/%s_error.txt", errorDir, id, qname);
			rename(tmp_f, tmp_e);

			result = check_error_warning(tmp_e);
		}
		else {
			result = check_error_warning(tmp_f);
			unlink(tmp_f);
		}

		return result;
	}

	unlink(tmp_f);
	return true;
}

//error와 warning체크 
double check_error_warning(char *filename)
{
	FILE *fp;
	char tmp[BUFLEN];
	double warning = 0;

	if ((fp = fopen(filename, "r")) == NULL) {
		fprintf(stderr, "fopen error for %s\n", filename);
		return false;
	}

	while (fscanf(fp, "%s", tmp) > 0) {
		if (!strcmp(tmp, "error:"))
			return ERROR;
		else if (!strcmp(tmp, "warning:"))
			warning += WARNING;//헤더에 -0.1감점 define
	}

	return warning;
}

//프로그램표준출력실행=>정보를 score_program()에서 가져가 채점함.   
int execute_program(char *id, char *filename)
{
	char std_fname[BUFLEN], ans_fname[BUFLEN];//각각 학생답안과 답안파일이름들
	char tmp[BUFLEN];
	char qname[FILELEN];
	time_t start, end;
	pid_t pid;
	int fd;

	memset(qname, 0, sizeof(qname));
	memcpy(qname, filename, strlen(filename) - strlen(strrchr(filename, '.')));

	sprintf(ans_fname, "%s.stdout", ansDir);
	fd = creat(ans_fname, 0666);
	//각 문제에 대한 정답프로그램인 "./ANS/문제번호.c"의 실행파일인 "./ANS/문제번호.exe"를
	sprintf(tmp, "%s.exe", ansDir);
	//"./ANS/문제번호.stdout"으로 저장.
	redirection(tmp, fd, STDOUT);
	close(fd);

	//'./ANS/문제번호.stout"을 학번디렉토리를 순회하면서 "./STD/학번/문제번호.stdout"과 비교하며 결과만우선채점.
	sprintf(std_fname, "%s/%s/%s.stdout", stuDir, id, qname);
	fd = creat(std_fname, 0666);

	//학생이 작성해서 답안으로 제출한 프로그램의 실행파일인 "./ANS/학번/문제번호.stdexe"를 자동으로실행시키고
	//실행결과를 "./STD/학번/문제번호.stdout"자동으로 저장. 
	sprintf(tmp, "%s/%s/%s.stdexe &", stuDir, id, qname);

	start = time(NULL);//채점실행시간 측정시작.
	redirection(tmp, fd, STDOUT);

	sprintf(tmp, "%s.stdexe", qname);
	while ((pid = inBackground(tmp)) > 0) {
		end = time(NULL);//채점실행시간 측정끝.

		//학생들이 제출한 프로그램의 실행이 5초 이상의 시간이 걸리는 프로그램은 0점처리 
		//(#define으로 정의-변경이용이하게)
		if (difftime(end, start) > OVER) {
			kill(pid, SIGKILL);
			close(fd);
			return false;
		}
	}

	close(fd);
	//'./ANS/문제번호.stout"을 학번디렉토리를 순회하면서 "./STD/학번/문제번호.stdout"과 비교하며 결과만우선채점.
	return compare_resultfile(std_fname, ans_fname);
}

pid_t inBackground(char *name)
{
	pid_t pid;
	char command[64];
	char tmp[64];
	int fd;
	off_t size;

	memset(tmp, 0, sizeof(tmp));//initialize tmp
	fd = open("background.txt", O_RDWR | O_CREAT | O_TRUNC, 0666);//background.txt생성 

	//채점프로그램이 실행되는 동안 정답파일을 실행하는 프로그램이 백그라운드로 실행된다. 
	//ps|grep명령어는 백그라운드로 실행되는 프로그램을 쉽게 캐치하고자 사용. 
	sprintf(command, "ps | grep %s", name);
	redirection(command, fd, STDOUT);

	lseek(fd, 0, SEEK_SET);
	read(fd, tmp, sizeof(tmp));

	if (!strcmp(tmp, "")) {//tmp에 아무내용도없다면
		unlink("background.txt");
		close(fd);
		return 0;
	}

	pid = atoi(strtok(tmp, " "));
	close(fd);

	unlink("background.txt");
	return pid;
}

//'./ANS/문제번호.stout"을 학번디렉토리를 순회하면서 "./STD/학번/문제번호.stdout"과 비교하며 결과만우선채점.
int compare_resultfile(char *file1, char *file2)
{
	int fd1, fd2;
	char c1, c2;//버퍼 
	int len1, len2;//fd1,fd2 각각에 있는 내용만큼만 읽기 위해 따로 len1,len2에 저장한다.(남는 버퍼의 내용을 더 저장하는것을 방지)

	//읽기전용모드로 
	fd1 = open(file1, O_RDONLY);
	fd2 = open(file2, O_RDONLY);



	while (1)
	{
		//len1에 fd1의 1byte만큼 c1버퍼에 저장한다. 
		while ((len1 = read(fd1, &c1, 1)) > 0) {
			if (c1 == ' ')
				continue;
			else
				break;
		}
		//len2에 fd2의 1byte만큼 c2버퍼에 저장한다. 
		while ((len2 = read(fd2, &c2, 1)) > 0) {
			if (c2 == ' ')
				continue;
			else
				break;
		}

		if (len1 == 0 && len2 == 0)
			break;

		//학생들이 제출한 프로그램의 실행결과의 대소문자(와 공백)을 구분하지 않음.
		to_lower_case(&c1);
		to_lower_case(&c2);

		if (c1 != c2) {
			close(fd1);
			close(fd2);
			return false;
		}
	}
	close(fd1);
	close(fd2);
	return true;
}

//system("./test.out >> test.txt")처럼 사용자가 하는게 아니라
//시스템환경이 알아서 커멘드해줌. 
void redirection(char *command, int new, int old)
{
	int saved;

	saved = dup(old);
	dup2(new, old);

	//system():커멘드나 프로그램이름을 커멘드나 호스트환경이 실행할수있게 함. 
	system(command);

	dup2(saved, old);
	close(saved);
}

//파일타입 파악
int get_file_type(char *filename)
{
	//확장자명 파악하기 위한 변수
	//strrchr()는 'filename'다음에 '.'위치에 어떤 character가 오는지 알려준다.
	char *extension = strrchr(filename, '.');
	//텍스트파일이라면
	if (!strcmp(extension, ".txt"))
		return TEXTFILE;
	//c파일이라면 
	else if (!strcmp(extension, ".c"))
		return CFILE;
	else
		return -1;
}

//디렉토리 제거  
void rmdirs(const char *path)
{
	struct dirent *dirp;
	struct stat statbuf;
	DIR *dp;
	char tmp[50];

	if ((dp = opendir(path)) == NULL)
		return;

	while ((dirp = readdir(dp)) != NULL)
	{
		if (!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, ".."))
			continue;

		sprintf(tmp, "%s", dirp->d_name);

		if (lstat(tmp, &statbuf) == -1)
			continue;

		if (S_ISDIR(statbuf.st_mode))//디렉토리 
			rmdirs(tmp);
		else//일반파일 
			unlink(tmp);
	}

	closedir(dp);
	rmdir(path);
}

//대문자->소문자로 아스키코드변환 
//학생들이 제출한 프로그램의 실행결과의 대소문자(와 공백)을 구분하지 않음.
void to_lower_case(char *c)
{
	if (*c >= 'A' && *c <= 'Z')
		*c = *c + 32;
}

void print_usage()
{
	printf("Usage : ssu_score <STUDENTDIR> <TRUEDIR> [OPTION]\n");
	printf("Option : \n");
	printf(" -m                modify question's score\n");
	printf(" -e <DIRNAME>      print error on 'DIRNAME/ID/qname_error.txt' file \n");
	printf(" -t <QNAMES>       compile QNAME.C with -lpthread option\n");
	printf(" -i <IDS>          print ID's wrong questions\n");
	printf(" -h                print usage\n");
	//printf(" -p                print student's score and total average\n");
}
