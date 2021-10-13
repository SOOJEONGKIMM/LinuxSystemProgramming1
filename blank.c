#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include "blank.h"
//key와id값을 생성한 후 전역변수 data에 공유메모리 생성 
char datatype[DATATYPE_SIZE][MINLEN] = { "int", "char", "double", "float", "long"
			, "short", "ushort", "FILE", "DIR","pid"
			,"key_t", "ssize_t", "mode_t", "ino_t", "dev_t"
			, "nlink_t", "uid_t", "gid_t", "time_t", "blksize_t"
			, "blkcnt_t", "pid_t", "pthread_mutex_t", "pthread_cond_t", "pthread_t"
			, "void", "size_t", "unsigned", "sigset_t", "sigjmp_buf"
			, "rlim_t", "jmp_buf", "sig_atomic_t", "clock_t", "struct" };


operator_precedence operators[OPERATOR_CNT] = {
	{"(", 0}, {")", 0}//괄호가 최우선
	,{"->", 1}	//포인터 구조체 indexing
	,{"*", 4}	,{"/", 3}	,{"%", 2} //곱셈.나눗셈.나머지연산
	,{"+", 6}	,{"-", 5} //덧셈. 뺄셈.
	,{"<", 7}	,{"<=", 7}	,{">", 7}	,{">=", 7}//비교 연산자
	,{"==", 8}	,{"!=", 8}//참 거짓.
	,{"&", 9}//AND비트
	,{"^", 10}//제곱
	,{"|", 11}//OR비트
	,{"&&", 12}//AND비교연산자
	,{"||", 13}//OR비교연산자
	,{"=", 14}	,{"+=", 14}	,{"-=", 14}	,{"&=", 14}	,{"|=", 14}//할당 연산자
};
//트리비교함수
//root1은 학생답트리. root2는 정답답안트리 
void compare_tree(node *root1, node *root2, int *result)
{
	node *tmp;
	int cnt1, cnt2;
	//비교하는 트리 두개 중 하나라도 빈트리면 0 리턴 
	if (root1 == NULL || root2 == NULL) {
		*result = false;
		return;
	}
	//학생답 루트노드가 < > <= >=라면  
	if (!strcmp(root1->name, "<") || !strcmp(root1->name, ">") || !strcmp(root1->name, "<=") || !strcmp(root1->name, ">=")) {
		//학생답 루트노드와 답 루트노드가 같지 않다면
		//순서가 바뀐 경우일 것임. 이 경우에 오답처리를 해선 안되므로
		if (strcmp(root1->name, root2->name) != 0) {
			//만약 답 루트노드가 <라면
			if (!strncmp(root2->name, "<", 1))
				//>로 바꾼다.
				strncpy(root2->name, ">", 1);
			//만약 답 루트노드가 >라면
			else if (!strncmp(root2->name, ">", 1))
				//<로 바꾼다.
				strncpy(root2->name, "<", 1);
			//만약 답 루트노드가 <=라면
			else if (!strncmp(root2->name, "<=", 2))
				//>=로 바꾼다.
				strncpy(root2->name, ">=", 2);
			//만약 답 루트노드가 >=라면
			else if (!strncmp(root2->name, ">=", 2))
				//<=로 바꾼다. 
				strncpy(root2->name, "<=", 2);

			//원래 child_head를 child_head->next가 새로운 child_head가 되도록 업데이트.
			root2 = change_sibling(root2);
		}
	}
	//학생답안과 정답답안의 루트노드(최우선순위연산자)가 같지않으면 -1리턴
	if (strcmp(root1->name, root2->name) != 0) {
		*result = false;
		return;
	}
	//학생답루트노드(최우선순위연산자)나 정답답투트노드 중 하나만 피연산자가 존재한다면 -1리턴.
	if ((root1->child_head != NULL && root2->child_head == NULL)
		|| (root1->child_head == NULL && root2->child_head != NULL)) {
		*result = false;
		return;
	}
	//학생답루트노드(최우선순위연산자)가 피연산자가 존재한다면 
	else if (root1->child_head != NULL) {
		//그 연산자에 대한 피연산자들의 수를 세었을 때 학생답과 답안의 개수가 다르다면 -1리턴. 
		if (get_sibling_cnt(root1->child_head) != get_sibling_cnt(root2->child_head)) {
			*result = false;
			return;
		}
		//최상위연산자가 ==나 !=라면 
		if (!strcmp(root1->name, "==") || !strcmp(root1->name, "!="))
		{
			//child_head가 그 다음 연산자이므로 재귀로 다시 트리비교함수 
			compare_tree(root1->child_head, root2->child_head, result);

			if (*result == false)
			{
				*result = true;
				//원래 child_head를 child_head->next가 새로운 child_head가 되도록 업데이트.
				root2 = change_sibling(root2);
				//재귀로 다시 비교
				compare_tree(root1->child_head, root2->child_head, result);
			}
		}
		//학생답안 루트노드가 + * | & || && 중에 하나라면 
		//+*|&||&&는 피연산자 둘이 순서가 바뀌어도 상관없음.
		else if (!strcmp(root1->name, "+") || !strcmp(root1->name, "*")
			|| !strcmp(root1->name, "|") || !strcmp(root1->name, "&")
			|| !strcmp(root1->name, "||") || !strcmp(root1->name, "&&"))
		{
			//루트노드의 연산자에 대한 피연산자들의 개수가 일치하지않는다면 -1리턴. 
			if (get_sibling_cnt(root1->child_head) != get_sibling_cnt(root2->child_head)) {
				*result = false;
				return;
			}
			//답안 루트노드의 첫번째 피연산자 
			tmp = root2->child_head;
			//현재 첫번째 피연산자 아니라면 첫번째 피연산자로 
			while (tmp->prev != NULL)
				tmp = tmp->prev;
			
			while (tmp != NULL)
			{
				//재귀로 비교 
				compare_tree(root1->child_head, tmp, result);

				if (*result == true)
					break;
				else {
					
					if (tmp->next != NULL)
						*result = true;
					tmp = tmp->next;
				}
			}
		}
		else {//== != + * | & || && 위의 경우가 모두 아니라면 child_head로 재귀
			//재귀로 비교 
			compare_tree(root1->child_head, root2->child_head, result);
		}
	}

	//학생답이 다음노드가 있을때
	if (root1->next != NULL) {
		//학생답과 답안의 형제노드 개수가 다르면 -1리턴.
		if (get_sibling_cnt(root1) != get_sibling_cnt(root2)) {
			*result = false;
			return;
		}
		//형제노드 개수가 같으면 1리턴.
		if (*result == true)
		{
			tmp = get_operator(root1);//root1 부모노드를 가져옴 

			//+ * | & || && 중에 하나라면 
			if (!strcmp(tmp->name, "+") || !strcmp(tmp->name, "*")
				|| !strcmp(tmp->name, "|") || !strcmp(tmp->name, "&")
				|| !strcmp(tmp->name, "||") || !strcmp(tmp->name, "&&"))
			{//+*|&||&&는 피연산자 둘이 순서가 바뀌어도 상관없음.
				tmp = root2;//왼쪽 오른쪽 이름 거꾸로인지 체크.
				//만약 정답이 a&&b라면 학생답이 b&&a여도 정답처리를 해야하므로 
				while (tmp->prev != NULL)
					tmp = tmp->prev;

				while (tmp != NULL)
				{
					compare_tree(root1->next, tmp, result);

					if (*result == true)
						break;
					else {
						if (tmp->next != NULL)
							*result = true;
						tmp = tmp->next;
					}
				}
			}

			else//+ * | & || && 가 아니라면 재귀 
				compare_tree(root1->next, root2->next, result);
		}
	}
}
//빈칸문제 채점을 위한 함수. s_answer/a_answer, tokens를 인자로 받아옴. 
//MINLEN 64    TOKEN_CNT 50
int make_tokens(char *str, char tokens[TOKEN_CNT][MINLEN])
{
	char *start, *end;//start는 답안문자열포인터, end는 그 문자열 안의 연산자.
	char tmp[BUFLEN];
	char str2[BUFLEN];
	char *op = "(),;><=!|&^/+-*\"";//연산자 
	int row = 0;
	int i;
	int isPointer;
	int lcount, rcount;
	int p_str;

	clear_tokens(tokens);//토큰 초기화 

	start = str;//정답 문자열 포인터 

	//is_typestatement()의 리턴값:
	//0: 의미없음(그냥 자료형이 포함된 문자열 같은 것들)
	//1: 캐스팅과 같은데서 사용
	//2: 자료형이 시작 단어임 
	if (is_typeStatement(str) == 0) //잘못된 사용 
		return false;

	while (1)
	{
		//strpbrk(): start와 op비교해서 같은 문자열 리턴(일치하는게 없다면 널리턴)
		//start와 op가 일치하는 문자열이 하나라도 없으면 break 
		if ((end = strpbrk(start, op)) == NULL)
			break;


		if (start == end) {//내용이 없거나
			//strcmp(): 같으면0리턴, 다르면 1,-1리턴 //if(!strncmp()): 같으면 true리턴 
			//--a나 ++a이거나
			if (!strncmp(start, "--", 2) || !strncmp(start, "++", 2)) {
				//++++a나 ----a이거나 하면 -1리턴.
				if (!strncmp(start, "++++", 4) || !strncmp(start, "----", 4))
					return false;


				//++a에 해당.
				//is_character(): 숫자와 알파벳 
				//ltrim(): start+2가 공백문자라면 ++(start+2)
				//왜냐하면 문자열 간 공백은 구분하지않음(정답처리함.)
				if (is_character(*ltrim(start + 2))) {//공백문자처리.
					//첫두글자 토큰이 숫자나 알파벳이라면
					if (row > 0 && is_character(tokens[row - 1][strlen(tokens[row - 1]) - 1]))
						return false;
					//end=start+2에 포함된 op(연산자)
					//ex) ++a의 '++'연산자가 end.
					end = strpbrk(start + 2, op);
					if (end == NULL)//op(연산자)없다면 
						//문자열내용이 담겨있는만큼만(strlen)  end에 배열저장
						end = &str[strlen(str)];

					//start=str, end=&str[strlen(str)]
					while (start < end) {
						if (*(start - 1) == ' ' && is_character(tokens[row][strlen(tokens[row]) - 1]))
							return false;//만약 공백문자고 숫자나알파벳이라면 0리턴 
						else if (*start != ' ')
							//strncat(): 문자열연결 
							//tokens배열에 문자열을 추가.
							strncat(tokens[row], start, 1);
						start++;//한칸옮김
					}
				}
				//여기부터 a++코드
				//문자열이나 알파벳이라면 ex)'a'
				else if (row > 0 && is_character(tokens[row - 1][strlen(tokens[row - 1]) - 1])) {
					//문자열 앞에 ++이나 --가 포함되어있다면 -1리턴 
					if (strstr(tokens[row - 1], "++") != NULL || strstr(tokens[row - 1], "--") != NULL)
						return false;

					memset(tmp, 0, sizeof(tmp));
					strncpy(tmp, start, 2);//tmp안에 start두글자 넣음. '++'들어감
					strcat(tokens[row - 1], tmp);//tokens배열'a'에 tmp'++'이어서 추가 
					start += 2;//'++'이므로 두칸옮김 
					row--;//한 칸 이전으로 옮김('a'위치)
				}
				else {//아니면 그냥 이어서 붙임.
					memset(tmp, 0, sizeof(tmp));
					strncpy(tmp, start, 2);
					strcat(tokens[row], tmp);
					start += 2;
				}
			}
			//start에 각 해당 연산자들이 하나라도 있는지 확인
			else if (!strncmp(start, "==", 2) || !strncmp(start, "!=", 2) || !strncmp(start, "<=", 2)
				|| !strncmp(start, ">=", 2) || !strncmp(start, "||", 2) || !strncmp(start, "&&", 2)
				|| !strncmp(start, "&=", 2) || !strncmp(start, "^=", 2) || !strncmp(start, "!=", 2)
				|| !strncmp(start, "|=", 2) || !strncmp(start, "+=", 2) || !strncmp(start, "-=", 2)
				|| !strncmp(start, "*=", 2) || !strncmp(start, "/=", 2)) {

				//연산자가 존재한다면 tokens에 추가해줌.
				strncpy(tokens[row], start, 2);
				//start는 두 칸 옮김.
				start += 2;
			}

			// 예시로 13-5.txt의 tms_end->tms_cstime ? tms_start->tms_cstime
			else if (!strncmp(start, "->", 2))
			{
				//end에는 '->'다음 위치인 start의 연산자를 복사 
				end = strpbrk(start + 2, op);

				if (end == NULL)//연산자가 없다면
					end = &str[strlen(str)];

				while (start < end) {//end가 start보다 뒤 위치의 메모리주소를 가리킨다면 
					if (*start != ' ')//start에 내용이 있다면
						strncat(tokens[row - 1], start, 1);//tokens배열에 ->tms_cstime의 'tms_cstime'를 이어넣는다.
					start++;//그다음칸(메모리주소)
				}
				row--;
			}

			//&a (address)
			else if (*end == '&')
			{
				if (row == 0 || (strpbrk(tokens[row - 1], op) != NULL)) {
					end = strpbrk(start + 1, op);
					if (end == NULL)
						end = &str[strlen(str)];
					//'a'이어서 붙인다.
					strncat(tokens[row], start, 1);
					start++;

					while (start < end) {
						if (*(start - 1) == ' ' && tokens[row][strlen(tokens[row]) - 1] != '&')
							return false;
						else if (*start != ' ')
							strncat(tokens[row], start, 1);
						start++;
					}
				}
				//a & b (bit) 
				else {
					strncpy(tokens[row], start, 1);
					start += 1;
				}

			}
			//ex)*a 포인터
			else if (*end == '*')
			{
				isPointer = 0;

				if (row > 0)
				{
					//char** (pointer)
					for (i = 0; i < DATATYPE_SIZE; i++) {
						//tokens배열에 자료형이 포함되어있다면
						if (strstr(tokens[row - 1], datatype[i]) != NULL) {
							//*는 자료형포인터이다.
							strcat(tokens[row - 1], "*");
							start += 1;
							isPointer = 1;
							break;
						}
					}
					if (isPointer == 1)
						continue;
					if (*(start + 1) != 0)
						end = start + 1;

					//a * **b (multiply then pointer)
					if (row > 1 && !strcmp(tokens[row - 2], "*") && (all_star(tokens[row - 1]) == 1)) {
						strncat(tokens[row - 1], start, end - start);
						row--;
					}

					//a*b(multiply)
					else if (is_character(tokens[row - 1][strlen(tokens[row - 1]) - 1]) == 1) {
						strncat(tokens[row], start, end - start);
					}

					//,*b (pointer)
					else if (strpbrk(tokens[row - 1], op) != NULL) {
						strncat(tokens[row], start, end - start);

					}
					else
						strncat(tokens[row], start, end - start);

					start += (end - start);
				}

				else if (row == 0)
				{
					if ((end = strpbrk(start + 1, op)) == NULL) {
						strncat(tokens[row], start, 1);
						start += 1;
					}
					else {
						while (start < end) {
							if (*(start - 1) == ' ' && is_character(tokens[row][strlen(tokens[row]) - 1]))
								return false;
							else if (*start != ' ')
								strncat(tokens[row], start, 1);
							start++;
						}
						if (all_star(tokens[row]))
							row--;

					}
				}
			}
			//괄호
			else if (*end == '(')
			{
				lcount = 0;
				rcount = 0;
				//ex) (&a) , (*a) 파라미터
				if (row > 0 && (strcmp(tokens[row - 1], "&") == 0 || strcmp(tokens[row - 1], "*") == 0)) {
					//"("괄호 개수만큼 lcount해준다.
					while (*(end + lcount + 1) == '(')
						lcount++;
					start += lcount;
					
					//")"오른쪽괄호가 존재하는지 검색한다.
					end = strpbrk(start + 1, ")");

					if (end == NULL)//오른쪽괄호가 존재하지않는다면 -1리턴.
						return false;
					else {//")"괄호 있는만큼 rcount해준다.
						while (*(end + rcount + 1) == ')')
							rcount++;
						end += rcount;

						//왼쪽과 오른쪽 괄호의 개수가 일치해야한다.
						if (lcount != rcount)
							return false;
						//숫자문자가 아닌경우 
						if ((row > 1 && !is_character(tokens[row - 2][strlen(tokens[row - 2]) - 1])) || row == 1) {
							strncat(tokens[row - 1], start + 1, end - start - rcount - 1);
							row--;
							start = end + 1;
						}
						else {
							strncat(tokens[row], start, 1);
							start += 1;
						}
					}

				}
				//그냥 괄호 "(a)"
				else {
					//tokens에 start 1만큼 문자열연결
					strncat(tokens[row], start, 1);
					start += 1;
				}

			}
			//\특수문자 
			//\n:개행 \t:수평탬 \\:역슬래시 \'작은따옴표, \"큰따옴표
			else if (*end == '\"')
			{
				end = strpbrk(start + 1, "\"");

				if (end == NULL)//내용이없다면 -1리턴.
					return false;

				else {//토큰배열에 이어넣어주기
					strncat(tokens[row], start, end - start + 1);
					start = end + 1;
				}

			}

			else {
				//a++ ++ +b 있을 수 없음
				if (row > 0 && !strcmp(tokens[row - 1], "++"))
					return false;

				//a-- -- -b 있을 수 없음
				if (row > 0 && !strcmp(tokens[row - 1], "--"))
					return false;

				//백업
				strncat(tokens[row], start, 1);
				start += 1;

				//-a or a, -b
				if (!strcmp(tokens[row], "-") || !strcmp(tokens[row], "+") || !strcmp(tokens[row], "--") || !strcmp(tokens[row], "++")) {


					//-a or -a+b
					if (row == 0)
						row--;

					//a+b=-c
					else if (!is_character(tokens[row - 1][strlen(tokens[row - 1]) - 1])) {

						if (strstr(tokens[row - 1], "++") == NULL && strstr(tokens[row - 1], "--") == NULL)
							row--;
					}
				}
			}
		}
		else {//start!=end 
		//all_star():str이 *을 포함하고 있으면 1리턴.
			//*을 포함하고 있고 숫자나 알파벳이 아니다. 
			if (all_star(tokens[row - 1]) && row > 1 && !is_character(tokens[row - 2][strlen(tokens[row - 2]) - 1]))
				row--;

			if (all_star(tokens[row - 1]) && row == 1)//첫토큰이 *로만 이루어져있다.
				row--;

			for (i = 0; i < end - start; i++) {
				if (i > 0 && *(start + i) == '.') {
					strncat(tokens[row], start + i, 1);

					while (*(start + i + 1) == ' ' && i < end - start)//널문자 있으면 다음꺼 계속 읽음.
						i++;
				}
				else if (start[i] == ' ') {
					while (start[i] == ' ')
						i++;
					break;
				}
				else//백업
					strncat(tokens[row], start + i, 1);
			}

			if (start[0] == ' ') {//널문자 있으면 한칸 옮김 
				start += i;
				continue;
			}
			start += i;
		}
		//좌우측 공백문자 제거 업데이트후 token[row]로 복사
		strcpy(tokens[row], ltrim(rtrim(tokens[row])));

		if (row > 0 && is_character(tokens[row][strlen(tokens[row]) - 1])
			&& (is_typeStatement(tokens[row - 1]) == 2//자료형선언이라면 리턴2
				|| is_character(tokens[row - 1][strlen(tokens[row - 1]) - 1])
				|| tokens[row - 1][strlen(tokens[row - 1]) - 1] == '.')) {
			//struct나 unsigned의 경우 데이터타입이 맨처음에 안 나오므로  따로 예외처리 ex) unsigned int a
			if (row > 1 && strcmp(tokens[row - 2], "(") == 0)
			{  //(struct ...) , (unsigned ...) 이 아니면 -1리턴
				if (strcmp(tokens[row - 1], "struct") != 0 && strcmp(tokens[row - 1], "unsigned") != 0)
					return false;
			}   //a(...)
			else if (row == 1 && is_character(tokens[row][strlen(tokens[row]) - 1])) {
				//자료형선언이 아닌데 struct나 extern이면 -1리턴.
				if (strcmp(tokens[0], "extern") != 0 && strcmp(tokens[0], "unsigned") != 0 && is_typeStatement(tokens[0]) != 2)
					return false;
			}
			//자료형 선언인데 
			else if (row > 1 && is_typeStatement(tokens[row - 1]) == 2) {
				//unsigned extern의 경우 데이터타입이 맨처음에 안 나와서 예외처리해줘야하는데 이 경우도 아니라면 -1리턴.  ex) unsigned int a
				if (strcmp(tokens[row - 2], "unsigned") != 0 && strcmp(tokens[row - 2], "extern") != 0)
					return false;
			}

		}
		//컴파일 구문이라면
		if ((row == 0 && !strcmp(tokens[row], "gcc"))) {
			//토큰 초기화
			clear_tokens(tokens);
			strcpy(tokens[0], str);
			return 1;
		}

		row++;
	}//while문 끝.

	//*가 포함되어있고 그 전 문자가 연산자라면
	if (all_star(tokens[row - 1]) && row > 1 && !is_character(tokens[row - 2][strlen(tokens[row - 2]) - 1]))
		row--;
	if (all_star(tokens[row - 1]) && row == 1)
		row--;

	for (i = 0; i < strlen(start); i++)
	{
		if (start[i] == ' ')//널문자있으면 계속 다음칸으로 옮겨줌
		{
			while (start[i] == ' ')
				i++;
			if (start[0] == ' ') {
				start += i;
				i = 0;
			}
			else
				row++;

			i--;
		}
		else
		{
			//start에 .가 있다면 
			strncat(tokens[row], start + i, 1);
			if (start[i] == '.' && i < strlen(start)) {
				//공백 없을때까지 이동
				while (start[i + 1] == ' ' && i < strlen(start))
					i++;

			}
		}
		//개행문자들 없앤 후로 업데이트 
		strcpy(tokens[row], ltrim(rtrim(tokens[row])));

		//스레드 문제의 경우 
		if (!strcmp(tokens[row], "lpthread") && row > 0 && !strcmp(tokens[row - 1], "-")) {
			strcat(tokens[row - 1], tokens[row]);
			memset(tokens[row], 0, sizeof(tokens[row]));
			row--;
		}
		//숫자나 알파벳의 경우, 자료형선언의 경우, '.'을 포함한 경우 
		else if (row > 0 && is_character(tokens[row][strlen(tokens[row]) - 1])
			&& (is_typeStatement(tokens[row - 1]) == 2
				|| is_character(tokens[row - 1][strlen(tokens[row - 1]) - 1])
				|| tokens[row - 1][strlen(tokens[row - 1]) - 1] == '.')) {
			//"("로 시작하는 경우 
			if (row > 1 && strcmp(tokens[row - 2], "(") == 0)
			{
				//struct나 unsigned가 아니라면 -1리턴 
				if (strcmp(tokens[row - 1], "struct") != 0 && strcmp(tokens[row - 1], "unsigned") != 0)
					return false;
			}
			//첫 문자가 숫자나 알파벳의 경우 
			else if (row == 1 && is_character(tokens[row][strlen(tokens[row]) - 1])) {
				//unsigned extern의 경우 데이터타입이 맨처음에 안 나와서 예외처리해줘야하는데 이 경우도 아니라면 -1리턴.  ex) unsigned int a
				if (strcmp(tokens[0], "extern") != 0 && strcmp(tokens[0], "unsigned") != 0 && is_typeStatement(tokens[0]) != 2)
					return false;
			}
			//자료형 선언인데
			else if (row > 1 && is_typeStatement(tokens[row - 1]) == 2) {
				//unsigned extern이 아닌 경우 -1리턴. 
				if (strcmp(tokens[row - 2], "unsigned") != 0 && strcmp(tokens[row - 2], "extern") != 0)
					return false;
			}
		}
	}


	if (row > 0)
	{

		//예를 들어 #include<stdio.h>
		if (strcmp(tokens[0], "#include") == 0 || strcmp(tokens[0], "include") == 0 || strcmp(tokens[0], "struct") == 0) {
			clear_tokens(tokens);
			strcpy(tokens[0], remove_extraspace(str));
		}
	}
	//자료형선언이거나 extern선언이라면 
	if (is_typeStatement(tokens[0]) == 2 || strstr(tokens[0], "extern") != NULL) {
		for (i = 1; i < TOKEN_CNT; i++) {
			//비어있다면 break
			if (strcmp(tokens[i], "") == 0)
				break;
			//그 다음 토큰 확인 
			if (i != TOKEN_CNT - 1)
				strcat(tokens[0], " ");
			strcat(tokens[0], tokens[i]);
			memset(tokens[i], 0, sizeof(tokens[i]));
		}
	}

	//특수한 경우,예를 들어 캐스팅의 경우 가 아닌 경우 
	while ((p_str = find_typeSpecifier(tokens)) != -1) {
		if (!reset_tokens(p_str, tokens))
			return false;
	}

	//예를 들어 구조체의 경우 
	while ((p_str = find_typeSpecifier2(tokens)) != -1) {
		if (!reset_tokens(p_str, tokens))
			return false;
	}

	return true;
}//make_tokens() 끝.


//빈칸 문제 위한 함수 
//인자를 std_root/ans_root, tokens, &idx, 0로 받음. //parenthese는 괄호라는 뜻임.
//리턴값은 트리의 루트 노드.
node *make_tree(node *root, char(*tokens)[MINLEN], int *idx, int parentheses)
{
	node *cur = root;
	node *new;
	node *saved_operator;
	node *operator;
	int fstart;
	int i;

	while (1)
	{
		//토큰에 아무 내용도 없다면 예외처리
		if (strcmp(tokens[*idx], "") == 0)
			break;
		//만약 ")"이라면 root노드를 가져옴.
		if (!strcmp(tokens[*idx], ")"))
			return get_root(cur);
		//만약 ","라면 root노드를 가져옴. 
		else if (!strcmp(tokens[*idx], ","))
			return get_root(cur);
		//(예를 들어 (7+3)에서) 만약 "("라면 
		//"("가 나타날때마다 재귀탐색으로 연산자비교과정으로 들어가게 됨.
		else if (!strcmp(tokens[*idx], "("))
		{
			//"("전에 글자가 연산자가 아니고 ,가 아니라면 
			if (*idx > 0 && !is_operator(tokens[*idx - 1]) && strcmp(tokens[*idx - 1], ",") != 0) {
				fstart = true;

				while (1)
				{
					*idx += 1;
					//만약 ")"라면 예외처리.. "()"는 안됨.
					if (!strcmp(tokens[*idx], ")"))
						break;
					//새노드 생성
					new = make_tree(NULL, tokens, idx, parentheses + 1);

					if (new != NULL) {
						//"("다음의 "7"을 자식노드로 설정한다. 
						if (fstart == true) {
							cur->child_head = new;
							new->parent = cur;

							fstart = false;
						}
						else {
							//부모자식노드 없이 그냥 next prev로 연결 
							cur->next = new;
							new->prev = cur;
						}
						//설정완료후 new 노드를 cur로 
						cur = new;
					}
					//만약 ")"라면 예외처리..."()"는 안됨.
					if (!strcmp(tokens[*idx], ")"))
						break;
				}
			}
			//"("전에 글자가 연산자거나 ","라면 
			else {
				*idx += 1;
				//새노드 생성
				new = make_tree(NULL, tokens, idx, parentheses + 1);
				
				//cur가 빈노드라면 cur을 new로 대체.
				if (cur == NULL)
					cur = new;

				//cur과 new노드의 글자가 같다면 
				else if (!strcmp(new->name, cur->name)) {
					//만약 | || & && 라면 
					if (!strcmp(new->name, "|") || !strcmp(new->name, "||")
						|| !strcmp(new->name, "&") || !strcmp(new->name, "&&"))
					{
						//	A|B|C|D라면 D를 리턴. 
						cur = get_last_child(cur);
						//	A|B|C|D에 A가 존재한다면 A를 new로
						if (new->child_head != NULL) {
							new = new->child_head;
							// | 연산자와 A를 연결한다. 
							new->parent->child_head = NULL;
							new->parent = NULL;
							//ex) A|B|C|D 의 경우 A B C D는 서로 나란히 prev next 노드 순서로 연결. 
							new->prev = cur;
							cur->next = new;
						}
					}
					//만약 + *라면 
					else if (!strcmp(new->name, "+") || !strcmp(new->name, "*"))
					{
						i = 0;

						while (1)
						{
							//내용이 없다면 예외처리 
							if (!strcmp(tokens[*idx + i], ""))
								break;
							//첫 글자가 연산자거나 ")"가 먼저 등장하면 예외처리 
							if (is_operator(tokens[*idx + i]) && strcmp(tokens[*idx + i], ")") != 0)
								break;

							i++;
						}

						//+ - 보다 더 우선인 연산자라면 
						//즉, ( ) -> * / % 라면 
						if (get_precedence(tokens[*idx + i]) < get_precedence(new->name))
						{
							//ex) A+B+C*D의 C를 리턴.
							cur = get_last_child(cur);
							//C와 D를 나란히 연결 
							cur->next = new;
							new->prev = cur;
							cur = new;
						}
						else
							//+ - 가 더 우선인 연산자라면 
						{
							//ex) A+B < C+D 에서 +이 먼저 연산된다. 

							//A+B<C+D의 A를 리턴한다.
							cur = get_last_child(cur);
							
							//A+B<C+D에 child_head C가 존재하므로 
							if (new->child_head != NULL) {
								new = new->child_head;

								//C와 D를 연결한다.
								new->parent->child_head = NULL;
								new->parent = NULL;
								new->prev = cur;
								cur->next = new;
							}
						}
					}
					//+ - 가 아니라면 
					else {//일자로 순서대로 노드 나열 
						cur = get_last_child(cur);
						cur->next = new;
						new->prev = cur;
						cur = new;
					}
				}

				else//cur과 new노드의 글자가 다르다면
				{
					//우선순위가 같지만 글자가 다른것이므로 일자로 나란히 연결.
					cur = get_last_child(cur);

					cur->next = new;
					new->prev = cur;

					cur = new;
				}
			}
		}
		//연산자라면 (괄호없음)
		else if (is_operator(tokens[*idx]))
		{
		//|| && | & + *라면 
			if (!strcmp(tokens[*idx], "||") || !strcmp(tokens[*idx], "&&")
				|| !strcmp(tokens[*idx], "|") || !strcmp(tokens[*idx], "&")
				|| !strcmp(tokens[*idx], "+") || !strcmp(tokens[*idx], "*"))
			{
				//연산자고 || && | & + * 중에 하나라면 
				if (is_operator(cur->name) == true && !strcmp(cur->name, tokens[*idx]))
					operator = cur;

				else
				{
					//새노드생성
					new = create_node(tokens[*idx], parentheses);
					//가장 높은 우선순위의 연산자 리턴. 
					operator = get_most_high_precedence_node(cur, new);

					//가장 최상의 노드라서 부모나 prev가 없으면 
					if (operator->parent == NULL && operator->prev == NULL) {
						//operator가 new보다 우선순위가 높다면 
						if (get_precedence(operator->name) < get_precedence(new->name)) {
							//operator을 new의 새로운 부모로 삽입.(DFS 구조 따라가는 것)
							//new가 cur로 됨. 
							cur = insert_node(operator, new);
						}
						//반대로 new가 operator보다 우선순위가 높다면 
						else if (get_precedence(operator->name) > get_precedence(new->name))
						{
							//operator로 연산하는 피연산자가 있다면 
							if (operator->child_head != NULL) {
								//operator와 name 순서 바꿈 
								operator = get_last_child(operator);
								//operator을 new의 새로운 부모로 삽입.(DFS 구조 따라가는 것)
								//new가 cur로 됨. 
								cur = insert_node(operator, new);
							}
						}
						else
						{
							operator = cur;

							while (1)
							{
								//operator가 연산자고 토큰과 일치할때까지 while돌아감.
								if (is_operator(operator->name) == true && !strcmp(operator->name, tokens[*idx]))
									break;
								//operator가 prev노드가 있다면 가장 앞의 prev노드로 바꿈.
								if (operator->prev != NULL)
									operator = operator->prev;
								else
									break;
							}

							//operator와 토큰이 일치한다면 
							if (strcmp(operator->name, tokens[*idx]) != 0)
								//operator의 부모로 이동.
								operator = operator->parent;
							//operator가 있다면
							if (operator != NULL) {
								//operator가 토큰과 일치하면
								if (!strcmp(operator->name, tokens[*idx]))
									//cur을 operator로
									cur = operator;
							}
						}
					}

					else
						//operator을 new의 새로운 부모로 삽입.(DFS 구조 따라가는 것)
						//new가 cur로 됨. 
						cur = insert_node(operator, new);
				}

			}
			else//|| && | & + *가 아니라면 
			{
				//노드생성
				new = create_node(tokens[*idx], parentheses);

				if (cur == NULL)
					cur = new;

				else
				{
					//가장최우선순위의 연산자 리턴.
					operator = get_most_high_precedence_node(cur, new);
					//new의 괄호가 operator의 괄호보다 우선이라면 
					if (operator->parentheses > new->parentheses)
						cur = insert_node(operator, new);
					//operator가 부모노드나 prev노드가 없다면 
					else if (operator->parent == NULL && operator->prev == NULL) {
						//new가 operator보다 우선순위가 높다면 
						if (get_precedence(operator->name) > get_precedence(new->name))
						{
							//operator연산하는 피연산자가 존재한다면 
							if (operator->child_head != NULL) {
								//operator와 new순서 바꿈
								operator = get_last_child(operator);
								//operator을 new의 새로운 부모로 삽입.(DFS 구조 따라가는 것)
								//new가 cur로 됨. 
								cur = insert_node(operator, new);
							}
						}
						//operator가 new보다 우선순위가 높다면
						else
							//operator을 new의 새로운 부모로 삽입.(DFS 구조 따라가는 것)
							//new가 cur로 됨. 
							cur = insert_node(operator, new);
					}
					//operator가 부모노드나 prev노드가 있다면
					else
						//operator을 new의 새로운 부모로 삽입.(DFS 구조 따라가는 것)
						//new가 cur로 됨. 
						cur = insert_node(operator, new);
				}
			}
		}
		else
		{
			//새노드생성
			new = create_node(tokens[*idx], parentheses);

			if (cur == NULL)
				cur = new;
			//cur연산하는 피연산자가 있다면
			else if (cur->child_head == NULL) {
				//cur을 new의 부모로 
				cur->child_head = new;
				new->parent = cur;

				cur = new;
			}
			//cur연산하는 피연산자가 없다면
			else {
				//일자로 순서대로 나열
				cur = get_last_child(cur);

				cur->next = new;
				new->prev = cur;

				cur = new;
			}
		}

		*idx += 1;
	}
	//cur의 root노드를 가져옴.
	return get_root(cur);
}
//원래 child_head를 child_head->next가 새로운 child_head가 되도록 업데이트.
node *change_sibling(node *parent)
{
	node *tmp;

	tmp = parent->child_head;

	parent->child_head = parent->child_head->next;
	parent->child_head->parent = parent;
	parent->child_head->prev = NULL;

	parent->child_head->next = tmp;
	parent->child_head->next->prev = parent->child_head;
	parent->child_head->next->next = NULL;
	parent->child_head->next->parent = NULL;

	return parent;
}
//노드 생성. 
node *create_node(char *name, int parentheses)
{
	node *new;
	//사이즈 받아오기 
	new = (node *)malloc(sizeof(node));
	//new의 문자열길이 
	new->name = (char *)malloc(sizeof(char) * (strlen(name) + 1));
	strcpy(new->name, name);

	new->parentheses = parentheses;
	new->parent = NULL;
	new->child_head = NULL;
	new->prev = NULL;
	new->next = NULL;

	return new;
}

//연산자끼리의 우선순위를 따져줌. 
int get_precedence(char *op)
{
	int i;

	for (i = 2; i < OPERATOR_CNT; i++) {
		//연산자 우선순위를 따져서 더 우선인 연산자라면 숫자가 더 작음.
		if (!strcmp(operators[i].operator, op))
			return operators[i].precedence;
	}
	return false;
}

//연산자인지 아닌지 판단 
int is_operator(char *op)
{
	int i;

	for (i = 0; i < OPERATOR_CNT; i++)
	{
		if (operators[i].operator == NULL)
			break;
		if (!strcmp(operators[i].operator, op)) {
			return true;
		}
	}

	return false;
}

void print(node *cur)
{
	if (cur->child_head != NULL) {
		print(cur->child_head);
		printf("\n");
	}

	if (cur->next != NULL) {
		print(cur->next);
		printf("\t");
	}
	printf("%s", cur->name);
}
//cur의 부모노드(prev)를 가져옴.
node *get_operator(node *cur)
{
	if (cur == NULL)
		return cur;
	//그 전 노드가 있다면
	if (cur->prev != NULL)
		while (cur->prev != NULL)
			cur = cur->prev;

	return cur->parent;
}

//root 노드 가져옴.
node *get_root(node *cur)
{
	if (cur == NULL)
		return cur;

	//그 전 노드가 존재한다면 가져옴.(root 노드 가져옴)
	while (cur->prev != NULL)
		cur = cur->prev;

	//부모노드가 있다면 부모노드를 가져옴.
	if (cur->parent != NULL)
		//재귀탐색으로 계속해서 마지막 부모노드를 가져옴(root)
		cur = get_root(cur->parent);

	return cur;
}
//더 높은 우선순위의 노드를 리턴
node *get_high_precedence_node(node *cur, node *new)
{
	if (is_operator(cur->name))
		//cur이 new보다 높은 우선순위라면 cur리턴.
		if (get_precedence(cur->name) < get_precedence(new->name))
			return cur;
	//prev노드가 있다면 가장 앞의 prev리턴 
	if (cur->prev != NULL) {
		while (cur->prev != NULL) {
			cur = cur->prev;

			return get_high_precedence_node(cur, new);
		}

		//부모노드가 있다면 
		if (cur->parent != NULL)
			return get_high_precedence_node(cur->parent, new);
	}
	//부모노드가 없다면 그대로 리턴.
	if (cur->parent == NULL)
		return cur;
}
//가장 높은 연산자 우선순위 노드 리턴
node *get_most_high_precedence_node(node *cur, node *new)
{
	node *operator = get_high_precedence_node(cur, new);
	node *saved_operator = operator;

	while (1)
	{
		//부모가없다면 예외처리 
		if (saved_operator->parent == NULL)
			break;
		//prev노드가 있다면 
		if (saved_operator->prev != NULL)
			operator = get_high_precedence_node(saved_operator->prev, new);
		//부모노드가 있다면 
		else if (saved_operator->parent != NULL)
			operator = get_high_precedence_node(saved_operator->parent, new);

		saved_operator = operator;
	}

	return saved_operator;
}
//prev<-old 이렇게 되어있던 것을 prev<-new^
//										  |
//										old 이렇게 바꿈. 
node *insert_node(node *old, node *new)
{
	if (old->prev != NULL) {
		new->prev = old->prev;
		old->prev->next = new;
		old->prev = NULL;
	}

	new->child_head = old;
	old->parent = new;

	return new;
}

//ex) " A||B||C||D "라면 D를 리턴. 
node *get_last_child(node *cur)
{
	//첫 자식 노드로 간다.
	if (cur->child_head != NULL)
		cur = cur->child_head;
	//거기서 부터 가장 오른쪽 노드로 간다.
	while (cur->next != NULL)
		cur = cur->next;

	return cur;
}
//형제 노드(같은부모를가짐)(일렬로 배치되어있음)(같은 연산자에 대한 피연산자들) 카운트.
int get_sibling_cnt(node *cur)
{
	int i = 0;
	//그 전 노드가 있다면
	while (cur->prev != NULL)
		//가장 앞의 노드로 일단 이동(카운트 하기위해) 
		cur = cur->prev;
	//그 다음 노드가 있다면
	while (cur->next != NULL) {
		//그 다음 노드로 이동을 반복하며 카운트
		cur = cur->next;
		i++;
	}

	return i;
}

void free_node(node *cur)//std_root나 ans_root를 인자로 받음
{
	if (cur->child_head != NULL)
		free_node(cur->child_head);

	if (cur->next != NULL)
		free_node(cur->next);

	if (cur != NULL) {//ans_root나 std_root에 정보가 있다면 
		cur->prev = NULL;//모든 포인터들을 끊어낸다.
		cur->next = NULL;
		cur->parent = NULL;
		cur->child_head = NULL;
		//void free(void *ptr)
		//원래 할당하고 있던 cur을 비할당한다.(할당하지않는상태로)
		free(cur);
	}
}

//숫자나 알파벳이면 1리턴.
int is_character(char c)
{
	return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

//자료형 선언 판별해주는 함수
//변수 선언이라면 return 2
//단순 자료형포함된 문자열이라면 return 0
//그외 아무것도 아니면 return 1
int is_typeStatement(char *str)
{
	char *start;
	char str2[BUFLEN] = { 0 };
	char tmp[BUFLEN] = { 0 };
	char tmp2[BUFLEN] = { 0 };
	int i;

	start = str;
	strncpy(str2, str, strlen(str));//str2에 str복사 
	remove_space(str2);//공백문자는 제거 

	//공백문자면 다음문자로 (공백없을때까지)
	while (start[0] == ' ')
		start += 1;

	//char *strstr(const char *haystack, const char *needle); 원형
	//haystack에서 needle이 포함되어있는지
	if (strstr(str2, "gcc") != NULL)//str2에 gcc가 포함되어있다면 
	{
		//tmp2에 start복사 
		strncpy(tmp2, start, strlen("gcc"));
		if (strcmp(tmp2, "gcc") != 0)
			return 0;
		else
			return 2;
	}

	for (i = 0; i < DATATYPE_SIZE; i++)
	{
		//datatype[i](특정자료형)이 포함되어있다면
		if (strstr(str2, datatype[i]) != NULL)
		{
			strncpy(tmp, str2, strlen(datatype[i]));
			strncpy(tmp2, start, strlen(datatype[i]));

			//tmp와 datatype[i]가 일치한다면 
			if (strcmp(tmp, datatype[i]) == 0)
				if (strcmp(tmp, tmp2) != 0)//tmp와tmp2가 일치하지않는다(일치하면0리턴)
					return 0;  //단순 자료형이 포함된 문자열인 경우 
				else
					return 2;	//변수 선언인 경우(자료형으로 시작되는 문자열) 
		}

	}
	return 1;//캐스팅 같은데서 사용된다 

}
//예를 들어 캐스팅해줄때 ('int')a를 (int)a로 
int find_typeSpecifier(char tokens[TOKEN_CNT][MINLEN])
{
	int i, j;

	for (i = 0; i < TOKEN_CNT; i++)
	{
		for (j = 0; j < DATATYPE_SIZE; j++)
		{
			if (strstr(tokens[i], datatype[j]) != NULL && i > 0)
			{
				if (!strcmp(tokens[i - 1], "(") && !strcmp(tokens[i + 1], ")")
					&& (tokens[i + 2][0] == '&' || tokens[i + 2][0] == '*'
						|| tokens[i + 2][0] == ')' || tokens[i + 2][0] == '('
						|| tokens[i + 2][0] == '-' || tokens[i + 2][0] == '+'
						|| is_character(tokens[i + 2][0])))
					return i;
			}
		}
	}
	return -1;
}
//예를 들어 구조체의 경우 
int find_typeSpecifier2(char tokens[TOKEN_CNT][MINLEN])
{
	int i, j;


	for (i = 0; i < TOKEN_CNT; i++)
	{
		for (j = 0; j < DATATYPE_SIZE; j++)
		{
			if (!strcmp(tokens[i], "struct") && (i + 1) <= TOKEN_CNT && is_character(tokens[i + 1][strlen(tokens[i + 1]) - 1]))
				return i;
		}
	}
	return -1;
}
//str이 '*'를 포함하고 있다면 1리턴, 아니면 0리턴 
int all_star(char *str)
{
	int i;
	int length = strlen(str);

	if (length == 0)//예외처리
		return 0;

	for (i = 0; i < length; i++)
		if (str[i] != '*')
			return 0;//str이 '*'없다면 리턴0
	return 1;//str이 '*'있다면 리턴1

}
//str이 전부 문자나 숫자로 이루어져있다면 1리턴, 아니면 0리턴 
int all_character(char *str)
{
	int i;

	for (i = 0; i < strlen(str); i++)
		if (is_character(str[i]))
			return 1;
	return 0;

}

int reset_tokens(int start, char tokens[TOKEN_CNT][MINLEN])
{
	int i;
	int j = start - 1;
	int lcount = 0, rcount = 0;
	int sub_lcount = 0, sub_rcount = 0;

	if (start > -1) {
		//구조체의 경우 
		if (!strcmp(tokens[start], "struct")) {
			strcat(tokens[start], " ");
			strcat(tokens[start], tokens[start + 1]);

			for (i = start + 1; i < TOKEN_CNT - 1; i++) {
				strcpy(tokens[i], tokens[i + 1]);
				memset(tokens[i + 1], 0, sizeof(tokens[0]));
			}
		}

		// unsigned의 경우 데이터타입이 맨처음에 안 나오므로  ex) unsigned int a
		else if (!strcmp(tokens[start], "unsigned") && strcmp(tokens[start + 1], ")") != 0) {
			strcat(tokens[start], " ");
			strcat(tokens[start], tokens[start + 1]);
			strcat(tokens[start], tokens[start + 2]);

			for (i = start + 1; i < TOKEN_CNT - 1; i++) {
				strcpy(tokens[i], tokens[i + 1]);
				memset(tokens[i + 1], 0, sizeof(tokens[0]));
			}
		}

		j = start + 1;
		//")"로 마무리되고 토큰 다 읽은 경우 break
		while (!strcmp(tokens[j], ")")) {
			rcount++;
			if (j == TOKEN_CNT)
				break;
			j++;
		}

		j = start - 1;
		//"("이고 더 이상 읽을게 없는 경우 
		while (!strcmp(tokens[j], "(")) {
			lcount++;
			if (j == 0)
				break;
			j--;
		}
		//숫자나 알파벳의 경우나 내용이 없는 경우 
		if ((j != 0 && is_character(tokens[j][strlen(tokens[j]) - 1])) || j == 0)
			lcount = rcount;

		//왼쪽 오른쪽 괄호 카운트가 다른경우 -1리턴. 
		if (lcount != rcount)
			return false;

		//sizeof 하는 경우 
		if ((start - lcount) > 0 && !strcmp(tokens[start - lcount - 1], "sizeof")) {
			return true;
		}
		//unsigned나 struct의 경우 데이터타입이 맨처음에 안 나오므로  ex) unsigned int a
		else if ((!strcmp(tokens[start], "unsigned") || !strcmp(tokens[start], "struct")) && strcmp(tokens[start + 1], ")")) {
			strcat(tokens[start - lcount], tokens[start]);
			strcat(tokens[start - lcount], tokens[start + 1]);
			strcpy(tokens[start - lcount + 1], tokens[start + rcount]);

			for (int i = start - lcount + 1; i < TOKEN_CNT - lcount - rcount; i++) {
				strcpy(tokens[i], tokens[i + lcount + rcount]);
				memset(tokens[i + lcount + rcount], 0, sizeof(tokens[0]));
			}


		}
		else {//괄호의 개수를 세준다.
			if (tokens[start + 2][0] == '(') {
				j = start + 2;
				while (!strcmp(tokens[j], "(")) {
					sub_lcount++;
					j++;
				}
				if (!strcmp(tokens[j + 1], ")")) {
					j = j + 1;
					while (!strcmp(tokens[j], ")")) {
						sub_rcount++;
						j++;
					}
				}
				else//"("가 처음에 없었다면 -1리턴.
					return false;
				//(괄호개수가 맞지않는다면 -1리턴. 
				if (sub_lcount != sub_rcount)
					return false;

				strcpy(tokens[start + 2], tokens[start + 2 + sub_lcount]);
				for (int i = start + 3; i < TOKEN_CNT; i++)
					memset(tokens[i], 0, sizeof(tokens[0]));

			}
			strcat(tokens[start - lcount], tokens[start]);
			strcat(tokens[start - lcount], tokens[start + 1]);
			strcat(tokens[start - lcount], tokens[start + rcount + 1]);

			for (int i = start - lcount + 1; i < TOKEN_CNT - lcount - rcount - 1; i++) {
				strcpy(tokens[i], tokens[i + lcount + rcount + 1]);
				memset(tokens[i + lcount + rcount + 1], 0, sizeof(tokens[0]));

			}
		}
	}
	return true;
}//reset_tokens 끝.

//initialize tokens 토큰초기화 
void clear_tokens(char tokens[TOKEN_CNT][MINLEN])
{
	int i;

	for (i = 0; i < TOKEN_CNT; i++)
		memset(tokens[i], 0, sizeof(tokens[i]));
}
//오른쪽 개행문자들을 오답이 아니게 처리해줌
char *rtrim(char *_str)
{
	char tmp[BUFLEN];
	char *end;

	strcpy(tmp, _str);
	end = tmp + strlen(tmp) - 1;
	//isspace(): 인수로 받은 문자가 공백문자(공백,개행..)문자인지 판별해줌 
	//우측 공백문자 거른다 
	while (end != _str && isspace(*end))//오른쪽 공백문자 거른다.
		--end;

	*(end + 1) = '\0';
	_str = tmp;
	return _str;
}
//왼쪽 공백문자들을 오답이 아니게 처리해줌
char *ltrim(char *_str)
{
	char *start = _str;
	//start가 개행문자가 아니라면 ++start
	//좌측 공백문자 거른다 
	while (*start != '\0' && isspace(*start))//왼쪽 공백문자 거른다.
		++start;
	//업데이트데이터로 백업 
	_str = start;
	return _str;
}

//공백문자 없애주는 함수 
char* remove_extraspace(char *str)
{
	int i;
	char *str2 = (char*)malloc(sizeof(char) * BUFLEN);
	char *start, *end;
	char temp[BUFLEN] = "";
	int position;

	if (strstr(str, "include<") != NULL) {
		start = str;
		end = strpbrk(str, "<");
		position = end - start;

		strncat(temp, str, position);
		strncat(temp, " ", 1);
		strncat(temp, str + position, strlen(str) - position + 1);

		str = temp;
	}

	for (i = 0; i < strlen(str); i++)
	{
		if (str[i] == ' ')
		{
			if (i == 0 && str[0] == ' ')
				while (str[i + 1] == ' ')
					i++;
			else {
				if (i > 0 && str[i - 1] != ' ')
					str2[strlen(str2)] = str[i];
				while (str[i + 1] == ' ')
					i++;
			}
		}
		else
			str2[strlen(str2)] = str[i];
	}

	return str2;
}



void remove_space(char *str)
{
	char* i = str;
	char* j = str;

	while (*j != 0)
	{
		*i = *j++;
		if (*i != ' ')
			i++;
	}
	*i = 0;
}

int check_brackets(char *str)
{
	char *start = str;
	int lcount = 0, rcount = 0;

	while (1) {
		//strpbrk(a,b); a와 b가 일치하는 문자열 있다면
		//"()"가ㅏ 포함 안되어있다면
		if ((start = strpbrk(start, "()")) != NULL) {
			if (*(start) == '(')
				lcount++;//left괄호
			else
				rcount++;//right괄호 

			start += 1;
		}
		else
			break;
	}
	//괄호개수가 일치하지않는다면 리턴0
	if (lcount != rcount)
		return 0;
	else//괄호 개수가 일치하면 리턴1
		return 1;
}

int get_token_cnt(char tokens[TOKEN_CNT][MINLEN])
{
	int i;

	for (i = 0; i < TOKEN_CNT; i++)
		if (!strcmp(tokens[i], ""))
			break;

	return i;
}
