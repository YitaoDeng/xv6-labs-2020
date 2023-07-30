#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void prime(int cnt, int l)
{
	printf("prime %d\n", cnt);
	int r[2];
	pipe(r);
	int has_child = 0;
	int num;
	while(read(l, &num, sizeof(num)) > 0){
		if(num % cnt != 0){
			if(has_child == 0){
				if(fork() == 0){
					// child
					close(r[1]);
					prime(num, r[0]);
					close(r[0]);
				}
				else{
					// parent
					close(r[0]);
					has_child = 1;
				}
			}
			write(r[1], &num, sizeof(num));
		}
	}
	close(r[1]);
	wait(0);
}

int
main(int argc, char *argv[])
{
	if(argc != 1){
		fprintf(2, "usage: primes\n");
		exit(1);
	}
	int r[2];
	pipe(r);
	if(fork() == 0){
		// child
		close(r[1]);
		prime(2, r[0]);
		close(r[0]);
	}
	else{
		// parent
		close(r[0]);
		for(int i = 3; i <= 35; i++){
			write(r[1], &i, sizeof(i));
		}
		close(r[1]);
		wait(0);
	}
  exit(0);
}
