#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]){
	


	FILE *fileptr;
	char ch;
	int num_count = 0;
	fileptr = fopen("nums.txt", "r");
	if (fileptr == NULL){
		printf("Error: file not found!\n\n");
		return 0;
	}
	else {
		while ((ch = fgetc(fileptr)) != EOF){
			if (ch == ' ' || ch == '\n' ){
				num_count++;
			}
		}
		printf("There were %d words found\n\n", num_count);
	}

	return 0;
}
