#include "const.h"
#include "models.h"
#include "display.h"
#include <pthread.h>
#include <stdio.h>

#define legal(x, n) ( (x)>=0 && (x)<(n) )
#define start_time clock_gettime(CLOCK_MONOTONIC, &start);
#define end_time clock_gettime(CLOCK_MONOTONIC, &finish); 
#define time_elapsed_ns (long long)(finish.tv_sec-start.tv_sec)*1000000000 + finish.tv_nsec - start.tv_nsec
#define time_elapsed_s (double)(finish.tv_sec-start.tv_sec) + (double)(finish.tv_nsec - start.tv_nsec)/1000000000
#define NOT_FIRE_PLACE i

int iteration, threads;
TemperatureField *field;
TemperatureField *tempField, *swapField;
pthread_t *threadPool;

int dx[4] = {0, -1, 0, 1};
int dy[4] = {1, 0, -1, 0};

int x, y, iter_cnt;

typedef struct JobData
{
    int lineStart, lineFinish;
    double error;
} JobData;

JobData *jobs;
int min(int x, int y){ if (x<y) return x; return y; }

void* iterateLine(void* data)
{
    int i, j, d;
    JobData *job = (JobData*)data;
    job->lineFinish = min(job->lineFinish, field->x);
    job->error = 0;
    while (1)
    {
	    for (i=job->lineStart; i<jobs->lineFinish; ++i)		    
		for (j=0; j<field->y; ++j)
		{
			tempField->t[i][j] = 0;
			for (d=0; d<4; ++d)
				if ( legal(i+dx[d], field->x) && legal(j+dy[d], field->y) )
					tempField->t[i][j] += field->t[i+dx[d]][j+dy[d]];
				else
					tempField->t[i][j] += ROOM_TEMP;
			tempField->t[i][j] /= 4;
			if (NOT_FIRE_PLACE)
				job->error += fabs(tempField->t[i][j] - field->t[i][j]);
		}
    }
    pthread_exit(NULL);
}

double temperature_iterate()
{
	++iter_cnt;
	refreshField(field, 0, 0, field->x, field->y, field->x, field->y);
	int i;

	int blockSize = field->x/threads + !!(field->x%threads);

	for (i=0; i<threads; ++i)
	{
		jobs[i].lineStart = i*blockSize;
		jobs[i].lineFinish = (i+1)*blockSize;
		pthread_create(&threadPool[i], NULL, iterateLine, (void*)&jobs[i]);
	}
	double error = 0;
	for (i=0; i<threads; ++i)
	{
		pthread_join(threadPool[i], NULL);
		error = error + jobs[i].error;
	}
	return error;
}

int main(int argc, char **argv)
{
    struct timespec start, finish;
    start_time
    if (argc<5)
    {
	    printf("Usage: %s x y iteration threads\n", argv[0]);
    }
    sscanf(argv[1], "%d", &x);
    sscanf(argv[2], "%d", &y);
    sscanf(argv[3], "%d", &iteration);
    sscanf(argv[4], "%d", &threads);

    field = malloc(sizeof(TemperatureField));
    tempField = malloc(sizeof(TemperatureField));
    threadPool = malloc(sizeof(pthread_t)*threads);
    jobs = malloc(sizeof(JobData)*threads);
    field->x = y;
    field->y = x;
#ifdef DISPLAY
    XWindow_Init(field);
#endif


    int iter, inc;
    int *X_Size = malloc(sizeof(int)*INCREMENT_TIME);
    int *Y_Size = malloc(sizeof(int)*INCREMENT_TIME);
    X_Size[INCREMENT_TIME-1] = x;
    Y_Size[INCREMENT_TIME-1] = y;
    for (inc=INCREMENT_TIME-2; inc>=0; --inc)
    {
	X_Size[inc] = X_Size[inc+1] / INCREMENT;
	Y_Size[inc] = Y_Size[inc+1] / INCREMENT;
    }

    for (inc=0; inc<INCREMENT_TIME; ++inc)
    {	
	if (!inc)
	{
            newField(field, X_Size[inc], Y_Size[inc], 0, 0);
	    newField(tempField, X_Size[inc], Y_Size[inc], 0, 0);	    
	    initField(field);
	}
	else 
	{
            newField(field, X_Size[inc], Y_Size[inc], X_Size[inc-1], Y_Size[inc-1]);
	    newField(tempField, X_Size[inc], Y_Size[inc], X_Size[inc-1], Y_Size[inc-1]);
	}
#ifdef DISPLAY
        XResize(field);
#endif
	for (iter=0; iter<iteration; iter++)
        {	
	   double error = temperature_iterate();
	   if (error<EPSILON){
		   printf("Finished. iteration=%d, error=%lf\n", iter, error);
		break;
	   }
	   swapField = field;
	   field = tempField;
	   tempField = swapField;
#ifdef DISPLAY
	   end_time
	   if (time_elapsed_ns > FRAME_INTERVAL*1000000)
	   {
		start_time;
		XRedraw(field);
	   }
#endif	
	}
    }
#ifdef DISPLAY
    XRedraw(field);
#endif
    deleteField(field);
    deleteField(tempField);
    free(X_Size);
    free(Y_Size);
    free(threadPool);
    free(jobs);
    printf("Finished in %d iterations.\n", iter_cnt);
    end_time;
    printf("%lf\n", time_elapsed_s);
    pthread_exit(NULL);
    return 0;
}
