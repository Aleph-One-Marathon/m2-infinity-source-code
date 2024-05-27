/*
MOUSE.C
Tuesday, January 17, 1995 2:51:59 PM  (Jason')
*/

// Work around lack of valid symbols in InterfaceLib
#define NICK_KLEDZIK_IS_A_FUCKER

/* marathon includes */
#include "macintosh_cseries.h"
#ifdef SUPPORT_INPUT_SPROCKET
#include "InputSprocket.h"
#include "macintosh_input.h"
#endif
#include "world.h"
#include "map.h"
#include "player.h"     // for get_absolute_pitch_range()
#include "mouse.h"
#include "shell.h"
#include <math.h>

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

static void get_mouse_location(Point *where);
static void set_mouse_location(Point where);
static CursorDevicePtr find_mouse_device(void);
static boolean trap_available(short trap_num);
static TrapType get_trap_type(short trap_num);
static short num_toolbox_traps(void);

#ifdef NICK_KLEDZIK_IS_A_FUCKER
enum
{
	kDeviceClassTrackPad = 3	// they neglected to publish this
};

extern pascal OSErr CrsrDevNextDevice(CursorDevicePtr *ourDevice)
 TWOWORDINLINE(0x700B, 0xAADB);
extern pascal OSErr CrsrDevMoveTo(CursorDevicePtr ourDevice, long absX, long absY)
 TWOWORDINLINE(0x7001, 0xAADB);
#endif

#define MBState *((byte *)0x0172)
#define RawMouse *((Point *)0x082c)
#define MTemp *((Point *)0x0828)
#define CrsrNewCouple *((short *)0x08ce)

/* ---------- globals */

static CursorDevicePtr mouse_device;
static fixed snapshot_delta_yaw, snapshot_delta_pitch, snapshot_delta_velocity;
static boolean snapshot_button_state;

/* ---------- code */

void enter_mouse(
	short type)
{
	#pragma unused (type)
	
	mouse_device= find_mouse_device(); /* will use cursor device manager if non-NULL */

#ifndef env68k
	vwarn(mouse_device, "no valid mouse/trackball device;g;"); /* must use cursor device manager on non-68k */
#endif
	
	snapshot_delta_yaw= snapshot_delta_pitch= snapshot_delta_velocity= FALSE;
	snapshot_button_state= FALSE;
	
	return;
}

void test_mouse(
	short type,
	long *action_flags,
	fixed *delta_yaw,
	fixed *delta_pitch,
	fixed *delta_velocity)
{
	#pragma unused (type)
	
	if (snapshot_button_state) *action_flags|= _left_trigger_state;
	
	*delta_yaw= snapshot_delta_yaw;
	*delta_pitch= snapshot_delta_pitch;
	*delta_velocity= snapshot_delta_velocity;
	
	return;
}

boolean mouse_available(
	short type)
{
	#pragma unused (type)
	
	return TRUE;
}

void exit_mouse(
	short type)
{
	#pragma unused (type)
	
	return;
}

/* 1200 pixels per second is the highest possible mouse velocity */
#define MAXIMUM_MOUSE_VELOCITY (1200/MACINTOSH_TICKS_PER_SECOND)
//#define MAXIMUM_MOUSE_VELOCITY ((float)1500/MACINTOSH_TICKS_PER_SECOND)

#define DEAD_ZONE ((UInt32) 0x07000000)

#define PIXELS_PER_INCH (72)

#ifdef SUPPORT_INPUT_SPROCKET

UInt32 axis_deadzone(UInt32 axis_value)
{
	UInt32 new_axis_value = axis_value;
	
	if (axis_value > (kISpAxisMiddle + DEAD_ZONE))
		{ new_axis_value -= DEAD_ZONE; }
	else if (axis_value < (kISpAxisMiddle - DEAD_ZONE))
		{ new_axis_value += DEAD_ZONE; }
	else
		{ new_axis_value = kISpAxisMiddle; }
	
	return new_axis_value;
	
}

fixed sprocket_do_xaxis(Boolean running, long ticks_elapsed)
{
	ISpElementReference xLook = input_sprocket_elements[_input_sprocket_yaw];
	ISpElementReference xDelta = input_sprocket_elements[_input_sprocket_yaw_delta];
	fixed delta_x;
	UInt32 int_x;
	float float_x;
	fixed fixed_x;
	OSStatus err;
	
	err = ISpElement_GetSimpleState(xDelta, &delta_x);
	if (err != noErr) { delta_x = 0; } // kISpDeltaMiddle
	
	delta_x *= PIXELS_PER_INCH;

	err = ISpElement_GetSimpleState(xLook, &int_x);
	if (err != noErr) { int_x = kISpAxisMiddle; }

	int_x = axis_deadzone(int_x);

	/* turn into -1 .. +1 */
	float_x = ((float)int_x - (float)kISpAxisMiddle) / (kISpAxisMiddle - DEAD_ZONE);

	/* make non-linear */
	float_x *= float_x;	// square x

	/* restore sign */
	if (int_x < kISpAxisMiddle) { float_x = -float_x; }

	// magnitude modification
	if (running) { float_x *= 15; }	// was 40
	else { float_x *= 10; }
	
	fixed_x = FLOAT_TO_FIXED(float_x);
	fixed_x += delta_x;	// <AMR 4/9/97> reverse axis per ISp engineers
	fixed_x /= (ticks_elapsed*MAXIMUM_MOUSE_VELOCITY);
	fixed_x = PIN(fixed_x, -FIXED_ONE/2, FIXED_ONE/2), fixed_x>>= 1, fixed_x*= (fixed_x<0) ? -fixed_x : fixed_x, fixed_x>>= 14;

	return fixed_x;
}

Fixed sprocket_do_yaxis(Boolean running, long ticks_elapsed)
{
	ISpElementReference yLook = input_sprocket_elements[_input_sprocket_pitch];
	ISpElementReference yDelta = input_sprocket_elements[_input_sprocket_pitch_delta];
	fixed delta_y;
	UInt32 int_y;
	float float_y;
	fixed fixed_y;
	OSStatus err;
	
	err = ISpElement_GetSimpleState(yDelta, &delta_y);
	if (err != noErr) { delta_y = 0; } // kISpDeltaMiddle
	
	delta_y *= PIXELS_PER_INCH;

	err = ISpElement_GetSimpleState(yLook, &int_y);
	if (err != noErr) { int_y = kISpAxisMiddle; }	

	int_y = axis_deadzone(int_y);
	
	/* float_x,float_y = -1 .. +1 */
	float_y = ((float)int_y - (float)kISpAxisMiddle) / (kISpAxisMiddle - DEAD_ZONE);

	/* non-linear & restore the sign */
	float_y *= float_y;	/* square y */
	if (int_y < kISpAxisMiddle) { float_y = -float_y; }	
	
	/* magnitude modification */
	if (running) { float_y *= 30; }	// was 30
	else { float_y *= 10; }
	
	fixed_y = FLOAT_TO_FIXED(float_y);
	fixed_y += delta_y;	// <AMR 4/9/97> reverse axis per ISp engineers
	fixed_y /= (ticks_elapsed*MAXIMUM_MOUSE_VELOCITY);
	fixed_y = PIN(fixed_y, -FIXED_ONE/2, FIXED_ONE/2), fixed_y>>= 1, fixed_y*= (fixed_y<0) ? -fixed_y : fixed_y, fixed_y>>= 14;

	return fixed_y;
}

#endif

/* take a snapshot of the current mouse state */
void mouse_idle(
	short type)
{
	Point where;
	Point center;
	static long last_tick_count;
	long tick_count= TickCount();
	long ticks_elapsed= tick_count-last_tick_count;

#ifdef SUPPORT_INPUT_SPROCKET
	if (use_input_sprocket)
	{
		OSStatus err;
		ISpElementReference run_element = input_sprocket_elements[_input_sprocket_run_dont_walk];
		UInt32 running = 0;
		
		if (ticks_elapsed)
		{
			err = ISpElement_GetSimpleState(run_element, &running);
			if (err != noErr) { running = 0; }
		
			snapshot_delta_yaw		= sprocket_do_xaxis(running, ticks_elapsed);
			snapshot_delta_pitch	= sprocket_do_yaxis(running, ticks_elapsed);
			snapshot_delta_velocity	= 0;
			last_tick_count			= tick_count;

			get_mouse_location(&where);
			center.h= CENTER_MOUSE_X, center.v= CENTER_MOUSE_Y;
			set_mouse_location(center);

			// if the cursor moved this must be a mouse input sprocket does not support
			if ((where.h != center.h) || (where.v != center.v))
			{
				/* calculate axis deltas */ // [-320 * FIXED_ONE, 320*FIXED_ONE] #define INTEGER_TO_FIXED(x) (x*FIXED_ONE)
				fixed vx= INTEGER_TO_FIXED(where.h-center.h)/(ticks_elapsed*MAXIMUM_MOUSE_VELOCITY);
				fixed vy= - INTEGER_TO_FIXED(where.v-center.v)/(ticks_elapsed*MAXIMUM_MOUSE_VELOCITY);
		
				/* pin and do nonlinearity */
				vx= PIN(vx, -FIXED_ONE/2, FIXED_ONE/2), vx>>= 1, vx*= (vx<0) ? -vx : vx, vx>>= 14;
				vy= PIN(vy, -FIXED_ONE/2, FIXED_ONE/2), vy>>= 1, vy*= (vy<0) ? -vy : vy, vy>>= 14;

				snapshot_delta_yaw   = vx;
				snapshot_delta_pitch = vy; 
			}
		}
	}
	else
#endif
	{
		get_mouse_location(&where);
	
		center.h= CENTER_MOUSE_X, center.v= CENTER_MOUSE_Y;
		set_mouse_location(center);
		
		if (ticks_elapsed)
		{
			/* calculate axis deltas */ // [-320 * FIXED_ONE, 320*FIXED_ONE] #define INTEGER_TO_FIXED(x) (x*FIXED_ONE)
			fixed vx= INTEGER_TO_FIXED(where.h-center.h)/(ticks_elapsed*MAXIMUM_MOUSE_VELOCITY);
			fixed vy= - INTEGER_TO_FIXED(where.v-center.v)/(ticks_elapsed*MAXIMUM_MOUSE_VELOCITY);
	
			/* pin and do nonlinearity */
			vx= PIN(vx, -FIXED_ONE/2, FIXED_ONE/2), vx>>= 1, vx*= (vx<0) ? -vx : vx, vx>>= 14;
			vy= PIN(vy, -FIXED_ONE/2, FIXED_ONE/2), vy>>= 1, vy*= (vy<0) ? -vy : vy, vy>>= 14;
//			vx= PIN(vx, -FIXED_ONE/2, FIXED_ONE/2);
//			vy= PIN(vy, -FIXED_ONE/2, FIXED_ONE/2);
	
			snapshot_delta_yaw= vx;
			
			switch (type)
			{
				case _mouse_yaw_pitch:
#ifdef SUPPORT_INPUT_SPROCKET
				case _input_sprocket_yaw_pitch:
#endif
					snapshot_delta_pitch= vy, snapshot_delta_velocity= 0;
					break;
				case _mouse_yaw_velocity:
					snapshot_delta_velocity= vy, snapshot_delta_pitch= 0;
					break;
				
				default:
					halt();
			}
			
			snapshot_button_state= Button();
			last_tick_count= tick_count;
			
//			dprintf("%08x %08x %08x;g;", snapshot_delta_yaw, snapshot_delta_pitch, snapshot_delta_velocity);
		}
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
		*where= RawMouse;
//		GetMouse(where);
//		LocalToGlobal(where);
	}
	
	return;
}

static void set_mouse_location(
	Point where)
{
	if (mouse_device)
	{
#ifdef NICK_KLEDZIK_IS_A_FUCKER
#if 0
		CursorDeviceMoveTo(mouse_device, where.h, where.v);
#else
		CrsrDevMoveTo(mouse_device, where.h, where.v);
#endif
#endif
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

static CursorDevicePtr find_mouse_device(
	void)
{
	CursorDevicePtr device= (CursorDevicePtr) NULL;

#ifdef NICK_KLEDZIK_IS_A_FUCKER
	if (trap_available(_CursorADBDispatch))
	{
		do
		{
#if 0
			CursorDeviceNextDevice(&device);
#else
			CrsrDevNextDevice(&device);
#endif
		}
		while (device &&
				device->devClass!=kDeviceClassMouse &&
				device->devClass!=kDeviceClassTrackball &&
				device->devClass!=kDeviceClassTrackPad);
	}
#endif
		
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
