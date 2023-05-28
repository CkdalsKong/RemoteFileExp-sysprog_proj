#include <stdio.h>
#include <string.h>
#include <stdlib.h>
extern char *filenames[100];
#define MALLOC(p, s) \
if (!(p = malloc(s))) {\
	perror("malloc() error");\
	exit(1);\
}
extern int fileCount;
extern int sflag;

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

void sort() {
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



