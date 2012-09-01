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
    newField(field, x, y, 0, 0);
    newField(tempField, x, y, 0, 0);
    initField(field);
#ifdef DISPLAY
    XWindow_Init(field);
#endif

    int iter;
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
    return 0;
}
