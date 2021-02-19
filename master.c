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


enum state { idle, want_in, in_cs};
int turn;


//used for perror
extern int errno;

//global variables, may be used directly by error handling function
pid_t *pidlist;
int max_proc = 20; //default
int max_time = 100; //default
int pid_count = 0;
int shmid;
int *shmptr;

void init_pidlist(){
	//sets all pids to zero, zero will indicate an open slot
	int i;
	for (i=0; i<max_proc; i++){
		pidlist[i] = 0;
	}

}

void display_help(){


}
int get_num_count(){
	//gets the number of numbers in the file
	int num_count = 0;
	FILE *fileptr;
	char ch;
	fileptr = fopen("nums.txt", "r");
	if (fileptr == NULL){
		printf("Error: file not found!\n\n");
		exit(0);
	}
	else {
		while ((ch = fgetc(fileptr)) != EOF){
			if (ch == '\n'){
				num_count++;
			}
		}
		printf("There were %d words found\n\n", num_count);
	}
	fclose(fileptr);
	return num_count;
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
int get_zeros(int num){
	//gets called if the number of numbers in the file is 
	bool greater_than = false;
	int k = 0;
	int zeros;
	while (!greater_than){
		if (int_power(2,k) > num){
			break;
		}
		else {
			k++;
		}
	}
	zeros = int_power(2,k) - num;
	return zeros;
	
}
int power_of_2(int num){
	//function checks to see if number of numbers in file is a power of 2
	if (num == 0){
		return 0;	//returns 0 if there are no numbers
	}
	else if (num == 1){
		return 3;	//returns 3 if there is only one number in file. This is the sum technically
	}
	else if ((num & (num-1)) == 0){
	int num_count = 0;
		return 1;	//returns 1 if there is a 2^k numbers
	}		
	else {
		return 2;	//returns 2 if there is not 2^k numbers
	}
}

void find_and_remove(pid_t pid){
	//finds passed pid in the global pid list and sets it to zero
	int i;
	for (i=0; i<max_proc; i++){
		if (pidlist[i] == pid){
			pidlist[i] = 0;
			break;
		}
	}
}

void ctrl_c_handler(){
	//function for handling ctrl c interupt
	// (insert kill all children)
	printf("signal caught\n\n");
	exit(0);
}

void child_handler(int signum){
	//function for handling when a child dies
	pid_t pid;
	int status;
	while ((pid = waitpid(-1, &status, WNOHANG)) != -1){
		pid_count--;
		find_and_remove(pid);
	}
}

void print_shm(int x){
	int i;
	for (i=0; i<x; i++){
		printf("%d\n", shmptr[i]);
	}
	printf("\n");
}

int main(int argc, char *argv[]){

	/*******************************************************************************/

	//sets up the signal handlers

	signal(SIGINT, ctrl_c_handler);
	signal(SIGCHLD, child_handler);

	/******************************************************************************/

	int *numbers;
	char int_buffer[50];
	FILE *fileptr;
	char ch;
	int total_slots, zeros, num_count, opt, childpid, i;

	/******************************************************************************/

	//pass through file, gets number of numbers
	
	num_count = get_num_count();
		
	/*****************************************************************************/

	//gets number of zeros required to make power of 2 is required

	if (power_of_2(num_count) == 0){
		printf("There were no numbers in the file\n\n");
		return 0;
	}
	else if (power_of_2(num_count) == 3){
		printf("The only number in the file is the sum\n\n");
		return 0;
	}
	else if (power_of_2(num_count) == 1){
		total_slots = num_count;
		zeros = 0;
	}
	else if (power_of_2(num_count) == 2){
		zeros = get_zeros(num_count);
		total_slots = num_count + zeros;
	}


	/*****************************************************************************/

	//gets the optional settings for time and # of children

	while ((opt = getopt(argc, argv, "hs:t:")) != -1 ){
		switch (opt){
		case 'h':
			display_help();
			exit(0);
			break;
		case 's':
			printf("setting children\n\n");
			max_proc = *optarg;
			break;
		case 't':
			printf("setting time\n\n");
			max_time = *optarg;
			break;
		default:
			printf("Error: invalid argument, calling help menu and exiting...\n\n");
			display_help();
			exit(0);
		}
	}
	
	//uses max_proc to give the pid list memory
	pidlist = malloc(sizeof(pid_t) * max_proc);
	init_pidlist();

	/*****************************************************************************/
	
	//request shared memory
	key_t key = ftok("./README", 'a');
	shmid = shmget(key, sizeof(int) * total_slots, IPC_CREAT | 0666);
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
	

	/*****************************************************************************/

	//pass through file, copy int to buffer, string to int the buffer into shared memory
	
	int counter=0;
	int shmindex=0;
	
	fileptr = fopen("nums.txt", "r");

	while ((ch = fgetc(fileptr)) != EOF){
		if (ch != '\n'){
			int_buffer[counter] = ch;
			counter++;
		}
		else {
			int_buffer[counter] = '\0';
			shmptr[shmindex] = atoi(int_buffer);
			shmindex++;
			counter=0;
		}
	}
	fclose(fileptr);
	//fills rest of slots with zeros if applicable
	if (zeros > 0){
		for (i = zeros; i>0; i--){
			shmptr[shmindex] = 0;
			shmindex++;
		}
	}

	/*****************************************************************************/

	
	shmdt(shmptr);
	printf("detached in master\n\n");
	shmctl(shmid, IPC_RMID, NULL);
	
	return 0;
}
