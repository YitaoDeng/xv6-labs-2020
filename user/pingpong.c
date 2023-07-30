#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
	if(argc != 1){
		fprintf(2, "usage: pingpong\n");
		exit(1);
	}
	int p[2];
	pipe(p);
	int fork_id = fork();
	int pid = getpid();
	char buf;
	if(fork_id == 0){
		// child
		read(p[0], &buf, sizeof(buf));
		close(p[0]);
		fprintf(1, "%d: received p%cng\n", pid, buf);
		buf = 'o';
		write(p[1], &buf, sizeof(buf));
		close(p[1]);
	}
	else{
		// parent
		buf = 'i';
		write(p[1], &buf, sizeof(buf));
		close(p[1]);
		wait(0);
		read(p[0], &buf, sizeof(buf));
		close(p[0]);
		fprintf(1, "%d: received p%cng\n", pid, buf);
	}
  exit(0);
}
