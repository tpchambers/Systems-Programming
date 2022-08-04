#include "gfx.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <string.h>
#include <complex.h>
#include <pthread.h>
#include <stdbool.h>
#define true 1
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
    int * tasks;
};

//initialize lock
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t task_lock = PTHREAD_MUTEX_INITIALIZER;
int count = 0;
void * compute_image ( void *arg) {
    int width = gfx_xsize();
    int height = gfx_ysize();
    int w;
    //grab responsible data points from struct
    struct thread_args *args =  arg;
    double xmin = args->xmin;
    double ymin = args->ymin;
    double xmax = args->xmax;
    double ymax = args->ymax;
    int maxiter = args->maxiter;
    int* tasks = args->tasks;
   //Task logic:
   //Here,we define a task as computing a single width, along with every increment of the height (task per width)
   //For example, if task 1 is in progress, then width 0 will be computed along with height 0,1,...<=height
   //Each element of the task array represents a single pixel out of the entire width
   //So,we lock the task array, and break out if the task corresponding to width i is equal to 0
   //If so, we set this task to one, and break out of the for loop
   //We then unlock the shared array, and proceed to perform the task
   //We also keep counter to make sure each task is unique
   while (true) { 
        //Locking tasks
        pthread_mutex_lock(&task_lock);
        bool flag = false;
        for (int i = 0; i < width && !flag; i++) {
                if (tasks[i] == 0) {
                    tasks[i] = 1;
                    w = i;
                    count ++;
                    flag = true; //break out of for loop and proceed into task 
                }
        }
        //debugging
        //printf("Counter: %d\n",count);
        pthread_mutex_unlock(&task_lock); // unlocking task array
        //proceed into critical section for each task
        //pthread_mutex_lock(&lock);
        for (int j=0; j<height;j++) {
            //scale from pixels i, j to coordinates x, y
            double x = xmin + w*(xmax-xmin)/width; //use width corresponding to task number
            double y = ymin + j*(ymax-ymin)/height;
            //compute the iterations at x, y 
            int iter = compute_point(x,y,maxiter);
            //convert a iteration number to an RGB color
            // (change this bit to get more intersting colors.)
            int gray = 255 * iter/maxiter;
            pthread_mutex_lock(&lock);
            gfx_color(gray,gray,gray);
            gfx_point(w,j);
            pthread_mutex_unlock(&lock);
        }
        //break once counter reaches the width number
        if (count == width) {
            break;}
        //pthread_mutex_unlock(&lock);
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
    int maxiter=500;

    //Open a new window.
    gfx_open(640,480,"Madelbrot Fractal");

    //Show the configuration, just in case you want to recreate it.
    //printf("coordinates: %lf %lf %lf %lf\n", xmin, xmax, ymin, ymax);

    //Fill it with a dark blue initially.
    gfx_clear_color(0,0,255);
    gfx_clear();
    
    //set default to 4 threads
    thread_arg = 4; 
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
        
        //Malloc the tasks array for the width
        int *tasks;
        int width = gfx_xsize();
        tasks = malloc((width)*sizeof(int));
        for (int i=0; i < width; i++) {
                tasks[i]= 0;
        }
        
        //pass in task pointer array as arg
        for (int i=0; i< thread_arg; i++) {
            struct thread_args *arg = malloc(sizeof(*arg));
            arg->xmin = xmin;
            arg->ymin = ymin;
            arg->xmax = xmax;
            arg->ymax = ymax;
            arg->maxiter = maxiter;
            arg->tasks = tasks;
            pthread_create(&p[i],0,compute_image,arg);
        }
         

        //suspend execution until all threads are terminated
        for (int j = 0; j<thread_arg; j++) {
            pthread_join(p[j],0);
        }
        
        //free memory after threads are joined
        free(tasks);
        
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
