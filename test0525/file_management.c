#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "interface.h"

#define COPYMODE 0644
extern int fileCount;
extern char *filenames[100];

void copy1(char *src, char *des){
	
	int newpid;
	int in_fd, out_fd, n_chars;
	char buf[4096];
	char msg[200];
	
	if ((newpid = fork()) == -1){
		perror("fork");
	}else if ( newpid == 0){
		/* #####des 이름이 이미 존재할 때 예외처리
		if(out_fd = open(des, O_RDONLY)){
		strcpy(msg,des);
		strcat(msg," already exist! Aborting copy...");
		alert(msg);
		close(out_fd);
		exit(17);
		}
		*/
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

void file_delete(int curRow)
{
	char path[4096];
	getcwd(path, sizeof(path));
	strcat(path, "/");
	strcat(path, filenames[curRow - 5]);
	
	// 파일 삭제
	if ((remove(path)) == -1)
		{
			fprintf(stderr, "Failed to delete the file %s", filenames[curRow - 5]);
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
			loadscr();
		}
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