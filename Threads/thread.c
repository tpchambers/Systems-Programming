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

//struct thread_args
struct thread_args {
    double xmin;
    double xmax;
    double ymin;
    double ymax;
    int maxiter;
    int thread_id;
    int thread_number;
};

//initialize lock
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void * compute_image ( void *arg) {
    int i, j;
    int width = gfx_xsize();
    int height = gfx_ysize();
    
    //uncomment to print out height for debugging
    //printf("height: %d\n",height);
    
    //grab responsible data points from struct
    struct thread_args *args =  arg;
    double xmin = args->xmin;
    double ymin = args->ymin;
    double xmax = args->xmax;
    double ymax = args->ymax;
    int maxiter = args->maxiter;
    
    //here we use the thread number (overall threads inputted), and thread id to generate the image ba    nds
    //this is taken from the struct 
    int thread_number = args->thread_number;
    int thread_id = args->thread_id;
    
    //for ever pixel, i, j in image...
    //For three separate bands of images, if image is 600, works as follows:
    //thread id 0 * 600 / 3 = 0 (start point), thread id 0 + 1 * 600/3 = 200 (end point)
    //thread id 1 * 600 / 3 = 200 (start point), thread id 1 + 1 * 600/3 = 400 (end point)
    //thread id 2 * 600 / 3 = 400 (start point), thread id 2 + 1 * 600/3 = 600 (end point)

    int start = thread_id*height/thread_number;
    int end = (thread_id+1)*height/thread_number;
    
    //debugging for start and end based on thread input
    printf("thread %d says  start: %d\n",(int) pthread_self(),start);
    printf("thread %d says end: %d\n",(int) pthread_self(),end);
    
    
    for (j = start; j< end; j ++) {
        for (i=0;i<width;i++) {
            //scale from pixels i, j to coordinates x, y
            double x = xmin + i *(xmax-xmin)/width;
            double y = ymin + j*(ymax-ymin)/height;
            //compute the iterations at x, y 
            int iter = compute_point(x,y,maxiter);
            //convert a iteration number to an RGB color
            // (change this bit to get more intersting colors.)
            int gray = 102 * iter/maxiter;
            
            //CRITICAL SECTION
            pthread_mutex_lock(&lock); //lock
            gfx_color(204,gray,102);
            gfx_point(i,j);
            pthread_mutex_unlock(&lock); //unlock
        }
    }
    return 0;
}


int main ( int argc, char * argv[] ) 
{
    //the initial boundaries of the fractal image in x,y space.
   
    double xmin = -1.5;
    double xmax = .5;
    double ymin = -1.0;
    double ymax = 1.0;
    int thread_arg;
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
    
    //set default to 8 threads
    thread_arg = 8; 
    
    while (1) {
        printf("coordinates: %lf %lf %lf %lf\n", xmin, xmax, ymin, ymax);
        //create array to track threads, full of struct thread_arg 
        pthread_t p[thread_arg];
        //For each loop, set the struct arguments to the inputs passed in
        //Create the thread for each thread arg desired
        //The thread id represents the integer of the thread (first thread is id 0)
        //The thread number is the length of the threads desired (passed in by gfx_wait() 
        //Pass in the thread_arg struct as the argument, to the compute_image function
        //Fill in the tid array for each thread
        for (int i=0; i< thread_arg; i++) {
            struct thread_args *arg = malloc(sizeof(*arg));
            arg->xmin = xmin;
            arg->ymin = ymin;
            arg->xmax = xmax;
            arg->ymax = ymax;
            arg->thread_id = i;
            arg->thread_number = thread_arg; //number of threads desired
            arg->maxiter = maxiter; 
            pthread_create(&p[i],0,compute_image,arg);
        }
        
        //suspend execution until all threads are terminated
        for (int j = 0; j<thread_arg; j++) {
            pthread_join(p[j],0);
        }


        //Wait for a key or mouse click.
        //grabs the key press
        int c = gfx_wait();
        
        //debug to find key mappings
        //printf("c key is : %d\n",c);
        
        //For thread computation
        //Based on input, change c to actual integer value to store array values
        if (c>='1' && c<= '8') {
            thread_arg = (c-'8')+8;
        }

        //ZOOM IN
        else if (c=='+') {
            //we we want the minimums to increase by some size
            //we want maximums to decrease by some size
            printf("Zooming in!\n");
            xmin += ((xmax-xmin)/6);
            xmax -=  ((xmax-xmin)/6);
            ymin +=  ((ymax-ymin)/6);
            ymax -=  ((ymax-ymin)/6);
        }
        //ZOOM OUT
        else if (c=='-') {
            printf("Zooming out!\n");
            xmin -= ((xmax-xmin)/6);
            xmax += ((xmax-xmin)/6);
            ymin -= ((ymax-ymin)/6);
            ymax -= ((ymax-ymin)/6);
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
        //increment max iter by 50 
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
