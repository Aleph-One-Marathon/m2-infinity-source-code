#ifndef __MY32BQD_H
#define __MY32BQD_H

/*
MY32BQD.H
Saturday, January 11, 1992 7:40:52 PM
*/

void initialize_my_32bqd(void);

QDErr myNewGWorld(GWorldPtr *offscreenGWorld, short PixelDepth,
	const Rect *boundsRect, CTabHandle cTable, GDHandle aGDevice,
	GWorldFlags flags);
void myDisposeGWorld(GWorldPtr offscreenGWorld);

GWorldFlags myUpdateGWorld(GWorldPtr *offscreenGWorld, short pixelDepth,
    const Rect *boundsRect, CTabHandle cTable, GDHandle aGDevice,
	GWorldFlags flags);

OSErr myQDError(void);

Boolean myLockPixels(GWorldPtr offscreenGWorld);
void myUnlockPixels(GWorldPtr offscreenGWorld);

PixMapHandle myGetGWorldPixMap(GWorldPtr offscreenGWorld);
Ptr myGetPixBaseAddr(GWorldPtr offscreenGWorld);

void myGetGWorld(CGrafPtr *port, GDHandle *gdh);
void mySetGWorld(CGrafPtr port, GDHandle gdh);

#endif
