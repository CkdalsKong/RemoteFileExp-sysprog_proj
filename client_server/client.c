#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <curses.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pwd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>
#include "client.h"
#include "server.h"

extern int nameCol;
extern int timeCol;
extern int sizeCol;
extern int typeCol;
extern char *curdir;
extern int sflag;
WINDOW* ilist;

void client(char *hostname) {
	int sock, pip[2];
	char message[BUF_SIZE];
	int str_len, end;
	struct sockaddr_in serv_adr;
	struct hostent *server;

	sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock == -1)
		error_handling("socket() error");
	
	server = gethostbyname(hostname);
	if (server == NULL)
		error_handling("gethostbyname() error!");
	
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_port = htons(PORT);
	memcpy(&serv_adr.sin_addr.s_addr, server->h_addr_list[0], server->h_length);
	
	if (connect(sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
		error_handling("connect() error!");
	
	if (pipe(pip) == -1)
		error_handling("pipe() error!");
	
	switch (fork()) {
		case -1:
			error_handling("fork() error!");
		case 0 :
			close(pip[0]);
			strcpy(message, "homedir");
			write(sock, message, strlen(message));
			while (1) {
				dirCount = 0;
				memset(dirlist, 0, dirCount * sizeof(FileInfo));
				
				receiveDir(sock);
				
				write(pip[1], &dirCount, sizeof(int));
				write(pip[1], dirlist, dirCount * sizeof(FileInfo));
			}
			break;
			
		default:
			close(pip[1]);
			end = pip[0];
			display_results(end, sock);
	}	
	endwin();
	close(sock);
}

void display_results(int end, int sock) {
	int ch;
	int curRow, curCol, rowMax, curPadLoc;

	initscr();
	resize_term(ROW + 1, COL);
	cbreak();
	noecho();
	start_color();
	init_pair(1, COLOR_BLUE, COLOR_WHITE);

	ilist = newpad(100,65);

	keypad(ilist, TRUE);
	clear();
	printScr_r();
	curRow = 0;
	curCol = 0;
	read(end, &dirCount, sizeof(int));
	read(end, dirlist, dirCount * sizeof(FileInfo));
	mvprintw(LINES - 1, 0, "read success");
	printDir2();
	
	rowMax = dirCount - 1;
	curPadLoc = 0;
	
	while(1) {
		touchwin(ilist);
		wmove(ilist, curRow, curCol);
		curdir = dirlist[curRow].filename;
		if (dirCount != 0)
			highlight_r(curdir, curRow, 1);
	
		prefresh(ilist, curPadLoc,0 , 5,nameCol , 24, 78);
		ch = wgetch(ilist);
		switch (ch) {
			case KEY_UP:
				if (curRow > 0) {
					highlight_r(curdir, curRow, 0);
					curRow--;
				} 
				if (curRow < curPadLoc){
					curPadLoc = curRow;
				} break;
			
			case KEY_DOWN:
				if (curRow < rowMax) {
					highlight_r(curdir, curRow, 0);
					curRow++;
				}
				if(curRow > curPadLoc + 19) {
					curPadLoc = curRow - 19;
				}
				break;
			case KEY_ENTER:
			case '\n'     :
				write(sock, curdir, strlen(curdir));
				
				memset(dirlist, 0, dirCount * sizeof(FileInfo));
				ssize_t bytes_read = read(end, &dirCount, sizeof(int));
				if (bytes_read < 0) {
					error_handling("read() error!");
				}
				
				bytes_read = read(end, dirlist, dirCount * sizeof(FileInfo));
				if (bytes_read < 0) {
					error_handling("read() error!");
				}
				printDir2();
				curRow = 0;
				curPadLoc = 0;
				
				rowMax = dirCount - 1;

				break;
			
			case KEY_BACKSPACE:
			case 127		  :
				write(sock, "..", strlen(".."));
				
				memset(dirlist, 0, dirCount * sizeof(FileInfo));
				read(end, &dirCount, sizeof(int));
				read(end, dirlist, dirCount * sizeof(FileInfo));
				printDir2();
				curRow = 0;
				
				rowMax = dirCount - 1;
				break;
			/*
			case 's':
				sflag = 1;
				printDir2();
				break;
			case 'S':
				sflag = 2;
				printDir2();
				break;
			*/
			default:
				continue;
		}
	}

	refresh();
	delwin(ilist);
}

void receiveDir(int sock) {
	char message[BUF_SIZE];
	ssize_t recv_size;
	int count = 0;
	FILE *fp;
	fp = fopen("output.txt", "a");
	
	while ((recv_size = read(sock, message, BUF_SIZE - 1)) > 0) {
		message[recv_size] = '\0';
		if (!strcmp(message, "END_OF_DIRECTORY")) {
			break;
		}
		
		parseDir(message);
		fprintf(fp,"Name: %s | Type: %s | Size: %lld bytes | ModTime: %s | CurDir: %s | Path: %s\n",
			dirlist[dirCount-1].filename ,dirlist[dirCount-1].filetype, dirlist[dirCount-1].filesize, dirlist[dirCount-1].modtime, dirlist[dirCount-1].curdir, dirlist[dirCount-1].path);
		memset(message, 0, sizeof(message));
		write(sock, "done", strlen("done"));
	}
	fclose(fp);
}

void parseDir(char *message) {
	// 데이터를 FileInfo 구조체에 파싱
	char *ptr = strstr(message, " | Type: ");
	strncpy(dirlist[dirCount].filename, &message[6], ptr - &message[6]);
	dirlist[dirCount].filename[ptr - &message[6]] = '\0'; // null 문자 추가
	sscanf(ptr, " | Type: %s | Size: %lld bytes | ModTime: %[^|] | CurDir: %[^|] | Path: %s\n",
		dirlist[dirCount].filetype, &dirlist[dirCount].filesize, dirlist[dirCount].modtime, dirlist[dirCount].curdir, dirlist[dirCount].path);
	dirCount++;
}

void printScr_r() {
	int i, j;
	for(i = 0; i < COL; i++) {
		mvprintw(0, i, "-");
		if (i > 12){
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

	for(i=0;i<100;i++){
		mvwprintw(ilist, i, timeCol-nameCol-1, "|");
		mvwprintw(ilist, i, sizeCol-nameCol-1, "|");
		mvwprintw(ilist, i, typeCol-nameCol-1, "|");
	}
	
	refresh();
}

void printDir2() {
	clear();
	wclear(ilist);
	printScr_r();

	attron(A_BOLD);
	mvprintw(1, 27, dirlist[0].curdir);
	attroff(A_BOLD);
	
	mvprintw(ROW - 4, nameCol + 1, dirlist[0].path);
	if (dirCount == 0) {
		mvwprintw(ilist, 5, 2, "directory is empty");
	}
	for (int i = 0; i < dirCount; i++) {
		printSize_r(i);
		mvwprintw(ilist, i, timeCol-nameCol, dirlist[i].modtime);
		mvwprintw(ilist, i, typeCol-nameCol, dirlist[i].filetype);
		if (!strcmp(dirlist[i].filetype, "directory"))
			mvwprintw(ilist, i, 1, "[%.24s]",dirlist[i].filename);
		else
			mvwprintw(ilist, i, 1, "%.24s",dirlist[i].filename);
	}
}

void highlight_r(char *filename, int row, int flag) {
	
	if (flag == 1)
		wattron(ilist, A_BOLD | COLOR_PAIR(1));
	if (!strcmp(dirlist[row].filetype, "directory"))
		mvwprintw(ilist, row, 1, "[%.24s]", filename);
	else
		mvwprintw(ilist, row, 1, "%.24s",filename);
	wattroff(ilist, A_BOLD | COLOR_PAIR(1));
}

void printSize_r(int i) {
	long size = dirlist[i].filesize;
	
	if (size < 1024)
		mvwprintw(ilist, i, sizeCol-nameCol, "%5ldBytes", size);
	else if (size < 1024*1024)
		mvwprintw(ilist, i, sizeCol-nameCol, "%8.2fKB", (double)size / 1024);
	else if (size < 1024*1024*1024)
		mvwprintw(ilist, i, sizeCol-nameCol, "%8.2fMB", (double)size / 1024*1024);
	else
		mvwprintw(ilist, i, sizeCol-nameCol, "%8.2fGB", (double)size / 1024*1024*1024);
}

