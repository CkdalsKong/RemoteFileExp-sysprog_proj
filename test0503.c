#include <stdio.h>
#include <curses.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <utime.h>
#include <sys/time.h>
#include <sys/types.h>
#include <pwd.h>

#define ROW 30
#define COL 80
#define BLANK "                    "
#define MALLOC(p, s) \
if (!(p = malloc(s))) {\
	perror("malloc() error");\
	exit(1);\
}

char *dirstack[100];
int stackcount = 0;
char *filenames[100];
int startRow = 5;
int nameCol = 12;
int timeCol = 42;
int sizeCol = 58;
int typeCol = 69;

int fileCount = 0;

void printDir(char *);
void printScr();
void doStat(char *, char*);
void printFileinfo(char*, struct stat*);
void printSize(struct stat*);
void printTime(struct stat*);
void printType(struct stat*);
void moveCur();
void highlightOn(char *, int);
void highlightOff(char *, int);
void freeFilenames();
int checkHome(char*);
ino_t get_inode(char*);
void printPredir(ino_t);
void stackpush(char *);


int main(int argc, char *argv[]) {
	struct passwd *pw = getpwuid(getuid());
	char *homedir = pw->pw_dir;
	
	initscr();
	resize_term(ROW + 1, COL);
	cbreak();
	noecho();
	start_color();
	init_pair(1, COLOR_BLUE, COLOR_WHITE);
	
	printScr();
	printDir(homedir);
	
	while(1) {
		moveCur();
		
		
	}
	endwin();
	
	return 0;
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

void printDir(char *dirname) {
	DIR *dir_ptr;
	struct dirent *dirinfo;
	struct stat fileinfo;
	char cur_dir[1024];
	char path[4096];
	startRow = 5;
	
	freeFilenames();
	
	if (chdir(dirname) != 0) {
		mvprintw(LINES - 1, nameCol + 1, "chdir() error: ");
		perror(dirname);
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
		perror("getcwd() error");
		return;
	}
	
	mvprintw(ROW - 4, nameCol + 1, cur_dir);
	
	dir_ptr = opendir(cur_dir);
	if (dir_ptr == NULL) {
		perror("opendir error");
		return;
	}
	else {
		while((dirinfo = readdir(dir_ptr)) != NULL && startRow < ROW - 5) {
			if (dirinfo->d_name[0] == '.') continue;
			if (strcmp(dirinfo->d_name, ".") == 0 ||
				strcmp(dirinfo->d_name, "..") == 0)
				continue;
			MALLOC(filenames[fileCount], sizeof(dirinfo->d_name));
			strcpy(filenames[fileCount++], dirinfo->d_name);
			if (fileCount > 20) continue;
			snprintf(path, sizeof(path), "%s/%s", cur_dir, dirinfo->d_name);
			doStat(path, dirinfo->d_name);
			
			startRow++;
		}
	}
	if (fileCount == 0)
	refresh();
}

void doStat(char *path, char *filename) {
	struct stat info;
	
	if (stat(path, &info) == -1) {
		perror(path);
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

void printFileinfo(char*filename, struct stat* info) {
	printTime(info);
	printSize(info);
	printType(info);
	if (S_ISDIR(info->st_mode))
		mvprintw(startRow, nameCol + 1, "[%s]", filename);
	else
		mvprintw(startRow, nameCol, filename);
	
}

void printSize(struct stat* info) {
	long size = info->st_size;
	
	if (size < 1024)
		mvprintw(startRow, sizeCol, "%5ldBytes", info->st_size);
	else if (size < 1024*1024)
		mvprintw(startRow, sizeCol, "%8.2fKB", (double)info->st_size);
	else if (size < 1024*1024*1024)
		mvprintw(startRow, sizeCol, "%8.2fMB", (double)info->st_size);
	else
		mvprintw(startRow, sizeCol, "%8.2fGB", (double)info->st_size);
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
	
	keypad(stdscr, TRUE);
	finishRow = startRow;
	curRow = 5;
	curCol = nameCol + 1;
	if (fileCount > 20)
		rowMax = ROW - 5;
	else rowMax = 4 + fileCount;
	
	while(1) {
		move(curRow, curCol);
		highlightOn(filenames[curRow - 5], curRow);
		
		ch = getch();
		switch (ch) {
			case KEY_UP:
				if (curRow > 5) {
					highlightOff(filenames[curRow - 5], curRow);
					curRow--;
				} break;
			
			case KEY_DOWN:
				if (curRow < rowMax) {
					highlightOff(filenames[curRow - 5], curRow);
					curRow++;
				} break;
			
			case KEY_LEFT:
			case KEY_RIGHT:
			case KEY_ENTER:
			case '\n'     :
				printDir(filenames[curRow - 5]);
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
			
			default:
				continue;
		}
	}
}

void highlightOn(char *filename, int row) {
	struct stat info;
	
	if (stat(filename, &info) == -1)
		perror(filename);
	else {
		attron(A_BOLD | COLOR_PAIR(1));
		if (S_ISDIR(info.st_mode))
			mvprintw(row, nameCol + 1, "[%s]", filename);
		else
			mvprintw(row, nameCol, filename);
		attroff(A_BOLD | COLOR_PAIR(1));
	}
}

void highlightOff(char *filename, int row) {
	struct stat info;
	
	if (stat(filename, &info) == -1)
		perror(filename);
	else {
		if (S_ISDIR(info.st_mode))
			mvprintw(row, nameCol + 1, "[%s]", filename);
		else
			mvprintw(row, nameCol, filename);
	}
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