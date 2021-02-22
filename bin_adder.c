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

typedef enum {idle, want_in, in_cs} state;
extern int turn;
int listid;
int stateid;
int *listptr;
state *stateptr;

void critical_section(){


}

void remainder_section(){


}


void cleanup(){
	shmdt(listptr);
	shmdt(stateptr);
}
int main(int argc, char *argv[]){	
	
	int index = atoi(argv[1]);
	int depth = atoi(argv[2]);
	int list_size = atoi(argv[3]);
	int max_proc = atoi(argv[4]);

	//request shared memory
	key_t key = ftok("./README", 'a');
	listid = shmget(key, sizeof(int) * list_size, IPC_EXCL);
	if (listid == -1){
		perror("Shared memory could not be created");
		printf("exiting\n\n");
		cleanup();
		exit(0);
	}
	
	//attach shared memory
	listptr = (int *)shmat(listid, 0, 0);
	
	if (listptr == (int *) -1) {
		perror("Shared memory could not be attached");
	}
	

	//request shared memory
	key_t key2 = ftok(".", 'a');
	stateid = shmget(key2, sizeof(state) * max_proc, IPC_EXCL);
	if (stateid == -1){
		perror("Shared memory could not be got - state");
		printf("exiting\n\n");
		cleanup();
		exit(0);
	}
	
	//attach shared memory
	stateptr = (state *)shmat(stateid, 0, 0);
	
	if (stateptr == (state *) -1) {
		perror("Shared memory could not be attached");
	}


	printf("success i think\n\n");
	cleanup();

	return 0;
}
