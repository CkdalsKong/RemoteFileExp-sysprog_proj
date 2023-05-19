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
/* 현재 directory 안에 존재하는 항목 수 */
int fileCount = 0;

