#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <pthread.h>
#include <string.h>

#define LAPLACIAN_THREADS 4     //change the number of threads as you run your concurrency experiment

/* Laplacian filter is 3 by 3 */
#define FILTER_WIDTH 3       
#define FILTER_HEIGHT 3      

#define RGB_COMPONENT_COLOR 255
#define NUM_THREAD 2

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    unsigned char r, g, b;
} PPMPixel;

struct parameter {
    PPMPixel *image;         //original image pixel data
    PPMPixel *result;        //filtered image pixel data
    unsigned long int w;     //width of image
    unsigned long int h;     //height of image
    unsigned long int start; //starting point of work
    unsigned long int size;  //equal share of work (almost equal if odd)
};


struct file_name_args {
    char *input_file_name;      //e.g., file1.ppm
    char output_file_name[20];  //will take the form laplaciani.ppm, e.g., laplacian1.ppm
};

double total_elapsed_time = 0;


/*This is the thread function. It will compute the new values for the region of image specified in params (start to start+size) using convolution.
    For each pixel in the input image, the filter is conceptually placed on top ofthe image with its origin lying on that pixel.
    The  values  of  each  input  image  pixel  under  the  mask  are  multiplied  by the corresponding filter values.
    Truncate values smaller than zero to zero and larger than 255 to 255.
    The results are summed together to yield a single output value that is placed in the output image at the location of the pixel being processed on the input.
 */
void *compute_laplacian_threadfn(void *params) {
    PPMPixel* image = ((struct parameter*) params)->image;
    PPMPixel* result = ((struct parameter*) params)->result;
    unsigned long int w = ((struct parameter*)params)->w;
    unsigned long int h = ((struct parameter*)params)->h;
    int startWork = ((struct parameter*)params)->start;
    int endWork = startWork + (((struct parameter*)params)->size);
    printf("start work: %d, end work: %d\n", startWork, endWork);
    int laplacian[FILTER_WIDTH][FILTER_HEIGHT] =
    {
        {-1, -1, -1},
        {-1,  8, -1},
        {-1, -1, -1}
    };

    int red, green, blue;
    int x_coordinate, y_coordinate;

    for (int row_iter = startWork; row_iter < endWork; row_iter++) {                            // iterate through the number of rows (ie. 1-> work)
        for (int col_iter = 0; col_iter < w; col_iter++) {                                      // iterate through the row (ie. 0->width)
            // iterate through 2D laplacian array
            for (int i = 0; i < FILTER_HEIGHT; i++) {
                for (int j = 0; j < FILTER_WIDTH; j++) {
                    x_coordinate = (col_iter - FILTER_WIDTH / 2 + j + w) % w;
                    y_coordinate = (row_iter - FILTER_HEIGHT / 2 + i + h) % h;
                    // pthread_mutex_lock(&mutex1);
                    red+= image[y_coordinate * w + x_coordinate].r * laplacian[i][j];
                    green+= image[y_coordinate * w + x_coordinate].g * laplacian[i][j];
                    blue+= image[y_coordinate * w + x_coordinate].b * laplacian[i][j];
                    // pthread_mutex_unlock(&mutex1);
                }
            }
            if (red > 255) red = 255;
            else if (red < 0) red = 0;
            if (green > 255) green = 255;
            else if (green < 0) green = 0;
            if (blue > 255) blue = 255;
            else if (blue < 0) blue = 0;

            result[row_iter * w + col_iter].r = red;
            result[row_iter * w + col_iter].g = green;
            result[row_iter * w + col_iter].b = blue;

            red = 0;
            green = 0;
            blue = 0;
        }
    }
    
    pthread_mutex_unlock(&mutex2);
    return NULL;
}

/* Apply the Laplacian filter to an image using threads.
 Each thread shall do an equal share of the work, i.e. work=height/number of threads. If the size is not even, the last thread shall take the rest of the work.
 Compute the elapsed time and store it in *elapsedTime (Read about gettimeofday).
 Return: result (filtered image)
 */
PPMPixel *apply_filters(PPMPixel *image, unsigned long w, unsigned long h, double *elapsedTime) {
    struct timeval begin, end;
    gettimeofday(&begin, 0);
    int work = h / NUM_THREAD;                                                // work each thread handles
    pthread_t t[NUM_THREAD];                                                  // create size number of threads
    struct parameter data;
    PPMPixel* result = malloc(data.w * data.h * sizeof(PPMPixel));
    data.w = w;
    data.h = h;
    data.result = result;
    
    for(int i = 0; i < NUM_THREAD; i++) {
        pthread_mutex_lock(&mutex2);                                           // lock data before accessing/changing
        data.size = work;
        if ((i == NUM_THREAD - 1) && (NUM_THREAD > 1)) {                       // set the work of last thread to equal height - last starting point
            data.size = data.h - data.start;
        } else if (i == 0) {
            data.start = 0;
        }
        else {
            data.start+= work;                                                 // increment the starting point by *work* amount of rows
        }

        printf("thread: %d, start: %ld\n", i, data.start);

        if (pthread_create(&t[i], NULL, compute_laplacian_threadfn, (void*) &data) != 0)         // create thread and each thread calls compute_laplacian_threadfn
            printf("Unable to create thread %d\n", i);                                             

        // pthread_mutex_unlock(&mutex2);

    }
    for(int i = 0; i<NUM_THREAD; i++)
        pthread_join(t[i], NULL);                                             // join threads

    gettimeofday(&end, 0);
    long sec = end.tv_sec - begin.tv_sec;
    long microsec = end.tv_usec - begin.tv_usec;
    *elapsedTime = sec + microsec*1e-6;
    printf("elapsed time: %.4f\n", *elapsedTime);
    return result;
}

/*Create a new P6 file to save the filtered image in. Write the header block
 e.g. P6
      Width Height
      Max color value
 then write the image data.
 The name of the new file shall be "filename" (the second argument).
 */
void write_image(PPMPixel *image, char *filename, unsigned long int width, unsigned long int height)
{
    const char* type = "P6";
   
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        fprintf(stderr, "Unable to open file '%s' for writing\n", filename);
        exit(1);
    }
    fprintf(fp, "%s\n", type);
    fprintf(fp, "%ld %ld\n", width, height);
    fprintf(fp, "%d\n", RGB_COMPONENT_COLOR);
    for (int i = 0; i < width*height; i++) {
        fwrite(&image[i].r, sizeof(char), sizeof(char), fp);
        fwrite(&image[i].g, sizeof(char), sizeof(char), fp);
        fwrite(&image[i].b, sizeof(char), sizeof(char), fp);
    }
    fclose(fp);
}

/* Open the filename image for reading, and parse it.
    Example of a ppm header:    //http://netpbm.sourceforge.net/doc/ppm.html
    P6                  -- image format
    # comment           -- comment lines begin with
    ## another comment  -- any number of comment lines
    200 300             -- image width & height 
    255                 -- max color value
 
 Check if the image format is P6. If not, print invalid format error message.
 If there are comments in the file, skip them. You may assume that comments exist only in the header block.
 Read the image size information and store them in width and height.
 Check the rgb component, if not 255, display error message.
 Return: pointer to PPMPixel that has the pixel data of the input image (filename).
 The pixel data is stored in scanline order from left to right (up to bottom) in 3-byte chunks (r g b values for each pixel) encoded as binary numbers.
 */
PPMPixel *read_image(const char *filename, unsigned long int *width, unsigned long int *height)
{
    FILE *fp;
    PPMPixel *img;
    unsigned long int w;
    unsigned long int h;
    char buf[1024];
    int line = 0;
    int component;
    unsigned char buffer[3];
    size_t bytesRead;

    fp = fopen(filename, "rb");

    if (fp == NULL) {
        fprintf(stderr, "Unable to open file '%s' for reading\n", filename);
        exit(1);
    }

    // get image format
    if (fgets(buf, sizeof(buf), fp) == NULL) {
        perror("fgets error\n");
    }
    else if (buf[0] != 'P' && buf[1] != '6') {
        printf("Invalid image format (must be 'P6')\n");
    }
    printf("image format: %s", buf);

    // skip comments
    fgets(buf, sizeof(buf), fp);
    while (strstr(buf,"#") == NULL) {
        fgets(buf, sizeof(buf), fp);
    }

    // get height and width
    int num;
    if (fgets(buf, sizeof(buf), fp) == NULL) {
        perror("fgets error\n");
    }
    else if ((num = sscanf(buf, "%ld %ld", &w, &h)) == 2) {
        *width = w;
        *height = h;           
        printf("width: %ld, height: %ld\n", *width, *height);
    }

    // get rgb component
    if (fgets(buf, sizeof(buf), fp) == NULL) {
        perror("fgets error\n");
    }
    else if (sscanf(buf, "%d", &component) == 1) {
        printf("rgb component: %d\n", component);
        if (component != RGB_COMPONENT_COLOR) {
            printf("Invalid rgb component color\n");
        }
    }
    // read up to sizeof(buffer) bytes
    int img_size = (*width) * (*height) * sizeof(PPMPixel);
    img = malloc(img_size);
    PPMPixel* pixel  = malloc(3);
    for (int i = 0; i < ((*width) * (*height)); i++) {
        if (fread(pixel, sizeof(char), sizeof(PPMPixel), fp) == 0) {
            perror("end of file!\n");
        }
        img[i] = *pixel;
    }
    free(pixel);
    fclose(fp);
    return img;
}

/* The thread function that manages an image file. 
 Read an image file that is passed as an argument at runtime. 
 Apply the Laplacian filter. 
 Save the result image in a file called laplaciani.ppm, where i is the image file order in the passed arguments.
 Example: the result image of the file passed third during the input shall be called "laplacian3.ppm".
*/
void *manage_image_file(void *args) {
    printf("debugging: in manage_file\n");
    pthread_mutex_lock(&mutex1);

    struct parameter data;
    data.image = read_image(((struct file_name_args*) args)->input_file_name, &data.w, &data.h);
    data.result = apply_filters(data.image, data.w, data.h, (void*) &total_elapsed_time);
    write_image(data.result,((struct file_name_args*) args)->output_file_name, data.w, data.h);

    pthread_mutex_unlock(&mutex1);

    free(data.image);
    free(data.result);
}
/*The driver of the program. Check for the correct number of arguments. If wrong print the message: "Usage ./a.out filename[s]"
  It shall accept n filenames as arguments, separated by whitespace, e.g., ./a.out file1.ppm file2.ppm    file3.ppm
  It will create a thread for each input file to manage.  
  It will print the total elapsed time in .4 precision seconds(e.g., 0.1234 s). 
  The total elapsed time is the total time taken by all threads to compute the edge detection of all input images .
 */
int main(int argc, char *argv[]) {
    FILE *fp;
    PPMPixel *img;
    unsigned long int width;
    unsigned long int height;
    struct file_name_args arguments;
    if (argc < 2) {
        printf("Usage: ./edge_detector filename[s]");
        exit(1);
    }
    pthread_t t[argc-1];
    for (int i = 1; i < argc; i++) {
        arguments.input_file_name = argv[i];
        snprintf(arguments.output_file_name, sizeof(arguments.output_file_name) + sizeof(int), "laplacian%d.ppm", i);
        if (pthread_create(&t[i-1], NULL, manage_image_file, (void*) &arguments) !=0) {
            perror("cannot create thread\n");
        }
    }

    for (int i = 0; i < argc - 1; i++) {
        pthread_join(t[i], NULL);
    }
    
    return 0;
}

