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
#include <string.h>

typedef enum { idle, want_in, in_cs} state;


//used for perror
extern int errno;

//global variables, may be used directly by error handling function
pid_t *pidlist;
int max_proc = 19; //default
int max_time = 100; //default
int pid_count = 0;
int shmid, shmid2, shmid3;
int *shmptr, *shmptr3;
state *shmptr2;

void init_pidlist(){
	//sets all pids to zero, zero will indicate an open slot
	int i;
	for (i=0; i<max_proc; i++){
		pidlist[i] = 0;
	}

}

void kill_pids(){
	//function to kill all pids on the event of early termination
	int i; 
	for (i=0; i<max_proc; i++){
		if (pidlist[i] != 0){
			kill(pidlist[i], SIGKILL);
		}
	}

}

void cleanup(){
	//used to detatch and delete all shared memory after both normal end of runtime
	//and on early termination
	shmdt(shmptr);
	shmdt(shmptr2);
	shmdt(shmptr3);
	kill_pids();
	shmctl(shmid, IPC_RMID, NULL);
	shmctl(shmid2, IPC_RMID, NULL);
	shmctl(shmid3, IPC_RMID, NULL);
	free(pidlist);
}


void display_help(){
	//displays how to use options/call program in general
	printf("\nHello, this program takes in a list of numbers from a file. Must be one number per line.\n");
	printf("The file that gets read from is nums.txt. Note: Make sure there are no empty lines at the end of the file or spaces between numbers\n");
	printf("The program is called in the following way:\n\n");
	printf("./master [-s] [max processes] [-t] [time]\n\n");
	printf(" or ./master [-h] \n\n");
	printf("The [-s] option allows you to overwrite the max number of processes allowed, this includes the parent proccess\n");
	printf("Note: if a number greater than 20 is entered, it is reset to 20. This is a max proccess allowment from the professor.\n\n");
	printf("The [-t] option allows you to overwrite the number of time that the program is allowed to run. By default this is 100 seconds.\n");
	printf("Note: if a number greater than 100 is entered, it is reset to 100. This is a max time allowment from the professor.\n\n");
}
void init_flags(){
	//used to set all flags to idle at start
	int i;
	for (i=0; i<max_proc; i++){
		shmptr2[i] = idle;

	}
	
}
int get_num_count(){
	//gets the number of numbers in the file
	int num_count = 0;
	FILE *fileptr;
	char ch;
	fileptr = fopen("nums.txt", "r");
	if (fileptr == NULL){
		perror("master: Error: file not found!\n\n");
		printf("File not found so exiting\n");
		exit(0);
	}
	else {
		while ((ch = fgetc(fileptr)) != EOF){
			if (ch == '\n'){
				num_count++;
			}
		}
		//printf("There were %d words found\n\n", num_count);
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
	pid_count--;
}

void ctrl_c_handler(){
	//function for handling ctrl c interupt
	// (insert kill all children)
	cleanup();
	exit(0);
}

void child_handler(int sig){
	//function for handling when a child dies
	pid_t pid;
	while ((pid = waitpid((pid_t)(-1), 0, WNOHANG)) > 0) {
		find_and_remove(pid);
	}
	
}

void print_shm(int x){
	//used for debugging and testing that everything is working
	int i;
	for (i=0; i<x; i++){
		printf("%d\n", shmptr[i]);
	}
	printf("\n");
}

int get_place(){
	//used to get the index of a pid in the list, used to tell the children what flag is theirs to control
	int place;
	for (place = 0; place < max_proc; place++){
		if (pidlist[place] == 0){
			return place;
		}
	}

}

int check_for_process(){
	//function to check to see if the pid_count lines up with the number of pids in the table
	int i;
	int flag = 0;
	for (i=0; i<max_proc; i++){
		if (pidlist[i] != 0){
			flag = 1;
		}
	}
	return flag;
}

void time_handler(){
	//function used to handle time out
	cleanup();
	exit(0);
}


int main(int argc, char *argv[]){


	system("touch adder_log.log");

	/*******************************************************************************/

	//sets up the signal handlers

	signal(SIGINT, ctrl_c_handler);
//	signal(SIGCHLD, child_handler);
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = child_handler;
	sigaction(SIGCHLD, &sa, NULL);
	signal(SIGALRM, time_handler);

	/******************************************************************************/

	int *numbers;
	char int_buffer[50];
	char count_string[12];
	char depth_string[12];
	char index_string[12];
	char proc_string[3];
	char id_string[3];
	char pid_string[10];
	FILE *fileptr;
	char ch;
	int total_slots, zeros, num_count, opt, i, j, loc;
	int depth = 0;
	int index = 0;
	int jump = 1;
	int temp_pid;
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

	depth = log2(total_slots);	

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
			max_proc = atoi(optarg);
			max_proc--;
			if (max_proc > 19){
				max_proc = 19;
				printf("Max process passed is too high, defaulting to 20\n");
			}
			//printf("mac proc is now %d\n", max_proc);
			break;
		case 't':
			printf("setting new time\n\n");
			max_time = atoi(optarg);
			if (max_time > 100){
				max_time = 100;
				printf("Max time passed is too high, defaulting to 100\n");
			}
			break;
		default:
			printf("Error: invalid argument, calling help menu and exiting...\n\n");
			display_help();
			exit(0);
		}
	}
	alarm(max_time);
	
	//uses max_proc to give the pid list memory
	pidlist = malloc(sizeof(pid_t) * max_proc);
	init_pidlist();


	/*****************************************************************************/
	
	//FIRST SHARED MEMORY SEGMENT IS FOR THE NUMBERS THAT ARE BEING ADDED TOGETHER
	
	//request shared memory
	key_t key = ftok("./README", 'a');
	shmid = shmget(key, sizeof(int) * total_slots, IPC_CREAT | 0666);
	if (shmid == -1){
		perror("master: Error: Shared memory could not be created");
		printf("exiting\n\n");
		cleanup();
		exit(0);
	}
	
	//attach shared memory
	shmptr = (int *)shmat(shmid, 0, 0);
	
	if (shmptr == (int *) -1) {
		perror("master: Error: Shared memory could not be attached");
		cleanup();
		exit(0);
	}

	//2ND SHARED MEMORY SEGMENT IS FOR THE PID LIST, THIS IS USED TO COORDINATE
	//THE INDEX OF EACH CHILD IN THE FLAG ARRAY SO THE PROCCESSES CAN BE TOLD 
	//WHERE EXACTLY THEIR FLAG IS IN THE ARRAY

	key_t key2 = ftok(".", 'a');
	shmid2 = shmget(key2, sizeof(state) * max_proc, IPC_CREAT | 0666);
	if (shmid2 == -1){
		perror("master: Error: Shared memory could not be created");
		printf("exiting\n\n");
		cleanup();
		exit(0);
	}
	
	//attach shared memory
	shmptr2 = (state *)shmat(shmid2, 0, 0);
	
	if (shmptr2 == (state *) -1) {
		perror("Shared memory could not be attached");
		cleanup();
		exit(0);
	}
	
	init_flags();


	//3RD SHARED MEMORY IS FOR TURN VARIABLE USED BY THE CHILDREN

	key_t key3 = ftok("./Makefile", 'a');
	shmid3 = shmget(key3, sizeof(int), IPC_CREAT | 0666);
	if (shmid3 == -1){
		perror("master: Error: Shared memory could not be created");
		printf("exiting\n\n");
		cleanup();
		exit(0);
	}
	
	//attach shared memory
	shmptr3 = (int *)shmat(shmid3, 0, 0);
	
	if (shmptr3 == (int *) -1) {
		perror("master: Error: Shared memory could not be attached");
		cleanup();
		exit(0);
	}

	shmptr3[0] = -1;

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

	sprintf(index_string, "%d", index);
	sprintf(proc_string, "%d", max_proc);
	sprintf(depth_string, "%d", depth);
	sprintf(count_string, "%d", total_slots);

	/*****************************************************************************/

	//
	//this code loops through the different depths spawning children to do work 
	//at the start of each depth it ensures all children have finished
	//
	for (i = depth; i>0; i--){
		while (pid_count != 0){
			if (check_for_process() == 0){
				//these errors were just for debugging
				//printf("Pid counter wasn't decremented properly and is being manually set to zero\n");
				//printf("becasue there are no pids in the table.\n\n");
				pid_count = 0;
			}

		}
	//	printf("entering depth: %d\n", i);
		//printf("before:\n");
		//print_shm(total_slots);
		shmptr3[0] = -1;
		sprintf(depth_string, "%d", i);
		jump *= 2;
		do {
			if (pid_count == 0){
				for (j=0; j<total_slots; j += jump){
					while (1){
						//printf("pid count - %d\n", pid_count);
						if (pid_count < max_proc){
							sprintf(index_string, "%d", j);
							pid_count++;
							loc = get_place();
							//printf("about to fork with pid of %d and a max of %d \n", pid_count, max_proc);
							pidlist[loc] = fork();
							if (pidlist[loc] == -1){
								perror("master: Error: fork failed");

							}
							if (pidlist[loc] != 0){
								temp_pid = pidlist[loc];
								break;
							}
							else if (pidlist[loc] == 0){
								sprintf(pid_string, "%d", temp_pid);
								sprintf(id_string, "%d", loc);
								//printf("forking child in depth %d\n", i);
								execl("./bin_adder", "./bin_adder", index_string, depth_string, count_string, proc_string, id_string, pid_string, (char*)0 );
							}
							
						}
					}
				}
				break;
			}
		} while (1);
		while (pid_count != 0){
			if (check_for_process() == 0){
				//errors were just used for debugging
				//printf("Pid counter wasn't decremented properly and is being manually set to zero\n");
				//printf("becasue there are no pids in the table.\n\n");
				pid_count = 0;
			}
		}
		//printf("after:\n");
		//print_shm(total_slots);
		//printf("finished pass %d\n", i);
	}
	while (pid_count != 0){
		if (check_for_process() == 0){
			pid_count = 0;
		}
	}
	printf("\nSum %d\n", shmptr[0]);
	//print_shm(total_slots);
	cleanup();
	return 0;
}
