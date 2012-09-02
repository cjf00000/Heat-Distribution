#include "const.h"
#include "models.h"
#include "display.h"
#include <mpi.h>
#include <math.h>
#include <assert.h>

#define start_time clock_gettime(CLOCK_MONOTONIC, &start);
#define end_time clock_gettime(CLOCK_MONOTONIC, &finish); 
#define time_elapsed_ns (long long)(finish.tv_sec-start.tv_sec)*1000000000 + finish.tv_nsec - start.tv_nsec
#define time_elapsed_s (double)(finish.tv_sec-start.tv_sec) + (double)(finish.tv_nsec - start.tv_nsec)/1000000000
#define fillReceiveBuffer for (i=0; i<line_buffer_size; ++i) recv_line_buffer[i]=ROOM_TEMP;

#define rank_x (world_rank/sq)
#define rank_y (world_rank%sq)
#define rank_id(x, y) ((x)*sq + (y))

int iteration;
TemperatureField *field, *allField;
TemperatureField *tempField;
double *recv_line_buffer;
double *send_line_buffer1, *send_line_buffer2;
int line_buffer_size;

int dx[4] = {0, -1, 0, 1};
int dy[4] = {1, 0, -1, 0};

int x, y, iter_cnt;
int world_size, world_rank;
int sq;

int blockSizeX;
int blockSizeY;

int max(int a, int b){ return a>b ? a: b; }

char temperature_iterate(TemperatureField *field, int initX, int initY)
{
	int i, j, d;
	++iter_cnt;
	refreshField(field, initX, initY, blockSizeX, blockSizeY, blockSizeX*sq, blockSizeY*sq);
	for (i=0; i<blockSizeX; ++i)
		for (j=0; j<blockSizeY; ++j)
			tempField->t[i+1][j+1] = field->t[i][j];

	/* Start sending process... */
	//Up
	if (rank_x>0) MPI_Send(tempField->t[1], blockSizeY, MPI_DOUBLE, world_rank-sq, 0, MPI_COMM_WORLD);
	//Down
	if (rank_x<sq-1) MPI_Send(tempField->t[blockSizeX]+1, blockSizeY, MPI_DOUBLE, world_rank+sq, 1, MPI_COMM_WORLD);
	//Left
	if (rank_y>0) {
		for (i=0; i<blockSizeX; ++i)
			send_line_buffer1[i] = tempField->t[i+1][1];		
		MPI_Send(send_line_buffer1, blockSizeX, MPI_DOUBLE, world_rank-1, 2, MPI_COMM_WORLD);
	}
	//Right
	if (rank_y<sq-1) {
		for (i=0; i<blockSizeX; ++i)
			send_line_buffer2[i] = tempField->t[i+1][blockSizeY];
		MPI_Send(send_line_buffer2, blockSizeX, MPI_DOUBLE, world_rank+1, 3, MPI_COMM_WORLD);
	}
	/* Start receiving process... */
	//Up
        if (rank_x<sq-1) MPI_Recv(recv_line_buffer, blockSizeY, MPI_DOUBLE, world_rank+sq, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
     	else fillReceiveBuffer;
     	for (i=0; i<blockSizeY; ++i) tempField->t[blockSizeX+1][i+1] = recv_line_buffer[i];
	//Down
	if (rank_x>0) MPI_Recv(recv_line_buffer, blockSizeY, MPI_DOUBLE, world_rank-sq, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	else fillReceiveBuffer;
	for (i=0; i<blockSizeY; ++i) tempField->t[0][i+1] = recv_line_buffer[i];
	//Left
	if (rank_y<sq-1) MPI_Recv(recv_line_buffer, blockSizeX, MPI_DOUBLE, world_rank+1, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	else fillReceiveBuffer;
	for (i=0; i<blockSizeX; ++i) tempField->t[i+1][blockSizeY+1] = recv_line_buffer[i];
	//Right
	if (rank_y>0) MPI_Recv(recv_line_buffer, blockSizeX, MPI_DOUBLE, world_rank-1, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	else fillReceiveBuffer;
	for (i=0; i<blockSizeX; ++i) tempField->t[i+1][0] = recv_line_buffer[i];

	/* Calculation */
	char ret = 0;
	for (i=0; i<blockSizeX; ++i){
		for (j=0; j<blockSizeY; ++j)
		{
			field->t[i][j]=0;
			for (d=0; d<4; ++d)
			    field->t[i][j] += tempField->t[i+dx[d]+1][j+dy[d]+1];
			field->t[i][j] /= 4;
			if (fabs(field->t[i][j] - tempField->t[i+1][j+1])>EPSILON && (i||rank_x) )
				ret = 1;
		}
	}
	return ret;
}

///Dest must be full, i.e. X*Y
void scatter(TemperatureField *source, int X, int Y, TemperatureField *dest)
{
    assert(dest->x==X && dest->y==Y);
    double *send_data;
    int i, j, k, cnt=0;
    if (world_rank==0) {
	    send_data = malloc(sizeof(double)*X*Y*world_size);
	    for (k=0; k<world_size; ++k) 
		for (i=0; i<X; ++i)
		    for (j=0; j<Y; ++j)
			send_data[cnt++] = source->t[i][j];
    }
    MPI_Scatter(send_data, X*Y, MPI_DOUBLE, dest->storage, X*Y, MPI_DOUBLE, 0, MPI_COMM_WORLD);
}

///Source must be full, i.e. X*Y
void gather(TemperatureField *dest, int X, int Y, TemperatureField *source)
{
    assert(source->x==X && source->y==Y);
    double *recv_data;
    int i, j, k, cnt=0;
    if (world_rank==0)
	    recv_data = malloc(sizeof(double)*X*Y*world_size);
    MPI_Gather(source->storage, X*Y, MPI_DOUBLE, recv_data, X*Y, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    if (world_rank==0)
    {
        for (k=0; k<world_size; ++k) 
	   for (i=0; i<X; ++i)
		   for (j=0; j<Y; ++j)
			   dest->t[k/sq*blockSizeX+i][k%sq*blockSizeY+j] = recv_data[cnt++];
    }
}

int main(int argc, char **argv)
{
    struct timespec start, finish;
    if (world_rank==0) start_time
    if (argc<4)
    {
	    printf("Usage: %s x y iteration\n", argv[0]);
	    return 0;
    }
    sscanf(argv[1], "%d", &x);
    sscanf(argv[2], "%d", &y);
    sscanf(argv[3], "%d", &iteration);

    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    if (world_size<4)
    {
	puts("At least 4 processes.");
	return 0;
    }

    field = malloc(sizeof(TemperatureField));
    tempField = malloc(sizeof(TemperatureField));
    field->x = y;
    field->y = x;
#ifdef DISPLAY
    if (world_rank==0) XWindow_Init(field);
#endif

    int iter, inc, i, j;
    int *X_Size = malloc(sizeof(int)*INCREMENT_TIME);
    int *Y_Size = malloc(sizeof(int)*INCREMENT_TIME);
    X_Size[INCREMENT_TIME-1] = x;
    Y_Size[INCREMENT_TIME-1] = y;
    sq = sqrt(world_size) + 0.001;
    for (inc=INCREMENT_TIME-2; inc>=0; --inc)
    {
	X_Size[inc] = X_Size[inc+1] / INCREMENT;
	Y_Size[inc] = Y_Size[inc+1] / INCREMENT;
	X_Size[inc] = ((X_Size[inc]/sq) + !!(X_Size[inc]%sq))*sq;
	Y_Size[inc] = ((Y_Size[inc]/sq) + !!(Y_Size[inc]%sq))*sq;
    }
    if (world_rank==0)
	    allField = malloc(sizeof(TemperatureField));

    for (inc=0; inc<INCREMENT_TIME; ++inc)
    {
	MPI_Barrier(MPI_COMM_WORLD);
	blockSizeX = X_Size[inc]/sq; 
	blockSizeY = Y_Size[inc]/sq;
	int lastX, lastY;
	if (!inc) lastX=0, lastY=0; else lastX = X_Size[inc-1], lastY = Y_Size[inc-1];
	line_buffer_size = max(blockSizeX, blockSizeY)+2;
	recv_line_buffer = malloc(sizeof(double)*line_buffer_size);
	send_line_buffer1 = malloc(sizeof(double)*line_buffer_size);
	send_line_buffer2 = malloc(sizeof(double)*line_buffer_size);
	//    printf("%d %d\n", blockSizeX, blockSizeY);

	newField(field, blockSizeX, blockSizeY, 0, 0);
	newField(tempField, blockSizeX+2, blockSizeY+2, 0, 0);
	if (world_rank==0)
	{
	    newField(allField, X_Size[inc], Y_Size[inc], lastX, lastY);
	    if (!inc) initField(allField);
	}
	scatter(allField, blockSizeX, blockSizeY, field);
//	if (world_rank==0)
//	{
//	    printf("All field:\n", world_rank);
//	    for (i=0; i<allField->x; ++i)
//	    {
//		    for (j=0; j<allField->y; ++j)
//		    	printf("%lf ", allField->t[i][j]);
//		    puts("");
//	    }
//	    puts("");
//	}

	for (iter=0; iter<iteration; iter++)
        {
	   char ret= temperature_iterate(field, world_rank/sq*blockSizeX, world_rank%sq*blockSizeY);
	   char recvedRes = 0;
	   MPI_Allreduce(&ret, &recvedRes, 1, MPI_CHAR, MPI_LOR, MPI_COMM_WORLD);
	   if (!recvedRes)
		break;
	   MPI_Barrier(MPI_COMM_WORLD);
	}
	free(recv_line_buffer);
	free(send_line_buffer1);
	free(send_line_buffer2);
	gather(allField, blockSizeX, blockSizeY, field);
//	puts("finish iteration");
//
//	if (world_rank==0)
//	{
//	    printf("Process %d field:\n", world_rank);
//	    for (i=0; i<field->x; ++i)
//	    {
//		    for (j=0; j<field->y; ++j)
//		    	printf("%lf ", field->t[i][j]);
//		    puts("");
//	    }
//	    puts("");
//	}
//	if (world_rank==0)
//	{
//	    printf("All field:\n", world_rank);
//	    for (i=0; i<allField->x; ++i)
//	    {
//		    for (j=0; j<allField->y; ++j)
//		    	printf("%lf ", allField->t[i][j]);
//		    puts("");
//	    }
//	    puts("");
//	}
#ifdef DISPLAY
	if (world_rank==0) XRedraw(allField);
#endif 
	deleteField(field);
	deleteField(tempField);
    }
    free(X_Size);
    free(Y_Size);
    if (world_rank==0)
    {
	    printf("Finished in %d iterations.\n", iter_cnt);
	    end_time;
	    printf("%lf\n", time_elapsed_s);
    }
    MPI_Finalize();
#ifdef DISPLAY
	if (world_rank==0) usleep(100000000);
#endif 
    return 0;
}
