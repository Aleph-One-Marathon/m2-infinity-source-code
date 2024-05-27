/*
MYTM.C
Sunday, June 26, 1994 11:10:36 PM

Wednesday, June 29, 1994 10:31:55 PM
	made calls to InsTime() and made myTMReset() work correctly. (ajr)
*/

#include "macintosh_cseries.h"
#include "mytm.h"

/* ---------- globals */

static TimerUPP myTMTaskUPP= (TimerUPP) NULL;

/* ---------- private prototypes */

#ifdef env68k
static pascal void myTMTaskProc(void);
#else
static void myTMTaskProc(TMTaskPtr tmTask);
#endif

/* ---------- code */

myTMTaskPtr myTMSetup(
	long period,
	myTMTaskProcPtr procedure)
{
	myTMTaskPtr myTask= (myTMTaskPtr) NewPtrClear(sizeof(myTMTask));

	if (!myTMTaskUPP) myTMTaskUPP= NewTimerProc(myTMTaskProc);

	if (myTask&&myTMTaskUPP)
	{
#ifdef env68k
		myTask->a5= get_a5();
#endif
		myTask->period= period;
		myTask->procedure= procedure;
		myTask->tmTask.tmAddr= (TimerUPP) myTMTaskUPP;
		
		InsTime((QElemPtr)myTask);
		PrimeTime((QElemPtr)myTask, period);
	}

	return myTask;
}

void myTMReset(
	myTMTaskPtr myTask)
{
	assert(myTask);
	
	RmvTime((QElemPtr)myTask);
	InsTime((QElemPtr)myTask);
	PrimeTime((QElemPtr)myTask, myTask->period);
	
	return;
}

myTMTaskPtr myTMRemove(
	myTMTaskPtr myTask)
{
	if (myTask)
	{
		RmvTime((QElemPtr) &myTask->tmTask);
		DisposePtr((Ptr)myTask);
	}
	
	return (myTMTaskPtr) NULL;
}

/* ---------- private code */

#ifdef env68k
static pascal void myTMTaskProc(
	void)
{
	myTMTaskPtr myTask= get_a1(); /* get the pointer to this Time Manager task record from a1 */
	long old_a5= set_a5(myTask->a5); /* set our a5 world */
	TMTaskPtr tmTask= &myTask->tmTask;
#else
static void myTMTaskProc(
	TMTaskPtr tmTask)
{
	myTMTaskPtr myTask= (myTMTaskPtr) tmTask;
#endif
	
	/* give time to caller; if he returns TRUE, reset the task to fire again */
	if (myTask->procedure())
	{
		PrimeTime((QElemPtr)myTask, myTask->period);
	}
	
#ifdef env68k
	set_a5(old_a5); /* restore whatever a5 was on enter */
#endif

	return;
}
