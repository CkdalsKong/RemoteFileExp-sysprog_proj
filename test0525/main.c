#include <stdio.h>
#include <curses.h>
#include <pwd.h>
#include <unistd.h>
#include "file_management.h"
#include "sorting.h"
#include "interface.h"
#include "display.h"

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

extern WINDOW* mypad;
extern WINDOW* flist;

int main(int argc, char *argv[]) {
	struct passwd *pw = getpwuid(getuid());
	homedir = pw->pw_dir;
	
	int i, j;
	
	initscr();
	resize_term(ROW + 1, COL);
	cbreak();
	noecho();
	start_color();
	init_pair(1, COLOR_BLUE, COLOR_WHITE);

	flist = newpad(100, 66);
	
	for(i=0;i<66;i++){
		for(j=0;j<100;j++){
			mvwprintw(flist,j,i,"@");
		}
	}
	
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