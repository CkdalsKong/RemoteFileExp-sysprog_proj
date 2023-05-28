#include <unistd.h>

#define PORT 9123
#define BUF_SIZE 4096
#define TRUE 1
char *dirstore[100];
static int storecount = 0;
static int memory;
char *modeinfo[2];

void error_handling(char *message);
char *get_file_type(mode_t mode);
void sendDirectoryInfo(int clnt_sock, char *dirname);
void server();
struct dirent *get_dirinfo();
int memory_space(char* filepath);
void parseMessage(char *message);
void sendFile(int sock);