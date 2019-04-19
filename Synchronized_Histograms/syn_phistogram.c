#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>      /* sem_open(), sem_destroy(), sem_wait().. */
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <fcntl.h>

//if any problem occurs change initialization of file arrays as malloc


int main(int arg1, char *arg2[]) 
{
	//semaphores that are necessary for reader-writer problem
	sem_t mutex;
	sem_t wrt;

	int writeCnt = 0;
	sem_init(&mutex, 1, 1);
	sem_init(&wrt, 1, 1);

    int minVal;   //minimum val that exists in the input files
    int maxVal;   //max val that exists in the input files
    int binCount; //number of bins in the histogram
    			  //bin_width: (maxvalue - minvalue) / binCount)
    			  //first bin interval: [minVal, maxVal + bin_width) and so on.
    int N; //number of input files.
    
    minVal = atoi(arg2[1]); 
    maxVal = atoi(arg2[2]); 
    binCount = atoi(arg2[3]); 
    N = atoi(arg2[4]); 

    if(arg1 != (6 + N)){
        printf("wrong argument format \n");
        exit(EXIT_FAILURE);
    }
    else if(binCount == 0) {
        printf("bin count is zero! Program can't continue.\n");
        exit(EXIT_FAILURE);
    }


    //Both input and intermediate files are stored as arrays:
    FILE *inputFiles[N];

	//For keeping the file_name to use in file operations
	char file_name[100];

    //bin width calculation:
    int bin_width = (maxVal - minVal) / binCount;
    //initializing a two dimensional bin array to hold bins 
    //int bins[binCount][2];

    //opening the output file to write into it
	sprintf(file_name, "%s", arg2[N+5]);
    FILE *outfile = fopen (file_name, "w");

    //outfile file will use outputArray buffer to take the inputs, 
    //which's modified only by the parent process
    int outputArray[binCount];
    for(int i = 0; i < binCount; i++) {
        outputArray[i] = 0;
    }

    //this pointer's reserved starting address of the shared memory
    int *shm_start;
    int mem_size = sizeof(int)*binCount*2;
    //initialization of shared memory
    //mmap parameters: address, size(bytes), protection, visibility, file_descriptor, offset(offset's multiple of page size)
    shm_start = mmap(NULL, mem_size, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);

    if(shm_start < 0) {
    	printf("error, shared memory was not mapped");
        exit(EXIT_FAILURE);
	}

	
    //For each index of the bins array, min value of a bin is assigned in the
    //first dimension. Second dimension stands for the number of values in that bin.
	
    //memory mapping usage: bin minimum value for each odd partition bin count for each even partition
    int counter = 0;
    for(int *i = shm_start; i<shm_start+mem_size; ) {
    	*i = minVal + (bin_width * (counter++));
    	i += sizeof(int);
    	*i = 0;
    	i += sizeof(int);
    }
    
    //variables to be used to observe child process ID 
    pid_t processID;

    //collecting all input files in an array of N members 
    for(int i = 0; i < N; i++) {

    	//we use fork system call for each file to generate a child process
    	processID = fork();

    	if( processID == 0 ) {
    		//CHILD PROCESS STARTED
			//child process will WRITE the values to shared memory in a synchronized manner

	    	//input files are opened before they are evaluated
	    	sprintf(file_name, "%s", arg2[i+5]);
	    	inputFiles[i] = fopen(file_name, "r");

    		double value = 0;


            //***********************************
	    	//CIRITICAL SECTION BEGINS for WRITER
            //***********************************

	    	//scanning values in the current file(feof means end of file)
            sem_wait(&wrt);

            writeCnt++;   

            if (writeCnt == 1)     
                sem_wait(&mutex);

            sem_post(&wrt);

	    	while(fscanf(inputFiles[i], "%lf", &value) == 1) {

				//printf("value %lf was found in %s\n", value, file_name);
			   	//find the correct bin with LINEAR SEARCH
			   	int *j;
			   	int *k;
			   	for(j = shm_start, k = shm_start+sizeof(int); j<shm_start+mem_size; ) {
			   		if(j+2*sizeof(int) != shm_start+mem_size) {
				   		if((value >= (*j)) && (value < ((*j) + bin_width))) {
				   			//if searched interval is found, increment the bin's 2nd dimension
				   			*k += 1;
				   		}
			   		}
			   		else {			   			
				   		if((value >= (*j)) && (value <= ((*j) + bin_width))) {
				   			//if searched interval is found, increment the bin's 2nd dimension
				   			*k += 1;
				   		}
			   		}
		   			j += 2*sizeof(int);
		   			k += 2*sizeof(int);
			   	}
		    	//sleep(1);
	    		
	    	}
            sem_wait(&wrt); 

            writeCnt--;

            // that is, no reader is left in the critical section,
            if (writeCnt == 0) 
                sem_post(&mutex);         // writers can enter

            sem_post(&wrt); // reader leaves
            //***********************************
	    	//CIRITICAL SECTION ENDS for WRITER
            //***********************************

	    	//close the files when they're done with
	  		fclose(inputFiles[i]);
    		exit(0);
    	}
    	else if(processID < 0){
    		printf("error while forking");
    		exit(EXIT_FAILURE);
    	}
    	else {
    		wait(NULL);
    		//PARENT PROCESS STARTED
	    	//parent process will READ the values from shared memory and WRITE them in outputArray

            //***********************************
	    	//CIRITICAL SECTION BEGINS for READER
            //***********************************
            sem_wait(&mutex);

	    	//printf("in parent\n");

	    	//increase readers's count by one
	    	counter = 0;
		    for(int *j = shm_start; j < shm_start+mem_size; ) {
			            
		        //since outfile(output file) is open, we can directly put the values in it.
		        j += sizeof(int);
		        outputArray[counter] += *j;
		        *j = 0;
		        counter++;
		        j += sizeof(int);
		    }
	    	//sleep(1);

            sem_post(&mutex);
            //***********************************
	    	//CIRITICAL SECTION ENDS for READER
            //***********************************
	    	
    	}
    }
    //wait until all child processes are finished.(pid = 0)
    //int status = 0;
    //while ((processID = waitpid(-1, &status,0)) != -1) {
    //    printf("Child Process numbered %d is finished\n",processID);
    //}


    for(int i = 0; i < binCount; i++) {
	            
        //since outfile(output file) is open, we can directly put the values in it.
        printf("%d\n",outputArray[i]);
        fprintf(outfile,"%d: %d\n",i+1,outputArray[i]);
    }
	
	
    //output file closed at the end of the program
    fclose(outfile);   
    sem_destroy(&mutex);
    sem_destroy(&wrt);

    return 0;
}
