/*

Name: Bijay Raj Raut
ID:   1001562222

*/
#include "bitmap.h"

#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

/*
 Struct myvalues keep all the parameters that is going to be passed along side with the
 multiple threads,
 W = image_width, H = image_height, max = maximum number of iteration
 SH = start height for the thread, EH = end height of the thread
 xcenter = x cordinate, y = y cordinate, scale = scale of the image
 bm = bitmap created in main
 */
typedef struct myvalues{
    int W,H,max,threadcount,SH,EH;
    double xcenter,ycenter,scale;
    struct bitmap *bm;
}myvalues;

int iteration_to_color( int i, int max );
int iterations_at_point( double x, double y, int max );
void *myfn(void *values);
void show_help()
{
    printf("Use: mandel [options]\n");
    printf("Where options are:\n");
    printf("-m <max>    The maximum number of iterations per point. (default=1000)\n");
    printf("-x <coord>  X coordinate of image center point. (default=0)\n");
    printf("-y <coord>  Y coordinate of image center point. (default=0)\n");
    printf("-s <scale>  Scale of the image in Mandlebrot coordinates. (default=4)\n");
    printf("-W <pixels> Width of the image in pixels. (default=500)\n");
    printf("-H <pixels> Height of the image in pixels. (default=500)\n");
    printf("-n <thread> n number of the threads. (default=1)\n");
    printf("-o <file>   Set output file. (default=mandel.bmp)\n");
    printf("-h          Show this help text.\n");
    printf("\nSome examples are:\n");
    printf("mandel -x -0.5 -y -0.5 -s 0.2\n");
    printf("mandel -x -.38 -y -.665 -s .05 -m 100\n");
    printf("mandel -x 0.286932 -y 0.014287 -s .0005 -m 1000\n\n");
}
//mutex to preserve the critical region (info for bitmapping the image)
//pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;


int main( int argc, char *argv[] )
{
    char c;

    // These are the default configuration values used
    // if no command line arguments are given.

    const char *outfile = "mandel.bmp";
    double xcenter = 0;
    double ycenter = 0;
    double scale = 4;
    int    image_width = 500;
    int    image_height = 500;
    int    max = 1000;
    int    threadcount = 1;

    // For each command line argument given,
    // override the appropriate configuration value.
    // n saves the number of threads
    while((c = getopt(argc,argv,"x:y:s:W:H:m:o:n:h"))!=-1) {
        switch(c) {
            case 'x':
                xcenter = atof(optarg);
                break;
            case 'y':
                ycenter = atof(optarg);
                break;
            case 's':
                scale = atof(optarg);
                break;
            case 'W':
                image_width = atoi(optarg);
                break;
            case 'H':
                image_height = atoi(optarg);
                break;
            case 'm':
                max = atoi(optarg);
                break;
            case 'o':
                outfile = optarg;
                break;
            case 'n':
                threadcount = atoi(optarg);
                break;
            case 'h':
                show_help();
                exit(1);
                break;
        }
    }

    // Display the configuration of the image.
    printf("mandel: x=%lf y=%lf scale=%lf max=%d thread=%d outfile=%s\n",xcenter,ycenter,scale,max,threadcount,outfile);

    // Create a bitmap of the appropriate size.
    struct bitmap *bm = bitmap_create(image_width,image_height);

    // Fill it with a dark blue, for debugging
    bitmap_reset(bm,MAKE_RGBA(0,0,255,0));

    //rc to check the status of the thread
    //SH = starting height to bitmap for the thread
    //EH = stopping height of each thread
    int t = 0 ,rc,SH = 0,EH = 0;
    //array of struct each will have all the parameters and varying SH & EH depending on the
    //number of threads
    myvalues values[threadcount];
    //array of threads
    pthread_t threads[threadcount];
    //range of each threads bitmapping height(use to slice the image for each threads)
    int range = image_height/threadcount;
    for(t=0;t<threadcount;t++)
    {
        //dividing height by thread may cause some pixel to be lost so
        //to avoid any unprocessed pixel caused by dividing up the height
        //the very last thread will run until image full height
        t==threadcount-1? (EH=image_height) : (EH+=range);
        values[t].xcenter = xcenter;
        values[t].ycenter = ycenter;
        values[t].scale = scale;
        values[t].max = max;
        values[t].W = image_width;
        values[t].bm = bm;
        values[t].H = image_height;
        values[t].threadcount = threadcount;
        values[t].SH = SH;
        values[t].EH = EH;
        //save the end height of the current thread as a starting
        // height for next thread
        SH = EH;
        //spawning off n number of threads, one at a time
        rc=pthread_create(&threads[t],NULL,myfn,(void*)&values[t]);
        if(rc) //handles error formed by not being able to spawn threads
        {
            printf("Error; return code from pthread_create() is %d\n",rc);
            exit(-1);
        }
    }
    //join all the spawned off thread
    for(t=0;t<threadcount;t++)
    {
        rc=pthread_join(threads[t],NULL);
        if(rc)//handles error formed by not being able to joined threads
        {
            printf("Error; return code from pthread_join() is %d\n",rc);
            exit(-1);
        }
    }

    // Compute the Mandelbrot image
    //compute_image(bm,xcenter-scale,xcenter+scale,ycenter-scale,ycenter+scale,max,threadcount);

    // Save the image in the stated file.
    if(!bitmap_save(bm,outfile))
    {
        fprintf(stderr,"mandel: couldn't write to %s: %s\n",outfile,strerror(errno));
        return 1;
    }

    return 0;
}

/*
 Compute a slice of Mandelbrot image for given starting and ending height,
 writing each point to the given bitmap.
 Scale the image to the range (xmin-xmax,ymin-ymax), limiting iterations to "max"
 provided through the structure as a void* argument
*/
void *myfn(void *values)
{
    //pthread_mutex_lock(&mutex);
    //newval will have the address of the typecasted mystruct values
    //retrieving the values of the struct by typecasting it back to the mystruct to copy
    //all the information on bitmapping
    myvalues *newval=(myvalues*)values;
    double xmin = newval->xcenter - newval->scale;
    double xmax = newval->xcenter + newval->scale;
    double ymin = newval->ycenter - newval->scale;
    double ymax = newval->ycenter + newval->scale;
    int startheight = newval->SH;
    int endheight = newval->EH;
    int width = newval->W;
    int max = newval->max;
    int height = newval->H;

    //pthread_mutex_unlock(&mutex);
    // For every pixel in the image...
    int i,j;
    //start from the passed startheight and stop on the endheight
    for(j=startheight;j<endheight;j++)
    {

        for(i=0;i<width;i++)
        {

            // Determine the point in x,y space for that pixel.
            double x = xmin + i*(xmax-xmin)/width;
            double y = ymin + j*(ymax-ymin)/height;

            // Compute the iterations at that point.
            int iters = iterations_at_point(x,y,max);

            // Set the pixel in the bitmap.
            bitmap_set(newval->bm,i,j,iters);
        }
    }

    return NULL;

}

/*
Return the number of iterations at point x, y
in the Mandelbrot space, up to a maximum of max.
*/

int iterations_at_point( double x, double y, int max )
{
    double x0 = x;
    double y0 = y;

    int iter = 0;

    while( (x*x + y*y <= 4) && iter < max ) {

        double xt = x*x - y*y + x0;
        double yt = 2*x*y + y0;

        x = xt;
        y = yt;

        iter++;
    }

    return iteration_to_color(iter,max);
}

/*
Convert a iteration number to an RGBA color.
Here, we just scale to gray with a maximum of imax.
Modify this function to make more interesting colors.
*/
//I have added some different number and used modulus as well and got a picture
//with multi colors. I just changed the values randomly and found the attached
// pattern in evaluation report.
int iteration_to_color( int i, int max )
{
    int gray = 19*i/2*max;
    return MAKE_RGBA(gray%8,gray/2,gray%7,0);
}
