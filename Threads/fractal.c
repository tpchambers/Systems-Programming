#include "gfx.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <string.h>
#include <complex.h>
#include <pthread.h>

/*
 * Compute the number of iterations at point x, y
 * in the complex space, up to a maximum of maxiter.
 * Return the number of iterations at that point.
 *
 * This example computes the Mandelbrot fractal:
 * z = z^2 + alpha
 *
 * Where z is initially zero, and alpha is the location x + iy
 * in the complex plane.  Note that we are using the "complex"
 * numeric type in C, which has the special functions cabs()
 * and cpow() to compute the absolute values and powers of
 * complex values.
 * */


static int compute_point (double x, double y, int max) {
    double complex z = 0;
    double complex alpha = x + I*y;
    int iter = 0;
    while (cabs(z)<4 && iter < max) {
        z = cpow(z,2) + alpha;
        iter++;
    }
    return iter;

}

/*
 * Compute an entire image, writing each point to the given bitmap.
 * Scale the image to the range (xmin-xmax,ymin-ymax).
 * */

void compute_image ( double xmin, double xmax, double ymin, double ymax, int maxiter) {
    int i, j;

    int width = gfx_xsize();
    int height = gfx_ysize();
       
    for (j = 0; j< height; j ++) {
        for (i=0;i<width;i++) {
            //scale from pixels i, j to coordinates x, y
            double x = xmin + i *(xmax-xmin)/width;
            double y = ymin + j*(ymax-ymin)/height;
            //compute the iterations at x, y 
            int iter = compute_point(x,y,maxiter);
            //convert a iteration number to an RGB color
            // (change this bit to get more interesting colors.)
            int gray = 255 * iter/maxiter;
            gfx_color(gray,gray,gray);
            //plot the point on the screen.
            gfx_point(i,j);
        }
    }
}


int main ( int argc, char * argv[] ) 
{
    //the initial boundaries of the fractal image in x,y space.
   
    double xmin = -1.5;
    double xmax = .5;
    double ymin = -1.0;
    double ymax = 1.0;

    //Maximum number of iterations to compute.
    //Higher values take longer but have more detail.
    int maxiter=100;

    //Open a new window.
    gfx_open(640,480,"Madelbrot Fractal");

    //Show the configuration, just in case you want to recreate it.
    //printf("coordinates: %lf %lf %lf %lf\n", xmin, xmax, ymin, ymax);

    //Fill it with a dark blue initially.
    gfx_clear_color(0,0,255);
    gfx_clear();

    //Display the fractal image
    
    while (1) {
        compute_image(xmin,ymax,ymin,ymax,maxiter);
        printf("coordinates: %lf %lf %lf %lf\n", xmin, xmax, ymin, ymax);
        //Wait for a key or mouse click.
        //grabs the key press
        int c = gfx_wait();
        //finding key mappings
        //printf("c key is : %d\n",c);
        
        //ZOOM IN
        if (c=='+') {
            printf("in\n");
            //we we want the minimums to increase by some size
            //we want maximums to decrease by some size
            xmin += ((xmax-xmin)/3);
            xmax -=  ((xmax-xmin)/3);
            ymin +=  ((ymax-ymin)/3);
            ymax -=  ((ymax-ymin)/3);
        }
        //ZOOM OUT
        else if (c=='-') {
            printf("out\n");
            xmin -= ((xmax-xmin)/3);
            xmax += ((xmax-xmin)/3);
            ymin -= ((ymax-ymin)/3);
            ymax -= ((ymax-ymin)/3);
        }
        //UP
        else if (c==133) {
            ymax += (ymax-ymin)/2;
            ymin += (ymax-ymin)/2;
        
        }
        //DOWN
        else if (c==131) {
            ymax -= (ymax-ymin)/2;
            ymin += (ymax-ymin)/2;
        }
        //LEFT
        else if (c == 130) {
            xmin -= (xmax-xmin)/2;
            xmax -= (xmax-xmin)/2;
        }
        //RIGHT
        else if (c == 132) {
            xmin += (xmax-xmin)/2;
            xmax += (xmax-xmin)/2;
        }
        //center image around mouse click
        else if (c== 1 || c == 2 || c == 3) {
            int x = gfx_xpos();
            int y = gfx_ypos();
            xmin += (xmax-xmin/(x));
            ymin += (ymax-ymin/(y));
            ymax += (ymax-ymin/(y));
            xmax += (xmax-xmin/(x));
        }
        //increment max iter by 100 
        else if (c == 'I') {
            maxiter += 100;
            printf("max iter incremented to:  %d\n", maxiter);
        }
        else if (c== 'D') {
            maxiter -= 100;
            printf("max iter decremented to: %d\n", maxiter);
        }
        //Quit if q is pressed.
        else if (c=='q') exit(0);

    }
    return 0;
}
