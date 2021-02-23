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

int listid;
int stateid;
int turnid;
int *listptr, *turn;
state *stateptr;

void cleanup(){
	shmdt(listptr);
	shmdt(stateptr);
	shmdt(turn);
	exit(0);
}

void kill_handler(){
	cleanup();

}
void critical_section(){


}

void remainder_section(){


}

int int_power(int base, int power){
	//integer exponent function 
	int result = 1;
	int i;
	if (power == 0){
		return 1;	//base^0 always = 1
	}
	for (i = power; i > 0; i--){
		result *= base;
	}
	return result;
}

int main(int argc, char *argv[]){	
	signal(SIGKILL, kill_handler);
	int index = atoi(argv[1]);
	int depth = atoi(argv[2]);
	int list_size = atoi(argv[3]);
	int max_proc = atoi(argv[4]);
	int id = atoi(argv[5]);

	//request shm
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

	key_t key3 = ftok("./Makefile", 'a');
	turnid = shmget(key3, sizeof(int), IPC_EXCL);
	if (turnid == -1){
		perror("Shared memory could not be created");
		printf("exiting\n\n");
		cleanup();
		exit(0);
	}
	
	//attach shared memory
	turn = (int *)shmat(turnid, 0, 0);
	
	if (turn == (int *) -1) {
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
	int next_index = index + list_size/int_power(2, depth);

	
	printf("in child proc index: %d next-index: %d  depth: %d nums: %d proc: %d flag id: %d adding %d to %d \n", index, next_index, depth, list_size, max_proc, id, listptr[index], listptr[next_index]);
	

	listptr[index] = listptr[index] + listptr[next_index];
	//following code is from solution 4 from notes
	/*
	int j;
	do {
		do {
			stateptr[id] = want_in;
			j=turn[0];
			while (j != id){
				j = (stateptr[j] != idle) ? *turn : (j + 1) % max_proc;
			}
			stateptr[id] = in_cs;
			for (j = 0; j < max_proc; j++){
				if ( ( j != id) && ( stateptr[j] == in_cs) ){
					break;
				}
			}
		} while ((j < max_proc) || ( *turn != id && stateptr[*turn] != idle));
		*turn = id;
		critical_section();

		j= (turn[0] +1) % max_proc;
		while (stateptr[j] == idle){
			j = (j + 1) % max_proc;
		}
		*turn = j;
		stateptr[id] = idle;
		remainder_section();
	} while (1);
	*/
	printf("success i think\n\n");
	cleanup();

	return 0;
}
