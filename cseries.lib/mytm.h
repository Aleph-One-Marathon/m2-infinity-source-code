/*
MYTM.H
Sunday, June 26, 1994 11:11:20 PM
*/

#include <Timer.h>

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
};
typedef struct myTMTask myTMTask, *myTMTaskPtr;

/* ---------- prototypes/MYTM.C */

myTMTaskPtr myTMSetup(long period, myTMTaskProcPtr procedure);
void myTMReset(myTMTaskPtr myTask);
myTMTaskPtr myTMRemove(myTMTaskPtr myTask); /* returns NULL */
