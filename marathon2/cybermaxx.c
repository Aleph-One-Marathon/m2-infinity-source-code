/**************************************************************************************************
 *
 *                                            cybermaxx.c
 *
 **************************************************************************************************/

/*
Friday, July 29, 1994 11:31:04 AM
	uses binary streaming mode, averaging of buffer at every frame.
*/

#include "macintosh_cseries.h"
#include "serial.h"
#include "world.h"  // for NUMBER_OF_ANGLES

#pragma segment input

/*** constants ***/
// the following constant is to save cybermaxx data that we get into a file.
//#define DO_REPORTING

#define SERIAL_DEFAULTS (baud19200|data8|noParity|stop10)

// constants for the cybermaxx’s commands (most of which are useless to us.)
#define GO_ABSOLUTE_COORDINATE_MODE  'A'
#define GO_RAW_DATA_MODE             'B'
#define GO_HIGH_ACCURACY_MODE        'C'
#define GO_HIGH_SPEED_MODE           'D'
#define GO_ASCII_MODE                'E'
#define GO_BINARY_MODE               'F'
#define GO_POLLING_MODE              'G'
#define GO_STREAMING_MODE            'H'
#define MICROSLOP_MOUSE_EMULATION    'I'
#define FIRST_MOUSE_MODE             '1'
#define SECOND_MOUSE_MODE            '2'
#define SEND_CURRENT_COORDINATES     'S'
#define HARD_RESET                   'R'

// other constants
#define ASCII_BUFFER_SIZE  20
#define BINARY_BUFFER_SIZE  6
#define CYBERMAXX_BUFFER_SIZE 1024
#define CYBERMAXX_QUEUE_SIZE  10

// empirically determined
// if the angle doesn't change by this much, then we won't tell the calling routine
// that the angle has changed. this prevents some of the jitter.
#define YAW_JITTER    0x5b  // 1 degree (360 degree untis)
#define PITCH_JITTER  0xb6  // 1/2 degree (360 degree units)

// if a change in angle is greater than these constants for a short time, we ignore the change.
#define YAW_SPIKE    0x0400 // 11 degrees
#define PITCH_SPIKE  0x1000 // 11 degrees

enum // what mode we are using for the cybermaxx
{
	_cybermaxx_uninitialized,
	_cybermaxx_ascii_mode,
	_cybermaxx_binary_mode
};

struct cybermaxx_data // used to queue stuff up
{
	word yaw, pitch, roll;
};

/*** Global variables ***/
static short current_cybermaxx_mode = _cybermaxx_uninitialized;
static struct cybermaxx_data cybermaxx_queue[CYBERMAXX_QUEUE_SIZE];
static byte cybermaxx_buffer[CYBERMAXX_BUFFER_SIZE];
static long queue_offset; // into cybermaxx_queue;

/*** Private Functions ***/
static OSErr get_cybermaxx_data(byte *buffer, short *count);
static void convert_ascii_cybermaxx_data(float yaw, float pitch, fixed *new_yaw, fixed *new_pitch);
static void convert_binary_cybermaxx_data(word yaw, word pitch, fixed *new_yaw, fixed *new_pitch);
static void read_cybermaxx_buffer(byte *buffer);
static void parse_cybermaxx_buffer(short *buffer, long size);
static void average_cybermaxx_queue(word *avg_yaw, word *avg_pitch, word *avg_roll);
static void remove_cybermaxx_connection(void);
static word cybermaxx_angle_distance(word theta1, word theta2);

/*----------------------------------------------------------------------------------
 *
 *                                   public code
 *
 *----------------------------------------------------------------------------------*/

/************************************************************************************
 *
 * Function: initialize_cybermaxx
 * Purpose:  get ready to receive from the cybermaxx
 *
 ************************************************************************************/
OSErr initialize_cybermaxx(void)
{
	char   initialize_string[5];
	short  i;
	OSErr  error;
	
	current_cybermaxx_mode = _cybermaxx_uninitialized;
	error = allocate_serial_port();
	if (error == noErr)
	{
		Handle resource= GetResource('sprt', 128);
		short port= sPortA; /* default is modem port */
		
		if (resource && GetHandleSize(resource) == 1 && (**resource == 0 || **resource == 1))
			port= **resource;
		
		error = configure_serial_port(port, SERIAL_DEFAULTS);
		if (error == noErr)
		{
			// some of these are defaults, but i prefer to set them anyway.
			initialize_string[0] = HARD_RESET;
			initialize_string[1] = GO_ABSOLUTE_COORDINATE_MODE;
			initialize_string[2] = GO_HIGH_ACCURACY_MODE;
			initialize_string[3] = GO_BINARY_MODE;
			initialize_string[4] = GO_STREAMING_MODE;
			
			error = send_serial_bytes(initialize_string, 4);
			if (error == noErr)
				current_cybermaxx_mode = _cybermaxx_binary_mode;
		}
	}
	
	// put error reporting in here.
	
	for (i = 0; i < CYBERMAXX_QUEUE_SIZE; i++)
	{
		(cybermaxx_queue+i)->yaw= (cybermaxx_queue+i)->pitch = (cybermaxx_queue+i)->roll = 0;
	}
	
	atexit(remove_cybermaxx_connection);
	
	return error;
}

/************************************************************************************
 *
 * Function: get_cybermaxx_coordinates
 * Purpose:  gets some coordinates from the cybermaxx that can be plugged into the
 *           game. it's up to the caller to do the right thing with the yaw and 
 *           pitch. (probably call instantiate_absolute_positioning_information)
 *
 ************************************************************************************/
OSErr get_cybermaxx_coordinates(fixed *yaw, fixed *pitch)
{
	       char    send_command = SEND_CURRENT_COORDINATES;
	       char    buffer[ASCII_BUFFER_SIZE+1];
	       word    binary_yaw, binary_pitch, binary_roll;
	static word    last_binary_yaw, last_binary_pitch;
	       short   count;
	       float   ascii_yaw, ascii_pitch, ascii_roll;
   	       OSErr   error;

	error = noErr;
	if (error == noErr)
	{
		switch(current_cybermaxx_mode)
		{
			case _cybermaxx_uninitialized:
				vhalt("HONK!!! Can’t get data from cybermaxx without initializing it first!");
				break;
			case _cybermaxx_ascii_mode:
				error = send_serial_bytes(&send_command, 1);
				count = ASCII_BUFFER_SIZE;
				get_cybermaxx_data((byte *) buffer, &count);
				buffer[18] = 0; // ignore the <cr><lf>
				sscanf(buffer, "Y%fP%fR%f", &ascii_yaw, &ascii_pitch, &ascii_roll);				
				convert_ascii_cybermaxx_data(ascii_yaw, ascii_pitch, yaw, pitch);
			case _cybermaxx_binary_mode:

#ifdef OBSOLETE // to switch into a “raw mode”
				error = send_serial_bytes(&send_command, 1);
				count = sizeof(binary_yaw); get_cybermaxx_data((byte *) &binary_yaw, &count);
				count = sizeof(binary_pitch); get_cybermaxx_data((byte *) &binary_pitch, &count);
				count = sizeof(binary_roll); get_cybermaxx_data((byte *) &binary_roll, &count);
//				if (abs(last_binary_yaw - binary_yaw) < YAW_JITTER)
//					binary_yaw = last_binary_yaw;
//				if (abs(last_binary_pitch - binary_pitch) < PITCH_JITTER)
//					binary_pitch = last_binary_pitch;
				last_binary_yaw = binary_yaw;
				last_binary_pitch = binary_pitch;
				convert_binary_cybermaxx_data(binary_yaw, binary_pitch, yaw, pitch);
#endif
				read_cybermaxx_buffer(cybermaxx_buffer);
				average_cybermaxx_queue(&binary_yaw, &binary_pitch, &binary_roll);
//				dprintf("y, p = %d %d", binary_yaw, binary_pitch);
				if (cybermaxx_angle_distance(binary_yaw, last_binary_yaw) < YAW_JITTER)
					binary_yaw = last_binary_yaw;
				if (abs(binary_pitch-last_binary_pitch) < PITCH_JITTER)
					binary_pitch = last_binary_pitch;
				convert_binary_cybermaxx_data(binary_yaw, binary_pitch, yaw, pitch);
				last_binary_yaw = binary_yaw; last_binary_pitch = binary_pitch;
				break;
			default:
				halt();
		}
	}
	
	return noErr;
}

/*----------------------------------------------------------------------------------
 *
 *                                   private code
 *
 *----------------------------------------------------------------------------------*/

/************************************************************************************
 *
 * Function: get_cybermaxx_data
 * Purpose:  receive a certain number of bytes from the cybermaxx through the
 *           serial port.
 *
 ************************************************************************************/
static OSErr get_cybermaxx_data(byte *buffer, short *count)
{
	short    i;
	OSErr    error;
	boolean  received;
	
	for (i = 0, error = noErr; i < *count && error == noErr; )
	{
		error = receive_serial_byte(&received, buffer + i);
		if (error == noErr && received)
			i++;
	}
	
	*count = i;
	
	return error;
}

/************************************************************************************
 *
 * Function: convert_ascii_cybermaxx_data
 * Purpose:  we have some floats from the cybermaxx and need to convert them 
 *           to fixed numbers in jason’s system.
 *
 ************************************************************************************/
static void convert_ascii_cybermaxx_data(float yaw, float pitch, fixed *new_yaw, fixed *new_pitch)
{
	float converted_yaw, converted_pitch;
	
	converted_yaw = yaw * NUMBER_OF_ANGLES / 360;
	converted_pitch = pitch * NUMBER_OF_ANGLES / 360;
	
	*new_yaw = INTEGER_TO_FIXED((short) converted_yaw);
	*new_pitch = INTEGER_TO_FIXED((short) converted_pitch);
}

/************************************************************************************
 *
 * Function: convert_binary_cybermaxx_data
 * Purpose:  same as above, but with cybermaxx's binary data.
 *
 ************************************************************************************/
static void convert_binary_cybermaxx_data(word yaw, word pitch, fixed *new_yaw, fixed *new_pitch)
{
	*new_yaw =  yaw << (FIXED_FRACTIONAL_BITS + 10 - 16);
//	if (*new_yaw >= INTEGER_TO_FIXED(HALF_CIRCLE))
//	{
//		*new_yaw -= INTEGER_TO_FIXED(FULL_CIRCLE); // we want [-180, 180] not [0, 360]
//	}
	*new_pitch = pitch << (FIXED_FRACTIONAL_BITS + 7 - 15);
	*new_pitch -= INTEGER_TO_FIXED(EIGHTH_CIRCLE); // change range from [0, 90] to [-45, 45]
}

/************************************************************************************
 *
 * Function: read_cybermaxx_buffer
 * Purpose:  get all the pending data from the cybermaxx
 *
 ************************************************************************************/
static void read_cybermaxx_buffer(byte *buffer)
{
	byte     junk;
	long     max = get_serial_buffer_size();
	long     count = MIN(CYBERMAXX_BUFFER_SIZE, max);
	OSErr    error;
	short    i;
	boolean  received;

	for (i = 0; i < count; i++)
	{
		error = receive_serial_byte(&received, buffer + i);
		vassert(error == noErr, csprintf(temporary, "error = %d", error));
	}
	// finish off the buffer, so as not to leave crap in it.
	for (i = count; i < max; i++)
	{
		receive_serial_byte(&received, &junk);
	}

	if (count)
	{
//		dprintf("buffer (%d bytes):;dm #%d #%d", count, buffer, count);
		parse_cybermaxx_buffer((short *) buffer, count);
	}
}

/************************************************************************************
 *
 * Function: parse_cybermaxx_buffer
 * Purpose:  look through the cybermaxx input, and shove it into the queue
 *
 ************************************************************************************/
static void parse_cybermaxx_buffer(short *buffer, long size)
{
	byte *start;

#ifdef DO_REPORTING
	FILE *fp;
	fp = fopen("cybermaxx recording", "a");
#endif
	
	// find first separator (0xffff)
	start = (byte *)buffer;
	while (*start != 0xff && size > 0)
	{
		*start++;
		size--;
	}
	// skip the separator
	while (*start == 0xff && size > 0)
	{
		*start++;
		size--;
	}
	buffer = (short *) start;
	while (size > 0)
	{
		(cybermaxx_queue+queue_offset)->yaw = *buffer++; size -= sizeof(short);
		(cybermaxx_queue+queue_offset)->pitch = *buffer++; size -= sizeof(short);
		(cybermaxx_queue+queue_offset)->roll = *buffer++; size -= sizeof(short);
#ifdef DO_REPORTING
		fprintf(fp, "y, p, r = %x %x %x (#%d #%d #%d)\n",
			(cybermaxx_queue+queue_offset)->yaw, (cybermaxx_queue+queue_offset)->pitch, (cybermaxx_queue+queue_offset)->roll,
			(cybermaxx_queue+queue_offset)->yaw, (cybermaxx_queue+queue_offset)->pitch, (cybermaxx_queue+queue_offset)->roll);
#endif
		queue_offset++;
		if (queue_offset >= CYBERMAXX_QUEUE_SIZE)
			queue_offset = 0;

		// skip the next separator
		buffer++;
		size -= sizeof(short);
	}

#ifdef DO_REPORTING	
	fclose(fp);
#endif
}

/************************************************************************************
 *
 * Function: average_cybermaxx_queue
 * Purpose:  take our cybermaxx queue and average what's inside it.
 *
 ************************************************************************************/
static void average_cybermaxx_queue(word *avg_yaw, word *avg_pitch, word *avg_roll)
{
	       long    yaw, pitch, roll, total_yaw, total_pitch, total_roll;
	       long    first_yaw, first_pitch;
	static long    last_yaw = NONE, last_pitch = NONE;	
   	       short   i, count;

#ifdef DO_REPORTING
	FILE  *fp;

	fp = fopen("spike data", "a");
#endif

	total_yaw = total_pitch = total_roll = 0;	
	for (i = 0, count = 0; i < CYBERMAXX_QUEUE_SIZE; i++)
	{
		yaw   = (cybermaxx_queue+i)->yaw;
		pitch = (cybermaxx_queue+i)->pitch;
		roll  = (cybermaxx_queue+i)->roll;
		
		last_yaw = yaw; last_pitch = pitch;
		
		if (i == 0)
		{
			first_yaw = yaw; first_pitch = pitch;
		}
		else
		{
			long d0, d1, d2, t1, t2;
			
			t1 = yaw - 0x8000; t2 = yaw + 0x8000;
			d0 = abs(yaw - first_yaw); d1 = abs(t1 - first_yaw); d2 = abs(t2 - first_yaw);
			if (d1 <= d0 && d1 <= d2)
				yaw = t1;
			if (d2 <= d0 && d2 <= d0)
				yaw = t2;

			t1 = pitch - 0x8000; t2 = pitch + 0x8000;
			d0 = abs(pitch - first_pitch); d1 = abs(t1 - first_pitch); d2 = abs(t2 - first_pitch);
			if (d1 <= d0 && d1 <= d2)
				pitch = t1;
			if (d2 <= d0 && d2 <= d0)
				pitch = t2;
		}
		
		// attempt spike removal.
		if (last_yaw != NONE && last_pitch != NONE)
		{
			if (cybermaxx_angle_distance(yaw, last_yaw) > YAW_SPIKE || abs(pitch - last_pitch) > PITCH_SPIKE
				|| yaw > 0x7fff || pitch > 0x7fff)
			{
#ifdef DO_REPORTING
				fprintf(fp, "dropping a spike: %x %x last yp = %x %x\n", yaw, pitch, last_yaw, last_pitch);
#endif
				continue;
			}
			else
				count++;
		}
		else
			count++;
			
		total_yaw += yaw; total_pitch += pitch; total_roll += roll;
	}
	
	if (count)
	{
		*avg_yaw   = total_yaw   / count;
		*avg_pitch = total_pitch / count;
		*avg_roll  = total_roll  / count;
	}
	else
	{
		*avg_yaw   = last_yaw;
		*avg_pitch = last_pitch;
		*avg_roll  = roll;
	}
	
	// get it into the correct range
	*avg_yaw   &= 0x7fff;
	*avg_pitch &= 0x7fff;
	
#ifdef DO_REPORTING
	fclose(fp);
#endif
}


/************************************************************************************
 *
 * Function: remove_cybermaxx_connection
 * Purpose:  close the connection to the cybermaxx
 *
 ************************************************************************************/
static void remove_cybermaxx_connection(void)
{
	if (current_cybermaxx_mode != _cybermaxx_uninitialized)
	{
		close_serial_port();
		current_cybermaxx_mode = _cybermaxx_uninitialized;
	}
}

/************************************************************************************
 *
 * Function: cybermaxx_angle_distance
 * Purpose:  given two yaws in the range [0, 0x7fff], returns a distance. that range
 *           corresponds to [0, 360) degrees
 *
 ************************************************************************************/
static word cybermaxx_angle_distance(word theta1, word theta2)
{
	word d1, d2, d3, d;
	
	d1 = abs(theta1 - theta2);
	d2 = abs(theta1 - (theta2 + 0x8000));
	d3 = abs(theta1 - (theta2 - 0x8000));
	
	d = MIN(MIN(d1, d2), d3);
	
	return d;
}
