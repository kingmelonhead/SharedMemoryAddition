#define _XOPEN_SOURCE 700
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <signal.h>
#include <getopt.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <sys/wait.h>

extern int errno;

enum state {idle, want_in, in_cs};
extern int turn;
extern state flag[n];

int shmid;
int *shmptr;

void critical_section(){


}

void remainder_section(){


}

int main(int argc, char *argv[]){	
	
	//request shared memory
	key_t key = ftok("./README", 'a');
	shmid = shmget(key, sizeof(int) * 32, IPC_EXCL);
	if (shmid == -1){
		perror("Shared memory could not be created");
		printf("exiting\n\n");
		exit(0);
	}
	
	//attach shared memory
	shmptr = (int *)shmat(shmid, 0, 0);
	
	if (shmptr == (int *) -1) {
		perror("Shared memory could not be attached");
	}
	
	shmdt(shmptr);

	return 0;
}
