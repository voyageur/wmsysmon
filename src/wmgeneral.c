#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <stdarg.h>

#include <X11/Xlib.h>
#include <X11/xpm.h>
#include <X11/extensions/shape.h>

#include "wmgeneral.h"

Window		Root;
int		screen;
int		x_fd;
int		d_depth;
XSizeHints	mysizehints;
XWMHints	mywmhints;
Pixel		back_pix, fore_pix;
char		*Geometry = "";
Window		iconwin, win;
GC		NormalGC;
XpmIcon		wmgen;
Pixmap		pixmask;


static void GetXPM(XpmIcon *, char **);
static Pixel GetColor(char *);
void RedrawWindow(void);


static void GetXPM(XpmIcon *wmgen, char *pixmap_bytes[]) {

	XWindowAttributes	attributes;
	int			err;

	/* For the colormap */
	XGetWindowAttributes(display, Root, &attributes);

	wmgen->attributes.valuemask |= (XpmReturnPixels | XpmReturnExtensions);

	err = XpmCreatePixmapFromData(display, Root, pixmap_bytes, &(wmgen->pixmap),
					&(wmgen->mask), &(wmgen->attributes));
	wmgen->dirty_x = wmgen->attributes.width;
	wmgen->dirty_y = wmgen->attributes.height;
	wmgen->dirty_w = 0;
	wmgen->dirty_h = 0;
	
	if (err != XpmSuccess) {
		fprintf(stderr, "Not enough free colorcells.\n");
		exit(1);
	}
}


static Pixel GetColor(char *name) {

	XColor				color;
	XWindowAttributes	attributes;

	XGetWindowAttributes(display, Root, &attributes);

	color.pixel = 0;
	if (!XParseColor(display, attributes.colormap, name, &color)) {
		fprintf(stderr, "wm.app: can't parse %s.\n", name);
	} else if (!XAllocColor(display, attributes.colormap, &color)) {
		fprintf(stderr, "wm.app: can't allocate %s.\n", name);
	}

	return color.pixel;
}


void RedrawWindow(void) {
	
	if(wmgen.dirty_w && wmgen.dirty_h)
	XCopyArea(display,
		  wmgen.pixmap,
		  iconwin,
		  NormalGC, 
		  wmgen.dirty_x,
		  wmgen.dirty_y,
		  wmgen.dirty_w,
		  wmgen.dirty_h,
		  wmgen.dirty_x,
		  wmgen.dirty_y);

	if(wmgen.dirty_w && wmgen.dirty_h)
	XCopyArea(display,
		  wmgen.pixmap,
		  win,
		  NormalGC,
		  wmgen.dirty_x,
		  wmgen.dirty_y,
		  wmgen.dirty_w,
		  wmgen.dirty_h,
		  wmgen.dirty_x,
		  wmgen.dirty_y);

#ifdef MONDEBUG
	printf("Dirty X: %i Y: %i width: %i height: %i\n",
		wmgen.dirty_x,
		wmgen.dirty_y,
		wmgen.dirty_w,
		wmgen.dirty_h);
#endif

	XFlush(display);
	wmgen.dirty_x = wmgen.attributes.width;
	wmgen.dirty_y = wmgen.attributes.height;
	wmgen.dirty_w = 0;
	wmgen.dirty_h = 0;

}


void RedrawWindowXY(int x, int y) {
	
	XCopyArea(display, wmgen.pixmap, iconwin, NormalGC, 
				x, y, wmgen.attributes.width, wmgen.attributes.height, 0,0);
	XCopyArea(display, wmgen.pixmap, win, NormalGC,
				x, y, wmgen.attributes.width, wmgen.attributes.height, 0,0);
}


void DirtyWindow(int x, int y, unsigned int w, unsigned int h) {
	static	int	nx, ny, nw, nh;

#ifdef MONDEBUG
	printf("currently dirty: X: %i Y: %i W: %u H: %u new: X: %i Y: %i W: %u H: %u\n",
		wmgen.dirty_x,
		wmgen.dirty_y,
		wmgen.dirty_w,
		wmgen.dirty_h,
		x,
		y,
		w,
		h);
#endif

        if(x < wmgen.dirty_x) nx = x;
        else nx = wmgen.dirty_x;

        if(y < wmgen.dirty_y) ny = y;
        else ny = wmgen.dirty_y;

        if((x + w) > (wmgen.dirty_x + wmgen.dirty_w)) nw = (x + w) - nx;
        else nw = (wmgen.dirty_x + wmgen.dirty_w) - nx;

        if((y + h) > (wmgen.dirty_y + wmgen.dirty_h)) nh = (y + h) - ny;
        else nh = (wmgen.dirty_y + wmgen.dirty_h) - ny;

	wmgen.dirty_x = nx;
	wmgen.dirty_y = ny;
	wmgen.dirty_w = nw;
	wmgen.dirty_h = nh;
#ifdef MONDEBUG
	printf("Dirty: X: %i Y: %i W: %i H: %i\n",
		wmgen.dirty_x,
		wmgen.dirty_y,
		wmgen.dirty_w,
		wmgen.dirty_h);
#endif
}


void createXBMfromXPM(char *xbm, char **xpm, int sx, int sy) {

	int		i,j,k;
	int		width, height, numcol, depth;
    int 	zero=0;
	unsigned char	bwrite;
    int		bcount;
    int     curpixel;

	
	sscanf(*xpm, "%d %d %d %d", &width, &height, &numcol, &depth);


    for (k=0; k!=depth; k++)
    {
        zero <<=8;
        zero |= xpm[1][k];
    }
        
	for (i=numcol+1; i < numcol+sy+1; i++) {
		bcount = 0;
		bwrite = 0;
		for (j=0; j<sx*depth; j+=depth) {
            bwrite >>= 1;

            curpixel=0;
            for (k=0; k!=depth; k++)
            {
                curpixel <<=8;
                curpixel |= xpm[i][j+k];
            }
                
            if ( curpixel != zero ) {
				bwrite += 128;
			}
			bcount++;
			if (bcount == 8) {
				*xbm = bwrite;
				xbm++;
				bcount = 0;
				bwrite = 0;
			}
		}
	}

}


void copyXPMArea(int sx, int sy, unsigned int w, unsigned int h, int dx, int dy) {

	XCopyArea(display,
		  wmgen.pixmap,
		  wmgen.pixmap,
		  NormalGC,
		  sx,
		  sy,
		  w,
		  h,
		  dx,
		  dy);

	DirtyWindow(dx, dy, w, h);
}


void copyXBMArea(int sx, int sy, unsigned int w, unsigned int h, int dx, int dy) {

	XCopyArea(display,
		  wmgen.mask,
		  wmgen.pixmap,
		  NormalGC,
		  sx,
		  sy,
		  w,
		  h,
		  dx,
		  dy);

	DirtyWindow(dx, dy, w, h);

}


void setMaskXY(int x, int y) {

	 XShapeCombineMask(display, win, ShapeBounding, x, y, pixmask, ShapeSet);
	 XShapeCombineMask(display, iconwin, ShapeBounding, x, y, pixmask, ShapeSet);
}


int openXwindow(int argc, char *argv[], char *pixmap_bytes[], char *pixmask_bits, int pixmask_width, int pixmask_height) {

	unsigned int	borderwidth = 1;
	XClassHint		classHint;
	char			*display_name = NULL;
	char			*wname = argv[0];
	XTextProperty	name;

	XGCValues		gcv;
	unsigned long	gcm;

	char			*geometry = NULL;

	int				dummy=0;
	int				i, wx, wy, fd;

	for (i=1; argv[i]; i++) {
		if (!strcmp(argv[i], "-display")) {
			display_name = argv[i+1];
			i++;
		}
		if (!strcmp(argv[i], "-geometry")) {
			geometry = argv[i+1];
			i++;
		}
	}

	if (!(display = XOpenDisplay(display_name))) {
		fprintf(stderr, "%s: can't open display %s\n", 
						wname, XDisplayName(display_name));
		return -1;
	}
	fd = ConnectionNumber(display);
	screen  = DefaultScreen(display);
	Root    = RootWindow(display, screen);
	d_depth = DefaultDepth(display, screen);
	x_fd    = XConnectionNumber(display);

	/* Convert XPM to XImage */
	GetXPM(&wmgen, pixmap_bytes);

	/* Create a window to hold the stuff */
	mysizehints.flags = USSize | USPosition;
	mysizehints.x = 0;
	mysizehints.y = 0;

	back_pix = GetColor("white");
	fore_pix = GetColor("black");

	XWMGeometry(display, screen, Geometry, NULL, borderwidth, &mysizehints,
				&mysizehints.x, &mysizehints.y,&mysizehints.width,&mysizehints.height, &dummy);

	mysizehints.width = 64;
	mysizehints.height = 64;
	DirtyWindow(0, 0, 64, 64);
		
	win = XCreateSimpleWindow(display, Root, mysizehints.x, mysizehints.y,
				mysizehints.width, mysizehints.height, borderwidth, fore_pix, back_pix);
	
	iconwin = XCreateSimpleWindow(display, win, mysizehints.x, mysizehints.y,
				mysizehints.width, mysizehints.height, borderwidth, fore_pix, back_pix);

	/* Activate hints */
	XSetWMNormalHints(display, win, &mysizehints);
	classHint.res_name = wname;
	classHint.res_class = wname;
	XSetClassHint(display, win, &classHint);

	XSelectInput(display, win, ExposureMask | StructureNotifyMask);
	XSelectInput(display, iconwin, ExposureMask | StructureNotifyMask);

	if (XStringListToTextProperty(&wname, 1, &name) == 0) {
		fprintf(stderr, "%s: can't allocate window name\n", wname);
		return -1;
	}

	XSetWMName(display, win, &name);

	/* Create GC for drawing */
	
	gcm = GCForeground | GCBackground | GCGraphicsExposures;
	gcv.foreground = fore_pix;
	gcv.background = back_pix;
	gcv.graphics_exposures = 0;
	NormalGC = XCreateGC(display, Root, gcm, &gcv);

	/* ONLYSHAPE ON */

	pixmask = XCreateBitmapFromData(display, win, pixmask_bits, pixmask_width, pixmask_height);

	XShapeCombineMask(display, win, ShapeBounding, 0, 0, pixmask, ShapeSet);
	XShapeCombineMask(display, iconwin, ShapeBounding, 0, 0, pixmask, ShapeSet);

	/* ONLYSHAPE OFF */

	mywmhints.initial_state = WithdrawnState;
	mywmhints.icon_window = iconwin;
	mywmhints.icon_x = mysizehints.x;
	mywmhints.icon_y = mysizehints.y;
	mywmhints.window_group = win;
	mywmhints.flags = StateHint | IconWindowHint | IconPositionHint | WindowGroupHint;

	XSetWMHints(display, win, &mywmhints);

	XSetCommand(display, win, argv, argc);
	XMapWindow(display, win);

	if (geometry) {
		if (sscanf(geometry, "+%d+%d", &wx, &wy) != 2) {
			fprintf(stderr, "Bad geometry string.\n");
			return -1;
		}
		XMoveWindow(display, win, wx, wy);
	}

	return fd;
}
