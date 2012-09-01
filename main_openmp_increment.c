#include "const.h"
#include "models.h"
#include "display.h"
#include <omp.h>

#define legal(x, n) ( (x)>=0 && (x)<(n) )
#define start_time clock_gettime(CLOCK_MONOTONIC, &start);
#define end_time clock_gettime(CLOCK_MONOTONIC, &finish); 
#define time_elapsed_ns (long long)(finish.tv_sec-start.tv_sec)*1000000000 + finish.tv_nsec - start.tv_nsec

int iteration, threads;
TemperatureField *field;
TemperatureField *tempField, *swapField;

int dx[4] = {0, -1, 0, 1};
int dy[4] = {1, 0, -1, 0};

int x, y;

char temperature_iterate(TemperatureField *field)
{
	refreshField(field);
	int i, j, d;
	char ret = 0;
#pragma omp parallel for schedule(dynamic) private(j) private(d)
	for (i=0; i<field->x; ++i){
		for (j=0; j<field->y; ++j)
		{
			tempField->t[i][j] = 0;
			for (d=0; d<4; ++d)
				if ( legal(i+dx[d], field->x) && legal(j+dy[d], field->y) )
					tempField->t[i][j] += field->t[i+dx[d]][j+dy[d]];
				else
					tempField->t[i][j] += ROOM_TEMP;
			tempField->t[i][j] /= 4;
			if (fabs(tempField->t[i][j] - field->t[i][j])>EPSILON && i)
				ret = 1;
		}
	}
	return ret;
}

int main(int argc, char **argv)
{
    struct timespec start, finish;
    if (argc<5)
    {
	    printf("Usage: %s x y iteration threads\n", argv[0]);
    }
    sscanf(argv[1], "%d", &x);
    sscanf(argv[2], "%d", &y);
    sscanf(argv[3], "%d", &iteration);
    sscanf(argv[4], "%d", &threads);
    omp_set_num_threads(threads);

    field = malloc(sizeof(TemperatureField));
    tempField = malloc(sizeof(TemperatureField));
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
	start_time
	for (iter=0; iter<iteration; iter++)
        {	
	   if (!temperature_iterate(field))
		break;
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
    deleteField(field);
    deleteField(tempField);
    free(X_Size);
    free(Y_Size);
    return 0;
}