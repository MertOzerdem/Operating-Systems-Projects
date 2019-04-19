#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>// wait()
#include <stdlib.h> // exit()
#include <sys/types.h>
#include <pthread.h> //threads
#include <string.h>

int **arr;// global variable for threads

char **files;
int o = -1;

/// use struct to pass min max bincount and filename to read
typedef struct inputs{
	float min;
	float w;
	int bincount;
	int N;
}inputs;


char *getFile(char **strs){
	o = o + 1;

	return strs[o];
}

int fileReader(int bincount, float w, float min, /*char *Ifile, */int n){
	//change max and min
	float num;

	FILE *fptr;
    fptr = fopen(getFile(files), "r");
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
   				arr[n][i] = arr[n][i] + 1;
   				//printf("loop number: %d\t number: %f\n", i%bincount, histArr[i%bincount]);
   				break;
   			}

   			if( i+1 == bincount && num == (((i+1)*w)+min)){
   				arr[n][i] = arr[n][i] + 1;
   				//printf("loop number: %d\t number: %f\n", i%bincount, histArr[i%bincount]);
   				break;
   			}
   		}
   	}
   	fclose(fptr); 
   	
   	return 0;
}


void *workerThread(void *u)// use process recursion again
{
	inputs *p = (inputs*)u;
	if (p->N != 0){// tek dosya sorunu output file

		pthread_t tid;
		inputs my_inputs = {p->min, p->w, p->bincount,p->N-1};
		pthread_create(&tid, NULL, workerThread, (void*)&my_inputs);
		fileReader(p->bincount, p->w, p->min, p->N);

		pthread_join(tid,NULL);
	}
	else{
		return 0;
	}
	return 0;
}

int main(int arg1, char *arg2[]){


	/*float minvalue, maxvalue, bincount;
	int N;

	printf("beginning of the program\n");

	printf("Enter minvalue:\t");
	scanf("%f", &minvalue);

	printf("Enter maxvalue:\t");
	scanf("%f", &maxvalue);

	printf("Enter bincount:\t");
	scanf("%f", &bincount);
	
	printf("Enter input file number (N):");
	scanf("%d", &N);*/

	float minvalue, maxvalue, bincount;
	int N;

	///////////////////
	minvalue = atoi(arg2[1]); 
    maxvalue = atoi(arg2[2]); 
    bincount = atoi(arg2[3]); 
    N = atoi(arg2[4]); 
    ///////////////////

	//struct memory allocation
	// global variable size creation
	arr = malloc(sizeof(int *)*N+1);
	for(int i = 0; i < N+1; i++)
		arr[i] = malloc(sizeof(int)*bincount);
	////////////

	/*for(int i =0 ; i<N; i++){
		for(int u = 0; u< bincount; u++){
			printf("%d %d\t %d\n",i, u, arr[i][u]);
		}
	}*/

	// file get
	//files size creation
	files = malloc(sizeof(int *)*N+1);
	for(int i = 0; i < N+1; i++)
		files[i] = malloc(sizeof(int)*255);

	for(int i = 0; i<N+1; i++){
		sprintf(files[i], "%s", arg2[i+5]);
		//scanf("%s", files[i]);
	}
	/////////////////////////////

	pthread_t tid;
	float w = (maxvalue - minvalue) / bincount;
	inputs my_inputs = {minvalue, w, bincount, N};
	pthread_create(&tid, NULL, workerThread, (void*)&my_inputs);
	pthread_join(tid,NULL);


	// make histogram from arr[0][] after this line
	for(int i =0 ; i<N+1; i++){
		for(int u = 0; u< bincount; u++){
			arr[0][u] = arr[0][u] + arr[i][u];
		}
	}


   	FILE *fptr;
   	fptr = fopen(getFile(files), "w");
    if(fptr == NULL)
   	{ 
        printf("\nError when opening file\n"); 
        exit (1); 
    } 

    // Inside this make one and only true histogram to rule them all
    for(int i = 0; i < bincount; i++)
    {
    	fprintf(fptr,"binnumber: %d\t%d\n",i+1 , arr[0][i]);
	}
   
  	fclose(fptr);
	return 0;
}