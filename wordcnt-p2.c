/************************************************************
compilation and execution: gcc wordcnt-p2.c -o wordcnt-p2
./wordcnt [number of workers0] [test filename] [keyword filename]
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



#include <signal.h>



#include <stdbool.h>



#define MAXLEN	116







//align to 8-byte boundary







struct output

{



    char keyword[MAXLEN];



    unsigned int count;



    int cid;



};







char * target; 	//store the filename of keyword file







//pre: input is a C string to be converted to lower case



//post: return the input C string after converting all the characters to lower case



char * lower(char * input)

{



    unsigned int i;



    for (i = 0; i < strlen(input); ++i)



        input[i] = tolower(input[i]);



    return input;



}







//pre: the keyword to search for



//post: the frequency of occurrence of the keyword in the file



unsigned int search(char * keyword)

{



    int count = 0;



    FILE * f;



    char * pos;



    char input[MAXLEN];



    char * word = strdup(keyword);		//clone the keyword







    lower(word);	//convert the word to lower case



    f = fopen(target, "r");		//open the file stream







    while (fscanf(f, "%s", input) != EOF)   //if not EOF

    {



        lower(input);	//change the input to lower case



        if (strcmp(input, word) == 0)	//perfect match



            count++;



        else

        {



            pos = strstr(input, word); //search for substring



            if (pos != NULL)   //keyword is a substring of this string

            {



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



inline unsigned long long getns(struct timespec start, struct timespec end)

{



    return (unsigned long long)(end.tv_sec-start.tv_sec)*1000000000ULL + (end.tv_nsec-start.tv_nsec);



}







void show_error(int child_id)

{



    printf("can't create child %d (errno = %s)\n", child_id, strerror(errno));



    printf("program aborted\n");



}







void do_parent(int * child, int ind)

{



    int id = waitpid(child[ind-1],NULL,0);



    //printf("Parent: Child %d with pid = %d has terminated\n", ind, (int)id);



}







bool usrready = false;



bool intready = false;



int task[2];



int data[2];



pid_t children_ids [9];



int tasks_per_children[9];



char word[MAXLEN];







int line;



struct output * all_results;



FILE * k;

//this is the handler for SIGUSR1. Here the function search(word) is called after reading the word from the task pipe
//then it writes the answer is put in the modified output type structure along with other useful information like process id and written to the data pipe. It also does the related displays

void sigusr1_handler(int signum)

{



    if (signum == SIGUSR1)

    {







        char word[MAXLEN];



        int count;



        struct output * result;



        result = (struct output *) malloc(sizeof(struct output));



        read(task[0], word, MAXLEN*sizeof(char));



        //word = result->keyword;



        printf("Child(%d) : search for keyword \"%s\"\n",(int)getpid(),word);



        count = search(word);



        result[0].cid = (int)getpid();



        //printf("in handler: word: %s, count: %d \n",word,count);



        strcpy(result[0].keyword, word);

        result[0].count=count;



        //printf("in handler word: %s, count: %d, child id: %d\n",result[0].keyword, result[0].count, result[0].cid);



        write(data[1], result, sizeof(struct output));



    }



}





//This is the handler for SIGINT, it does the termination and the related display

void sigint_handler(int signum)

{



    if (signum == SIGINT)

    {



        printf("Process %d is interrupted by ^C. Finish all task. Bye Bye\n", (int)getpid());



        exit(0);



    }



}



int position = 0;



int i = 0;





//a function to fill an array with the number of tasks each process did

void fill_tasks(int pid, int size)

{



    for (i=0; i<size; i++)



        if (children_ids[i] == pid)

        {



            tasks_per_children[i] ++;



            break;



        }



}







int main(int argc, char * argv[])

{



    if (argc < 4)

    {



        printf("Usage: ./wordcnt0  [num of processes] [target plaintext file] [keyword file]\n");



        return 0;



    }



    struct timespec start, end;



    int children = (int) strtol (argv[1], (char**)NULL,10);



    //printf("children: %d", children);



    struct sigaction sa;



    //argv[2] = "key3.txt";



    //argv[1] = "test3.txt";



    pipe (task);



    pipe (data);



    clock_gettime(CLOCK_REALTIME, &start);



    k = fopen(argv[3], "r");



    fscanf(k, "%d", &line);



    all_results = (struct output *) malloc (line*sizeof(struct output));



    target = strdup(argv[2]);



//========installing the above made handlers========



    sigaction(SIGUSR1, NULL, &sa);



    sa.sa_handler = sigusr1_handler;



    sigaction(SIGUSR1, &sa, NULL);



    sigaction(SIGINT, NULL, &sa);



    sa.sa_handler = sigint_handler;



    sigaction(SIGINT, &sa, NULL);

//================================================

    pid_t child_check;

    char word_check[MAXLEN];

    bool all_read = false;

    int x = 0;

//making the desired number of children and assigning an initial word to each

    if (children >1 && children <10)

    {







        for (x = 0; x<children; x++)

        {



            if (line==(x)) all_read=true;



            fscanf(k, "%s", word);



            //printf("word in file is: %s\n",word);



            child_check = fork();



            children_ids[x] = child_check;



            //write(task[1], &word, MAXLEN*sizeof(char));



            if (child_check < 0)

            {



                printf("error");



            }



            else if (child_check == 0)

            {



                printf("Child(%d) : Start up. Wait for task!\n", (int)getpid());



                while (1) sleep(10);



            }







            else

            {





                if (!all_read)

                {



                    printf("Parent: place keyword \"%s\" in task queue\n", word);



                    write(task[1], &word, MAXLEN*sizeof(char));



                    kill(child_check, SIGUSR1);

                }

                //else printf("all words are read\n");



            }

            //printf("word: %s, word_check: %s\n", word, word_check);

            //strcpy(word_check, word);



        }



    }



    else

    {



        printf("The number of worker processes must be between 1 to 10\n");



        exit(0);



    }



    position=0;



    all_results = (struct output *) malloc(line * sizeof(struct output));



    for (i=0; i<line; i++){

        all_results[i].cid = -1;

    }



    int read_count=0;

//a while loop to handle further distribution of the tasks to the child processes/workers and collect results as they are produced

    while(all_results [line-1].cid < 0)

    //while (position < line)

    {

        //if (line==x+1) all_read=true;



        struct output * temp;



        temp = (struct output *) malloc(line * sizeof(struct output));



        //printf("Going to read: \n");



        if (read_count <= line){

            read(data[0], (void*)&temp[0], sizeof(struct output));

            read_count++;

            //position = position+1;

        }



        //printf("Read cid: %d\n", temp[0].cid);

        //printf("Position: %d\n", position);

        all_results [position] = temp [0];



        fill_tasks(all_results[position].cid, children);



        printf("Parent : Get result of \"%s\"\n", all_results[position].keyword);



        //printf("allresults[line-1]: %d\n", all_results[line-1].cid);



        fscanf(k, "%s", word);

        if(!all_read)

        {



            printf("Parent: place keyword \"%s\" in task queue\n", word);



            write(task[1], &word, MAXLEN*sizeof(char));



            kill(child_check, SIGUSR1);

            //position++;



            //x++;

        }

        position = position+1;







    }



    //sending SIGINT to the workers

    for (i=0; i<children; i++)



        if (all_results [line-1].cid >= 0)



            kill(children_ids[i], SIGINT);
//waiting for the workers to terminate



    for (i=0; i<children; i++)



        waitpid(children_ids[i], NULL,0);



    fclose(k);
//displaying which worker did how many tasks



    for (i=0; i<children; i++)



        printf("Child process %d terminated and completed %d tasks\n",children_ids[i], tasks_per_children[i]);
//word count display



    for (i=0; i<line; i++)



        printf("%s : %d\n", all_results[i].keyword, all_results[i].count);







    clock_gettime(CLOCK_REALTIME, &end);

//displaying elapsed time

    printf("\nTotal elapsed time: %.2lf ms\n", getns(start, end)/1000000.0);

    free(target);

    free(all_results);



    return 0;



}