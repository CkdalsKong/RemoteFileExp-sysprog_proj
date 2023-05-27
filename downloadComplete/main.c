#include <stdio.h>
#include <curses.h>
#include <pwd.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include "file_management.h"
#include "sorting.h"
#include "interface.h"
#include "display.h"
#include "client.h"
#include "server.h"

#define ROW 30
#define COL 80
#define BLANK "                    "
#define COPYMODE 0644
#define MALLOC(p, s) \
if (!(p = malloc(s))) {\
	perror("malloc() error");\
	exit(1);\
}
int startRow = 5;
int nameCol = 14;
int timeCol = 42;
int sizeCol = 58;
int typeCol = 69;
int ctrlCol = 1;
int sflag = 0;
char *curdir;			//현재 디렉토리 위치
char *dirstack[100];
int stackcount = 0;
char *filenames[100];
char *homedir;
/* 현재 directory 안에 존재하는 항목 수 */
int fileCount = 0;
FILE *fplocal;

extern WINDOW* mypad;
extern WINDOW* flist;

int main(int argc, char *argv[]) {
	struct passwd *pw = getpwuid(getuid());
	homedir = pw->pw_dir;
	char message[64];
	
	while(1) {
		printf("local or remote(Q to quit) : ");
		scanf(" %s", message);
		if (!strcmp(message, "remote")) {
			while(1) {
				printf("server or client(Q to quit) : ");
				scanf(" %s", message);
				if (!strcmp(message, "server")) {
					server();
					break;
				}
				else if (!strcmp(message, "client")) {
					printf("Hostname : ");
					scanf(" %s", message);
					client(message);
					break;
				}
				else if (!strcmp(message, "Q")) {
					printf("Program terminated\n");
					exit(0);
				}
				else
					continue;
			}
		}
		else if (!strcmp(message, "local"))
			break;
		else if (!strcmp(message, "Q")) {
			printf("Program terminated\n");
			exit(0);
		}
		else
			continue;
	}
	
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	signal(SIGUSR2, sig2_handle);
	fplocal = fopen("localoutput.txt", "a");
	initscr();
	resize_term(ROW + 1, COL);
	cbreak();
	noecho();
	start_color();
	init_pair(1, COLOR_BLUE, COLOR_WHITE);

	flist = newpad(100, 65);
	
	loadMan();
	curdir = homedir;
	printScr();
	printDir(homedir);
	showctrl();
	moveCur();
	
	
	delwin(flist);
	delwin(mypad);
	endwin();
	
	return 0;
}
