#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>// wait()
#include <stdlib.h> // exit()
#include <sys/types.h>
#include <stdarg.h> // dynamic argument
#include <string.h> // string array
//#include "fileReader.c"


// double w can change everything

int u = 0;

int fileReader(int bincount, float w, float min, char *Ifile){
	//change max and min
	float num;
	FILE *fptr;
    fptr = fopen(Ifile, "r");
    int histArr[bincount];
    //array initialize
    for (int i= 0; i < bincount; i++){
    	histArr[i] = 0;
    }
    if(fptr == NULL)
   	{ 
        printf("\nError opening file\n"); 
        exit (1); 
    }

   	// Inside this make histogram array
   	while(fscanf(fptr,"%f\n", &num) != EOF){
   		for(int i = 0; i < bincount; i++)
   		{
   			if( num >= (((i)*w)+min) && num < ((((i)+1)*w)+min) ){
   				histArr[i] = histArr[i] + 1;
   				//printf("loop number: %d\t number: %f\n", i%bincount, histArr[i%bincount]);
   				break;
   			}

   			if( i+1 == bincount && num == (((i+1)*w)+min)){
   				histArr[i] = histArr[i] + 1;
   				//printf("loop number: %d\t number: %f\n", i%bincount, histArr[i%bincount]);
   				break;
   			}
   		}
   	}

   	fclose(fptr);
   	//intermediate File creation
   	FILE *wptr;
   	char str[255];
   	sprintf(str,"IntermediateFile%d.txt", u);
   	wptr = fopen(str, "w");
    if(wptr == NULL)
   	{ 
        printf("\nError when opening file\n"); 
        exit (1); 
    } 

    u = u + 1;
    // Inside this make histogram file
    for(int i = 0; i < bincount; i++)
    {
    	fprintf(wptr,"%d\n", histArr[i]);
	}
   
  	fclose(wptr);  
   	return 0;
}


int processCreator(int bincount, float w, float min, int n, char strs[][255]){
	
	if (n != 0){
		pid_t pid = fork();

		if(pid == 0){
			//Read data here
			char Ifile[255];
			strcpy(Ifile, strs[n-1]);

			n = n - 1;

			fileReader(bincount, w, min, Ifile);
			// for every recursive call strs[a++] a = 0
			processCreator(bincount, w, min, n, strs);
			//Send data to parent
			exit(0);
		}
		else if(pid > 0){
			wait(NULL);
			//Send data to parent
			//printf("after waiting%d---%d\n", getpid(), getppid());
			//printf("%d\n",n );
		}
		else{
			perror("fork() error");
			exit(-1);
		}
	}
	else{
		return 0;
	}
	return 0;
}


int phistogram(float minvalue, float maxvalue, int bincount, int N, char files[][255]){

	//width calculation
	float w = (maxvalue - minvalue) / bincount;

	/*int fileCount = N + 1;

	char files[N][255];

	for(int i = 0; i<fileCount; i++){
		printf("Enter fileeee:\t");
		scanf("%s", &files[i]);
	}

	for(int i = 0; i<N+1; i++){
		printf("Entered file:\t");
		printf("%s\n", files[i]);
	}*/

	char bugfix[255];
	strcpy(bugfix, files[N]);

	processCreator(bincount, w, minvalue, N, files);
	/////
	// Open one write and read all to N and add them up
	/////

	// initialize output file variables
	int outBin[bincount];
	for (int i= 0; i < bincount; i++){
    	outBin[i] = 0;
    }

    int interCount = 0;

    for(int c = 0; c < N; c++){
	    
		int num;
		FILE *fptr;

		char str[255];
		sprintf(str,"IntermediateFile%d.txt", interCount);
	    fptr = fopen(str, "r");
	    if(fptr == NULL)
	   	{ 
	        printf("\nError when opening file\n"); 
	        exit (1); 
	    }
	    interCount = interCount + 1;

	    // copy and add all values to outputfile from interfiles
	    int i = 0;
	   	while(fscanf(fptr,"%d\n", &num) != EOF){
	   			outBin[i] = outBin[i] + num;
	   			i = i + 1;	
	   	}
	 	
	   	fclose(fptr);
	}

   	FILE *fptr;
   	fptr = fopen(bugfix, "w");
    if(fptr == NULL)
   	{ 
        printf("\nError when opening file\n"); 
        exit (1); 
    } 

    // Inside this make one and only true histogram to rule them all
    for(int i = 0; i < bincount; i++)
    {
    	fprintf(fptr,"binnumber: %d\t%d\n",i+1 , outBin[i]);
	}
   
  	fclose(fptr);

	return 0;
}

int main(int arg1, char *arg2[]){

	//int a = 10, b = 16, c = 6;
	float a, b, c;
	int N;

	///////////////////
	a = atoi(arg2[1]); 
    b = atoi(arg2[2]); 
    c = atoi(arg2[3]); 
    N = atoi(arg2[4]); 
    ///////////////////


    int fileCount = N + 1;

	char files[N][255];

	for(int i = 0; i<fileCount; i++){
		sprintf(files[i], "%s", arg2[i+5]);
	}

	/*for(int i = 0; i<N+1; i++){
		printf("Entered file:\t");
		printf("%s\n", files[i]);
	}*/

	phistogram(a, b, c, N, files);
	return 0;
}