/*
SERIAL.C
Tuesday, November 19, 1991 9:14:56 PM

Tuesday, November 26, 1991 11:28:04 PM
	high hopes that this will fix our serial overrun problems of past years; there is a
	reasonably high probability that we had a short/long conflict.
Saturday, December 14, 1991 6:52:25 PM
	then again, shouldn’t MPW have dealt with that gracefully?  in any case, a few more calls
	have been added and the header file update-- this is done now.
Monday, January 6, 1992 4:39:28 PM
	fCTS to FALSE.  is this our ‘DTR’ problem?  we need to send things regardless— and what the
	hell is the deal with System 6.0.x w/o Multifinder?
Tuesday, April 14, 1992 8:57:12 AM
	character losses were never fixed, the  serial protocol code is now being rev’d to trap and
	support this sort of shit.
Wednesday, July 20, 1994 12:46:14 PM
	I'll borrow this for use in Marathon, to talk to the CyberMaxx Tracking Module (VR Helmet)
	I suspect it will change only very little.
*/

#include "macintosh_cseries.h"
#include "serial.h"
#include <string.h>  // memcpy()

/* ---------- constants */

#define MAXIMUM_OUTPUT_CHARACTERS 2048

#define INPUT_BUFFER_SIZE 4096

/* ---------- structures */

struct serial_port_globals
{
	boolean driver_open;
	SPortSel port;
	short input_refnum, output_refnum;
	
	SerShk handshake;
	short configure;
	
	/* used for asynchronous serial output */
	IOParam output_block;
	byte output_buffer[MAXIMUM_OUTPUT_CHARACTERS];
	
	byte input_buffer[INPUT_BUFFER_SIZE];
};

/* ---------- globals */

struct serial_port_globals *serial_port= (struct serial_port_globals *) NULL;

/* ---------- private prototypes */

OSErr open_serial_drivers(void);
OSErr close_serial_drivers(void);

/* ---------- code */
#pragma segment modules

boolean is_serial_port_open(
	void)
{
	if (serial_port&&serial_port->driver_open)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

boolean is_serial_port_in_use(
	SPortSel port)
{
	if (port==serial_port->port&&serial_port->driver_open)
	{
		/* we really mean ‘in use by somebody else’ */
		return FALSE;
	}
	
	return FALSE;

/*	
	switch(port)
	{
		case sPortA:
			return (*((byte*)PortAUse)&0x80) ? FALSE : TRUE;
		
		case sPortB:
			return (*((byte*)PortBUse)&0x80) ? FALSE : TRUE;
	}
*/
}

boolean is_appletalk_active(
	void)
{
	switch(LMGetSPConfig() & 0xf)
	{
		case useFree:
		case useATalk:
			return TRUE;
		
		default:
			return FALSE;
	}
}

OSErr configure_serial_port(
	SPortSel port,
	short configure)
{
	OSErr error= noErr;

	assert(serial_port);
	
	if (serial_port->driver_open&&port!=serial_port->port)
	{
		error= close_serial_drivers();
	}

	serial_port->port= port;
	serial_port->configure= configure;
	
	if (error==noErr)
	{
		if (!serial_port->driver_open)
		{
			error= open_serial_drivers();
		}
	}
	
	if (error==noErr)
	{
		error= SerHShake(serial_port->output_refnum, &serial_port->handshake);
		if (error==noErr)
		{
			error= SerHShake(serial_port->input_refnum, &serial_port->handshake);
			if (error==noErr)
			{
				error= SerReset(serial_port->output_refnum, serial_port->configure);
				if (error==noErr)
				{
					error= SerReset(serial_port->input_refnum, serial_port->configure);
				}
			}
		}
	}
	
	return error;
}

OSErr allocate_serial_port(
	void)
{
	OSErr error= noErr;
	
	assert(!serial_port);
	
	serial_port= (struct serial_port_globals *) NewPtr(sizeof(struct serial_port_globals));
	error= MemError();

	if (error==noErr)
	{
		serial_port->handshake.fXOn= FALSE;
		serial_port->handshake.fCTS= FALSE;
		serial_port->handshake.errs= parityErr|hwOverrunErr|framingErr;
		serial_port->handshake.evts= 0;
		serial_port->handshake.fInX= FALSE;
		serial_port->handshake.fDTR= 0;
		
		serial_port->driver_open= FALSE;
	}
	
	return error;
}

OSErr close_serial_port(
	void)
{
	OSErr error;
	
	assert(serial_port);
	assert(serial_port->driver_open);
	
	error= close_serial_drivers();
	
	return error;
}

OSErr send_serial_bytes(
	byte *buffer,
	short count)
{
	OSErr error= noErr;

	assert(serial_port);
	assert(serial_port->driver_open);
	assert(count<MAXIMUM_OUTPUT_CHARACTERS);
	
	while(serial_port->output_block.ioResult==1);
	error= serial_port->output_block.ioResult;
	if (error==noErr)
	{
		memcpy(serial_port->output_buffer, buffer, count);

		serial_port->output_block.ioCompletion= (ProcPtr) NULL;
		serial_port->output_block.ioResult= noErr;
		serial_port->output_block.ioVRefNum= 0;
		serial_port->output_block.ioRefNum= serial_port->output_refnum;
		serial_port->output_block.ioBuffer= (Ptr) &serial_port->output_buffer;
		serial_port->output_block.ioReqCount= count;
		serial_port->output_block.ioPosMode= fsAtMark;
		serial_port->output_block.ioPosOffset= 0;
	
		error= PBWrite((ParmBlkPtr)&serial_port->output_block, TRUE);
	}
	
	return error;
}

OSErr receive_serial_byte(
	boolean *received,
	byte *buffer)
{
	IOParam input_block;
	OSErr error;
	long count; 

	assert(serial_port);
	assert(serial_port->driver_open);
	
	error= SerGetBuf(serial_port->input_refnum, &count);
	if (error==noErr)
	{
		if (count>=1)
		{
			input_block.ioCompletion= (ProcPtr) NULL;
			input_block.ioRefNum= serial_port->input_refnum;
			input_block.ioBuffer= (Ptr) buffer;
			input_block.ioReqCount= 1;
		
			error= PBRead((ParmBlkPtr)&input_block, FALSE);
			*received= TRUE;
		}
		else
		{
			*received= FALSE;
		}
	}
	
	if (error!=noErr)
	{
		*received= FALSE;
	}

	return error;
}

long get_serial_buffer_size(void)
{
	long count;
	
	SerGetBuf(serial_port->input_refnum, &count);
	
	return count;
}

/* ---------- private code */

OSErr open_serial_drivers(
	void)
{
	OSErr error;
	
	assert(serial_port);
	assert(!serial_port->driver_open);
	
	if (is_serial_port_in_use(serial_port->port))
	{
		return badUnitErr;
	}
	
	switch(serial_port->port)
	{
		case sPortA:
			error= OpenDriver("\p.AOut", &serial_port->output_refnum);
			if (error==noErr)
			{
				error= OpenDriver("\p.AIn", &serial_port->input_refnum);
			}
			break;
			
		case sPortB:
			error= OpenDriver("\p.BOut", &serial_port->output_refnum);
			if (error==noErr)
			{
				error= OpenDriver("\p.BIn", &serial_port->input_refnum);
			}
			break;
		
		default:
			halt();
	}

	if (error==noErr)
	{
		error= SerSetBuf(serial_port->input_refnum, &serial_port->input_buffer, INPUT_BUFFER_SIZE);
		if (error==noErr)
		{
			serial_port->driver_open= TRUE;
			serial_port->output_block.ioResult= noErr;
		}
	}
	
	return error;
}

OSErr close_serial_drivers(
	void)
{
	OSErr error;
	IOParam kill_block;
	
	assert(serial_port);
	assert(serial_port->driver_open);
	
	SerSetBuf(serial_port->input_refnum, (Ptr) NULL, 0);

	kill_block.ioRefNum= serial_port->input_refnum;
	PBKillIO((ParmBlkPtr)&kill_block, FALSE);
	kill_block.ioRefNum= serial_port->output_refnum;
	PBKillIO((ParmBlkPtr)&kill_block, FALSE);
	
	error= CloseDriver(serial_port->output_refnum);
	if (error==noErr)
	{
		error= CloseDriver(serial_port->input_refnum);
	}
	
	serial_port->driver_open= FALSE;
	
	return error;
}
