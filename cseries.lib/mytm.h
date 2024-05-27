/*
MYTM.H
Sunday, June 26, 1994 11:11:20 PM
*/

#include <Timer.h>

#define myTMSetup(period, proc) myTimeManagerSetup(period, proc, FALSE)
#define myXTMSetup(period, proc) myTimeManagerSetup(period, proc, TRUE)

/* ---------- types */

typedef boolean (*myTMTaskProcPtr)(void);

/* ---------- structures */

struct myTMTask
{
	TMTask tmTask;
	
#ifdef env68k
	long a5;
#endif

	long period;
	myTMTaskProcPtr procedure;
	
	boolean active;
	boolean useExtendedTM;
};
typedef struct myTMTask myTMTask, *myTMTaskPtr;

/* ---------- prototypes/MYTM.C */

myTMTaskPtr myTimeManagerSetup(long period, myTMTaskProcPtr procedure, boolean useExtendedTM);
void myTMReset(myTMTaskPtr myTask);
myTMTaskPtr myTMRemove(myTMTaskPtr myTask); /* returns NULL */
