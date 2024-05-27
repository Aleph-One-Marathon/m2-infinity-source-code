/* MOUSE.C
 * Thursday, August 18, 1994 6:45:16 PM (ajr)
 *
 */
 
/* marathon includes */
#include "macintosh_cseries.h"
#include "world.h"
#include "map.h"
#include "player.h"     // for get_absolute_pitch_range()
#include "mouse.h"

/* macintosh includes */
#include <CursorDevices.h>
#include <Traps.h>

#ifdef mpwc
#pragma segment input
#endif

/* constants */
#define _CursorADBDispatch 0xaadb
#define CENTER_MOUSE_X      320
#define CENTER_MOUSE_Y      240

/* private prototypes */
void pin_mouse_deltas(void);
void calculate_mouse_yaw(short minimum_pixel_delta);
static CrsrDevicePtr find_mouse_device(void);
static boolean trap_available(short trap_num);
static TrapType get_trap_type(short trap_num);
static short num_toolbox_traps(void);

#define MBState *((byte *)0x0172)
#define RawMouse *((Point *)0x082c)
#define MTemp *((Point *)0x0828)
#define CrsrNewCouple *((short *)0x08ce)

/* ---------- globals */

static CrsrDevicePtr mouse_device;
static fixed snapshot_delta_yaw, snapshot_delta_pitch, snapshot_delta_velocity;
static boolean snapshot_button_state;

/* ---------- code */

void enter_mouse(
	short type)
{
	mouse_device= find_mouse_device(); /* will use cursor device manager if non-NULL */
#ifndef env68k
	assert(mouse_device); /* must use cursor device manager on non-68k */
#endif
	
	delta_yaw= delta_pitch= delta_velocity= FALSE;
	button_state= FALSE;
	
	return;
}

void test_mouse(
	short type,
	long *action_flags,
	fixed *delta_yaw,
	fixed *delta_pitch,
	fixed *delta_velocity)
{
	if (snapshot_button_state) *action_flags|= _left_trigger_state;
	
	*delta_yaw= snapshot_delta_yaw;
	*delta_pitch= snapshot_delta_pitch;
	*delta_velocity= snapshot_delta_velocity;
	
	return;
}

boolean mouse_available(
	short type)
{
	return;
}

void exit_mouse(
	short type)
{
	return;
}

/* 1200 pixels per second is the highest possible mouse velocity */
#define MAXIMUM_MOUSE_VELOCITY (INTEGER_TO_FIXED(1200)/MACINTOSH_TICKS_PER_SECOND)

/* take a snapshot of the current mouse state */
void mouse_idle(
	short type)
{
	Point where;
	Point center;
	long ticks_elapsed;
	static long last_tick_count;

	GetMouseLocation(&where);

	center.h= CENTER_MOUSE_X, center.v= CENTER_MOUSE_Y;
	SetMouseLocation(&center);
	
	ticks_elapsed= TickCount()-last_tick_count;
	if (ticks_elapsed)
	{
		/* calculate axis deltas */
		fixed vx= INTEGER_TO_FIXED(location.h-center.h)/ticks_elapsed;
		fixed vy= INTEGER_TO_FIXED(location.v-center.v)/ticks_elapsed;
		
		/* pin to maximum velocity (pixels/second) */
		vx= PIN(-MAXIMUM_MOUSE_VELOCITY, MAXIMUM_MOUSE_VELOCITY);
		vy= PIN(-MAXIMUM_MOUSE_VELOCITY, MAXIMUM_MOUSE_VELOCITY);
		
		/* scale to [-FIXED_ONE,FIXED_ONE] */
		vx/= FIXED_INTEGERAL_PART(MAXIMUM_MOUSE_VELOCITY);
		vy/= FIXED_INTEGERAL_PART(MAXIMUM_MOUSE_VELOCITY);

		snapshot_delta_yaw= vx;
		
		switch (type)
		{
			case _mouse_yaw_pitch:
				snapshot_delta_pitch= vy, snapshot_delta_velocity= 0;
				break;
			case _mouse_yaw_velocity:
				snapshot_delta_velocity= vy, snapshot_delta_pitch= 0;
				break;
			
			default:
				halt();
		}
		
		snapshot_button_state= Button();
	}

	return;
}

/* ---------- private code */

// unused 
static void get_mouse_location(
	Point *where)
{
	if (mouse_device)
	{
		where->h = mouse_device->whichCursor->where.h;
		where->v = mouse_device->whichCursor->where.v;
	}
	else
	{
		GetMouse(where);
		LocalToGlobal(where);
	}
	
	return;
}

static void set_mouse_location(
	Point where)
{
	if (mouse_device)
	{
		CrsrDevMoveTo(mouse_device, where.h, where.v);
	}
#ifdef env68k
	else
	{
		RawMouse= where;
		MTemp= where;
		CrsrNewCouple= 0xffff;
	}
#endif
	
	return;
}

static CrsrDevicePtr find_mouse_device(
	void)
{
	CrsrDevicePtr device= (CrsrDevicePtr) NULL;
	
	if (trap_available(_CursorADBDispatch))
	{
		do
		{
			CrsrDevNextDevice(&device);
		}
		while (device && device->devClass!=kDeviceClassMouse && device->devClass!=kDeviceClassTrackball);
	}
		
	return device;
}

/* ---------- from IM */

static boolean trap_available(short trap_num)
{
	TrapType type;
	
	type = get_trap_type(trap_num);
	if (type == ToolTrap)
		trap_num &= 0x07ff;
	if (trap_num > num_toolbox_traps())
		trap_num = _Unimplemented;
	
	return NGetTrapAddress(trap_num, type) != NGetTrapAddress(_Unimplemented, ToolTrap);
}

#define TRAP_MASK  0x0800

static TrapType get_trap_type(short trap_num)
{
	if ((trap_num & TRAP_MASK) > 0)
		return ToolTrap;
	else
		return OSTrap;
}

static short num_toolbox_traps(void)
{
	if (NGetTrapAddress(_InitGraf, ToolTrap) == NGetTrapAddress(0xaa6e, ToolTrap))
		return 0x0200;
	else
		return 0x0400;
}
