#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>

typedef struct ThreadArgs{
    int bin_width;
    FILE *inputFile;
    int binCount;
    int N;
    int fileIndex;
    int *binIntervals;
    int batchSize;
} ThreadArgs;

typedef struct Node{
    double *batch;
    struct Node *next;
} Node;


//adds node to the end of the list
Node * appendNode(Node **head, double *batch)
{
    /* go to the last node */
    Node *new_node = (Node*)malloc(sizeof(Node));
    Node *last_node = *head;

    new_node->next = NULL;
    new_node->batch = batch;

    if(*head == NULL) {
        *head = new_node;
        return new_node;
    }

    while(last_node->next != NULL)
        last_node = last_node->next;

    last_node->next = new_node;
 
    return new_node;
}

void printList(Node** head)
{
   	Node* tmp = *head;
   	//int counter = 0; 
    while (tmp)
    {	
    	if(tmp->next != NULL) {
	       printf("node%d->",tmp);
        }
        else {
	       printf("node%d",tmp);
        }
        tmp = tmp->next;
    }
    printf("\n");
}

//head of the linked list
Node *ll_head = NULL;

void *workerThread(void *args) {
    //scanning values in the current file(feof means end of file)

    //printf("thread starts\n");//THERE'S A PROBLEM WITH THIS LNIE

    //*********************************************
    //CRITICAL SECTION OF THE PRODUCER THREAD STARTS
    //*********************************************

    ThreadArgs *theArgs = (ThreadArgs *)args;
    double value = 0;
    //batch array initialized, which will be used to enter the file entries
    double *batch = malloc(sizeof(double) * theArgs->batchSize);

    Node *new_node = appendNode(&ll_head, batch);

    //counter that counts until the batch size is reached
    int counter = 0;
    //if read char from file is succesful, increment the matching index by 1
    while(fscanf(theArgs->inputFile, "%lf", &value) == 1) {

        if(counter == theArgs->batchSize) {
            counter = 0;
            batch = (double*)malloc((theArgs->batchSize) * sizeof(double));
            new_node = appendNode(&ll_head, batch);
        }
        new_node->batch[counter] = value;
        //printf("value %lf read. node %d's batch[%d] is %lf\n",value,new_node,counter,new_node->batch[counter]);
        //printf("new_node->batch[%d] is %d\n",counter,new_node->batch[counter]);
        counter++;
    } 

    //*********************************************
    //CRITICAL SECTION OF THE PRODUCER THREAD ENDS
    //*********************************************

    free(theArgs);
    pthread_exit(0);
}

int main(int arg1, char *arg2[]) 
{   
    int minVal;   //minimum val that exists in the input files
    int maxVal;   //max val that exists in the input files
    int binCount; //number of bins in the histogram
    			  //bin_width: (maxvalue - minvalue) / binCount)
    			  //first bin interval: [minVal, maxVal + bin_width) and so on.
    int N; //number of input files.
    int B; //number of batch size.

    minVal = atoi(arg2[1]); 
    maxVal = atoi(arg2[2]); 
    binCount = atoi(arg2[3]); 
    N = atoi(arg2[4]); 
    B = atoi(arg2[6+N]);

    if(arg1 != (7 + N)){
        printf("wrong argument format \n");
        exit(EXIT_FAILURE);
    }
    //the binCount must be at least 1.
    if(binCount == 0) {
    	printf("Bin count is zero! Program can't continue.\n");
        exit(EXIT_FAILURE);
    }

    if(B < 1 || B > 100) {
        printf("Batch size must be a value chosen between 1 and 100.\n");
        exit(EXIT_FAILURE);
    }


    //Input files are stored as arrays:
    FILE *inputFiles[N];

	//For keeping the file_name to use in file operations
	char file_name[100];

    //bin width calculation:
    int bin_width = (maxVal - minVal) / binCount;

    int binIntervals[binCount];
    for(int j = 0; j < binCount; j++) 
        binIntervals[j] = minVal + bin_width*j;


    //opening the output file to write into it
    sprintf(file_name, "%s", arg2[N+5]);
    FILE *outfile = fopen (file_name, "w");

    //i's used for each for loop
    int i;

    //id of the created threads will be stored in this array.
    pthread_t tid[N];
    //set of attributes for the created thread.

    for(i = 0; i < N; i++) {
        //input files are opened before they are evaluated
        sprintf(file_name, "%s", arg2[5+i]);
        inputFiles[i] = fopen(file_name, "r");
    }


    //indexing all input files in an array of N members 
    for(i = 0; i < N; i++) {
        
        //THREADS WILL BE CREATED AND ASSIGNED IN THIS LOOP

        /*we had to initialize a ThreadArgs struct unless we'd be unable to pass 
        all arguments to the working thread*/
        ThreadArgs *args = (ThreadArgs *) malloc(sizeof(ThreadArgs));

        //each value in the struct is assigned 
        args->bin_width = bin_width;
        args->inputFile = inputFiles[i];
        args->binCount = binCount;
        args->N = N;
        args->fileIndex = i;
        args->binIntervals = binIntervals;
        args->batchSize = B;

        //pthread_attr_init(&attr);  
        pthread_create(&tid[i], NULL, workerThread,  args); 

        //sleep(1);
    }

    //wait for all created threads to finish before main thread proceeds.
    for(i = 0; i < N; i++) {
        pthread_join(tid[i], NULL);  
        //printf("thread finished\n");
    }
    //printf("all threads are finished.\n");

    //REST WILL BE HANDLED BY THE MAIN THREAD.

    int outArray[binCount];

    for(int i = 0; i < binCount; i++) {
    	outArray[i] = 0;
    }
    //printList(&ll_head);

    //currentNode is initialized to iterate over the global linked list
    Node *currentNode = ll_head;

    //*********************************************
    //CRITICAL SECTION OF THE CONSUMER STARTS
    //*********************************************
    while(currentNode) {
        for(int i = 0; i < B; i++) {
            for(int k = 0; k < binCount; k++) {
            	if(k+1 == binCount) {
	                if((currentNode->batch[i] >= binIntervals[k]) && (currentNode->batch[i] <= binIntervals[k]+bin_width)) {
	                	//printf("value %lf put in binIntervals[%d]\n",currentNode->batch[i], k);
	                    outArray[k]++;
	                }
                }
                else {
	                if((currentNode->batch[i] >= binIntervals[k]) && (currentNode->batch[i] < binIntervals[k]+bin_width)) {
	                	//printf("value %lf put in binIntervals[%d]\n",currentNode->batch[i], k);
	                    outArray[k]++;
	                }
                }
            }
        }
        currentNode = currentNode->next;
    }

    //*********************************************
    //CRITICAL SECTION OF THE CONSUMER ENDS
    //*********************************************

	for(int i = 0; i < binCount; i++) {

        //print each binnumber(j+1): count(sum) into the outfile file.
    	printf("%d\n",outArray[i]);
        //sum = 0;
    }

    for(int i = 0; i < binCount; i++) {

        //print each binnumber(j+1): count(sum) into the outfile file.
        fprintf(outfile, "%d: %d\n", i+1, outArray[i]);
        //sum = 0;
    }

    //output file closed at the end of the program

    //close input files, since the task's done
    for(i = 0; i < N; i++)
        fclose(inputFiles[i]);
    fclose(outfile);  

    return 0; 
}