#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <utime.h>
#include <sys/time.h>
#include <sys/types.h>
#include <pwd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>
#include "server.h"
#include "display.h"

#include <sys/statvfs.h>

#define MALLOC(p, s) \
if (!(p = malloc(s))) {\
	perror("malloc() error");\
	exit(1);\
}

void server() {
	struct passwd *pw = getpwuid(getuid());
	char *homedir = pw->pw_dir;
	char dirname[BUF_SIZE];
	char message[BUF_SIZE];
	char mode[BUF_SIZE];
	int str_len, option;
	int serv_sock, clnt_sock;
	socklen_t optlen;
	struct sockaddr_in serv_adr;
	struct sockaddr_in clnt_adr;
	
	socklen_t clnt_adr_sz;
	
	serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	if (serv_sock == -1)
		error_handling("socket() error");
	
	option = 1;
	optlen = sizeof(optlen);
	setsockopt(serv_sock, SOL_SOCKET, SO_REUSEPORT, &option, optlen);
	
	
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_port = htons(PORT);
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	if (bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
		error_handling("bind() error");
	
	if (listen(serv_sock, 1) == -1)
		error_handling("listen() error");
	
	clnt_adr_sz = sizeof(clnt_adr);
	
	clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
	if (clnt_sock == -1)
		error_handling("accept() error");
	else
		printf("Connected clinet\n");
	
	int recv_size;

    struct dirent *info = get_dirinfo();
    memory = memory_space(info->d_name);

	while ((recv_size = read(clnt_sock, message, BUF_SIZE - 1)) > 0) {
		message[recv_size] = '\0';
		printf("%s\n", message);
		if (!strcmp(message, "homedir"))
			sendDirectoryInfo(clnt_sock, homedir);
		else {
			sendDirectoryInfo(clnt_sock, message);
		}
		
		memset(message, 0, sizeof(message));
	}

	close(serv_sock);
}

void sendDirectoryInfo(int clnt_sock, char *dirname) {
	struct passwd *pw = getpwuid(getuid());
	char *homedir = pw->pw_dir;
	char *cur_dir;
	DIR *dir;
	struct dirent *dirinfo;
	struct stat fileinfo;
	ssize_t send_size;
	
	char *ctime(const time_t *);
	
	if (!strcmp(dirname, "..")) {
		if (storecount <= 1) {
			dirname = homedir;
			storecount--;
		}
	}
	if (chdir(dirname) == -1)
		error_handling("chdir() error");
	
	
	MALLOC(cur_dir, 1024*sizeof(char));
	if (getcwd(cur_dir, 1024*sizeof(char)) != NULL) {
		if (!strcmp(dirname, "..")) {
			if (storecount > 1) {
				free(dirstore[storecount--]);
				free(dirstore[storecount--]);
				dirname = dirstore[storecount];
			}
			else
				dirname = homedir;
		}
		else {
			MALLOC(dirstore[storecount], 1024 * sizeof(char));
			strcpy(dirstore[storecount++], dirname);
		}
	}
	else
		error_handling("getcwd() error!");

	
	dir = opendir(cur_dir);
	if (dir == NULL) {
		perror("opendir()");
		return;
	}
	int count = 0;
	// 디렉토리의 모든 파일을 순환하며 클라이언트에게 정보 전송
	char message[BUF_SIZE];
	while ((dirinfo = readdir(dir)) != NULL) {
		stat(dirinfo->d_name, &fileinfo);
		if (dirinfo->d_name[0] == '.')
			continue;
		if (strcmp(dirinfo->d_name, ".") == 0 || strcmp(dirinfo->d_name, "..") == 0) {
			continue;
		}
		
		// 파일의 종류 확인 및 시간 정보 구하기
		char *file_type = get_file_type(fileinfo.st_mode);
		char timebuf[64];
		snprintf(timebuf, sizeof(timebuf), "%.12s", 4+ctime(&fileinfo.st_mtime));
		
		// 파일 정보를 버퍼에 기록
		snprintf(message, sizeof(message), "Name: %s | Type: %s | Size: %lld bytes | ModTime: %s | CurDir: %s | Path: %s | Memory: %d\n",
			dirinfo->d_name, file_type, fileinfo.st_size, timebuf, dirname, cur_dir, memory);
		
		// 클라이언트에게 파일 정보 전송
		if ((send_size = write(clnt_sock, message, strlen(message))) < 0) {
			perror("send()");
			break;
		}
		printf("%s\n", message);
		memset(message, 0, sizeof(message));
		while(1) {
			read(clnt_sock, message, BUF_SIZE);
			if (!strcmp(message, "done")){
				//printf("from client: done\n");
				break;
			}
		}
		memset(message, 0, sizeof(message));
		//printf("%d\n", count++);
	}
	printf("%s read end\n", dirname);
	closedir(dir);
	free(cur_dir);
	write(clnt_sock, "END_OF_DIRECTORY", strlen("END_OF_DIRECTORY"));
}

char *get_file_type(mode_t mode) {
	switch (mode & S_IFMT) {
		case S_IFBLK:
			return "block device";
		case S_IFCHR:
			return "character device";
		case S_IFDIR:
			return "directory";
		case S_IFIFO:
			return "FIFO/pipe";
		case S_IFLNK:
			return "symlink";
		case S_IFREG:
			return "regular file";
		case S_IFSOCK:
			return "socket";
		default:
			return "unknown";
	}
}

void error_handling(char *message) {
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

struct dirent *get_dirinfo()
{
	struct dirent *dirinfo;
	DIR *dir_ptr;
	char cur_dir[4096];
	const char* home_dir = getenv("HOME");
	chdir(home_dir);
	getcwd(cur_dir, sizeof(cur_dir));
	dir_ptr = opendir(cur_dir);
	dirinfo = readdir(dir_ptr);
	
	closedir(dir_ptr);
	
	return dirinfo;
}

int memory_space(char* filepath)
{
	int percentage;
	struct statvfs s;
	if (statvfs(filepath, &s) != -1)
		{
			long long free_blocks = s.f_bfree;               // 유효 블록 수
			long long total_blocks = s.f_blocks;             // 전체 블록 수
			
			double used_ratio = 1.0 - (double)free_blocks / total_blocks; // 사용된 비율 계산
			percentage = (int)(used_ratio * 100);                     // 사용된 메모리 비율(퍼센트)
		}
	return percentage;
}