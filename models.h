#ifndef _MODELS
#define _MODELS

#include <memory.h>
#include <stdlib.h>
#include "const.h"

typedef struct TemperatureField
{
	int x, y;
	double **t;
	double *storage;
}TemperatureField;

void deleteField(TemperatureField *field);

void newField(TemperatureField *field, int x, int y, int sourceX, int sourceY)
{
	TemperatureField temp = *field;
	field->storage = malloc( sizeof(double) * x * y );
	field->t = malloc( sizeof(double*) * x );
	field->x = x;
	field->y = y;
	int i, j;
	for (i=0; i<x; ++i)
		field->t[i] = &field->storage[i*y];
	if (sourceX)
	{
		double scaleFactorX = (double)sourceX/x;
		double scaleFactorY = (double)sourceY/y;
		for (i=0; i<x; ++i)
			for (j=0; j<y; ++j)
				field->t[i][j] = temp.t[(int)(i*scaleFactorX)][(int)(j*scaleFactorY)];
		deleteField(&temp);
	}
}

void initField(TemperatureField *field)
{
	int i, j;
	for (i=0; i<field->x; ++i)
		for (j=0; j<field->y; ++j)
			field->t[i][j] = 20.0f;
}

void refreshField(TemperatureField *field)
{
	int j;
	for (j=field->y*3/10; j<field->y*7/10; ++j)
	   field->t[0][j] = 100.0f;
}

void deleteField(TemperatureField *field)
{
	free(field->t);
	free(field->storage);
}

#endif
