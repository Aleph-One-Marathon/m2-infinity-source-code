/*
SPHERE.C
Monday, May 23, 1994 11:16:19 PM

Tuesday, May 24, 1994 5:07:28 PM
	dithering, specular reflections not supported yet.
*/

#include "cseries.h"
#include "render.h"

#include <math.h>

#ifdef mpwc
#pragma segment texture
#endif

/* ---------- constants */

#define MAXIMUM_BUFFER_SIZE (30000)

#define AMBIENT_COEFFICIENT 0.00
#define DIFFUSE_COEFFICIENT 1.00
#define SPECULAR_COEFFICIENT 0.00

/* ---------- globals */

static short *spherical_decision_table= (short *) NULL;

/* ---------- code */

#ifdef OBSOLETE
void allocate_spherical_decision_table(
	void)
{
	spherical_decision_table= malloc(MAXIMUM_BUFFER_SIZE*sizeof(short));
	assert(spherical_decision_table);
	
	return;
}

void precalculate_spherical_decision_table(
	short texture_width,
	short texture_height,
	short r)
{
	short *decision_table;
	short x, y;
	long size;
	double l, lx, ly, lz;
	double pi= 4.0*atan(1.0);

	/* size keeps track of how much room is left in our buffer so we can not overflow it */
	size= 0;
	
	/* incident light vector */
	lx= 1.0, ly= 0.5, lz= -0.5, l= sqrt(lx*lx+ly*ly+lz*lz);

	decision_table= spherical_decision_table;

	/* save initial conditions to check against conditions at render-time */
	assert((size+=3)<MAXIMUM_BUFFER_SIZE);
	*decision_table++= r;
	*decision_table++= texture_width;
	*decision_table++= texture_height;
	
	for (y=-r+1;y<=r-1;++y)
	{
		short x0, x1;
		
		x1= sqrt(r*r-y*y);
		if (x1>=r) x1= r-1;
		x0= -x1;
		
		assert((size+=2)<MAXIMUM_BUFFER_SIZE);
		*decision_table++= x0+r-1;
		*decision_table++= x1-x0+1;

		for (x=x0;x<=x1;++x)
		{
			short dx, dy; /* offsets into texture to map this point */
			double cos_alpha; /* alpha is the angle between the normal and incident light vector */
			double theta, phi; /* spherical coordinates of this point */
			double intensity; /* light intensity [0,1] at this point */
			double z; /* depth of this point */
			
			/* do some math */
			assert(r*r>=x*x+y*y);
			z= sqrt(r*r-x*x-y*y);
			cos_alpha= (lx*x+ly*y+lz*z)/(r*l);
			phi= atan(y/sqrt(x*x+z*z)), theta= atan(x/z);
			intensity= AMBIENT_COEFFICIENT+DIFFUSE_COEFFICIENT*(1-cos_alpha)/2;
			dx= texture_width*(theta+pi/2)/(2*pi);
			dy= texture_height*(phi+pi/2)/pi;
			
			/* save the x,y texture offsets and the intensity at this point; the PIN() is a
				gesture toward inaccuracies somewhere in the math above, where we might get
				weird dx,dy values */
			assert((size+=3)<MAXIMUM_BUFFER_SIZE);
			*decision_table++= PIN(dx, 0, texture_width);
			*decision_table++= PIN(dy, 0, texture_height);
			*decision_table++= NUMBER_OF_SHADING_TABLES*intensity;
		}
	}

	/* there is no end-of-table code because we assert the initial conditions of the table
		before trying to use it */

	return;
}

/* phase is in [0,FIXED_ONE) */
void texture_sphere(
	fixed phase,
	struct bitmap_definition *texture,
	struct bitmap_definition *destination,
	pixel8 *shading_tables)
{
	short *decision_table;
	pixel8 *write, **read_row_addresses;
	short y;
	short dx;
	
	dx= (phase*texture->width)>>FIXED_FRACTIONAL_BITS;

	decision_table= spherical_decision_table;
	assert(2*decision_table[0]-1==destination->width);
	assert(destination->width==destination->height);
	assert(decision_table[1]==texture->width);
	assert(decision_table[2]==texture->height);
	decision_table+= 3;

	read_row_addresses= texture->row_addresses;
	for (y=0;y<destination->height;++y)
	{
		pixel8 pixel;
		short offset, count;
		
		write= destination->row_addresses[y] + *decision_table++;
		count= *decision_table++;
		while ((count-=1)>=0)
		{
			offset= (dx+*decision_table++)&(short)0xff;
			pixel= *(read_row_addresses[*decision_table++]+offset);
			*write++= shading_tables[PIXEL8_MAXIMUM_COLORS*(*decision_table++)+pixel];
		}
	}
	
	return;
}
#endif