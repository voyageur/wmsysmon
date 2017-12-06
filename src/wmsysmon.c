/*
	wmsysmon -- system monitoring dockapp

	Copyright (C) 1998-1999 Dave Clark - clarkd@skynet.ca
	Copyright (C) 2000 Vito Caputo - swivel@gnugeneration.com

	Kernel 2.6 support and CPU load changes made by
	Michele Noberasco - s4t4n@gentoo.org

	wmsysmon is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	wmsysmon is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <math.h>
#include <sys/poll.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/utsname.h>
#include <X11/Xlib.h>
#include <X11/xpm.h>
#include <X11/extensions/shape.h>


#include "wmgeneral.h"
#include "cpu.h"

#ifdef HI_INTS
#include "wmsysmon-master-alpha.xpm"
#else
#include "wmsysmon-master-i386.xpm"
#endif

#define WMSYSMON_VERSION "0.7.7"

#define CHAR_WIDTH 5
#define CHAR_HEIGHT 7

#define BCHAR_WIDTH 6
#define BCHAR_HEIGHT 9

#define BOFFX   (93)
#define BOFFY   (66)
#define BREDX   (81)
#define BREDY   (66)
#define BGREENX (87)
#define BGREENY (66)

#define LITEW   (4)
#define LITEH   (4)

#define B_OFF   (0)
#define B_RED   (1)
#define B_GREEN (2)

/* meter definitions */
#define VBAR_W	(4)
#define VBAR_H	(4)

#define INT_LITES	(1)
#define INT_METERS	(2)

/* for CPU percetage */
#define CPUNUM_NONE -1
cpu_options cpu_opts;

char wmsysmon_mask_bits[64*64];
int	 wmsysmon_mask_width  = 64;
int	 wmsysmon_mask_height = 64;

long start_time = 0;
long start_uptime = 0;
/* int	counter = 0; */
int	intr_l;	/* line in /proc/stat "intr" is on     */
int	rio_l;  /* line in /proc/stat "disk_rio" is on */
int	wio_l;  /* line in /proc/stat "disk_wio" is on */
int	page_l; /* line in /proc/stat "page" is on     */
int	swap_l; /* line in /proc/stat "swap" is on     */

int	meter[4][2];

typedef enum
{
	NEWER_2_6 = 0,
	OLDER_2_4
} kernel_versions;
kernel_versions kernel_version;

#ifdef HI_INTS
long	_last_ints[24];
long	_ints[24];
int	int_peaks[24];
#else
long	_last_ints[16];
long	_ints[16];
int	int_peaks[16];
#endif
long	*last_ints;
long	*ints;
long	int_mode = INT_METERS;

long	last_pageins=0;
long	last_pageouts=0;

long	last_swapins=0;
long	last_swapouts=0;

char	buf[1024];
FILE	*statfp;
FILE	*memfp;
FILE  *vmstatfp;

int	update_rate = 50;

char	*ProgName;

time_t	curtime;
time_t	prevtime;


kernel_versions Get_Kernel_version(void);
void usage(void);
void printversion(void);
void BlitString(char *name, int x, int y);
void BlitNum(int num, int x, int y);
void wmsysmon_routine(int, char **);
void DrawBar(int sx, int sy, int w, int h, float percent, int dx, int dy);
void DrawLite(int state, int dx, int dy);
void DrawUptime(void);
void DrawStuff( void );
void DrawMem( void );
void DrawCpuPercentage( void );
void DrawMeter(unsigned int, unsigned int, int, int);

int main(int argc, char *argv[])
{
	int	i;

	/* set meter x,y pairs */
	meter[3][0] = 108;
	meter[3][1] = 66;
	meter[2][0] = 116;
	meter[2][1] = 66;
	meter[1][0] = 124;
	meter[1][1] = 66;
	meter[0][0] = 132;
	meter[0][1] = 66;

	last_ints = _last_ints;
	ints = _ints;
    
 	/* some defaults for CPU percentage */
	cpu_opts.ignore_nice = False;
	cpu_opts.cpu_number = CPUNUM_NONE;
	cpu_opts.ignore_procs = 0;
 
 	kernel_version = Get_Kernel_version();
 
 	/* Parse Command Line */

	ProgName = argv[0];
	if (strlen(ProgName) >= 5)
		ProgName += (strlen(ProgName) - 5);

 	for (i = 1; i < argc; i++)
 	{
		char *arg = argv[i];
  
 		if (*arg == '-')
 		{
 			switch (arg[1])
 			{
 				case 'd':
 					if (strcmp(arg + 1, "display"))
 					{
 						usage();
 						exit(1);
 					}
 					break;
 				case 'g':
 					if (strcmp(arg+1, "geometry"))
 					{
 						usage();
 						exit(1);
 					}
 					break;
 				case 'v':
 					printversion();
 					exit(0);
 					break;
 				case 'r':
 					if (argc > (i + 1))
 					{
 						update_rate = (atoi(argv[i + 1]) * 1000);
 						i++;
 					}
 					break;
 				case 'l':
 					int_mode = INT_LITES;
 					break;
 				case 'n':
 					cpu_opts.ignore_nice = True;
 					break;
 				case 'p':
 					if (argc == (i + 1))
 					{
 						fprintf(stderr, "Option -p needs an argument!\n");
 						exit(1);
 					}
					i++;
 					if (strlen(argv[i]) >= COMM_LEN)
 					{
 						fprintf(stderr, "Command name %s is longer than 15 characters\n", argv[i]);
 						exit(1);
 					}
 					if (cpu_opts.ignore_procs == MAX_PROC)
 					{
 						fprintf(stderr, "Maximum number of command names is %d\n", MAX_PROC);
 						exit(1);
 					}
 					cpu_opts.ignore_proc_list[cpu_opts.ignore_procs] = argv[i];
 					cpu_opts.ignore_procs++;
 					break;
 
 				default:
 					usage();
 					exit(0);
 					break;
			}
		}
	}
  
 	/* Init CPU percentage stuff */
 	/* NOT NEEDED ON LINUX */
 	/* cpu_init(); */
 

	wmsysmon_routine(argc, argv);


	return 0;
}


void wmsysmon_routine(int argc, char **argv)
{
	int		i;
	XEvent		Event;
	FILE		*fp;
	int		xfd;
	struct pollfd	pfd;
    

	createXBMfromXPM(wmsysmon_mask_bits, wmsysmon_master_xpm, wmsysmon_mask_width, wmsysmon_mask_height);
    
	xfd = openXwindow(argc, argv, wmsysmon_master_xpm, wmsysmon_mask_bits, wmsysmon_mask_width, wmsysmon_mask_height);
	if(xfd < 0) exit(1);

	pfd.fd = xfd;
	pfd.events = (POLLIN);

	/* init ints */
	bzero(&_last_ints, sizeof(_last_ints));
	bzero(&_ints, sizeof(_ints));
	bzero(&int_peaks, sizeof(int_peaks));

    
	/* init uptime */
	fp = fopen("/proc/uptime", "r");
	if (fp) {
		fscanf(fp, "%ld", &start_time);
		fclose(fp);
		start_uptime = time(NULL);
	}

	statfp = fopen("/proc/stat", "r");
	memfp = fopen("/proc/meminfo", "r");
	if (kernel_version == NEWER_2_6) vmstatfp = fopen("/proc/vmstat", "r");
 
  
 	/* here we find tags in /proc/stat and note their
	 * lines, for faster lookup throughout execution.
	 */
	for(i = 0; fgets(buf, 1024, statfp); i++)
	{
		if (kernel_version == OLDER_2_4)
 		{
 			if(strstr(buf, "page")) page_l = i;
 			if(strstr(buf, "swap")) swap_l = i;
 		}
 		if(strstr(buf, "intr")) intr_l = i;
	}
  
	while(1)
	{
		curtime = time(0);
  
		DrawCpuPercentage();
		DrawUptime();
		DrawStuff();
		DrawMem();
		RedrawWindow();
 
		/* X Events */
		poll(&pfd, 1, update_rate);
		while (XPending(display))
		{
			XNextEvent(display, &Event);
			switch (Event.type)
			{
				case Expose:
					DirtyWindow(Event.xexpose.x,
											Event.xexpose.y,
											Event.xexpose.width,
											Event.xexpose.height);
					break;
				case DestroyNotify:
					XCloseDisplay(display);
					exit(0);
					break;
#ifdef MONDEBUG
				default:
					printf("got: %i\n", Event.type);
#endif
			}
		}

		/* usleep(update_rate); */

	}
}


void DrawMeter(unsigned int level, unsigned int peak, int dx, int dy)
{
	static unsigned int	a;

	/* meters are on a per interruptscale, dictated by the peak, maintain
	 * this peak outside of here, you can use a fixed peak for all ints but
	 * since we're using only 4 lines for our level, the timer interrupt
	 * usually kills the peak for the others so they never move. */

	if(peak) {
		a = level * 3 / peak;
	} else a = 0;

#ifdef MONDEBUG
	printf("level: %u peak: %u selection: %u\n", level, peak, a);
	fflush(stdout);
#endif

	if(a > 3) a = 3;

	copyXPMArea(meter[a][0],
							meter[a][1],
							VBAR_W,
							VBAR_H,
							dx,
							dy);
}



void DrawBar(int sx, int sy, int w, int h, float percent, int dx, int dy)
{
	static int	tx;

	tx = (float)((float)w * ((float)percent / (float)100.0));

	copyXPMArea(sx, sy, tx, h, dx, dy);

	copyXPMArea(sx, sy + h + 1, w - tx, h, dx + tx, dy);

}


void DrawLite(int state, int dx, int dy)
{

	switch(state) {
		case B_RED:
			copyXPMArea(BREDX, BREDY, LITEW, LITEH, dx, dy);
			break;
		case B_GREEN:
			copyXPMArea(BGREENX, BGREENY, LITEW, LITEH, dx, dy);
			break;
		default:
		case B_OFF:
			copyXPMArea(BOFFX, BOFFY, LITEW, LITEH, dx, dy);
			break;
	}

}


void DrawCpuPercentage(void)
{
	int cpupercentage = cpu_get_usage(&cpu_opts);
	static int oldcpupercentage = -1;
	
	if (cpupercentage != oldcpupercentage)
	{
#ifdef HI_INTS
		DrawBar(67, 36, 58, 4, cpupercentage, 3, 4);
#else
		DrawBar(67, 36, 58, 6, cpupercentage, 3, 4);
#endif
		oldcpupercentage = cpupercentage;
	}
}


void DrawUptime(void)
{
	static int	first = 1;
	static int	old_days = 0, old_hours = 0, old_minutes = 0;
	static long	uptime;
	static int	i;

    
	uptime = curtime - start_uptime + start_time;

	/* blit minutes */
	uptime /=60;
	i = uptime % 60;

	if(old_minutes != i || first) {
		old_minutes = i;
		sprintf(buf, "%02i", i);
#ifdef HI_INTS
		BlitString(buf, 45, 29);
#else
		BlitString(buf, 45, 37);
#endif
	}
    
	/* blit hours */
	uptime /=60;
	i = uptime % 24;

	if(old_hours != i || first) {
		old_hours = i;
		sprintf(buf, "%02i", i);
#ifdef HI_INTS
		BlitString(buf, 29, 29);
#else
		BlitString(buf, 29, 37);
#endif
	}

    
	/* blit days */
	uptime /= 24;
	i = uptime;
	if(old_days != i || first) {
		old_days = i;
		sprintf(buf, "%03i", i);
#ifdef HI_INTS
		BlitString(buf, 6, 29);
#else
		BlitString(buf, 6, 37);
#endif
	}

	first = 0;
}


void DrawStuff( void )
{
#ifdef HI_INTS
	static int	int_lites[24] =
		{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	/* to keep track of on/off status */
#else
	static int	int_lites[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
#endif
	static int	pagein_lite = 0, pageout_lite = 0, swapin_lite = 0, swapout_lite = 0;
	static int	i, ents;

	static long	pageins;
	static long	pageouts;
	static long	swapins;
	static long	swapouts;
	static long	intdiff;
	static long	*tints;

	pageins = pageouts = swapins = swapouts = 0;

	statfp = freopen("/proc/stat", "r", statfp);

	if (kernel_version == NEWER_2_6)
	{
		static char *pageins_p=NULL;
		static char *pageouts_p;
		static char *swapins_p;
		static char *swapouts_p;
 
		vmstatfp = freopen("/proc/vmstat", "r", vmstatfp);
		fread_unlocked (buf, 1024, 1, vmstatfp);
		if (!pageins_p)
		{
			pageins_p  = strstr(buf, "pgpgin"  ) + 6;
			pageouts_p = strstr(buf, "pgpgout" ) + 7;
			swapins_p  = strstr(buf, "pswpin"  ) + 6;
			swapouts_p = strstr(buf, "pswpout" ) + 7;
		}
		sscanf(pageins_p,  "%ld", &pageins  );
		sscanf(pageouts_p, "%ld", &pageouts );
		sscanf(swapins_p,  "%ld", &swapins  );
		sscanf(swapouts_p, "%ld", &swapouts );												
	}

	for(i = 0, ents = 0; ents < 5 && fgets(buf, 1024, statfp); i++)
	{
		if (kernel_version == OLDER_2_4)
		{
			if(i == page_l)
			{
				sscanf(buf, "%*s %ld %ld", &pageins, &pageouts);
				ents++;
			}
			if(i == swap_l)
			{
				sscanf(buf, "%*s %ld %ld", &swapins, &swapouts);
				ents++;
			}
		}
		if(i == intr_l)
		{
			sscanf(	buf,
#ifdef HI_INTS
							"%*s %*d %ld %ld %ld %ld %ld"
							"%ld %ld %ld %ld %ld %ld %ld"
							"%ld %ld %ld %ld %ld %ld %ld"
							"%ld %ld %ld %ld %ld",
							&ints[0], &ints[1], &ints[2],
							&ints[3], &ints[4], &ints[5],
							&ints[6], &ints[7], &ints[8],
							&ints[9], &ints[10], &ints[11],
							&ints[12], &ints[13], &ints[14],
							&ints[15], &ints[16], &ints[17],
							&ints[18], &ints[19], &ints[20],
							&ints[21], &ints[22], &ints[23]);
#else
			"%*s %*d %ld %ld %ld %ld %ld"
				"%ld %ld %ld %ld %ld %ld %ld"
				"%ld %ld %ld %ld",
				&ints[0], &ints[1], &ints[2],
				&ints[3], &ints[4], &ints[5],
				&ints[6], &ints[7], &ints[8],
				&ints[9], &ints[10], &ints[11],
				&ints[12], &ints[13], &ints[14],
				&ints[15]);
#endif
		ents++;
	}
}


if(int_mode == INT_LITES) {
	/* top 8 ints */
	for (i = 0; i < 8; i++) {
		if ( ints[i] > last_ints[i] && !int_lites[i]) {
			int_lites[i] = 1;
#ifdef HI_INTS
			DrawLite(B_GREEN, 4 + (i * LITEW) + i, 43);
#else
			DrawLite(B_GREEN, 4 + (i * LITEW) + i, 51);
#endif
		} else if(ints[i] == last_ints[i] && int_lites[i]) {
			int_lites[i] = 0;
#ifdef HI_INTS
			DrawLite(B_OFF, 4 + (i * LITEW) + i, 43);
#else
			DrawLite(B_OFF, 4 + (i * LITEW) + i, 51);
#endif
		}
	}
	/* middle/bottom 8 */
	for (i = 8; i < 16; i++) {
		if ( ints[i] > last_ints[i] && !int_lites[i]) {
			int_lites[i] = 1;
#ifdef HI_INTS
			DrawLite(B_GREEN, 4 + ((i - 8) *LITEW) + (i - 8), 48);
#else
			DrawLite(B_GREEN, 4 + ((i - 8) *LITEW) + (i - 8), 56);
#endif
		} else if(ints[i] == last_ints[i] && int_lites[i]) {
			int_lites[i] = 0;
#ifdef HI_INTS
			DrawLite(B_OFF, 4 + ((i - 8) * LITEW) + (i - 8), 48);
#else
			DrawLite(B_OFF, 4 + ((i - 8) * LITEW) + (i - 8), 56);
#endif
		}
	}

#ifdef HI_INTS
	/* bottom 8 on alpha/smp x86 */
	for (i = 16; i < 24; i++) {
		if (ints[i] > last_ints[i] && !int_lites[i] ) {
			int_lites[i] = 1;
			DrawLite(B_GREEN, 4 + ((i - 16) * LITEW) + (i - 16), 53);
		} else if(ints[i] == last_ints[i] && int_lites[i]) {
			int_lites[i] = 0;
			DrawLite(B_OFF, 4 + ((i -16) * LITEW) + (i - 16), 53);
		}
	}
#endif
} else if(int_mode == INT_METERS) {
	for (i = 0; i < 8; i++) {
		if(last_ints[i]) {
			intdiff = ints[i] - last_ints[i];
			int_peaks[i] = (int_peaks[i] + intdiff) >> 1;
#ifdef HI_INTS
			DrawMeter(intdiff,
								int_peaks[i],
								VBAR_H + (i * VBAR_W) + i,
								43);
#else
			DrawMeter(intdiff,
								int_peaks[i],
								VBAR_H + (i * VBAR_W) + i,
								51);
#endif
		}
	}

	for (i = 8; i < 16; i++) {
		if(last_ints[i]) {
			intdiff = ints[i] - last_ints[i];
			int_peaks[i] = (int_peaks[i] + intdiff) >> 1;
#ifdef HI_INTS
			DrawMeter(intdiff,
								int_peaks[i],
								VBAR_H + ((i - 8) * VBAR_W) + (i - 8),
								48);
#else
			DrawMeter(intdiff,
								int_peaks[i],
								VBAR_H + ((i - 8) * VBAR_W) + (i - 8),
								56);
#endif
		}
	}

#ifdef HI_INTS
	for (i = 16; i < 24; i++) {
		if(last_ints[i]) {
			intdiff = ints[i] - last_ints[i];
			int_peaks[i] = (int_peaks[i] + intdiff) >> 1;

			DrawMeter(intdiff,
								int_peaks[i],
								VBAR_H + ((i - 16) * VBAR_W) + (i - 16),
								53);
		}
	}
#endif
}

tints = last_ints;
last_ints = ints;
ints = tints;

/* page in / out */

if (pageins > last_pageins && !pagein_lite) {
	pagein_lite = 1;
#ifdef HI_INTS
	DrawLite(B_RED, 51, 43);
#else
	DrawLite(B_RED, 51, 51);
#endif
} else if(pagein_lite) {
	pagein_lite = 0;
#ifdef HI_INTS
	DrawLite(B_OFF, 51, 43);
#else
	DrawLite(B_OFF, 51, 51);
#endif
}

if (pageouts > last_pageouts && !pageout_lite) {
	pageout_lite = 1;
#ifdef HI_INTS
	DrawLite(B_RED, 56, 43);
#else
	DrawLite(B_RED, 56, 51);
#endif
} else if(pageout_lite) {
	pageout_lite = 0;
#ifdef HI_INTS
	DrawLite(B_OFF, 56, 43);
#else
	DrawLite(B_OFF, 56, 51);
#endif
}

last_pageins = pageins;
last_pageouts = pageouts;

/* swap in/out */

if(swapins > last_swapins && !swapin_lite) {
	swapin_lite = 1;
#ifdef HI_INTS
	DrawLite(B_RED, 51, 48);
#else
	DrawLite(B_RED, 51, 56);
#endif
} else if(swapin_lite) {
	swapin_lite = 0;
#ifdef HI_INTS
	DrawLite(B_OFF, 51, 48);
#else
	DrawLite(B_OFF, 51, 56);
#endif
}

if (swapouts > last_swapouts && !swapout_lite) {
	swapout_lite = 1;
#ifdef HI_INTS
	DrawLite(B_RED, 56, 48);
#else
	DrawLite(B_RED, 56, 56);
#endif
} else if(swapout_lite) {
	swapout_lite = 0;
#ifdef HI_INTS
	DrawLite(B_OFF, 56, 48);
#else
	DrawLite(B_OFF, 56, 56);
#endif
}

last_swapins = swapins;
last_swapouts = swapouts;

}


void DrawMem(void)
{
	static char *p_mem_tot=NULL, *p_mem_free, *p_mem_buffers, *p_mem_cache;
	static char *p_swap_total, *p_swap_free;

	static int	last_mem = 0, last_swap = 0, first = 1;
	static long 	mem_total = 0;
	static long 	mem_free = 0;
	static long 	mem_buffers = 0;
	static long 	mem_cache = 0;
	static long 	swap_total = 0;
	static long 	swap_free = 0;

	static int 	mempercent = 0;
	static int 	swappercent = 0;


/* 	counter--; */

/* 	if(counter >= 0) return; /\* polling /proc/meminfo is EXPENSIVE *\/ */
/* 	counter = 1500 / update_rate; */

	memfp = freopen("/proc/meminfo", "r", memfp);
	fread_unlocked (buf, 1024, 1, memfp);

	if (!p_mem_tot)
	{
		p_mem_tot     = strstr(buf, "MemTotal:" ) + 13;
		p_mem_free    = strstr(buf, "MemFree:"  ) + 13;
		p_mem_buffers = strstr(buf, "Buffers:"  ) + 13;
		p_mem_cache   = strstr(buf, "Cached:"   ) + 13;
		p_swap_total  = strstr(buf, "SwapTotal:") + 13;
		p_swap_free   = strstr(buf, "SwapFree:" ) + 13;
	}
	
	sscanf(p_mem_tot,     "%ld", &mem_total  );
	sscanf(p_mem_free,    "%ld", &mem_free   );
	sscanf(p_mem_buffers, "%ld", &mem_buffers);
	sscanf(p_mem_cache,   "%ld", &mem_cache  );
	sscanf(p_swap_total,  "%ld", &swap_total );
	sscanf(p_swap_free,   "%ld", &swap_free  );

	/* could speed this up but we'd lose precision, look more into it to see
	 * if precision change would be noticable, and if speed diff is significant
	 */
	mempercent = ((float)(mem_total - mem_free - mem_buffers - mem_cache)
								/
								(float)mem_total) * 100;

	if (!swap_total) swappercent = 0;
	else swappercent = ((float)(swap_total - swap_free)
											/
											(float)swap_total) * 100;

	if(mempercent != last_mem || first) {
		last_mem = mempercent;
#ifdef HI_INTS
		DrawBar(67, 36, 54, 4, mempercent, 3, 12);
#else
		DrawBar(67, 36, 54, 6, mempercent, 3, 15);
#endif
	}

	if(swappercent != last_swap || first) {
		last_swap = swappercent;
#ifdef HI_INTS
		DrawBar(67, 36, 58, 4, swappercent, 3, 20);
#else
		DrawBar(67, 36, 58, 6, swappercent, 3, 26);
#endif
	}

	first = 0;

}


/* Blits a string at given co-ordinates */
void BlitString(char *name, int x, int y)
{
	static int	i, c, k;

	k = x;
	for (i=0; name[i]; i++) {

		c = toupper(name[i]); 
		if (c >= 'A' && c <= 'Z') {
			c -= 'A';
			copyXPMArea(c * 6, 74, 6, 8, k, y);
			k += 6;
		} else {
			c -= '0';
			copyXPMArea(c * 6, 64, 6, 8, k, y);
			k += 6;
		}
	}

}


void BlitNum(int num, int x, int y)
{
	static int	newx;


	newx = x;

	sprintf(buf, "%03i", num);

	BlitString(buf, newx, y);

}
    

void usage(void)
{
	fprintf(stderr, "\nwmsysmon - http://www.gnugeneration.com\n\n");
	fprintf(stderr, ".-----------------.\n"
                  "|[---------------]|  <--- CPU Use %%\n"
                  "|[---------------]|  <--- Memory Use %%\n"
                  "|[---------------]|  <--- Swap Use %%\n"
                  "|                 |\n"
                  "|   000:00:00     |  <--- Uptime days:hours:minutes\n"
                  "|                 |\n"
#ifdef HI_INTS
                  "| 01234567   UV   |  <--- 0-N are hardware interrupts 0-23\n"
                  "| 89ABCDEF   WX   |  <--- U,V are Page IN/OUT, W,X are Swap IN/OUT\n"
                  "| GHIJKLMN   YZ   |\n"
#else
                  "| 01234567   WX   |  <--- 0-F are hardware interrupts 0-15\n"
                  "| 89ABCDEF   YZ   |  <--- W,X are Page IN/OUT, Y,Z are Swap IN/OUT\n"
#endif
                  "'-----------------'\n");

	fprintf(stderr, "usage:\n");
	fprintf(stderr, "\t-display <display name>\n");
	fprintf(stderr, "\t-geometry +XPOS+YPOS\tinitial window position\n");
	fprintf(stderr, "\t-r\t\t\tupdate rate in milliseconds (default:300)\n");
	fprintf(stderr, "\t-l\t\t\tblinky lights for interrupts vs. meters(default)\n");
	fprintf(stderr, "\t-n\t\t\tignore nice processes for CPU percentage\n");
	fprintf(stderr, "\t-p <process name>\tdo not count <process name> in CPU percentage\n");
	fprintf(stderr, "\t-h\t\t\tthis help screen\n");
	fprintf(stderr, "\t-v\t\t\tprint the version number\n");
	fprintf(stderr, "\n");
}


void printversion(void)
{
	fprintf(stderr, "wmsysmon v%s\n", WMSYSMON_VERSION);
}

kernel_versions Get_Kernel_version(void)
{
	struct utsname buffer;
	char *p;
    long ver[2];
    int i=0;

    if (uname(&buffer) != 0) {
		return OLDER_2_4;
    }

    p = buffer.release;
    while (*p) {
        if (isdigit(*p)) {
            ver[i] = strtol(p, &p, 10);
            i++;
			if (i == 2) break;
        } else {
            p++;
        }
    }

	if (ver[0] >= 3 || (ver[0] == 2 && ver[1] >= 6)) {
		return NEWER_2_6;
	}
	return OLDER_2_4;
}
