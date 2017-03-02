/************************************************************
* Description:
* Implement this keyword searching and counting task in C
*************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#define MAXLEN	116

//align to 8-byte boundary
struct output {
	char keyword[MAXLEN];
	unsigned int count;
};

char * target; 	//store the filename of keyword file

//pre: input is a C string to be converted to lower case
//post: return the input C string after converting all the characters to lower case
char * lower(char * input){
	unsigned int i;	
	for (i = 0; i < strlen(input); ++i)
		input[i] = tolower(input[i]);
	return input;
}

//pre: the keyword to search for
//post: the frequency of occurrence of the keyword in the file
unsigned int search(char * keyword){
	int count = 0;
	FILE * f;
	char * pos;
	char input[MAXLEN];
	char * word = strdup(keyword);		//clone the keyword	

	lower(word);	//convert the word to lower case
	f = fopen(target, "r");		//open the file stream

    while (fscanf(f, "%s", input) != EOF){  //if not EOF
        lower(input);	//change the input to lower case
        if (strcmp(input, word) == 0)	//perfect match
			count++;
		else {
			pos = strstr(input, word); //search for substring
			if (pos != NULL) { //keyword is a substring of this string
				//if the character before keyword in this input string
				//is an alphabet, skip the checking
				if ((pos - input > 0) && (isalpha(*(pos-1)))) continue; 
				//if the character after keyword in this input string
				//is an alphabet, skip the checking
				if (((pos-input+strlen(word) < strlen(input))) 
					&& (isalpha(*(pos+strlen(word))))) continue;
				//Well, we count this keyword as the characters before 
				//and after the keyword in this input string are non-alphabets
				count++;  
			}
		}
    }
    fclose(f);
	free(word);
	
    return count;
}

//pre: Given the start and end time
//post: return the elapsed time in nanoseconds (stored in 64 bits)
inline unsigned long long getns(struct timespec start, struct timespec end) {
	return (unsigned long long)(end.tv_sec-start.tv_sec)*1000000000ULL + (end.tv_nsec-start.tv_nsec);
}

void show_error(int child_id) { 
	printf("can't create child %d (errno = %s)\n", child_id, strerror(errno));
	printf("program aborted\n");
}

void do_parent(int * child, int ind) {
	int id = waitpid(child[ind-1],NULL,0);
	//printf("Parent: Child %d with pid = %d has terminated\n", ind, (int)id);
}


int main(int argc, char * argv[]){
	if (argc < 3){
		printf("Usage: ./wordcnt0 [target plaintext file] [keyword file]\n");
		return 0;
	}

	int line, i, num;
	char word[MAXLEN];
	struct output * result;
	FILE * k;
	struct timespec start, end;

	//set the target file name
	target = strdup(argv[1]);

	clock_gettime(CLOCK_REALTIME, &start);	//record starting time
	k = fopen(argv[2], "r");
	fscanf(k, "%d", &line);		//read in the number of keywords
	int child[line];
	//allocate space to store the results
	result = (struct output *) malloc(line * sizeof(struct output));	
	pid_t pid;
	int pfd[2];
	pipe (pfd);
	//perform the search
	for (i = 0; i <line; ++i) {
		fscanf(k, "%s", word);
				
		pid=fork();
			if (pid<0) {
			// this block is entered if can't create child
			show_error(i);
			exit(-1);
			}
		 else if (pid==0) {
			// this block is entered by child only
			//printf("child process created");
			//printf("%s",word);
			result[i].count = search(word);
			strcpy(result[i].keyword, word);
			write(pfd[1], (void*)&result[i], sizeof(struct output));
			// quit the process explicitly
			exit(0);
		}
		else {
			child[i-1]=pid;
		}
	}
	fclose(k);
	clock_gettime(CLOCK_REALTIME, &end);	//record ending time

	//output the result to stdout
	for (i = 0; i < line; ++i)
		do_parent(child, i);
	for (i = 0; i <line; i++){
		read(pfd[0], (void*)&result[i], sizeof(struct output));
		printf("%s : %d\n", result[i].keyword, result[i].count);
	}
	
	//print timing
	printf("\nTotal elapsed time: %.2lf ms\n", getns(start, end)/1000000.0);
	
	free(target);
	free(result);
	return 0;
}