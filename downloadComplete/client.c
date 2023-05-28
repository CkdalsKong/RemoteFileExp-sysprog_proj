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
#include "sorting.h"
#include "interface.h"

extern int nameCol;
extern int timeCol;
extern int sizeCol;
extern int typeCol;
extern char *curdir;
extern int sflag;
WINDOW* ilist;

#define BLANK "                                  "

void client(char *hostname) {
	int pip[2];
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
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	signal(SIGUSR2, sig2_handle);
	
	pid = fork();
	switch (pid) {
		case -1:
			error_handling("fork() error!");
		case 0 :
			close(pip[0]);
			signal(SIGUSR1, signal_handle_child);
			strcpy(message, "homedir");
			write(sock, message, strlen(message));
			while (1) {
				dirCount = 0;
				memset(dirlist, 0, dirCount * sizeof(FileInfo));
				
				receiveDirinfo(sock);
				
				write(pip[1], &dirCount, sizeof(int));
				write(pip[1], dirlist, dirCount * sizeof(FileInfo));
			}
			break;
			
		default:
			close(pip[1]);
			signal(SIGUSR1, signal_handle_parent);
			end = pip[0];
			display_results(end, sock);
	}	
	endwin();
	close(sock);
}

void sig2_handle(int sig) {
	if (pid)
		wait(NULL);
	exit(0);
}

void signal_handle_child(int sig) {
	struct passwd *pw = getpwuid(getuid());
	char *homedir = pw->pw_dir;
	FILE *fp;
	char buf[BUF_SIZE];
	char filename[BUF_SIZE];
	char dir_path[BUF_SIZE];
	char file_path[BUF_SIZE];
	int read_size;
	long file_size;
	long remain;
	
	recv(sock, filename, BUF_SIZE, 0);
	recv(sock, &file_size, sizeof(long), 0);
	
	snprintf(dir_path, BUF_SIZE, "%s/download_by_server", homedir);
	mkdir(dir_path, 0700);
	
	snprintf(file_path, BUF_SIZE, "%s/%s", dir_path, filename);
	
	fp = fopen(file_path, "wb");
	if (fp == NULL)
		error_handling("fopen() error");
	
	remain = file_size;
	
	while((remain > 0) && ((read_size = recv(sock, buf, BUF_SIZE, 0)) >0)) {
		if (fwrite(buf, 1, read_size, fp) != read_size)
			error_handling("fwrite error");
		remain -= read_size;
	}
	
	fclose(fp);
	kill(getppid(), SIGUSR1);
}

void signal_handle_parent(int sig) {
	sigflag = 1;
}

void display_results(int end, int sock) {
	int ch;
	int curRow, curCol, rowMax, curPadLoc;
	char *temp;
	ssize_t bytes_read;
	char modemsg[BUF_SIZE];
	
	initscr();
	resize_term(ROW + 1, COL);
	cbreak();
	noecho();
	start_color();
	init_pair(1, COLOR_BLUE, COLOR_WHITE);

	ilist = newpad(100,65);

	keypad(ilist, TRUE);
	clear();
//	printScr_r();
	curRow = 0;
	curCol = 0;
	read(end, &dirCount, sizeof(int));
	read(end, dirlist, dirCount * sizeof(FileInfo));
	mvprintw(LINES - 1, 0, "read success");
	printDir2();
	
	rowMax = dirCount - 1;
	curPadLoc = 0;
	
	while(1) {
//		touchwin(ilist);
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
				}
				mvprintw(LINES - 1, nameCol + 1, BLANK);
				mvprintw(LINES - 1, nameCol + 1, "pressed: key_up");
				break;
			
			case KEY_DOWN:
				if (curRow < rowMax) {
					highlight_r(curdir, curRow, 0);
					curRow++;
				}
				if(curRow > curPadLoc + 19) {
					curPadLoc = curRow - 19;
				}
				mvprintw(LINES - 1, nameCol + 1, BLANK);
				mvprintw(LINES - 1, nameCol + 1, "pressed: key_down");
				break;
			case 'd':
				mvprintw(LINES - 1, nameCol + 1, BLANK);
				mvprintw(LINES - 1, nameCol + 1, "pressed: key_d");
				sigflag = 0;
				snprintf(modemsg, BUF_SIZE, "%s | download\n", curdir);
				write(sock, modemsg, strlen(modemsg));
				
				kill(pid, SIGUSR1);
				
				while(!sigflag) {
					sleep(1)	;
				}
				alert("Download Complete!!");
				printDir2();
				curRow = 0;
				curPadLoc = 0;
				
				break;
			case KEY_ENTER:
			case '\n'     :
				snprintf(modemsg, BUF_SIZE, "%s | Enter\n", curdir);
				write(sock, modemsg, strlen(modemsg));
				
				memset(dirlist, 0, dirCount * sizeof(FileInfo));
				bytes_read = read(end, &dirCount, sizeof(int));
				if (bytes_read < 0) {
					error_handling("read() error!");
				}
				
				bytes_read = read(end, dirlist, dirCount * sizeof(FileInfo));
				if (bytes_read < 0) {
					error_handling("read() error!");
				}
				printDir2();
				mvprintw(LINES - 1, nameCol + 1, BLANK);
				mvprintw(LINES - 1, nameCol + 1, "pressed: key_enter");
				curRow = 0;
				curPadLoc = 0;
				
				rowMax = dirCount - 1;

				break;
			
			case KEY_BACKSPACE:
			case 127		  :
				snprintf(modemsg, BUF_SIZE, "%s | Enter\n", "..");
				write(sock, modemsg, strlen(modemsg));
				
				memset(dirlist, 0, dirCount * sizeof(FileInfo));
				read(end, &dirCount, sizeof(int));
				read(end, dirlist, dirCount * sizeof(FileInfo));
				printDir2();
				mvprintw(LINES - 1, nameCol + 1, BLANK);
				mvprintw(LINES - 1, nameCol + 1, "pressed: key_backspace");
				curRow = 0;
				
				rowMax = dirCount - 1;
				break;
			case 's':
				sflag = 1;
				printDir2();
				mvprintw(LINES - 1, nameCol + 1, BLANK);
				mvprintw(LINES - 1, nameCol + 1, "pressed: key_s");
				break;
			case 'S':
				sflag = 2;
				printDir2();
				mvprintw(LINES - 1, nameCol + 1, BLANK);
				mvprintw(LINES - 1, nameCol + 1, "pressed: key_S");
				break;
			case 'Q':
				mvprintw(LINES - 1, nameCol + 1, BLANK);
				mvprintw(LINES - 1, nameCol + 1, "pressed: key_Q");
				kill(-getpid(), SIGUSR2);
				break;
			
			default:
				continue;
		}
		refresh();
	}

	
	delwin(ilist);
}

void receiveDirinfo(int sock) {
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
		fprintf(fp,"Name: %s | Type: %s | Size: %lld bytes | ModTime: %s | CurDir: %s | Path: %s | Memory: %d\n",
			dirlist[dirCount-1].filename ,dirlist[dirCount-1].filetype, dirlist[dirCount-1].filesize, dirlist[dirCount-1].modtime, dirlist[dirCount-1].curdir, dirlist[dirCount-1].path, dirlist[dirCount].memoryspace);
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
	sscanf(ptr, " | Type: %s | Size: %lld bytes | ModTime: %[^|] | CurDir: %[^|] | Path: %s | Memory: %d\n",
		dirlist[dirCount].filetype, &dirlist[dirCount].filesize, dirlist[dirCount].modtime, dirlist[dirCount].curdir, dirlist[dirCount].path, &dirlist[dirCount].memoryspace);
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
	mvprintw(1, 1, "How to use");
	mvprintw(2, 1, "============");
	mvprintw(3, 1, "DownloadFile");
	mvprintw(4, 1, ": \'d\'");
	mvprintw(6, 1, "Quit program");
	mvprintw(7, 1, ": \'Q\'");
	display_memory_space();
	
	attron(A_BOLD);
	mvprintw(1, 27, dirlist[0].curdir);
	attroff(A_BOLD);
	
	mvprintw(ROW - 4, nameCol + 1, dirlist[0].path);

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
	sort2();
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

void display_memory_space()
{
	init_pair(3, COLOR_WHITE, COLOR_BLUE);
	int length = (COL - 2) - (nameCol + 1);
	int graph_width = length * dirlist[0].memoryspace / 100;
	
	// 메모리 사용량 퍼센트를 하이라이트된 부분 중간에 표시한다.
	char pct_str[16];
	snprintf(pct_str, sizeof(pct_str), "%d%%", dirlist[0].memoryspace);
	int pct_pos = nameCol + 1 + graph_width / 2 - (int)(strlen(pct_str) / 2);

	// 그래프를 화면에 그린다.
	for (int i = 0; i < length; i++)
		{
			if (i < graph_width)
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
	
	attron(A_BOLD | COLOR_PAIR(1));
	mvprintw(ROW - 2, pct_pos, "%s", pct_str);
	attroff(A_BOLD | COLOR_PAIR(1));
}


void sort2() {
	int t;
	for (int i = 0; i < dirCount; i++)	 {
		for (int j = i+1; j < dirCount; j++) {
			if (sflag == 1) {
				if (compare(dirlist[i].filename, dirlist[j].filename) > 0)
					swap2(i, j);
			}
			if (sflag == 2) {
				if (compare(dirlist[i].filename, dirlist[j].filename) < 0)
					swap2(i, j);
			}
		}
	}
}

void swap2(int i, int j)	 {
	char *temp;
	MALLOC(temp, sizeof(dirlist[i].filename))
	strcpy(temp, dirlist[i].filename);
	strcpy(dirlist[i].filename, dirlist[j].filename);
	strcpy(dirlist[j].filename, temp);
}