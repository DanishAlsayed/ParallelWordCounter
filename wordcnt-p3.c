/************************************************************
* Description:
* Implement this keyword searching and counting task in C using multithreading and synchronization
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
#include <pthread.h>
#include <stdbool.h>
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
pthread_mutex_t buffer_lock;
pthread_mutex_t results_lock;
pthread_cond_t buffer_not_full;
pthread_cond_t buffer_not_empty;
int wordcount = 0;
int buffercount = 0;
char **buffer;
pthread_t * threads;
struct output * temporary;
bool terminate;
int line;
struct output * results;
int resultcount = 0;
char temp2[MAXLEN];
int workers;
int buffersize;

//this is the thread function that worker threads will execute upon creation
void *threadjob (void *arg){
	int totaltasks = 0; //a vraiable that keeps count of the number of tasks the thread completed
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	int i;
	char check[MAXLEN];
	//the floow of events inside this while loop is exactly as per the procedure suggested in the project description in the consumer model
	while (true){
		printf("Worker (%d): Start up. Wait for task!\n", (int)arg);
		pthread_mutex_lock(&buffer_lock);
		while (buffercount == 0)
			pthread_cond_wait(&buffer_not_empty, &buffer_lock);
		buffercount = buffercount -1;
		strcpy(temp2, buffer[buffercount]);
		if (strcmp(temp2, "___XXX___")==0){//this if statement will make the thread leave the while loop to eventually exit
			pthread_cond_signal(&buffer_not_full);
			pthread_mutex_unlock(&buffer_lock);
			break;
		}
		buffer[buffercount] = NULL;
		printf("Worker (%d): Search for keyword %s\n", (int)arg, temp2);
		results[resultcount].count = search(temp2);
		strcpy(results[resultcount].keyword, temp2);
		resultcount++;
		pthread_cond_signal(&buffer_not_full);
		pthread_mutex_unlock(&buffer_lock);
		totaltasks++;
	}
	pthread_exit((void*)totaltasks);//the exit value is the number of tasks the thread has completed
}
//a function to determine if the buffer is empty or not
bool bufferempty(){
	int counter;
	for(counter=0; counter<buffersize; counter++)
		if (buffer[counter] != NULL) return false;
	return true;
}

int main(int argc, char * argv[]){
	if (argc < 5){
		printf("Usage: ./wordcnt-p3 [number of workers] [number of buffers] [target plaintext file] [keyword file]\n");
		return 0;
	}
	workers = (int) strtol (argv[1], (char**)NULL,10);
	buffersize = (int) strtol (argv[2], (char**)NULL,10);
	target = strdup(argv[3]);
	if (workers < 1 || workers > 10){
		printf("The number of worker threads must be between 1 to 10\n");
		return 0;
	}
	if (buffersize < 1 || buffersize > 10){
		printf("The number of buffers in task pool must be between 1 to 10\n");
		return 0;
	}
	struct timespec start, end;	
	clock_gettime(CLOCK_REALTIME, &start);	//record starting time
	int i, num;
	char word[MAXLEN];
	struct output * result;
	FILE * k;
	
	terminate = false;
	//initializing the mutex locks and condition variables
	pthread_mutex_init(&buffer_lock, NULL);
	pthread_mutex_init(&results_lock, NULL);
	pthread_cond_init(&buffer_not_full , NULL);
	pthread_cond_init(&buffer_not_empty , NULL);

	k = fopen(argv[4], "r");
	fscanf(k, "%d", &line);		//read in the number of keywords
	int child[line];
	results = (struct output *) malloc(line * sizeof(struct output)); //allocate space to store the results, this array of output structure will maintain the final results
	
	
	char temp[MAXLEN];
	buffer = malloc(line * sizeof(char*));
	int pt;
	//allocating space and intializing the buffer array
	for (i = 0; i < buffersize; i++)
    		buffer[i] = malloc((MAXLEN) * sizeof(char*));

	for (i = 0; i < buffersize; i++) buffer[i] = NULL;
	//allocating space to the threads array, the array of the total number of possible threads
	threads = malloc(workers * sizeof(pthread_t));
	//creating the desired number of threads
    	for (i=0; i<workers; i++)
        	pthread_create(&threads[i], NULL, threadjob, (void*)i);
	//the floow of events inside this while loop is exactly as per the procedure suggested in the project description in the producer model
	while (wordcount < line){
		fscanf(k, "%s", temp);
		pthread_mutex_lock(&buffer_lock);
		while(buffercount == buffersize)
			pthread_cond_wait(&buffer_not_full, &buffer_lock);
		buffer[buffercount] = strdup(temp);
		wordcount++;
		buffercount++;
		pthread_cond_signal(&buffer_not_empty);
		pthread_mutex_unlock(&buffer_lock);
	}
	//the floow of events inside this while loop is exactly as per the procedure suggested in the project description to signal the threads that its time to terminate after finishing the tasks you have in hand by the sentinal words method
	while (true){
		if (bufferempty() == true){
		for (i=0; i<workers; i++){
			pthread_mutex_lock(&buffer_lock);
			while(buffercount == buffersize)
				pthread_cond_wait(&buffer_not_full, &buffer_lock);
			buffer[buffercount] = "___XXX___";
			buffercount++;
			pthread_cond_signal(&buffer_not_empty);
			pthread_mutex_unlock(&buffer_lock);
		}
		break;
		}
	}

	int workertasks[workers];//an array to store the number of tasks by each thread on the boss side
	//this for loop waits till the threads terminate and collect the number of tasks they completed
	for (i=0; i<workers; i++){
		pthread_join (threads[i], (void **) & workertasks[i]);
		
	}
	//printing the tasks completed by each thread
	for (i=0; i<workers; i++)
		printf("Worker thread %d has terminated and completed %d tasks.\n", i, workertasks[i]);
	//printing the count results
	for (i=0; i<line; i++)
		printf("%s: %d\n", results[i].keyword, results[i].count);
	
	clock_gettime(CLOCK_REALTIME, &end);
	//printing elapsed time
	printf("\nTotal elapsed time: %.2lf ms\n", getns(start, end)/1000000.0);

	//destrying the used locks and condition variables and returning occupied resources
	pthread_mutex_destroy(&buffer_lock);
	pthread_mutex_destroy(&results_lock);
	pthread_cond_destroy(&buffer_not_full);
	pthread_cond_destroy(&buffer_not_empty);
	free(buffer);
	free(threads);
	fclose(k);
	free(target);
	free(results);
	return 0;
}