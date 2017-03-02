/************************************************************
* Description:
* Implement this keyword searching and counting task in C
*************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

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

int main(int argc, char * argv[]){
	if (argc < 3){
		printf("Usage: ./wordcnt0 [target plaintext file] [keyword file]\n");
		return 0;
	}

	int line, i;
	char word[MAXLEN];
	struct output * result;
	FILE * k;
	struct timespec start, end;

	//set the target file name
	target = strdup(argv[1]);

	clock_gettime(CLOCK_REALTIME, &start);	//record starting time
	k = fopen(argv[2], "r");
	fscanf(k, "%d", &line);		//read in the number of keywords
	
	//allocate space to store the results
	result = (struct output *) malloc(line * sizeof(struct output));	
	
	//perform the search
	for (i = 0; i < line; ++i) {
		fscanf(k, "%s", word);
		result[i].count = search(word);
		strcpy(result[i].keyword, word);
	}
	fclose(k);
	clock_gettime(CLOCK_REALTIME, &end);	//record ending time
	
	//output the result to stdout
	for (i = 0; i <line; ++i)
		printf("%s : %d\n", result[i].keyword, result[i].count);	
	//print timing
	printf("\nTotal elapsed time: %.2lf ms\n", getns(start, end)/1000000.0);
	
	free(target);
	free(result);
	return 0;
}
