#ifndef _MODELS
#define _MODELS

#include <memory.h>
#include <stdlib.h>
#include "const.h"

#define legal(x, n) ( (x)>=0 && (x)<(n) )

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
	else memset(field->storage, 0, sizeof(double)*x*y);
}

void initField(TemperatureField *field)
{
	int i, j;
	for (i=0; i<field->x; ++i)
		for (j=0; j<field->y; ++j)
			field->t[i][j] = 20.0f;
}

void refreshField(TemperatureField *field, int initX, int initY, int thisX, int thisY, int allX, int allY)
{
	int j;
	for (j=allY*3/10; j<allY*7/10; ++j)
	    if (legal(-initX, thisX)&&legal(j-initY, thisY))
		field->t[-initX][j-initY] = 100.0f;
}

TemperatureField* myClone(TemperatureField *field, int X, int Y)
{
	int i, j;
        TemperatureField *ret = malloc(sizeof(TemperatureField));
	ret->x = X;
	ret->y = Y;
	ret->storage = malloc(sizeof(double)*ret->x*ret->y);
	ret->t = malloc(sizeof(double*)*ret->x);
	for (i=0; i<ret->x; ++i)
		ret->t[i] = &ret->storage[i*ret->y];
	for (i=0; i<X; ++i)
		for (j=0; j<Y; ++j)
			ret->t[i][j] = field->t[i][j];
	return ret;
}

void deleteField(TemperatureField *field)
{
	free(field->t);
	free(field->storage);
	//free(field);
}

#endif
