#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <pwd.h>
#include <curses.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include "display.h"
#include "sorting.h"
#include "file_management.h"
#include "interface.h"

#define ROW 30
#define COL 80
#define BLANK "                    "
#define COPYMODE 0644
#define MALLOC(p, s) \
if (!(p = malloc(s))) {\
	perror("malloc() error");\
	exit(1);\
}

extern int startRow;
extern int nameCol;
extern int timeCol;
extern int sizeCol;
extern int typeCol;
extern char *curdir;			//현재 디렉토리 위치
extern char *dirstack[100];
extern int stackcount;
extern char *filenames[100];
/* 현재 directory 안에 존재하는 항목 수 */
extern int fileCount;
extern sflag;


void printDir(char* dirname) {
	DIR *dir_ptr;
	struct dirent *dirinfo;
	struct stat fileinfo;
	char cur_dir[4096];
	char path[8192];
	startRow = 5;
	
	freeFilenames();
	if (strcmp(dirname, ".."))
		dirname = checkDir(dirname);
	if (dirname == NULL) {
		return;
	}
	
	if (chdir(dirname) != 0) {
		mvprintw(LINES - 1, nameCol + 1, "chdir(%s) error: %s",dirname, strerror(errno));
		return;
	}
	
	if (getcwd(cur_dir, sizeof(cur_dir)) != NULL) {
		attron(A_BOLD);
		if (!strcmp(dirname, "..")) {
			if (stackcount > 1) {
				free(dirstack[stackcount]);
				mvprintw(1, 27, BLANK);
				mvprintw(1, 27, dirstack[--stackcount]);
			}
		}
		else {
			stackpush(dirname);
			mvprintw(1, 27, BLANK);
			mvprintw(1, 27, dirname);
		}
		attroff(A_BOLD);
	}
	else {
		mvprintw(LINES - 1, nameCol + 1, "getcwd() error: %s", strerror(errno));
		return;
	}
	
	// 맨 밑에 현재 경로 표시
	mvprintw(ROW - 4, nameCol + 1, cur_dir);
	
	dir_ptr = opendir(cur_dir);
	if (dir_ptr == NULL) {
		mvprintw(LINES - 1, nameCol + 1, "opendir() error: %s", strerror(errno));
		return;
	}
	else {
		while((dirinfo = readdir(dir_ptr)) != NULL) {
			if (dirinfo->d_name[0] == '.') continue;
			if (strcmp(dirinfo->d_name, ".") == 0  ||
				strcmp(dirinfo->d_name, "..") == 0 ||
				strcmp(dirinfo->d_name, "") == 0)
				continue;
			MALLOC(filenames[fileCount], sizeof(dirinfo->d_name));
			strcpy(filenames[fileCount++], dirinfo->d_name);
			
		}
		sort();
		
		for(int i = 0; i < fileCount; i++) {
			if (i > 19) continue;
			snprintf(path, sizeof(path), "%s/%s", cur_dir, filenames[i]);
			doStat(path, filenames[i]);
			startRow++;
		}
	}
	if (fileCount == 0) {
		mvprintw(startRow + 8, nameCol + 1, "directory is empty");
	}
	
	refresh();
}


void printScr() {
	int i, j;
	for(i = 0; i < COL; i++) {
		mvprintw(0, i, "-");
		if (i > 11){
			mvprintw(2, i, "-");
			mvprintw(4, i, "-");
			mvprintw(ROW - 3, i, "-");
			mvprintw(ROW - 5, i, "-");
		}
		mvprintw(ROW - 1, i, "-");
	}
	for(i = 1; i < ROW - 1; i++) {
		mvprintw(i, 0, "|");
		mvprintw(i, nameCol-1, "|");
		mvprintw(i, COL-1, "|");
		if (i == 3 || (i > 4 && i < ROW - 5)) {
			mvprintw(i, timeCol-1, "|");
			mvprintw(i, sizeCol-1, "|");
			mvprintw(i, typeCol-1, "|");
		}
	}
	
	mvprintw(3, nameCol + 7, "File_Name");
	mvprintw(3, timeCol + 1, "Modified Time");
	mvprintw(3, sizeCol + 1, "File_Size");
	mvprintw(3, typeCol + 1, "File_Type");
	
	refresh();
}

char* checkDir(char* dirname) {
	char resolved_dir[4096];
	
	mvprintw(LINES, nameCol + 1, dirname);
	if (realpath(dirname, resolved_dir) == NULL) {
		mvprintw(LINES - 1, nameCol + 1, "realpath error : %s", dirname, strerror(errno));
		return NULL;
	}
	else if (access(dirname, F_OK) == -1) {
		mvprintw(LINES - 1, nameCol + 1, "not exist directory : %s", dirname, strerror(errno));
		return NULL;
	}
	else if (access(dirname, R_OK | X_OK) == -1) {
		mvprintw(LINES - 1, nameCol + 1, "access error: %s", dirname, strerror(errno));
		return NULL;
	}
	
	strcpy(dirname, resolved_dir);
	return dirname;
}

void doStat(char* path, char* filename) {
	struct stat info;
	
	if (stat(path, &info) == -1) {
		mvprintw(LINES - 1, nameCol + 1, "stat( %s ) error: %s", path, strerror(errno));
		return;
	}
	else {
		if (S_ISDIR(info.st_mode) || S_ISREG(info.st_mode))
			printFileinfo(filename, &info);
		else {
			startRow--;
			return;
		}
	}
}

void printFileinfo(char* filename, struct stat* info) {
	printTime(info);
	printSize(info);
	printType(info);
	if (S_ISDIR(info->st_mode))
		mvprintw(startRow, nameCol + 1, "[%.24s]", filename);
	else
		mvprintw(startRow, nameCol, "%.24s", filename);
}

void printSize(struct stat* info) {
	long size = info->st_size;
	
	if (size < 1024)
		mvprintw(startRow, sizeCol, "%5ldBytes", info->st_size);
	else if (size < 1024*1024)
		mvprintw(startRow, sizeCol, "%8.2fKB", (double)info->st_size / (1024));
	else if (size < 1024*1024*1024)
		mvprintw(startRow, sizeCol, "%8.2fMB", (double)info->st_size / (1024*1024));
	else
		mvprintw(startRow, sizeCol, "%8.2fGB", (double)info->st_size / (1024*1024*1024));
}


void printTime(struct stat* info) {
	char *ctime();
	mvprintw(startRow, timeCol + 1, "%.12s", 4+ctime(&info->st_mtime));
}


void printType(struct stat* info) {
	if (S_ISDIR(info->st_mode))
		mvprintw(startRow, typeCol + 1, "directory");
	else if (S_ISREG(info->st_mode))
		mvprintw(startRow, typeCol + 6, "file");
	else
		mvprintw(startRow, typeCol + 3, "unknown");
}

void moveCur() {
	int ch;
	int finishRow, curRow, curCol, rowMax;
	char des[1024];
	
	keypad(stdscr, TRUE);
	finishRow = startRow;
	curRow = 5;
	curCol = nameCol + 1;
	if (fileCount > 20)
		rowMax = ROW - 5;
	else rowMax = 4 + fileCount;
	
	while(1) {
		move(curRow, curCol);
		if (fileCount != 0)
			highlight(filenames[curRow - 5], curRow, 1);
		
		ch = getch();
		switch (ch) {
			case KEY_UP:
				if (curRow > 5) {
					highlight(filenames[curRow - 5], curRow, 0);
					curRow--;
				} break;
			
			case KEY_DOWN:
				if (curRow < rowMax) {
					highlight(filenames[curRow - 5], curRow, 0);
					curRow++;
				} break;
			
			
			//case KEY_LEFT:
			//case KEY_RIGHT:
			case KEY_ENTER:
			case '\n'     :
				curdir = filenames[curRow - 5];
				printDir(curdir);
				curRow = 5;
				
				if (fileCount > 20)
					rowMax = ROW - 5;
				else rowMax = 4 + fileCount;
				break;
			
			case KEY_BACKSPACE:
			case 127		  :
				if (checkHome("."))
					break;
				else
					printDir("..");
				curRow = 5;
				if (fileCount > 20)
					rowMax = ROW - 5;
				else rowMax = 4 + fileCount;
				break; // home일 경우 메시지 출력 구현하기
			case 'c':
				//strcpy(des, "copy_of_");
				//strcat(des, filenames[curRow-5]);
				
				alerti("Type name of new copy and Enter", des);
				copy1(filenames[curRow-5], des);
				loadscr();
				break;
			case 'a':					//테스트용
				alert("This is not a drill.");
				loadscr();
				break;
			case 'h':
				showMan();
				loadscr();
				break;
			case 'r':
				rname(filenames[curRow-5]);
				loadscr();
				break;
			case 's':
				sflag = 1;
				printDir(curdir);
				break;
			case 'S':
				sflag = 2;
				printDir(curdir);
				break;
			
			default:
				continue;
		}
	}
}

void highlight(char* filename, int row, int flag) {
	struct stat info;
	
	if (stat(filename, &info) == -1)
		mvprintw(LINES - 1, nameCol + 1, "stat(%s) error: %s", filename, strerror(errno));
	
	if (flag == 1)
		attron(A_BOLD | COLOR_PAIR(1));
	if (S_ISDIR(info.st_mode))
		mvprintw(row, nameCol + 1, "[%.24s]", filename);
	else
		mvprintw(row, nameCol, "%.24s",filename);
	attroff(A_BOLD | COLOR_PAIR(1));
}

void freeFilenames() {
	for(int i = 0; i < fileCount; i++) {
		free(filenames[i]);
	}
	fileCount = 0;
	clear();
	printScr();
}

int checkHome(char* dirname) {
	struct passwd *pw = getpwuid(getuid());
	char *homedir = pw->pw_dir;
	ino_t curinode = get_inode(dirname);
	ino_t homeinode = get_inode(homedir);
	
	if (curinode == homeinode)
		return 1;
	else return 0;
}

ino_t get_inode(char* fname) {
	struct stat info;
	if (stat(fname, &info) == -1) {
		fprintf(stderr, "Cannot stat ");
		perror(fname);
		exit(1);
	}
	return info.st_ino;
}

void stackpush(char *dirname) {
	MALLOC(dirstack[++stackcount], sizeof(*dirname));
	strcpy(dirstack[stackcount], dirname);
}

