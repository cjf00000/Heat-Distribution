#include "const.h"
#include "models.h"
#include "display.h"

#define legal(x, n) ( (x)>=0 && (x)<(n) )

int iteration;
TemperatureField *field;
TemperatureField *tempField, *swapField;

int dx[4] = {0, -1, 0, 1};
int dy[4] = {1, 0, -1, 0};

void temperature_iterate(TemperatureField *field)
{
	int i, j, d;
	for (i=0; i<field->x; ++i)
		for (j=0; j<field->y; ++j)
		{
			int cnt = 0;
			tempField->t[i][j] = 0;
			for (d=0; d<4; ++d)
				if ( legal(i+dx[d], field->x) && legal(j+dy[d], field->y) )
				{
					tempField->t[i][j] += field->t[i+dx[d]][j+dy[d]];
					++cnt;
				}
			tempField->t[i][j] /= cnt;
		}
	tempField->t[0][0] = 100.0f;
}

int main(int argc, char **argv)
{
    if (argc<4)
    {
	    printf("Usage: %s x y iteration\n", argv[0]);
    }
    sscanf(argv[1], "%d", &x);
    sscanf(argv[2], "%d", &y);
    sscanf(argv[3], "%d", &iteration);

    field = malloc(sizeof(TemperatureField));
    tempField = malloc(sizeof(TemperatureField));
    newField(field, x, y);
    newField(tempField, x, y);
    initField(field);
    XWindow_Init(field);

    int iter;
    for (iter=0; iter<iteration; iter++)
    {
	temperature_iterate(field);
	swapField = field;
	field = tempField;
	tempField = swapField;
	XRedraw(field);
    }
    return 0;
}
