#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char buf[512];

char*
fmtname(char *path)
{
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--);
  p++;

  return p;
}

void
find(struct stat st, char* path)
{
	struct dirent de;
	switch(st.type){
  case T_FILE:
		if(strcmp(fmtname(path), buf) == 0){
			printf("%s\n", path);
		}
    break;

  case T_DIR:
		{
			int path_len = strlen(path);
			int fd = open(path, 0);
			path[path_len] = '/';
			path_len++;
			while(read(fd, &de, sizeof(de)) == sizeof(de)){
				if(de.inum == 0)
					continue;
				if(strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
					continue;
				strcpy(path+path_len, de.name);
				path[path_len+strlen(de.name)] = '\0';
				if(stat(path, &st) < 0){
					printf("find: cannot stat %s\n", buf);
					continue;
				}
				find(st, path);
			}
			break;
		}
  }
}
int
main(int argc, char *argv[])
{
	if(argc != 3){
		fprintf(2, "usage: find name\n");
		exit(1);
	}
	char path[512];
	strcpy(path, argv[1]);
	strcpy(buf, argv[2]);

	struct stat st;
  if(stat(path, &st) < 0){
    fprintf(2, "find: cannot stat %s\n", path);
    exit(1);
  }
	find(st,path);

  exit(0);
}
