/* Initial Mandelbrot program */


#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include "models.h"
#include "const.h"

Window          win;                            /* initialization for a window */
unsigned
int             width, height,                  /* window size */
		border_width,                   /*border width in pixels */
		idth, display_height,  /* size of screen */
		screen;                         /* which screen */

char            *window_name = "Temperature Simulation", *display_name = NULL;
GC              gc;
unsigned
long            valuemask = 0;
XGCValues       values;
Display         *display;
XSizeHints      size_hints;
Pixmap          bitmap;
FILE            *fp, *fopen ();	
Colormap	default_cmap;
XColor		color[256];

int temperatue_to_color_pixel(double t)
{
	return color[(int)(t/5.0f)].pixel;
}

void XWindow_Init(TemperatureField *field)
{    
        XSetWindowAttributes attr[1];       
       
        /* connect to Xserver */

        if (  (display = XOpenDisplay (display_name)) == NULL ) {
           fprintf (stderr, "drawon: cannot connect to X server %s\n",
                                XDisplayName (display_name) );
        exit (-1);
        }
        
        /* get screen size */

        screen = DefaultScreen (display);

        /* set window size *///XFlush (display);

        width = field->y;
	height = field->x;

        /* create opaque window */

        border_width = 4;
        win = XCreateSimpleWindow (display, RootWindow (display, screen),
                                width, height, width, height, border_width, 
                                BlackPixel (display, screen), WhitePixel (display, screen));

        size_hints.flags = USPosition|USSize;
        size_hints.x = 0;
        size_hints.y = 0;
        size_hints.width = width;
        size_hints.height = height;
        size_hints.min_width = 300;
        size_hints.min_height = 300;
        
        XSetNormalHints (display, win, &size_hints);
        XStoreName(display, win, window_name);

        /* create graphics context */

        gc = XCreateGC (display, win, valuemask, &values);

	default_cmap = DefaultColormap(display, screen);
        XSetBackground (display, gc, WhitePixel (display, screen));
        XSetForeground (display, gc, BlackPixel (display, screen));
        XSetLineAttributes (display, gc, 1, LineSolid, CapRound, JoinRound);

        attr[0].backing_store = Always;
        attr[0].backing_planes = 1;
        attr[0].backing_pixel = BlackPixel(display, screen);

        XChangeWindowAttributes(display, win, CWBackingStore | CWBackingPlanes | CWBackingPixel, attr);

        XMapWindow (display, win);
        XSync(display, 0); 

	/* create color */
	int i;
	for (i=0; i<20; ++i)
	{
	    color[i].green = rand()%65535;
	    color[i].red = rand()%65535;
	    color[i].blue = rand()%65535;
	    color[i].flags = DoRed | DoGreen | DoBlue;
	    XAllocColor(display, default_cmap, &color[i]);
	}
}

void XResize(TemperatureField *field)
{
    XResizeWindow(display, win, field->y, field->x);
}

void XRedraw(TemperatureField *field)
{
    int i, j;
    for (i=0; i<field->x; ++i)
        for (j=0; j<field->y; ++j)
	{
		XSetForeground(display, gc, temperatue_to_color_pixel(field->t[i][j]));
	        XDrawPoint (display, win, gc, j, i);
	}
    XFlush (display);
}

