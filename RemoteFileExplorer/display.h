#include <sys/stat.h>

/* parameter로 전달받은 경로에 존재하는 directory, file들을 출력 */
void printDir(char* dirname);

/* 프로그램 기본 화면 출력 */
void printScr();

/* directory 확인 */
char* checkDir(char* dirname);

/* stat에 filename 전달하여 파일정보 얻고 화면에 출력 */
void doStat(char* filename);

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

void freestack();