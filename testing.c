#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <pthread.h>
#include <string.h>

#define RGB_COMPONENT_COLOR 255
#define FILTER_WIDTH 3       
#define FILTER_HEIGHT 3    

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

PPMPixel *read_image(const char *filename, unsigned long int *width, unsigned long int *height);
void write_image(PPMPixel *image, char *filename, unsigned long int width, unsigned long int height);

int main(int argc, char *argv[])
{
    FILE *fp;
    PPMPixel *img;
    unsigned long int width;
    unsigned long int height;
    const char* filename = "photos/falls_2.ppm";
    char* writename = "out.ppm";

    img = read_image(filename, &width, &height);
    write_image(img, writename, width, height);

    free(img);
    
    return 0;
}

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
        perror("Error opening file\n");
        exit (1);
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
    if (fgets(buf, sizeof(buf), fp) == NULL) {
        perror("fgets error\n");
    }
    else if (sscanf(buf, "%ld %ld", &w, &h) == 2) {
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

void write_image(PPMPixel *image, char *filename, unsigned long int width, unsigned long int height) {
    const char* type = "P6";
   
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        fprintf(stderr, "Unable to open file '%s'\n", filename);
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
