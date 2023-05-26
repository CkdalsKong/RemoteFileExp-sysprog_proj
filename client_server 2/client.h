#include <unistd.h>
#define PORT 9123
#define BUF_SIZE 4096
#define ROW 30
#define COL 80
#define MALLOC(p, s) \
if (!(p = malloc(s))) {\
	perror("malloc() error");\
	exit(1);\
}

typedef struct FileInfo{
	char filename[256];
	char filetype[64];
	off_t filesize;
	char modtime[64];
	char curdir[256];
	char path[256];
} FileInfo;

FileInfo dirlist[100];
static int dirCount = 0;
static int memoryspace;
static char *servdirpath;
static char *servcurdir;

void printDir2();
void receiveDirinfo(int sock);
void parseDir(char *message);
void display_results(int end, int sock);
void printSize_r(int i);
void highlight_r(char *filename, int row, int flag);
void client(char *hostname);
void printScr_r();