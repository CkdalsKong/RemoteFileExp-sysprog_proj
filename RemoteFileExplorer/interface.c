#include <stdio.h>
#include <curses.h>
#include <stdlib.h>
#include <sys/statvfs.h>
#include <string.h>
#include "display.h"
#include "interface.h"

extern int ctrlCol;
extern char* curdir;
extern int nameCol;
#define ROW 30
WINDOW *alertwin;
WINDOW *mypad;

void alert(char *msg){
	
	int r, c;
	char i;
	
	alertwin = newwin(10,40,10,20);
	for(c=0;c<40;c++){			//int box(alertwin,'#','#');로 대체가능
		mvwprintw(alertwin, 0, c,"#");
		mvwprintw(alertwin, 9, c,"#");
	}
	for(r=0;r<10;r++){
		mvwprintw(alertwin, r, 0, "#");
		mvwprintw(alertwin, r, 39, "#");
	}
	
	
	mvwprintw(alertwin, 3, 2, "%s", msg);
	mvwprintw(alertwin, 7, 2, "y : ok");
	touchwin(alertwin);
	wrefresh(alertwin);
	
	if((i = getch()) == 'y'){
		delwin(alertwin);
	}
}

void alerti(char *msg, char* useri){
	
	alertwin = newwin(10,40,10,20);
	box(alertwin,'#','#');
	
	mvwprintw(alertwin, 3, 2, "%-36s", msg);
	wmove(alertwin, 7, 2);
	echo();
	nocbreak();
	touchwin(alertwin);
	wrefresh(alertwin);
	wgetnstr(alertwin, useri, 100);
	noecho();
	cbreak();
}

void showctrl(){
	
	int i=1;
	
	mvprintw(i++,ctrlCol, "How to Use");
	mvprintw(i++,ctrlCol, "============");
	mvprintw(i++,ctrlCol, "Move Cursor");
	mvprintw(i++,ctrlCol, ": Up & Down");
	mvprintw(i++,ctrlCol, "");
	mvprintw(i++,ctrlCol, "Go into Dir");
	mvprintw(i++,ctrlCol, ": Enter");
	mvprintw(i++,ctrlCol, "");
	mvprintw(i++,ctrlCol, "To parentDir");
	mvprintw(i++,ctrlCol, ": Backspace");
	mvprintw(i++,ctrlCol, "");
	mvprintw(i++,ctrlCol, "Copy");
	mvprintw(i++,ctrlCol, ": \'c\'");
	mvprintw(i++,ctrlCol, "Rename");
	mvprintw(i++,ctrlCol, ": \'r\'");
	mvprintw(i++,ctrlCol, "");
	mvprintw(i++,ctrlCol, "Detail man");
	mvprintw(i++,ctrlCol, ": \'h\'");
	mvprintw(i++,ctrlCol, "Quit program");
	mvprintw(i++,ctrlCol, ": \'Q\'");
}

void loadscr(){
	printScr();
	printDir(curdir);
	showctrl();
	refresh();
}

void loadMan(){
	
	FILE *fp = NULL;
	int i=0;
	char buf[80];
	int maxpage = 1;
	
	fp = fopen("manual.txt", "r");
	if(fp == NULL){
		fprintf(stderr, "manual.txt doesn't exist. Aborting...\n");
		exit(1);
	}
	
	mypad = newpad(maxpage*30,80);
	
	while(fgets(buf,80,fp)){
		mvwprintw(mypad,i++,0,buf);
		if(i>=maxpage*30){
			fprintf(stderr,"manual.txt is longer than pad\n");
			break;
		}
	}
	
	fclose(fp);
}

void showMan(){
	
	int page = 1;
	int maxpage=2;
	int stop = 0;
	
	prefresh(mypad, 0,0,0,0,29,79);
	while(!stop){
		touchwin(mypad);
		switch(getch()){
			case KEY_LEFT:
				if(page>1){
					clear();
					page--;
					prefresh(mypad, page*30-30,0,0,0,29,79);
				}
				break;
			case KEY_RIGHT:
				if(page<maxpage){
					clear();
					page++;
					prefresh(mypad, page*30-30,0,0,0,29,79);
				}
				break;
			case 'q':
				erase();
				stop = 1;
				break;
			default:
				continue;
		}
	}
	
}
void print_memory_space(char* filepath)
{
	struct statvfs s;
	if(statvfs(filepath, &s) != -1)
		{
			long long block_size = s.f_bsize; // 파일 시스템 블록 크기
			long long free_blocks = s.f_bfree; // 유효 블록 수
			long long free_space = block_size * free_blocks; // 사용가능한 저장공간
			long long total_blocks = s.f_blocks;
			char str[32];
			int length = 64;
			init_pair(2, COLOR_WHITE, COLOR_RED);
			init_pair(3, COLOR_WHITE, COLOR_BLUE);
			sprintf(str, "%lld", free_space);
			//mvprintw(ROW - 2, nameCol + 1, "Memory Space : %s", str);
			
			double used_ratio = 1.0 - (double)free_blocks / total_blocks;
			int graph_width = (int)(length * used_ratio);
			int percentage = (int)(used_ratio * 100);
			
			char pct_str[16];
			snprintf(pct_str, sizeof(pct_str), "%d%%", percentage);
			int pct_pos = nameCol + 1 + graph_width / 2 - (int)(strlen(pct_str) / 2);
			//mvprintw(ROW - 2, nameCol + 1, "Memory Space");
			//attron(A_BOLD | COLOR_PAIR(1));
			//mvprintw(ROW - 2, pct_pos, "%s", pct_str);
			//attroff(A_BOLD | COLOR_PAIR(1));
			
			for(int i = 0 ; i < length ; i ++)
				{
					if(i < graph_width)
						{
							attron(A_BOLD | COLOR_PAIR(1));
							mvprintw(ROW - 2, nameCol + 1 + i, " ");
							attroff(A_BOLD | COLOR_PAIR(1));
						}
					else
						{
							attron(A_BOLD | COLOR_PAIR(3));
							mvprintw(ROW - 2, nameCol + 1 + i, " ");
							attroff(A_BOLD | COLOR_PAIR(3));
						}
				}
			//mvprintw(ROW - 2, nameCol + 1, "Memory Space");
			attron(A_BOLD | COLOR_PAIR(1));
			mvprintw(ROW - 2, pct_pos, "%s", pct_str);
			attroff(A_BOLD | COLOR_PAIR(1));
		}
}