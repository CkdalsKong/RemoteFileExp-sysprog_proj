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
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/statvfs.h>


#define ROW 30
#define COL 80
#define BLANK "                    "
#define COPYMODE 0644
#define MALLOC(p, s) \
if (!(p = malloc(s))) {\
	perror("malloc() error");\
	exit(1);\
}

char *dirstack[100];
int stackcount = 0;
char *filenames[100];
int startRow = 5;
int nameCol = 14;
int timeCol = 42;
int sizeCol = 58;
int typeCol = 69;
int ctrlCol = 1;
int sflag = 0;
char *curdir;			//현재 디렉토리 위치


/* 현재 directory 안에 존재하는 항목 수 */
int fileCount = 0;

WINDOW *alertwin;
WINDOW *mypad;
WINDOW *flist;

/* parameter로 전달받은 경로에 존재하는 directory, file들을 출력 */
void printDir(char* dirname);

/* 프로그램 기본 화면 출력 */
void printScr();

/* directory 확인 */
char* checkDir(char* dirname);

/* stat에 path전달해주고, filename은 프로그램 화면에 출력위해 전달 */
void doStat(char* path, char* filename);

/* stat정보들을 통해 file 이름, 수정시간, 크기, 종류들을 화면상에 출력 */
void printFileinfo(char* filename, struct stat* info);

/* stat.st_size를 통해 byte, KB, MB, GB 형태로 파일크기 출력 */
void printSize(struct stat* info);

/*  */
void printTime(struct stat* info);

/*  */
void printType(struct stat* info);

/*  */
void moveCur();

/* 현재 row에 존재하는 filename을 flag를 통해 highlight on(1), off(0) */
void highlight(char* filename, int row, int flag);

/* 디렉토리가 변경되었을 때 이전 디렉토리의 내용들을 초기화 */
void freeFilenames();

/* 현재 디렉토리가 home인지 확인 */
int checkHome(char* dirname);

/* stat을 이용해 fname의 inode number return */
ino_t get_inode(char* fname);

/* 현재 디렉토리의 이름 dirstack에 전달 */
void stackpush(char* dirname);

/* sflag를 통한 디렉토리 정렬 */
void sort();

void swap(int i, int j);

int compare(char*, char*);

void copy1(char*, char*);	//복제
void alert(char*);			//입력없는 알림창
void alerti(char*, char*);	//입력있는 알림창
void loadscr();			//탐색기 폴더 새로고침
void showctrl();		//좌측 사이드바 조작법 표시
void loadMan();			//매뉴얼 페이지 만들어놓기
void showMan();			//매뉴얼 페이지 표시
void rname(char*);		//이름 변경

void print_memory_space(char* filepath);
void file_delete(int curRow);
void create_directory();
void find_file();

int main(int argc, char *argv[]) {
	struct passwd *pw = getpwuid(getuid());
	char *homedir = pw->pw_dir;

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

	while(1) {
		moveCur();
		
	}
	delwin(flist);
	delwin(mypad);
	endwin();
	
	return 0;
}
void printScr() {
    struct dirent *dirinfo;
    DIR *dir_ptr;
    char cur_dir[4096];
    getcwd(cur_dir, sizeof(cur_dir));
    dir_ptr = opendir(cur_dir);
    dirinfo = readdir(dir_ptr);
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
	}
	for(i=0;i<100;i++){
		mvwprintw(flist, i, timeCol-nameCol-1, "|");
		mvwprintw(flist, i, sizeCol-nameCol-1, "|");
		mvwprintw(flist, i, typeCol-nameCol-1, "|");
	}

	mvprintw(3, timeCol-1, "|");
	mvprintw(3, sizeCol-1, "|");
	mvprintw(3, typeCol-1, "|");
	mvprintw(3, nameCol + 7, "File_Name");
	mvprintw(3, timeCol + 1, "Modified Time");
	mvprintw(3, sizeCol + 1, "File_Size");
	mvprintw(3, typeCol + 1, "File_Type");
    print_memory_space(dirinfo->d_name);
	
	refresh();
}

void printDir(char *dirname) {
	DIR *dir_ptr;
	struct dirent *dirinfo;
	struct stat fileinfo;
	char cur_dir[4096];
	char path[8192];
	startRow = 0;
	
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
			snprintf(path, sizeof(path), "%s/%s", cur_dir, filenames[i]);
			doStat(path, filenames[i]);
			startRow++;
		}
	}
	if (fileCount == 0) {
		mvwprintw(flist, 8, nameCol + 1, "directory is empty");
	}
	
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

void doStat(char *path, char *filename) {
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

void printFileinfo(char*filename, struct stat* info) {
	printTime(info);
	printSize(info);
	printType(info);
	if (S_ISDIR(info->st_mode))
		mvwprintw(flist, startRow, 1, "[%.24s]", filename);
	else
		mvwprintw(flist, startRow, 0, "%.24s", filename);

	mvwprintw(flist, startRow, timeCol-nameCol-1, "|");
	mvwprintw(flist, startRow, sizeCol-nameCol-1, "|");
	mvwprintw(flist, startRow, typeCol-nameCol-1, "|");	
}

void printSize(struct stat* info) {
	long size = info->st_size;
	
	if (size < 1024)
		mvwprintw(flist, startRow, sizeCol-nameCol, "%5ldBytes", info->st_size);
	else if (size < 1024*1024)
		mvwprintw(flist, startRow, sizeCol-nameCol, "%8.2fKB", (double)info->st_size);
	else if (size < 1024*1024*1024)
		mvwprintw(flist, startRow, sizeCol-nameCol, "%8.2fMB", (double)info->st_size);
	else
		mvwprintw(flist, startRow, sizeCol-nameCol, "%8.2fGB", (double)info->st_size);
}

void printTime(struct stat* info) {
	char *ctime();
	mvwprintw(flist, startRow, timeCol -nameCol + 1, "%.12s", 4+ctime(&info->st_mtime));
}

void printType(struct stat* info) {
	if (S_ISDIR(info->st_mode))
		mvwprintw(flist, startRow, typeCol -nameCol + 1, "directory");
	else if (S_ISREG(info->st_mode))
		mvwprintw(flist, startRow, typeCol -nameCol + 6, "file");
	else
		mvwprintw(flist, startRow, typeCol -nameCol + 3, "unknown");
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

void moveCur() {
	int ch;
	int finishRow, curRow, curCol, rowMax, curWinLoc;
	char des[1024];

	keypad(flist, TRUE);
	startRow = 0;
	finishRow = startRow;
	curRow = 0;
	curCol = 0;
	rowMax = fileCount-1;
	curWinLoc = 0;
	
	while(1) {
		wmove(flist, curRow, curCol);
		if (fileCount != 0)
			highlight(filenames[curRow], curRow, 1);
		
		touchwin(flist);
		prefresh(flist, curWinLoc,0 , 5,nameCol , 24,79);
		ch = wgetch(flist);
		switch (ch) {
			case KEY_UP:
				if (curRow >= 0) {
					highlight(filenames[curRow], curRow, 0);
					curRow--;
				}
				if (curRow < curWinLoc){
					curWinLoc = curRow;
				}
				break;
			
			case KEY_DOWN:
				if (curRow < rowMax) {
					highlight(filenames[curRow], curRow, 0);
					curRow++;
				}
				if (curRow > curWinLoc + 19){
					curWinLoc = curRow - 19;
				}
				break;
			
			
			//case KEY_LEFT:
			//case KEY_RIGHT:
			case KEY_ENTER:
			case '\n'     :
				curdir = filenames[curRow];
				printDir(curdir);
				curRow = 0;
				curWinLoc = 0;
				
				rowMax = fileCount - 1;
				break;
			
			case KEY_BACKSPACE:
			case 127		  :
				if (checkHome("."))
					break;
				else
					printDir("..");
				curRow = 0;
				rowMax = fileCount - 1;
				break; // home일 경우 메시지 출력 구현하기
			case 'c':
				//strcpy(des, "copy_of_");
				//strcat(des, filenames[curRow-5]);

				alerti("Type name of new copy and Enter", des);
				copy1(filenames[curRow], des);
				//loadscr();
				break;
            case 'd':
                file_delete(curRow);
                //loadscr();
                break;
            case 'm':
                create_directory();
                break;
	    	case 'f':
				find_file();
				//loadscr();
		break;
            case 'a':                   //테스트용
				alert("This is not a drill.");
				loadscr();
				break;
			case 'h':
				showMan();
				loadscr();
				break;
			case 'r':
				rname(filenames[curRow]);
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

void highlight(char *filename, int row, int flag) {
	struct stat info;
	
	if (stat(filename, &info) == -1)
		mvprintw(LINES - 1, nameCol + 1, "stat(%s) error: %s", filename, strerror(errno));
	
	if (flag == 1)
		wattron(flist, A_BOLD | COLOR_PAIR(1));
		if (S_ISDIR(info.st_mode))
		mvwprintw(flist, row, 1, "[%.24s]", filename);
		else
		mvwprintw(flist, row, 0, "%.24s", filename);
		wattroff(flist, A_BOLD | COLOR_PAIR(1));
	}

void freeFilenames() {
	for(int i = 0; i < fileCount; i++) {
		free(filenames[i]);
	}
	fileCount = 0;
	wclear(flist);
	//printScr();
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

void find_file()
{
	char search_str[256];
	int i;
	int found = 0;
	char result[2048] = "";

	alerti("Enter the search keyword: ", search_str);
	for(i = 0 ; i < fileCount ; i ++)
	{
		if((strstr(filenames[i], search_str)) != NULL)
		{
			strcat(result, "Found: ");
			strcat(result, filenames[i]);
			strcat(result, "\n");
			found++;
		}
	}
	if(found == 0)
	{
		alert("No matching files found.");
	}
	else
	{
		alert(result);
		//alert("Press any key to continue.");
	}
}

void file_delete(int curRow)
{
    char path[4096];
    getcwd(path, sizeof(path));
    strcat(path, "/");
    strcat(path, filenames[curRow]);

    // 파일 삭제
    if ((remove(path)) == -1)
    {
        fprintf(stderr, "Failed to delete the file %s", filenames[curRow]);
    }
}

void create_directory()
{
    char new_dir_name[256];
    char cwd[4096];
    char path[8192];

    alerti("Enter the name of the new directory", new_dir_name);
    getcwd(cwd, sizeof(cwd));

    snprintf(path, sizeof(path), "%s/%s", cwd, new_dir_name);

    if (mkdir(path, 0755) == -1)
    {
        fprintf(stderr, "Error creating directory %s", new_dir_name);
    }
    else
    {
        //loadscr();
    }
}

void copy1(char *src, char *des){

	int newpid;
	int in_fd, out_fd, n_chars;
	char buf[4096];
	char msg[200];
	int i, found;

	if ((newpid = fork()) == -1){
		perror("fork");
	}else if ( newpid == 0){

		found = 0;
		for(i=0;i<fileCount;i++){
			if(strcmp(des,filenames[i])==0){
				found = 1;
				break;
			}
		}
	
		if(found){
			strcpy(msg, des);
			strcat(msg, " already exists. Copying aborted...");
			alert(msg);
			exit(17);
		}
		if ((in_fd = open(src, O_RDONLY)) == -1){
			fprintf(stderr, "Cannot open %s\n", src);
			exit(1);
		}
	
		if ((out_fd = creat(des, COPYMODE)) == -1){
			fprintf(stderr, "Cannot creat %s\n", des);
			exit(1);
		}

		while ((n_chars = read(in_fd, buf, 4096)) > 0){
			if (write(out_fd, buf, n_chars) != n_chars){
				fprintf(stderr, "write error to %s\n", des);
				exit(1);
			}
		}

		if (n_chars == -1){
			fprintf(stderr, "read error from %s", src);
			exit(1);
		}

		if (close(in_fd) == -1 || close(out_fd) == -1){
			fprintf(stderr, "error closing file");
			exit(1);
		}

		exit(17);
	}
	else{
		wait(NULL);
	}
}

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
void loadscr(){
	printScr();
	printDir(curdir);
	showctrl();
	refresh();
	touchwin(flist);
	prefresh(flist, 0,0 , 5,nameCol , 24,79);
}

void showctrl(){

	int i=1;
	
	mvprintw(i++,ctrlCol, "How to Use");
	mvprintw(i++,ctrlCol, "==========");
	mvprintw(i++,ctrlCol, "Arrows:");
	mvprintw(i++,ctrlCol, "");
	mvprintw(i++,ctrlCol, " Up&Down:");
	mvprintw(i++,ctrlCol, "  Move Cursor");
	mvprintw(i++,ctrlCol, "");
	mvprintw(i++,ctrlCol, " Left&Right");
	mvprintw(i++,ctrlCol, "  Move Page");
	mvprintw(i++,ctrlCol, "");
	mvprintw(i++,ctrlCol, "Enter:");
	mvprintw(i++,ctrlCol, " Go into Dir");
	mvprintw(i++,ctrlCol, "");
	mvprintw(i++,ctrlCol, "Backspace:");
	mvprintw(i++,ctrlCol, " To Parent dir");
	mvprintw(i++,ctrlCol, "");
	mvprintw(i++,ctrlCol, "c:copy");
	mvprintw(i++,ctrlCol, "a:alert");
	mvprintw(i++,ctrlCol, "r:rename");
	mvprintw(i++,ctrlCol, "h:help");
	mvprintw(i++,ctrlCol, "f:Favorite");
	mvprintw(i++,ctrlCol, "");
	mvprintw(i++,ctrlCol, "Ctrl+c/z:");
	mvprintw(i++,ctrlCol, " Quit");
	
}

void loadMan(){

	FILE *fp = NULL;
	int i=0;
	char buf[80];
	int maxpage = 1;

	fp = fopen("manual.txt", "r");
	if(fp == NULL){
		fprintf(stderr, "manual,txt doesn't exist. Aborting...\n");
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

void rname(char* src){

	char newname[25];
	int newpid;
	char msg[200];
	int i, found;
	
	if((newpid = fork()) == -1){
		perror("fork");

	} else if(newpid == 0) {

		alerti("Type new name of the file",newname);
		
		found = 0;
		for(i=0;i<fileCount;i++){
			if(strcmp(newname,filenames[i])==0){
				found = 1;
				break;
			}
		}
	
		if(found){
			strcpy(msg, newname);
			strcat(msg, " already exists. Rename aborted...");
			alert(msg);
			exit(17);
		}

		if(rename(src,newname) == 0){
			exit(17);
		} else {
			fprintf(stderr, "rename() failed");
			exit(0);
		}

	} else {
		wait(NULL);
	}
}	

void sort() {
	int t;
	int i;
	for (int i = 0; i < fileCount; i++)	 {
		for (int j = i+1; j < fileCount; j++) {
			if (sflag == 1) {
				if (compare(filenames[i], filenames[j]) > 0)
					swap(i, j);
			}
			if (sflag == 2) {
				if (compare(filenames[i], filenames[j]) < 0)
					swap(i, j);
			}
		}
	}
}

int compare(char* a, char* b) {
	for (int i = 0; ; i++) {
		if (a[i]==0 && b[i]==0) return 0;
		if (a[i]==0) return -1;
		if (b[i]==0) return 1;
		if (a[i] - b[i] > 0) return 1;
		if (a[i] - b[i] < 0) return -1;
	}
	return 0;
}

void swap(int i, int j)	 {
	char *temp;
	MALLOC(temp, sizeof(filenames[i]))
	strcpy(temp, filenames[i]);
	strcpy(filenames[i], filenames[j]);
	strcpy(filenames[j], temp);
}