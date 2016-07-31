#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define __STDC_WANT_LIB_EXT1__ 1
#define MAX_LABELS 255
#define ZERO6 "00"          /* octals (6bits) */
#define ZERO3 "0"           /* octals (3bits) */
#define ZERO18 "000000"     /* octals (18bits)*/
#define NOOP "777777"

int START_ADDRESS = 020000; /* octal number */

char line[82] = "\0";
char firstCol[2] = "\0";
char label[9] = "\0";
char opcode[9] = "\0";
char argument[13] = "\0";
char comment[51] = "\0";
char ou[255] = "\0";
char program[5000][255] = { "\0" };


char symbTable[MAX_LABELS][10] = { "\0" }; /* max 255 labels, the address of that LABEL is in memTable at index findLabel(<label>)     */
int memTable[MAX_LABELS] = { 0 };

char equTable[MAX_LABELS][7] = { "\0" };   /* max 255 EQUated constants : string fromat : <EQUNAME>|<EQUVAL> where EQUVAL is max 18bit */
                                           /*                                                                      EQUNAME is max 9    */
                                           /* STORED IN OCTAL                                                                          */
int noLabel = 1;   /* boolean value */
int noArg = 1;     /* boolean value */
int noComment = 1; /* boolean value */

int numLabels = 0;
int numEQU = 0;
int PC = 0;

typedef struct {
	char t;
	int v;
	short n;
} t_arg;

enum opcodes {
	SA=050, /* 8 instr */
	SB=060, /* 8 instr */
	SX=070, /* 3 instr 070-071-072 */
	PS=000, /* long instruction 30bit*/
	RJ=001,
	JP=002,
	ZR=030, /* long instruction 30bit*/
	NZ=031, /* long instruction 30bit*/
	PL=032, /* long instruction 30bit*/
	NG=033, /* long instruction 30bit*/
	EQ=004,
	NE=005,
	GE=006,
	LT=007,
	NO=046,
	IX=036, /* integer arith : 2 instr */
	FX=030 /* short instruction 15bit */
};

enum pseudoop {
	EQU = 80,
	BSS = 81,
	CON = 82
};

char* removeSpace(char t[]) {
	char* tt1 = strdup(t);
	char* tt = strtok(tt1, " ");
	return tt;
}

char* removeZeros(char t[]) {
	while (t[0] == '0') t++;
	char res[8] = "\0";
	strcat(res, "0");
	strcat(res, t);
	return res;
}

int isEmpty(char t[]) {
	if (t[0] == '\0') {
		return 1;
	}
	else {
		if (memcmp(t, " ", 1) == 0) return 1;
	}
	return 0;
}

int isReg(char t) {
	if (t == 'A') {
		return 65;
	}
	else if (t == 'B') {
		return 66;
	}
	else if (t == 'X') {
		return 88;
	}
	return 0;
}

void resetReading() {
	noLabel = 1; /* boolean value */
	noArg = 1; /* boolean value */
	noComment = 1; /* boolean value */

	memset(label, 0, sizeof label);
	memset(opcode, 0, sizeof opcode);
	memset(argument, 0, sizeof argument);
	memset(comment, 0, sizeof comment);
	memset(ou, 0, sizeof ou);
	memset(line, 0, sizeof line);
}

int findLabel(char s1[]) {
	
	size_t l_s1, l_s2 = 0;
	int i = 0;
	char s2[10] = "\0";

	l_s1 = strlen(s1);

	for (i = 0; i < numLabels; i++) {
		memcpy(s2, symbTable[i], 9);
		l_s2 = strlen(s2);
		if (l_s1 == l_s2) {
			if (memcmp(s1, s2, l_s1) == 0) return i;
		}
	}

	return -1;
}

char* getEQU(char s1[]) {

	size_t l_s1 = 0;
	int i = 0;
	int pos = 0;
	char s2[30] = "\0";
	static char res[30] = "\0";
	
	l_s1 = strlen(s1);

	for (i = 0; i < numEQU; i++) {
		memcpy(s2, equTable[i], 29);
		char* t_s2 = strtok(s2, "|");
		if (memcmp(s1,s2,strlen(s2)) == 0) {
			t_s2 = strtok(NULL, "|");
			strcpy(res, t_s2);
			return res;
		}
	}

	return NULL;
}

int memLabel(char l[]) {
	return memTable[findLabel(l)];
}

void reverse_string(char *str)
{
	/* skip null */
	if (str == 0)
	{
		return;
	}

	/* skip empty string */
	if (*str == 0)
	{
		return;
	}

	/* get range */
	char *start = str;
	char *end = start + strlen(str) - 1; /* -1 for \0 */
	char temp;

	/* reverse */
	while (end > start)
	{
		/* swap */
		temp = *start;
		*start = *end;
		*end = temp;

		/* move */
		++start;
		--end;
	}
}

char* tobinstr(int value, int bitsCount)
{
	int i;
	static char output[19] = "\0";

	memset(output, 0, 19);

	for (i = bitsCount - 1; i >= 0; --i, value >>= 1)
	{
		output[i] = (value & 1) + '0';
	}
	//reverse_string(strstr(output,output));
	strstr(output, output);
	return output;
}

char* tooctstring(int value, int bitsCount) {
	char fmt[255]="\0";
	char bitsString[255]="\0";
	static char res[255]="\0";

	strcpy(fmt, "%0");
	itoa((bitsCount/3),bitsString,10);
	strcat(fmt, bitsString);
	strcat(fmt, "o");
	
	sprintf(res, fmt, value);

	return res;
}

#define tobinstr tooctstring /* laziness mode on! */

t_arg* check_address(char a[]) { /* checks the address is specified as LABEL or number */
	char part[13] = "\0";
	char t_a[13] = "\0";
	char t_a1[13] = "\0";
	static t_arg res[2];

	memcpy(t_a, a, sizeof t_a);
	memcpy(t_a1, a, sizeof t_a1);

	if (isalpha(t_a[0])) {
		res[0].n = 1;
		res[0].t = 'L';
		res[0].v = memLabel(t_a);
	}
	else {
		res[0].n = 1;
		res[0].t = 'L';
		res[0].v = atoi(t_a);
	}
	return res;
}

t_arg* check_arg_plus(char a[]) {
	char part[13] = "\0";
	char t_a[13] = "\0";
	char t_a1[13] = "\0";
	char tt[60] = "\0";
	static t_arg res[2];
	char* end;
	
	memcpy(t_a, a, sizeof t_a); /* used in strtok. modified by the function! */
	memcpy(t_a1, a, sizeof t_a1); /* original copy of the argument */

	strcpy(part,strtok(t_a, "+"));
	if (strcmp(part,t_a1) == 0) return NULL;
	if (memcmp(part,t_a1,strlen(t_a1)) == 0) { /* no addition in the arg */
		res[0].n = 1;
		if ((strlen(t_a) == 2) && (isReg(t_a[0])>0)) { /* it's a registry */
			res[0].t = t_a[0];
			res[0].v = t_a[1] - '0';
		}
		else if (isdigit(t_a[0])) { /* it's only a numeric konstant */
			res[0].t = 'K';
			res[0].v = atoi(t_a);
		}
		else if (isalpha(t_a[0])) { /* it's an EQUated konstant */
			res[0].t = 'K';
			res[0].v = strtoll(getEQU(t_a),&end,0);
 		}
	}
	else { /* there two parts */
		res[0].n = 2;
		if ((strlen(part) == 2) && (isReg(part[0])>0)) { /* it's a registry */
			res[0].t = part[0];
			res[0].v = part[1] - '0';
		}
		else if (isdigit(t_a[0])) { /* it's only a numeric konstant */
			res[0].t = 'K';
			res[0].v = atoi(part);
		}
		else if (isalpha(t_a[0])) { /* it's an EQUated konstant */
			res[0].t = 'K';
			res[0].v = strtol(getEQU(part),&end,0);
 		}

		strcpy(part,strtok(NULL, "+")); /* the other element (max 2)*/
		if ((strlen(part) == 2) && (isReg(part[0])>0)) { /* it's a registry */
			res[1].t = part[0];
			res[1].v = part[1] - '0';
		}
		else if (isdigit(t_a[0])) { /* it's only a numeric konstant */
			res[1].t = 'K';
			res[1].v = atoi(part);
		}
		else if (isalpha(t_a[0])) { /* it's an EQUated konstant */
			res[1].t = 'K';
			res[1].v = strtol(getEQU(part),&end,0);
		}
	}
	
	return res;
}

t_arg* check_arg_min(char a[]) {
	char part[13] = "\0";
	char t_a[13] = "\0";
	char t_a1[13] = "\0";
	char tt[60] = "\0";
	static t_arg res[2];
	char* end;

	memcpy(t_a, a, sizeof t_a);
	memcpy(t_a1, a, sizeof t_a1);

	strcpy(part, strtok(t_a, "-"));
	if (memcmp(part, t_a1, strlen(t_a1)) == 0) { /* no addition in the arg */
		res[0].n = 1;
		if ((strlen(t_a) == 2) && (isReg(t_a[0])>0)) { /* it's a registry */
			res[0].t = t_a[0];
			res[0].v = t_a[1] - '0';
		}
		else if (isdigit(t_a[0])) { /* it's only a numeric konstant */
			res[0].t = 'K';
			res[0].v = atoi(t_a);
		}
		else if (isalpha(t_a[0])) { /* it's an EQUated konstant */
			res[0].t = 'K';
			res[0].v = strtoll(getEQU(t_a), &end, 0);
		}
	}
	else { /* there two parts */
		res[0].n = 2;
		if ((strlen(part) == 2) && (isReg(part[0])>0)) { /* it's a registry */
			res[0].t = part[0];
			res[0].v = part[1] - '0';
		}
		else if (isdigit(t_a[0])) { /* it's only a numeric konstant */
			res[0].t = 'K';
			res[0].v = atoi(part);
		}
		else if (isalpha(t_a[0])) { /* it's an EQUated konstant */
			res[0].t = 'K';
			res[0].v = strtol(getEQU(part), &end, 0);
		}

		strcpy(part, strtok(NULL, "-")); /* the other element (max 2)*/
		if ((strlen(part) == 2) && (isReg(part[0])>0)) { /* it's a registry */
			res[1].t = part[0];
			res[1].v = part[1] - '0';
		}
		else if (isdigit(t_a[0])) { /* it's only a numeric konstant */
			res[1].t = 'K';
			res[1].v = atoi(part);
		}
		else if (isalpha(t_a[0])) { /* it's an EQUated konstant */
			res[1].t = 'K';
			res[1].v = strtol(getEQU(part), &end, 0);
		}
	}

	return res;
}

t_arg* check_arg_comm(char a[]) {
	char part[13] = "\0";
	char t_a[13] = "\0";
	char t_a1[13] = "\0";
	static t_arg res[2];

	memcpy(t_a, a, sizeof t_a);
	memcpy(t_a1, a, sizeof t_a1);

	strcpy(part, strtok(t_a, ","));
	if (memcmp(part, t_a1, strlen(t_a1)) == 0) { /* no commer in the arg */
		res[0].n = 1;
		if ((strlen(t_a) == 2) && isalpha(t_a[0])) { /* it's a registry */
			res[0].t = t_a[0];
			res[0].v = t_a[1] - '0';
		}
		else { /* it's only a konstant */
			res[0].t = 'K';
			res[0].v = atoi(t_a);
		}
	}
	else { /* there two parts */
		res[0].n = 2;
		if ((strlen(part) == 2) && isalpha(part[0])) { /* it's a registry */
			res[0].t = part[0];
			res[0].v = part[1] - '0';
		}
		else { /* it's only a konstant */
			res[0].t = 'K';
			res[0].v = atoi(part);
		}
		strcpy(part, strtok(NULL, "-")); /* the other element (max 2)*/
		if ((strlen(part) == 2) && isalpha(part[0])) { /* it's a registry */
			res[1].t = part[0];
			res[1].v = part[1] - '0';
		}
		else { /* it's only a konstant */
			res[1].t = 'K';
			res[1].v = atoi(part);
		}
	}

	return res;
}

char* convOpcode(char opc[], char arg[]) {
	char t_opc[3] = "\0";
	char t_idx[2] = "\0";
	char lp[13] = "\0";
	char rp[13] = "\0";
	
	char a_res[31] = "\0";
	char b_res[2][31] = { "\0" };
	char x_res[31] = "\0";
	char k_res[31] = "\0";
	char k_temp[31] = "\0";
	char t_res[31] = "\0";

	static char l_res[31]; /* long result 30 bits*/
	static char s_res[16]; /* short result 15 bits*/

	t_arg* arg_parsed;

	int add_opc = 0;
	int isK = 0;
	int isA = 0;
	int isB = 0;
	int isX = 0;
	int ic = 0;
	int cB = 0;

	int i = 0;
	int kVal = 0;

	memset(l_res, 0, 31);
	memcpy(t_opc, opc, 2);
	
	if (arg != NULL) { /* the operand has argument */
		if (memcmp(t_opc, "EQU", strlen(t_opc)) == 0) {
			itoa(EQU, l_res, 10);
			return l_res;
		}
		else if (memcmp(t_opc, "SA", strlen(t_opc)) == 0) { // SA
			arg_parsed = check_arg_plus(arg);
			if (arg_parsed == NULL) goto argminSA;
			memcpy(t_idx, opc + 2, 1);     // SAi
			if (arg_parsed[0].n == 1) {
				if (arg_parsed[0].t == 'K') { /* only constant, supposed B0 : SAi [B0] + K*/
					strcat(l_res, tobinstr(SA + 1, 6));
					strcat(l_res, tobinstr(t_idx[0] - '0', 3));
					strcat(l_res, ZERO3);
					strcat(l_res, tobinstr(arg_parsed[0].v, 18));
				}
				else {
					if (arg_parsed[0].t == 'A') { // SAi Aj + [B0]
						strcat(l_res, tobinstr(SA + 4, 6)); 
						strcat(l_res, tobinstr(t_idx[0] - '0', 3));
						strcat(l_res, tobinstr(arg_parsed[0].v, 3));
						strcat(l_res, ZERO3);
					}
					else if (arg_parsed[0].t == 'B') { // SAi [B0] + Bk
						strcat(l_res, tobinstr(SA + 6, 6));
						strcat(l_res, tobinstr(t_idx[0] - '0', 3));
						strcat(l_res, ZERO3);
						strcat(l_res, tobinstr(arg_parsed[0].v, 3));
					}
					else if (arg_parsed[0].t == 'X') { // SAi Xj + [B0]
						strcat(l_res, tobinstr(SA + 3, 6));
						strcat(l_res, tobinstr(t_idx[0] - '0', 3));
						strcat(l_res, tobinstr(arg_parsed[0].v, 3));
						strcat(l_res, ZERO3);
					}
				}
				return l_res;
			}
			else { /* there are two operands */
				for (i = 0; i < arg_parsed[0].n; i++) {
					if (arg_parsed[i].t == 'K') {
						if (isK == 1) { /* the other arg is a K too */
							kVal += arg_parsed[i].v;
							strcat(k_temp, tobinstr(kVal, 18));
						}
						else if (isK == 0) {
							isK = 1;
							kVal = arg_parsed[i].v;
							strcat(k_temp, tobinstr(kVal, 18));
						}
						strcat(k_res, k_temp);
					}
					else if (arg_parsed[i].t == 'A') {
						isA = 1;
						strcat(a_res, tobinstr(arg_parsed[i].v, 3));
					}
					else if (arg_parsed[i].t == 'B') { /* could be two B regs */
						isB = 1;
						strcat(b_res[ic], tobinstr(arg_parsed[i].v, 3));
						ic++;
					}
					else if (arg_parsed[i].t == 'X') {
						isX = 1;
						strcat(x_res, tobinstr(arg_parsed[i].v, 3));
					}
				}
				if (isA && isK) {
					add_opc = 0;
					strcat(l_res, tobinstr(SA + add_opc, 6));
					strcat(l_res, tobinstr(t_idx[0] - '0', 3));
					strcat(l_res, a_res);
					strcat(l_res, k_res);
				}
				else if (isB && isK) {
					add_opc = 1;
					strcat(l_res, tobinstr(SA + add_opc, 6));
					strcat(l_res, tobinstr(t_idx[0] - '0', 3));
					strcat(l_res, b_res[0]);
					strcat(l_res, k_res);
				}
				else if (isX && isK) {
					add_opc = 2;
					strcat(l_res, tobinstr(SA + add_opc, 6));
					strcat(l_res, tobinstr(t_idx[0] - '0', 3));
					strcat(l_res, x_res);
					strcat(l_res, k_res);
				}
				else if (isX && isB) {
					add_opc = 3;
					strcat(l_res, tobinstr(SA + add_opc, 6));
					strcat(l_res, tobinstr(t_idx[0] - '0', 3));
					strcat(l_res, x_res);
					strcat(l_res, b_res[0]);
				}
				else if (isA && isB) {
					add_opc = 4;
					strcat(l_res, tobinstr(SA + add_opc, 6));
					strcat(l_res, tobinstr(t_idx[0] - '0', 3));
					strcat(l_res, a_res);
					strcat(l_res, b_res[0]);
				}
				else if (isB) {
					add_opc = 6;
					strcat(l_res, tobinstr(SA + add_opc, 6));
					strcat(l_res, tobinstr(t_idx[0] - '0', 3));
					strcat(l_res, b_res[0]);
					strcat(l_res, b_res[1]);
				}
				return l_res;
			}
argminSA:
			arg_parsed = check_arg_min(arg);
			memcpy(t_idx, opc + 2, 1);     // SAi
			if (arg_parsed[0].n == 1) {
				if (arg_parsed[0].t == 'K') { /* only constant, supposed B0 : SAi [B0] + K*/
					strcat(l_res, tobinstr(SA + 1, 6));
					strcat(l_res, tobinstr(t_idx[0] - '0', 3));
					strcat(l_res, ZERO3);
					strcat(l_res, tobinstr(arg_parsed[0].v, 18));
				}
				else {
					if (arg_parsed[0].t == 'A') { // SAi Aj + [B0]
						strcat(l_res, tobinstr(SA + 4, 6));
						strcat(l_res, tobinstr(t_idx[0] - '0', 3));
						strcat(l_res, tobinstr(arg_parsed[0].v, 3));
						strcat(l_res, ZERO3);
					}
					else if (arg_parsed[0].t == 'B') { // SAi [B0] + Bk
						strcat(l_res, tobinstr(SA + 6, 6));
						strcat(l_res, tobinstr(t_idx[0] - '0', 3));
						strcat(l_res, ZERO3);
						strcat(l_res, tobinstr(arg_parsed[0].v, 3));
					}
					else if (arg_parsed[0].t == 'X') { // SAi Xj + [B0]
						strcat(l_res, tobinstr(SA + 3, 6));
						strcat(l_res, tobinstr(t_idx[0] - '0', 3));
						strcat(l_res, tobinstr(arg_parsed[0].v, 3));
						strcat(l_res, ZERO3);
					}
				}
				return l_res;
			}
			else { /* there are two operands */
				for (i = 0; i < arg_parsed[0].n; i++) {
					if (arg_parsed[i].t == 'K') {
						if (isK == 1) { /* the other arg is a K too */
							kVal += arg_parsed[i].v;
							strcat(k_temp, tobinstr(kVal, 18));
						}
						else if (isK == 0) {
							isK = 1;
							kVal = arg_parsed[i].v;
							strcat(k_temp, tobinstr(kVal, 18));
						}
						strcat(k_res, k_temp);
					}
					else if (arg_parsed[i].t == 'A') {
						isA = 1;
						strcat(a_res, tobinstr(arg_parsed[i].v, 3));
					}
					else if (arg_parsed[i].t == 'B') { /* could be two B regs */
						isB = 1;
						strcat(b_res[ic], tobinstr(arg_parsed[i].v, 3));
						ic++;
					}
					else if (arg_parsed[i].t == 'X') {
						isX = 1;
						strcat(x_res, tobinstr(arg_parsed[i].v, 3));
					}
				}
				if (isA && isK) {
					add_opc = 0;
					strcat(l_res, tobinstr(SA + add_opc, 6));
					strcat(l_res, tobinstr(t_idx[0] - '0', 3));
					strcat(l_res, a_res);
					strcat(l_res, k_res);
				}
				else if (isB && isK) {
					add_opc = 1;
					strcat(l_res, tobinstr(SA + add_opc, 6));
					strcat(l_res, tobinstr(t_idx[0] - '0', 3));
					strcat(l_res, b_res[0]);
					strcat(l_res, k_res);
				}
				else if (isX && isK) {
					add_opc = 2;
					strcat(l_res, tobinstr(SA + add_opc, 6));
					strcat(l_res, tobinstr(t_idx[0] - '0', 3));
					strcat(l_res, x_res);
					strcat(l_res, k_res);
				}
				else if (isX && isB) {
					add_opc = 3;
					strcat(l_res, tobinstr(SA + add_opc, 6));
					strcat(l_res, tobinstr(t_idx[0] - '0', 3));
					strcat(l_res, x_res);
					strcat(l_res, b_res[0]);
				}
				else if (isA && isB) {
					add_opc = 4;
					strcat(l_res, tobinstr(SA + add_opc, 6));
					strcat(l_res, tobinstr(t_idx[0] - '0', 3));
					strcat(l_res, a_res);
					strcat(l_res, b_res[0]);
				}
				else if (isB) {
					add_opc = 6;
					strcat(l_res, tobinstr(SA + add_opc, 6));
					strcat(l_res, tobinstr(t_idx[0] - '0', 3));
					strcat(l_res, b_res[0]);
					strcat(l_res, b_res[1]);
				}
				return l_res;
			}
		}
		else if (memcmp(t_opc, "SB", strlen(t_opc)) == 0) {
			arg_parsed = check_arg_plus(arg);
			if (arg_parsed == NULL) goto argminSB;
			memcpy(t_idx, opc + 2, 1);     // SAi
			if (arg_parsed[0].n == 1) {
				if (arg_parsed[0].t == 'K') { /* only constant, supposed B0 : SAi [B0] + K*/
					strcat(l_res, tobinstr(SB + 1, 6));
					strcat(l_res, tobinstr(t_idx[0] - '0', 3));
					strcat(l_res, ZERO3);
					strcat(l_res, tobinstr(arg_parsed[0].v, 18));
				}
				else {
					if (arg_parsed[0].t == 'A') { // SAi Aj + [B0]
						strcat(l_res, tobinstr(SB + 4, 6));
						strcat(l_res, tobinstr(t_idx[0] - '0', 3));
						strcat(l_res, tobinstr(arg_parsed[0].v, 3));
						strcat(l_res, ZERO3);
					}
					else if (arg_parsed[0].t == 'B') { // SAi [B0] + Bk
						strcat(l_res, tobinstr(SB + 6, 6));
						strcat(l_res, tobinstr(t_idx[0] - '0', 3));
						strcat(l_res, ZERO3);
						strcat(l_res, tobinstr(arg_parsed[0].v, 3));
					}
					else if (arg_parsed[0].t == 'X') { // SAi Xj + [B0]
						strcat(l_res, tobinstr(SB + 3, 6));
						strcat(l_res, tobinstr(t_idx[0] - '0', 3));
						strcat(l_res, tobinstr(arg_parsed[0].v, 3));
						strcat(l_res, ZERO3);
					}
				}
				return l_res;
			}
			else { /* there are two operands */
				for (i = 0; i < arg_parsed[0].n; i++) {
					if (arg_parsed[i].t == 'K') {
						if (isK == 1) { /* the other arg is a K too */
							kVal += arg_parsed[i].v;
							strcat(k_temp, tobinstr(kVal, 18));
						}
						else if (isK == 0) {
							isK = 1;
							kVal = arg_parsed[i].v;
							strcat(k_temp, tobinstr(arg_parsed[i].v, 18));
						}
						strcat(k_res, k_temp);
					}
					else if (arg_parsed[i].t == 'A') {
						isA = 1;
						strcat(a_res, tobinstr(arg_parsed[i].v, 3));
					}
					else if (arg_parsed[i].t == 'B') { /* could be two B regs */
						isB = 1;
						strcat(b_res[ic], tobinstr(arg_parsed[i].v, 3));
						ic++;
					}
					else if (arg_parsed[i].t == 'X') {
						isX = 1;
						strcat(x_res, tobinstr(arg_parsed[i].v, 3));
					}
				}
				if (isA && isK) {
					add_opc = 0;
					strcat(l_res, tobinstr(SB + add_opc, 6));
					strcat(l_res, tobinstr(t_idx[0] - '0', 3));
					strcat(l_res, a_res);
					strcat(l_res, k_res);
				}
				else if (isB && isK) {
					add_opc = 1;
					strcat(l_res, tobinstr(SB + add_opc, 6));
					strcat(l_res, tobinstr(t_idx[0] - '0', 3));
					strcat(l_res, b_res[0]);
					strcat(l_res, k_res);
				}
				else if (isX && isK) {
					add_opc = 2;
					strcat(l_res, tobinstr(SB + add_opc, 6));
					strcat(l_res, tobinstr(t_idx[0] - '0', 3));
					strcat(l_res, x_res);
					strcat(l_res, k_res);
				}
				else if (isX && isB) {
					add_opc = 3;
					strcat(l_res, tobinstr(SB + add_opc, 6));
					strcat(l_res, tobinstr(t_idx[0] - '0', 3));
					strcat(l_res, x_res);
					strcat(l_res, b_res[0]);
				}
				else if (isA && isB) {
					add_opc = 4;
					strcat(l_res, tobinstr(SB + add_opc, 6));
					strcat(l_res, tobinstr(t_idx[0] - '0', 3));
					strcat(l_res, a_res);
					strcat(l_res, b_res[0]);
				}
				else if (isB) {
					add_opc = 6;
					strcat(l_res, tobinstr(SB + add_opc, 6));
					strcat(l_res, tobinstr(t_idx[0] - '0', 3));
					strcat(l_res, b_res[0]);
					strcat(l_res, b_res[1]);
				}
				return l_res;
			}
argminSB:
			arg_parsed = check_arg_min(arg);
			memcpy(t_idx, opc + 2, 1);     // SAi
			if (arg_parsed[0].n == 1) {
				if (arg_parsed[0].t == 'K') { /* only constant, supposed B0 : SAi [B0] + K*/
					strcat(l_res, tobinstr(SB + 1, 6));
					strcat(l_res, tobinstr(t_idx[0] - '0', 3));
					strcat(l_res, ZERO3);
					strcat(l_res, tobinstr(arg_parsed[0].v, 18));
				}
				else {
					if (arg_parsed[0].t == 'A') { // SAi Aj + [B0]
						strcat(l_res, tobinstr(SB + 4, 6));
						strcat(l_res, tobinstr(t_idx[0] - '0', 3));
						strcat(l_res, tobinstr(arg_parsed[0].v, 3));
						strcat(l_res, ZERO3);
					}
					else if (arg_parsed[0].t == 'B') { // SAi [B0] + Bk
						strcat(l_res, tobinstr(SB + 6, 6));
						strcat(l_res, tobinstr(t_idx[0] - '0', 3));
						strcat(l_res, ZERO3);
						strcat(l_res, tobinstr(arg_parsed[0].v, 3));
					}
					else if (arg_parsed[0].t == 'X') { // SAi Xj + [B0]
						strcat(l_res, tobinstr(SB + 3, 6));
						strcat(l_res, tobinstr(t_idx[0] - '0', 3));
						strcat(l_res, tobinstr(arg_parsed[0].v, 3));
						strcat(l_res, ZERO3);
					}
				}
				return l_res;
			}
			else { /* there are two operands */
				for (i = 0; i < arg_parsed[0].n; i++) {
					if (arg_parsed[i].t == 'K') {
						if (isK == 1) { /* the other arg is a K too */
							kVal += arg_parsed[i].v;
							strcat(k_temp, tobinstr(kVal, 18));
						}
						else if (isK == 0) {
							isK = 1;
							kVal = arg_parsed[i].v;
							strcat(k_temp, tobinstr(arg_parsed[i].v, 18));
						}
						strcat(k_res, k_temp);
					}
					else if (arg_parsed[i].t == 'A') {
						isA = 1;
						strcat(a_res, tobinstr(arg_parsed[i].v, 3));
					}
					else if (arg_parsed[i].t == 'B') { /* could be two B regs */
						isB = 1;
						strcat(b_res[ic], tobinstr(arg_parsed[i].v, 3));
						ic++;
					}
					else if (arg_parsed[i].t == 'X') {
						isX = 1;
						strcat(x_res, tobinstr(arg_parsed[i].v, 3));
					}
				}
				if (isA && isK) {
					add_opc = 0;
					strcat(l_res, tobinstr(SB + add_opc, 6));
					strcat(l_res, tobinstr(t_idx[0] - '0', 3));
					strcat(l_res, a_res);
					strcat(l_res, k_res);
				}
				else if (isB && isK) {
					add_opc = 1;
					strcat(l_res, tobinstr(SB + add_opc, 6));
					strcat(l_res, tobinstr(t_idx[0] - '0', 3));
					strcat(l_res, b_res[0]);
					strcat(l_res, k_res);
				}
				else if (isX && isK) {
					add_opc = 2;
					strcat(l_res, tobinstr(SB + add_opc, 6));
					strcat(l_res, tobinstr(t_idx[0] - '0', 3));
					strcat(l_res, x_res);
					strcat(l_res, k_res);
				}
				else if (isX && isB) {
					add_opc = 3;
					strcat(l_res, tobinstr(SB + add_opc, 6));
					strcat(l_res, tobinstr(t_idx[0] - '0', 3));
					strcat(l_res, x_res);
					strcat(l_res, b_res[0]);
				}
				else if (isA && isB) {
					add_opc = 4;
					strcat(l_res, tobinstr(SB + add_opc, 6));
					strcat(l_res, tobinstr(t_idx[0] - '0', 3));
					strcat(l_res, a_res);
					strcat(l_res, b_res[0]);
				}
				else if (isB) {
					add_opc = 6;
					strcat(l_res, tobinstr(SB + add_opc, 6));
					strcat(l_res, tobinstr(t_idx[0] - '0', 3));
					strcat(l_res, b_res[0]);
					strcat(l_res, b_res[1]);
				}
				return l_res;
			}
		}
		else if (memcmp(t_opc, "SX", strlen(t_opc)) == 0) {
			arg_parsed = check_arg_plus(arg);
			memcpy(t_idx, opc + 2, 1);     // SAi
			if (arg_parsed[0].n == 1) {
				if (arg_parsed[0].t == 'K') { /* only constant, supposed B0 : SAi [B0] + K*/
					strcat(l_res, tobinstr(SX + 1, 6));
					strcat(l_res, tobinstr(t_idx[0] - '0', 3));
					strcat(l_res, ZERO3);
					strcat(l_res, tobinstr(arg_parsed[0].v, 18));
				}
				else {
					if (arg_parsed[0].t == 'A') { // SAi Aj + [B0]
						strcat(l_res, tobinstr(SX + 4, 6));
						strcat(l_res, tobinstr(t_idx[0] - '0', 3));
						strcat(l_res, tobinstr(arg_parsed[0].v, 3));
						strcat(l_res, ZERO3);
					}
					else if (arg_parsed[0].t == 'B') { // SAi [B0] + Bk
						strcat(l_res, tobinstr(SX + 6, 6));
						strcat(l_res, tobinstr(t_idx[0] - '0', 3));
						strcat(l_res, ZERO3);
						strcat(l_res, tobinstr(arg_parsed[0].v, 3));
					}
					else if (arg_parsed[0].t == 'X') { // SAi Xj + [B0]
						strcat(l_res, tobinstr(SX + 3, 6));
						strcat(l_res, tobinstr(t_idx[0] - '0', 3));
						strcat(l_res, tobinstr(arg_parsed[0].v, 3));
						strcat(l_res, ZERO3);
					}
				}
				return l_res;
			}
			else { /* there are two operands */
				for (i = 0; i < arg_parsed[0].n; i++) {
					if (arg_parsed[i].t == 'K') {
						if (isK == 1) { /* the other arg is a K too */
							kVal += arg_parsed[i].v;
							strcat(k_temp, tobinstr(arg_parsed[i].v, 18));
						}
						else if (isK == 0) {
							isK = 1;
							kVal = arg_parsed[i].v;
							strcat(k_temp, tobinstr(arg_parsed[i].v, 18));
						}
						strcat(k_res, k_temp);
					}
					else if (arg_parsed[i].t == 'A') {
						isA = 1;
						strcat(a_res, tobinstr(arg_parsed[i].v, 3));
					}
					else if (arg_parsed[i].t == 'B') { /* could be two B regs */
						isB = 1;
						strcat(b_res[ic], tobinstr(arg_parsed[i].v, 3));
						ic++;
					}
					else if (arg_parsed[i].t == 'X') {
						isX = 1;
						strcat(x_res, tobinstr(arg_parsed[i].v, 3));
					}
				}
				if (isA && isK) {
					add_opc = 0;
					strcat(l_res, tobinstr(SX + add_opc, 6));
					strcat(l_res, tobinstr(t_idx[0] - '0', 3));
					strcat(l_res, a_res);
					strcat(l_res, k_res);
				}
				else if (isB && isK) {
					add_opc = 1;
					strcat(l_res, tobinstr(SX + add_opc, 6));
					strcat(l_res, tobinstr(t_idx[0] - '0', 3));
					strcat(l_res, b_res[0]);
					strcat(l_res, k_res);
				}
				else if (isX && isK) {
					add_opc = 2;
					strcat(l_res, tobinstr(SX + add_opc, 6));
					strcat(l_res, tobinstr(t_idx[0] - '0', 3));
					strcat(l_res, x_res);
					strcat(l_res, k_res);
				}
				else if (isX && isB) {
					add_opc = 3;
					strcat(l_res, tobinstr(SX + add_opc, 6));
					strcat(l_res, tobinstr(t_idx[0] - '0', 3));
					strcat(l_res, x_res);
					strcat(l_res, b_res[0]);
				}
				else if (isA && isB) {
					add_opc = 4;
					strcat(l_res, tobinstr(SX + add_opc, 6));
					strcat(l_res, tobinstr(t_idx[0] - '0', 3));
					strcat(l_res, a_res);
					strcat(l_res, b_res[0]);
				}
				else if (isB) {
					add_opc = 6;
					strcat(l_res, tobinstr(SX + add_opc, 6));
					strcat(l_res, tobinstr(t_idx[0] - '0', 3));
					strcat(l_res, b_res[0]);
					strcat(l_res, b_res[1]);
				}
				return l_res;
			}
		}
		else if (memcmp(t_opc, "RJ", strlen(t_opc)) == 0) {
			arg_parsed = check_address(arg);
			strcat(l_res, tobinstr(RJ, 6));
			strcat(l_res, ZERO6);
			strcat(l_res, tobinstr(arg_parsed[0].v, 18));
			return l_res;
		}
		else if (memcmp(t_opc, "NE", 2) == 0) {
			arg_parsed = check_arg_comm(arg);
			for (i = 0; i < arg_parsed[0].n; i++) {
				if (arg_parsed[i].t == 'B') {
					cB++;
					strcat(t_res, tobinstr(arg_parsed[i].v, 3));
				}
				else {
					if (cB == 1) {
						strcat(t_res, ZERO3, 3);
						strcat(t_res, tobinstr(arg_parsed[i].v, 18));
					}
					else if (cB == 0) {
						strcat(t_res, ZERO6, 6);
						strcat(t_res, tobinstr(arg_parsed[i].v, 18));
					}
					else {
						strcat(t_res, tobinstr(arg_parsed[i].v, 18));
					}
				}
			}
			strcat(l_res, tobinstr(EQ, 6));
			strcat(l_res, t_res);
			return l_res;
		}
		else if (memcmp(t_opc, "JP", strlen(t_opc)) == 0) {
			arg_parsed = check_arg_plus(arg);
			for (i = 0; i < arg_parsed[0].n; i++) {
				if (arg_parsed[i].t == 'B') {
					strcat(t_res, tobinstr(arg_parsed[i].v, 3));
				}
				else if (arg_parsed[i].t == 'K') {
					strcat(t_res, tobinstr(arg_parsed[i].v, 18));
				}
				else {
					strcat(t_res, "FORMAT ERROR");
				}
			}
			strcat(l_res, tobinstr(JP, 6));
			strcat(l_res, t_res);
			return l_res;
		}
		else if ((memcmp(t_opc, "PL", strlen(t_opc)) == 0)) {
			arg_parsed = check_arg_comm(arg);
			isX = 0;
			isB = 0;
			for (i = 0; i < arg_parsed[0].n; i++) {
				if (arg_parsed[i].t == 'X') {
					isX = 1;
					strcat(t_res, tobinstr(arg_parsed[i].v, 3));
				}
				else if (arg_parsed[i].t == 'B') {
					isB = 1;
					strcat(t_res, tobinstr(arg_parsed[i].v, 3));
					strcat(t_res, ZERO3);
				}
				else {
					strcat(t_res, tobinstr(arg_parsed[i].v,18));
				}
			}
			if (isX) {
				strcat(l_res, tobinstr(PL, 9));
			}
			else {
				strcat(l_res, tobinstr(GE, 6));
			}
			strcat(l_res, t_res);
			return l_res;
		}
		else if ((memcmp(t_opc, "NZ", strlen(t_opc)) == 0)) {
			arg_parsed = check_arg_comm(arg);
			isX = 0;
			isB = 0;
			for (i = 0; i < arg_parsed[0].n; i++) {
				if (arg_parsed[i].t == 'X') {
					isX = 1;
					strcat(t_res, tobinstr(arg_parsed[i].v, 3));
				} 
				else if (arg_parsed[i].t == 'B') {
					isB = 1;
					strcat(t_res, tobinstr(arg_parsed[i].v, 3));
					strcat(t_res, ZERO3);
				}
				else {
					strcat(t_res, tobinstr(arg_parsed[i].v, 18));
				}
			}
			if (isX) {
				strcat(l_res, tobinstr(NZ, 9));
			}
			else {
				strcat(l_res, tobinstr(NE, 6));
			}
				strcat(l_res, t_res);
			return l_res;
		}
		else if ((memcmp(t_opc, "GE", strlen(t_opc)) == 0)) {
			arg_parsed = check_arg_comm(arg);
			for (i = 0; i < arg_parsed[0].n; i++) {
				if (arg_parsed[i].t == 'B') {
					strcat(t_res, tobinstr(arg_parsed[i].v, 3));
				}
				else {
					strcat(t_res, tobinstr(arg_parsed[i].v, 18));
				}
			}
			strcat(l_res, tobinstr(GE, 6));
			strcat(l_res, t_res);
			return l_res;
		}
		else if ((memcmp(t_opc, "ZR", strlen(t_opc)) == 0)) {
			arg_parsed = check_arg_comm(arg);
			for (i = 0; i < arg_parsed[0].n; i++) {
				if (arg_parsed[i].t == 'X') {
					strcat(t_res, tobinstr(arg_parsed[i].v, 3));
				}
				else {
					strcat(t_res, tobinstr(arg_parsed[i].v, 18));
				}
			}
			strcat(l_res, tobinstr(ZR, 9));
			strcat(l_res, t_res);
			return l_res;
		}
		else if ((memcmp(t_opc, "NG", strlen(t_opc)) == 0)) {
			arg_parsed = check_arg_comm(arg);
			for (i = 0; i < arg_parsed[0].n; i++) {
				if (arg_parsed[i].t == 'X') {
					strcat(t_res, tobinstr(arg_parsed[i].v, 3));
				}
				else {
					strcat(t_res, tobinstr(arg_parsed[i].v, 18));
				}
			}
			strcat(l_res, tobinstr(NG, 9));
			strcat(l_res, t_res);
			return l_res;
		}
		else if (memcmp(t_opc, "EQ", strlen(t_opc)) == 0) {
			arg_parsed = check_arg_comm(arg);
			for (i = 0; i < arg_parsed[0].n; i++) {
				if (arg_parsed[i].t == 'B') {
					cB++;
					strcat(t_res, tobinstr(arg_parsed[i].v, 3));
				}
				else {
					if (cB == 1) {
						strcat(t_res, ZERO3, 3);
						strcat(t_res, tobinstr(arg_parsed[i].v, 18));
					}
					else if (cB == 0) {
						strcat(t_res, ZERO6, 6);
						strcat(t_res, tobinstr(arg_parsed[i].v, 18));
					}
					else {
						strcat(t_res, tobinstr(arg_parsed[i].v, 18));
					}
				}
			}
			strcat(l_res, tobinstr(EQ, 6));
			strcat(l_res, t_res);
			return l_res;
		}
		else if (memcmp(t_opc, "IX", strlen(t_opc)) == 0) {
			return IX;
		}
		else if (memcmp(t_opc, "FX", strlen(t_opc)) == 0) {
			return FX;
		}
		return NOOP;
	} else { /* No second operands, etc */
		if (memcmp(t_opc, "SA", strlen(t_opc)) == 0) {
			memcpy(t_idx, opc + 2, 1);
			return SA + (t_idx[0] - '0');
		}
		else if (memcmp(t_opc, "SB", strlen(t_opc)) == 0) {
			memcpy(t_idx, opc + 2, 1);
			return SB + (t_idx[0] - '0');
		}
		else if (memcmp(t_opc, "SX", strlen(t_opc)) == 0) {
			memcpy(t_idx, opc + 2, 1);
			return SX + (t_idx[0] - '0');
		}
		else if (memcmp(t_opc, "PS", strlen(t_opc)) == 0) {
			return "000000000000000000000000000000";
		}
		else if (memcmp(t_opc, "NG", strlen(t_opc)) == 0) {
			return NG;
		}
		else if (memcmp(t_opc, "NE", strlen(t_opc)) == 0) {
			return PS;
		}
		else if (memcmp(t_opc, "JP", strlen(t_opc)) == 0) {
			return JP;
		}
		else if (memcmp(t_opc, "PL", strlen(t_opc)) == 0) {
			return PL;
		}
		else if (memcmp(t_opc, "EQ", strlen(t_opc)) == 0) {
			return EQ;
		}
		else if (memcmp(t_opc, "NO", strlen(t_opc)) == 0) {
			return "100110000000000";
		}
		else if (memcmp(t_opc, "IX", strlen(t_opc)) == 0) {
			return IX;
		}
		else if (memcmp(t_opc, "FX", strlen(t_opc)) == 0) {
			return FX;
		}
		return NOOP;
	}
}
int main() {
	FILE *filn;
	FILE *filout;
	fopen_s(&filn, "test.asm", "r");
	fopen_s(&filout, "test.cdc", "w");
	char* c_opc;
	char* t_opc;
	char* pline;
	size_t i = 0;
	size_t ll = 0;
	int isShort = 0;
	int isLong = 0;

	pline = malloc(255);

	if (!filn) {
		perror("File opening failed");
		return EXIT_FAILURE;
	}

	/* analyzing asm file */

	/* doing first pass */
	while (fgets(line, sizeof line, filn) != NULL) {
		ll = strnlen_s(line, sizeof line);
		memcpy_s(firstCol, 1, line, 1); /* skip first col... maybe for future usage */
		memcpy_s(label, 8, line + 1, 7);
		for (i = 0; i < strlen(label); i++) {
			if (isspace(label[i])) {
				label[i] = '\0';
				break;
			}
		}
		memcpy_s(opcode, 8, line + 10, 7);

		if (ll >= 18) {
			memcpy_s(argument, 12, line + 17, 11);
			if (ll >= 29) {
				memcpy_s(comment, 50, line + 28, 49);
				comment[strlen(comment) - 1] = '\0';
			}
			argument[strlen(argument) - 1] = '\0';
		}
		else {
			opcode[strlen(opcode) - 1] = '\0';
		}

		/* checks the results */
		noLabel = isEmpty(label);
		noComment = isEmpty(comment);
		noArg = isEmpty(argument);
		t_opc = removeSpace(opcode);

		if (noLabel) {
			printf("NO LABEL\n");
		}
		else {
			if (memcmp(t_opc, "EQU", strlen(t_opc)) != 0) {
				if (findLabel(label) < 0) { // this is a new LABEL
					memcpy_s(symbTable[numLabels], strlen(label), label, strlen(label));
					memTable[numLabels] = START_ADDRESS + numLabels;
					numLabels++;
					printf("LAB (len : %zd) is : %s, poiting at address : %zd\n", strlen(label), label, memLabel(label));
				}
			}
			else if (getEQU(label) == NULL) { // this is an new EQUated constant
				memcpy_s(equTable[numEQU], strlen(label), label, strlen(label));
				strcat(equTable[numEQU], "|");
				strcat(equTable[numEQU], removeZeros(tobinstr(atoi(argument), 18)));
				numEQU++;
				printf("EQU constant (len : %zd) is : %s with value: %s\n", strlen(label), label, getEQU(label));
			}
		}
	}
	rewind(filn);
	resetReading();

	while (fgets(line,sizeof line,filn) != NULL) {
		ll = strnlen_s(line, sizeof line);
		memcpy_s(firstCol, 1, line, 1); /* skip first col... maybe for future usage */
		memcpy_s(label, 8, line+1, 7);
		for (i = 0; i < strlen(label); i++) {
			if (isspace(label[i])) {
				label[i] = '\0';
				break;
			}
		}
		memcpy_s(opcode, 8, line + 10, 7);
		
		if (ll >= 18) {
			memcpy_s(argument, 12, line + 17, 11);
			if (ll >= 29) {
				memcpy_s(comment, 50, line + 28, 49);
				comment[strlen(comment) - 1] = '\0';
			}
			argument[strlen(argument) - 1] = '\0';
		}
		else {
			opcode[strlen(opcode) - 1] = '\0';
		}
		
		/* checks the results */
		
		noComment = isEmpty(comment);
		noArg = isEmpty(argument);
		t_opc = removeSpace(opcode);

		printf("OPC (len : %zd) is : %s|\n", strlen(opcode), opcode);

		if (noArg) {
			printf("NO ARGUMENT\n");
			c_opc = convOpcode(t_opc, NULL);
			printf("Translated opcode: %s\n", c_opc);
			if ((memcmp(c_opc,"80", 3) !=0) && (memcmp(c_opc,NOOP,strlen(NOOP))) != 0) {
				strcpy(program[PC], c_opc);
				PC++;
			}
			printf("Opcode length: %zd\n", strlen(c_opc));
		}
		else {
			printf("ARG (len : %zd) is : %s|\n", strlen(argument), argument);
			c_opc = convOpcode(t_opc, argument);
			if ((memcmp(c_opc, "80", 3) != 0) && (memcmp(c_opc, NOOP, strlen(NOOP))) != 0) {
				strcpy(program[PC], c_opc);
				PC++;
			}
			printf("Translated opcode: %s\n", c_opc);
			printf("Opcode length: %zd\n", strlen(c_opc));
		}
		if (noComment) {
			printf("NO COMMENT\n");
		}
		else {
			printf("COM (len : %zd) is : %s|\n", strlen(comment), comment);
		}
		printf("%%%%%%%%%%%%%%%%%%%%%%\n");

		// cleaning for the next ride...
		resetReading();
	}
	printf("writing output file...\n");
	for (i = 0; i < PC; i++) {
		if ((isShort == 0) && (isLong == 0)) _itoa(START_ADDRESS, pline, 8);
		if (strlen(program[i]) == 10) {     /* long instruction opcode 30bits */
			if (isShort >= 1) {
				fprintf(filout, "%s\n", pline);
				isShort = 0;
			}
			if (isLong == 1) {              /* get the second long instruction: PRINT THEM! */
				strcat(pline, program[i]);
				fprintf(filout, "%s\n", pline);
				START_ADDRESS++;
				isLong = 0;
			}
			else {
				strcat(pline, program[i]);
				isLong++;
			}
		}
		else if (strlen(program[i]) == 5) { /* short instruction opcode 15bits */
			if (strlen(pline) + 5 == 20) {
				strcat(pline, program[i]);
				fprintf(filout, "%s\n", pline);
				isShort = 0;
			}
			else {
				strcat(pline, program[i]);
				isShort++;
			}
		}
	}
	printf("done!\n");
	fclose(filn);
	fclose(filout);
}