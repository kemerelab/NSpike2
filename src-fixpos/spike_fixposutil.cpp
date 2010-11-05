/* 
 * spike_fixpos_util: utility programs for spike_fixpos
 *
 * Copyright 2005 Loren M. Frank
 *
 * This program is part of the nspike data acquisition package.
 * nspike is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * nspike is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with nspike; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "spikecommon.h"
#include "spike_fixpos_defines.h"
#define MAX_CLUSTERS	100
#define NUM_CLUSTERS	3		// the number of desired clusters
#define MAX_CLUSTER_POINTS	1000
#define CLUSTER_RADIUS		3	// 3 pixel radius for inclusion in a cluster
#define MAX_BRIGHT_PIXELS	10000
#define VAR_THRESHOLD		4.0     // the proportion required to prevent
					// a possible direction flip
#define MAX_DIR_CHANGE		0.7854	// pi / 4
#define FLIP_CRITERION		0.2618	// pi / 12
#define MAX_BRIGHT		76800   // for 320x240 pixels ; FIX THIS

typedef struct _Cluster {
    Point 	p[MAX_CLUSTER_POINTS];
    int		npoints;
} Cluster;


extern DiodePosInfo CurrentSpecs;
extern DiodePosInfo NextSpecs;
extern SysInfo sysinfo;

/* the cluster variable is used only when the line fit fails */
Cluster	cluster[MAX_CLUSTERS];

void line_fit (int sumx, int sumy, int *x, int *y, float *dist, int nbright2, float *slope, float *b, int perp);
void sort_index(int n, float a[], int index[]);
void CreateNewCenterOfMass(int sumx, int sumy, int nbright2);
void CreateNewCenter();
void CreateNewCheckPoints();
int CalculateOwnership(int *x, int *y, int nbright2, int ownership_index[], float lastdir, float radius);
int cluster_points(int nbright2, int *x, int *y, float *frontx, float *backx,
	           float *fronty, float *backy, float rad, float lastdir);
float pointdist(Point p1, Point p2);
int IsInPolygon(float wx,float wy,float *xvert,float *yvert,int ncoords);



void position_assign(float *frontx, float *backx, float *fronty, float *backy,
	             int nbright, int *brightpixels, int *flag, float *slope,
		     float *b, float lastdir, int nskipped, float maxassigneddist, Oval *oval, int trackblack, int numlights)
{

    float  sxoss, sumxfloat, nbrightfloat;
    float  xsumbot, ysumbot, xsumtop, ysumtop;
    /* HACK ;  fix dist allocation */
    float  dist[MAX_BRIGHT], max_dist, max_dist2, top_dist, bottom_dist;
    int  ownership_index[MAX_BRIGHT];
    int sumx, sumy, row, dist_ratio, column,i, x[MAX_BRIGHT], y[MAX_BRIGHT], num_bottom, num_top, dist_index[MAX_BRIGHT] ;
    float ave_botx, ave_boty, ave_topx, ave_topy;
    float var_topx, var_topy, var_botx, var_boty;
    float dir, dirdiff;
    float radius;
    int   allowflip;
    int   brightcounter, nbright2;
    int   include;

    float pixeldistance;
    Point tmppoint;



    sumx=0; sumy=0; *flag=0;
    xsumbot=0; xsumtop=0; ysumbot=0; ysumtop=0;

    /* nskipped will normally be zero, so we increment it so that we can
     * multiply it by FLIP_CRITERION to allow us to keep the direction
     * consistent over multiple frames */
    nskipped++;

    /*this if statement was added solely for the purpose of drawing best-fit line -- sets these values in a way
     *that flags output to spike_fixpos.c, and draws a characteristic line -- otherwise the fit would be bad
     * because there were so few pixels, and we might incorrectly think there was a more general problem with
     *fitting
    */



    /*This for loop takes brightpixels, which is a list of positions from 1 to 76,800
     *320*240 array compressed into 1-D, and assigns these positions into a 2-D array
     *First position in this array is (0,0)...sysinfo.posimagesize[0] gives number of columns.
     */


    radius = MAX(oval->majoraxis / 2, oval->minoraxis / 2);
    brightcounter = 0;
    for (i = 0; i < nbright; i++) {
        y[brightcounter]=brightpixels[i]/sysinfo.posimagesize[0];
	x[brightcounter]=brightpixels[i]%sysinfo.posimagesize[0];

	pixeldistance = sqrt(sqr(x[brightcounter] - oval->xcenter) + 
		sqr(y[brightcounter] - oval->ycenter));
        //fprintf(stderr, "%f ", pixeldistance);

	include = 0;
	if (radius == 0) {
	    include = 1;
	}
	else if (pixeldistance < radius) {
	    /* we're within a circle the size of the major axis, so check to se
	     * if we're inside the polygon */
	    if (IsInPolygon(x[brightcounter], y[brightcounter], oval->xcoord,
			oval->ycoord, NUM_OVAL_VERTECES))
		include = 1;
	}
	if (include) {
	    /* there is no exclude oval */
	    sumx += x[brightcounter];
	    sumy += y[brightcounter];
	    brightcounter++;
	}
    }
    nbright2 = brightcounter;

    if ((nbright2<3) || (CurrentSpecs.Front.x == 0)){

	*flag = 1;
	return;
    }

    CreateNewCenterOfMass(sumx, sumy, nbright2); //calculates the average x and y position of the bright pixels

    
    if ((!trackblack)&&(numlights==2)) {
      //CreateNewCenter creates a temporary midpoint between the front and back diodes.
      //It is put in the same relative position to the center of mass as in the previous frame.
      
      
      CreateNewCenter();
      
   
      //CreateNewCheckPoints defines temporary front and back diode positions
      //that are in the same relative positions to the center as in the previous frame.
      
      
      CreateNewCheckPoints();
      
   
      //CalculateOwnership moves all temporary assignments to fit nearby clusters is bright pixels
      //then does error checking.  flag of 1 is returned if the values are not valid
      
      *flag = CalculateOwnership(x, y, nbright2, ownership_index, lastdir, radius);
    }
    if ((trackblack)||(numlights == 1)) {
     
     flag = 0;
     NextSpecs.Center.x = NextSpecs.Front.x = NextSpecs.Back.x = NextSpecs.CenterOfMass.x;
     NextSpecs.Center.y = NextSpecs.Front.y = NextSpecs.Back.y = NextSpecs.CenterOfMass.y;
    }
    
    //if the values were valid, then CurrentSpecs will contain all of the needed info for later use
    CurrentSpecs = NextSpecs;

    return;
}


float pointdist(Point p1, Point p2)
{
   return (float) sqrt((double)(sqr(p1.x - p2.x) + sqr(p1.y - p2.y)));
}


void CreateNewCenterOfMass(int sumx, int sumy, int nbright2) //calculates the average x and y position of the bright pixels
{
	NextSpecs.CenterOfMass.x = sumx/nbright2;
	NextSpecs.CenterOfMass.y = sumy/nbright2;
	return;
}

void CreateNewCenter()  //creates temporary midpoint between the to-be front and back diodes
{
	int oldydifference, oldxdifference;
	oldydifference = (int) (CurrentSpecs.Center.y - 
		CurrentSpecs.CenterOfMass.y);
	oldxdifference = (int) (CurrentSpecs.Center.x - 
		CurrentSpecs.CenterOfMass.x);
	NextSpecs.Center.y = NextSpecs.CenterOfMass.y + oldydifference;
	NextSpecs.Center.x = NextSpecs.CenterOfMass.x + oldxdifference;
	return;
}

void CreateNewCheckPoints()  //assigns temporary front and back diode positions
{
	int OldFrontYDiff, OldFrontXDiff, OldBackYDiff, OldBackXDiff;
	OldFrontYDiff = (int) (CurrentSpecs.Front.y - CurrentSpecs.Center.y);
	OldFrontXDiff = (int) (CurrentSpecs.Front.x - CurrentSpecs.Center.x);
	OldBackYDiff = (int) (CurrentSpecs.Back.y - CurrentSpecs.Center.y);
	OldBackXDiff = (int) (CurrentSpecs.Back.x - CurrentSpecs.Center.x);
	NextSpecs.Front.y = NextSpecs.Center.y + OldFrontYDiff;
	NextSpecs.Front.x = NextSpecs.Center.x + OldFrontXDiff;
	NextSpecs.Back.y = NextSpecs.Center.y + OldBackYDiff;
	NextSpecs.Back.x = NextSpecs.Center.x + OldBackXDiff;
	return;
}

int CalculateOwnership(int *x, int *y, int nbright2, int ownership_index[], float lastdir, float radius)
{
	Point temppoint;
	int i;
	float tempmaxdist;
	int farback;
	float distance1, distance2;
	int sumFrontX, sumFrontY, sumBackX, sumBackY, FrontCount, BackCount;
	sumFrontX = sumFrontY = sumBackX = sumBackY = FrontCount = BackCount = 0;
	float NextDir, CurrentDir, dirdiff;
	float var_frontx, var_fronty, var_backx, var_backy;
	var_frontx = var_fronty = var_backx = var_backy = 0;

	//For each bright pixel, the checkpoint that is closest will 'own' it
	//Assuming that the front and back diodes are separated enough, this should allow us to
	//define each bright pixel as being part of the front or back diode.

	for (i=0; i < nbright2; i++) {
		temppoint.x = x[i];
		temppoint.y = y[i];
		distance1 = pointdist(temppoint, NextSpecs.Front);
		distance2 = pointdist(temppoint, NextSpecs.Back);

		//closer to front checkpoint
		if (distance1 < distance2) {
			ownership_index[i] = 1;
			sumFrontX += (int) temppoint.x;
			sumFrontY += (int) temppoint.y;
			FrontCount++;
		}
		//closer to back checkpoint
		else {
			ownership_index[i] = 2;
			sumBackX += (int) temppoint.x;
			sumBackY += (int) temppoint.y;
			BackCount++;
		}
	}




	if ((FrontCount < 2) || (BackCount < 1)) {
		//if there are not enough brights in front or back, throw the flag
		return 1;
	}

	else {
		//new front diode position is the average of the 'front' pixels
		NextSpecs.Front.y = sumFrontY/FrontCount;
		NextSpecs.Front.x = sumFrontX/FrontCount;

		//find the back pixel that is farthest from the average front
		//this is done in case the back accidentally claimed any of the front pixels
		tempmaxdist = 0;
		farback = 0;
		for (i=0; i < nbright2; i++) {
			if (ownership_index[i] == 2) {
				temppoint.x = x[i];
				temppoint.y = y[i];
				if (pointdist(temppoint, NextSpecs.Front) > tempmaxdist) {
					tempmaxdist = pointdist(temppoint, NextSpecs.Front);
					farback = i;
				}
			}
		}



		NextSpecs.Back.y = y[farback];
		NextSpecs.Back.x = x[farback];

		FrontCount = 0;
		BackCount = 0;
		sumFrontX = 0;
		sumFrontY = 0;
		sumBackX = 0;
		sumBackY = 0;


		//with the landmarks closer to their correct locations, redo the claiming procedure
		for (i=0; i < nbright2; i++) {
			temppoint.x = x[i];
			temppoint.y = y[i];
			distance1 = pointdist(temppoint, NextSpecs.Front);
			distance2 = pointdist(temppoint, NextSpecs.Back);

			//closer to front checkpoint
			if (distance1 < distance2) {
				ownership_index[i] = 1;
				sumFrontX += (int) temppoint.x;
				sumFrontY += (int) temppoint.y;
				FrontCount++;
			}
			//closer to back checkpoint
			else {
				ownership_index[i] = 2;
				sumBackX += (int) temppoint.x;
				sumBackY += (int) temppoint.y;
				BackCount++;
			}
		}

		//now the front and back diodes are the averages of the pixel locations for the front and back
		//if notzero?
		if (FrontCount > 0) {
			NextSpecs.Front.y = sumFrontY/FrontCount;
			NextSpecs.Front.x = sumFrontX/FrontCount;
			NextSpecs.Back.y = sumBackY/BackCount;
			NextSpecs.Back.x = sumBackX/BackCount;
		}
	}

	CurrentDir = atan2(CurrentSpecs.Front.y - CurrentSpecs.Back.y, CurrentSpecs.Front.x - CurrentSpecs.Back.x);
	NextDir = atan2(NextSpecs.Front.y - NextSpecs.Back.y, NextSpecs.Front.x - NextSpecs.Back.x);
	if ((dirdiff = fabs(NextDir-CurrentDir)) > PI) {
		dirdiff = fabs(dirdiff - TWOPI);
    	}
	if ((CurrentDir != -10) && (dirdiff > MAX_DIR_CHANGE)) {
		//if the direction change is too large, exit with a flag
		return 1;
	}

	//calculate the variances of the front and back diodes
	for (i = 0 ; i < nbright2; i++){
		if (ownership_index[i] == 1) {
			var_frontx += sqr(x[i] - NextSpecs.Front.x);
			var_fronty += sqr(y[i] - NextSpecs.Front.y);
		}
		else {
			var_backx += sqr(x[i] - NextSpecs.Back.x);
			var_backy += sqr(y[i] - NextSpecs.Back.y);
		}
	}
	var_frontx /= FrontCount;
	var_fronty /= FrontCount;
	var_backx /= BackCount;
	var_backy /= BackCount;

	//If the variance of the front is not greater than the back, exit with a flag
	if (MAX(var_frontx, var_fronty) < (2 * MAX(var_backx, var_backy))) {
		return 1;
	}

	if (pointdist(NextSpecs.Front, NextSpecs.Back) < 6) {
		return 1;
	}

	//if the exclude ring is on, use it to define a minimum allowable distance
	//between the front and back diodes.
	if ((radius) && (pointdist(NextSpecs.Front, NextSpecs.Back) < (.66 * radius))) {
		return 1;
	}
	NextSpecs.Center.x = (NextSpecs.Front.x + NextSpecs.Back.x)/2;
	NextSpecs.Center.y = (NextSpecs.Front.y + NextSpecs.Back.y)/2;

	return 0;
}


int IsInPolygon(float wx,float wy,float *xvert,float *yvert,int ncoords)
{
int	i;
int	pcross;
float	FY,bx;
    
    if(xvert == NULL) return(0);
    /*
    ** look for odd number of intersections with poly segments
    */
    pcross = 0;
    /*
    ** extend a horizontal line along the positive x axis and
    ** look at intersections
    */
    for(i=0;i<ncoords-1;i++){
	/*
	** only examine segments whose endpoint y coords
	** bracket those of the test coord
	*/
	if((wy > yvert[i] && wy <= yvert[i+1]) ||
	(wy < yvert[i] && wy >= yvert[i+1])){
	    /*
	    ** count those which are on the positive x side 
	    ** by computing the intercept.
	    ** find the x value of the line between the two fcoords
	    ** which is at wy
	    */
/*
	    (yvert[i] - wy)/(xvert[i] - bx) = 
		(yvert[i+1] - wy)/(xvert[i+1] - bx);

	    (f2x - bx)/(f1x - bx) = (f2y - wy)/(f1y - wy);
	    (f2x - bx) = (f1x - bx)*(f2y - wy)/(f1y - wy);
	    bx((f2y-wy)/(f1y-wy) -1)= f1x*(f2y-wy)/(f1y-wy) - f2x;
	    bx = (f1x*(f2y-wy)/(f1y-wy)-f2x)/((f2y-wy)/(f1y-wy)-1)

	    FY = (f2y-wy)/(f1y-wy);
	    bx = (f1x*FY - f2x)/(FY-1);
*/
	    FY = (yvert[i+1]-wy)/(yvert[i]-wy);
	    if(FY == 1){
		pcross++;
	    } else {
		bx = (xvert[i]*FY - xvert[i+1])/(FY-1);

		if(bx >= wx){
		    pcross++;
		}
	    }
	}
    }
    if(i == ncoords-1){
	/*
	** compute the final point which closes the polygon
	*/
	if((wy > yvert[i] && wy <= yvert[0]) ||
	(wy < yvert[i] && wy >= yvert[0])){
	    FY = (yvert[0]-wy)/(yvert[i]-wy);
	    if(FY == 1){
		pcross++;
	    } else {
		bx = (xvert[i]*FY - xvert[0])/(FY-1);
		if(bx > wx){
		    pcross++;
		}
	    }
	}
    }
    /*
    ** now look for an odd number of crossings
    */
    if(pcross > 0 && (pcross%2 == 1)){
	return(1);
    } else {
	return(0);
    }
}


