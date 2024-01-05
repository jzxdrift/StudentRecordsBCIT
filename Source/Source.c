#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

#define IN_LENGTH			20

#define CNT_CMD				(-3)
#define EXIT_CMD			(-2)
#define ABORT_CMD			(-1)
#define MODIFY_FCMD			2

#define NW_LN				"\n"
#define NULL_TRM			'\0'

#define READ_STR_FMT			"%s %u"
#define WRITE_STR_FMT			"%s %-3u\n"
#define DISPLAY_STR_FMT			"%u %s %u\n"

typedef enum
{
	MAIN_COMMAND,
	ID_COMMAND,
	SCORE_COMMAND,
} CommandFlags;

typedef struct
{
	char id[10];
	unsigned score;
} Record;

typedef void(*commandFunctions[])(FILE*, Record*, const unsigned);

void showCommandPrompt(void);
void showIDPrompt(void);
void showScorePrompt(void);

void getInput(char [], const CommandFlags*, int*);
void executeCommand(const int, const commandFunctions, FILE*, Record*);
void clearInput(Record*, char [], const size_t);

void displayAllRecords(FILE*, Record*, const unsigned);
void appendRecord(FILE*, Record*, const unsigned);
void modifyRecord(FILE*, Record*, const unsigned);

int validateID(const char []);
int validateScore(const char [], unsigned*);

void addRecord(FILE *const, Record*);

int main(int argc, char *argv[])
{
	if(argc > 1)
	{
		const char *F_ACCESS_MODE = "wb+";

		FILE *fStream = fopen(argv[1], F_ACCESS_MODE);
		if(fStream != NULL)
		{
			commandFunctions cmdFuncPtr = {displayAllRecords, appendRecord, modifyRecord};

			CommandFlags flag;
			int mainCommand;

			Record record;
			char input[IN_LENGTH];

			do
			{
				flag = MAIN_COMMAND;
				mainCommand = CNT_CMD;

				/*initialize/reset record and input to 0*/
				clearInput(&record, input, sizeof input);
				getInput(input, &flag, &mainCommand);
				executeCommand(mainCommand, cmdFuncPtr, fStream, &record);

			} while(mainCommand != EXIT_CMD);

			fclose(fStream);
			puts("Exiting...");

			return 0;
		}
		else
		{
			perror("Error opening file");
			exit(EXIT_FAILURE);
		}
	}
	else
	{
		puts("Error: No command line arguments provided");
		exit(EXIT_FAILURE);
	}
}

void showCommandPrompt(void)
{
	puts("MAIN COMMANDS");
	puts("-2 - quit");
	puts("-1 - append a record");
	puts(" 0 - display all records");
	puts(" n - modify record number n (positive integer)");

	printf("\nEnter command: ");
}

void showIDPrompt(void)
{
	puts("ID COMMANDS");
	puts("-1 - back to main menu");

	puts("\nStudent ID must start with \"a\", followed by 8 digits");
	printf("Enter ID: ");
}

void showScorePrompt(void)
{
	puts("SCORE COMMANDS");
	puts("-1 - back to main menu");

	puts("\nScore must be between 0 and 100");
	printf("Enter score: ");
}

void getInput(char inp[], const CommandFlags *flg, int *cmd)
{
	/*show prompt based on input flag*/
	switch(*flg)
	{
		case MAIN_COMMAND:
			showCommandPrompt();
			break;
		case ID_COMMAND:
			showIDPrompt();
			break;
		case SCORE_COMMAND:
			showScorePrompt();
			break;
	}

	fgets(inp, IN_LENGTH, stdin);

	/*leading white space and empty input are invalid commands*/
	if(isspace(inp[0]))
	{
		puts("");
		return;
	}

	/*consider first word only*/
	inp[strcspn(inp, " ")] = NULL_TRM;
	/*remove new line from the word if input is shorter than array size*/
	inp[strcspn(inp, NW_LN)] = NULL_TRM;

	/*minus sign is invalid command*/
	/*prevent from falling into EOF case on sscanf return*/
	if(strcmp(inp, "-") == 0)
	{
		puts("");
		return;
	}

	switch(sscanf(inp, "%d", cmd))
	{
		/*number of successful assignments*/
		case 1:
			/*only for append or modify commands*/
			if(*flg != MAIN_COMMAND)
			{
				/*set command to exit on abort operation command*/
				if(*cmd == ABORT_CMD)
					*cmd = EXIT_CMD;
				/*prevent input score operation from exiting to main menu with input -2*/
				else if(*cmd == EXIT_CMD)
					*cmd = CNT_CMD;
			}
			break;
		/*EOF*/
		case -1:
			/*set command to exit on EOF*/
			*cmd = EXIT_CMD;
			break;
	}

	puts("");
}

void executeCommand(const int cmd, const commandFunctions cfp, FILE *fs, Record *rec)
{
	if(cmd >= -1 && cmd <= 0)
		cfp[abs(cmd)](fs, rec, 0);			/*invoke function based on command*/
	else if(cmd >= 1)
		cfp[MODIFY_FCMD](fs, rec, (unsigned)cmd);	/*invoke modify function with command as index*/
}

void clearInput(Record *rec, char inp[], const size_t isz)
{
	if(rec != NULL)
		memset(rec, 0, sizeof *rec);

	if(inp != NULL && isz > 0)
		memset(inp, 0, isz);
}

void displayAllRecords(FILE *fs, Record *rec, const unsigned pos)
{
	rewind(fs);

	/*check for number of successful assignments*/
	if(fscanf(fs, READ_STR_FMT, rec->id, &rec->score) == 2)
	{
		for(unsigned recNum = 1; ; ++recNum)
		{
			fprintf(stderr, DISPLAY_STR_FMT, recNum, rec->id, rec->score);

			/*check for number of successful assignments*/
			/*replaces EOF check in loop*/
			if(fscanf(fs, READ_STR_FMT, rec->id, &rec->score) != 2)
				break;
		}
		fprintf(stderr, NW_LN);
	}
}

void appendRecord(FILE *fs, Record *rec, const unsigned pos)
{
	fseek(fs, 0, SEEK_END);
	addRecord(fs, rec);
}

void modifyRecord(FILE *fs, Record *rec, const unsigned pos)
{
	/*each record is stored as text in file*/
	/*each record occupies 14 bytes in file*/
	/*9 bytes id, 1 byte space, 3 bytes score, 1 byte newline*/
	const long OFFSET_BYTES = 14;
	fseek(fs, (pos - 1) * OFFSET_BYTES, SEEK_SET);

	/*check for number of successful assignments*/
	if(fscanf(fs, READ_STR_FMT, rec->id, &rec->score) == 2)
	{
		fprintf(stderr, DISPLAY_STR_FMT, pos, rec->id, rec->score);
		fprintf(stderr, NW_LN);

		fseek(fs, (pos - 1) * OFFSET_BYTES, SEEK_SET);
		addRecord(fs, rec);
	}
}

int validateID(const char inp[])
{
	if(inp[0] == 'a' || inp[0] == 'A')
	{
		size_t i;
		for(i = 1; inp[i] != NULL_TRM; ++i)
		{
			if(!isdigit(inp[i]) || i > 8)		/*ID must have 8 digits*/
				return 0;
		}

		if(i <= 8)					/*ID must have 8 digits*/
			return 0;
	}
	else
		return 0;

	return 1;
}

int validateScore(const char inp[], unsigned *scr)
{
	for(size_t i = 0; inp[i] != NULL_TRM; ++i)
	{
		if(!isdigit(inp[i]) || i > 2)			/*score must be 3 characters at max*/
			return 0;
	}

	const unsigned MAX_SCORE = 100;
	*scr = (unsigned)atoi(inp);

	return *scr <= MAX_SCORE ? 1 : 0;
}

void addRecord(FILE *const fs, Record *rec)
{
	char inpID[IN_LENGTH], inpScore[IN_LENGTH];

	CommandFlags flag = ID_COMMAND;
	int cmd = CNT_CMD;

	/*reset record to 0*/
	clearInput(rec, NULL, 0);

	do
	{
		/*initialize/reset input to 0*/
		clearInput(NULL, inpID, sizeof inpID);
		getInput(inpID, &flag, &cmd);

		if(validateID(inpID))
		{
			flag = SCORE_COMMAND;
			unsigned score;

			do
			{
				/*initialize/reset input to 0*/
				clearInput(NULL, inpScore, sizeof inpScore);
				getInput(inpScore, &flag, &cmd);

				/*prevent record append or modify on EOF,*/
				/*since inpScore is initialized to 0 (valid score)*/
				if(cmd == EXIT_CMD)
					break;

				score = 0;
				if(validateScore(inpScore, &score))
				{
					strcpy(rec->id, inpID);
					rec->score = score;
					fprintf(fs, WRITE_STR_FMT, rec->id, rec->score);

					/*set command to exit on append or modify operation success*/
					cmd = EXIT_CMD;
				}
			} while(cmd != EXIT_CMD);
		}
	} while(cmd != EXIT_CMD);
}
