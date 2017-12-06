#ifndef WMGENERAL_H_INCLUDED
#define WMGENERAL_H_INCLUDED

  /***********/
 /* Defines */
/***********/

#define MAX_MOUSE_REGION (16)

  /************/
 /* Typedefs */
/************/

typedef struct _rckeys rckeys;

struct _rckeys {
	const char	*label;
	char		**var;
};

typedef struct _rckeys2 rckeys2;

struct _rckeys2 {
	const char	*family;
	const char	*label;
	char		**var;
};

typedef struct {
	Pixmap		pixmap;
	Pixmap		mask;
	XpmAttributes	attributes;
	int		dirty_x, dirty_y;
	unsigned int	dirty_w, dirty_h;
} XpmIcon;

  /*******************/
 /* Global variable */
/*******************/

Display		*display;

  /***********************/
 /* Function Prototypes */
/***********************/

void AddMouseRegion(int index, int left, int top, int right, int bottom);
int CheckMouseRegion(int x, int y);

int openXwindow(int argc, char *argv[], char **, char *, int, int);
void RedrawWindow(void);
void RedrawWindowXY(int x, int y);

void createXBMfromXPM(char *, char **, int, int);
void copyXPMArea(int, int, unsigned int, unsigned int, int, int);
void copyXBMArea(int, int, unsigned int, unsigned int, int, int);
void setMaskXY(int, int);

void parse_rcfile(const char *, rckeys *);
inline void DirtyWindow(int, int, unsigned int, unsigned int);

#endif
