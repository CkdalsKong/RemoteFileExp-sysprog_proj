#include <unistd.h>

#define PORT 9123
#define BUF_SIZE 4096
char *dirstore[100];
static int storecount = 0;

void error_handling(char *message);
char *get_file_type(mode_t mode);
void sendDirectoryInfo(int clnt_sock, char *dirname);
void server();