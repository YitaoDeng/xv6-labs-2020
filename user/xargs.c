#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

int
main(int argc, char *argv[])
{
	char* args[MAXARG];
	for(int i=1; i<argc; i++){
		args[i-1] = argv[i];
	}
	char buf[512];
	read(0, buf, sizeof(buf));
	char* p = buf;
	int buf_len = strlen(buf);
	for(int i=0;i<buf_len;i++){
		if(buf[i] == '\n'){
			buf[i] = '\0';
			args[argc-1] = p;
			if(fork() == 0){
				exec(args[0], args);
			}
			else{
				wait(0);
				p = buf+i+1;
			}
		}
	}
	if(p!=buf+strlen(buf)){
		args[argc-1] = p;
		if(fork() == 0){
			exec(args[0], args);
		}
		else{
			wait(0);
		}
	}
  exit(0);
}
