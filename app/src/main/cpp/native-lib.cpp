#include <jni.h>
#include <string>
#include <cpu-features.h>
#include <cmath>    // for PI, ceil, sqrt
#include <iostream>

using namespace std;

/* function prototype */
void kernel(double* blurKernel,int r, float sigma);

double conv(double* pixels, int index,int width, int height, double* blurKernel, int r);

double blur(double* pixels, int currIndex, int width, int height, float sigma, int dim1, int dim2, int isGradient);


extern "C"
JNIEXPORT jint JNICALL  // JNI -> Java Native Interface
Java_edu_asu_ame_meteor_speedytiltshift2022_SpeedyTiltShift_tiltshiftcppnative(JNIEnv *env,
                                                                               jobject instance,
                                                                               jintArray inputPixels_,
                                                                               jintArray outputPixels_,
                                                                               jint width,
                                                                               jint height,
                                                                               jfloat sigma_far,
                                                                               jfloat sigma_near,
                                                                               jint a0, jint a1,
                                                                               jint a2, jint a3) {
    jint *pixels = env->GetIntArrayElements(inputPixels_, NULL);
    jint *outputPixels = env->GetIntArrayElements(outputPixels_, NULL);

    int size = width*height;

    // declare pointers of arrays on the heap

    double *blue = new double[size];
    double *red = new double[size];
    double *green = new double[size];

    // Separate colors from pixels
    for (int i{};i<size;i++) {
        blue[i] = static_cast<double>(pixels[i]%0x100);
        green[i] = static_cast<double>((pixels[i]>>8)%0x100);
        red[i] = static_cast<double>((pixels[i]>>16)%0x100);
    }

    /** *********************************************************************
     *  Gaussian Blur filter
     *  size of kernel = [2r+1]x[2r+1], where r = Math.ceil(2*sigma)
     *  then,
     *      x = [-r, r]
     *      u = [-r, r]
     *  Blur(int[] pixelsOut, int currIndex, int width, float sigma)
     * **********************************************************************/

    double* qBlue = new double[size];
    double* qGreen = new double[size];
    double* qRed = new double[size];

    /* Horizontal convolution */
    for (int i{}; i < size; i++) {
        /** Define local variables:
         *      isGradient is a classifier for which gradient to use with 0 = no gradient, 1 = far gradient, and 2 = near gradient
         *      isBlur checks if the image should be blurred, dim1 and dim2 are assigned to a0,a1,a2,a3 depending on the type of blur, sigma assigns the sigma to use
         */
        int isGradient {};
        bool isBlur {false};
        int dim1 {};
        int dim2 {};
        float sigma {};

        if (i < (a0+1)*width) {  // Far Gaussian blur   // height: 0~a0, width: input.getWidth()
            isGradient = 0;
            sigma = sigma_far;
            isBlur = true;
        } else if ((i < (a1+1)*width) && (i >= ((a0+1)*width) )) {    // Gradient Far Gaussian blur
            isGradient = 1;
            sigma = sigma_far;
            isBlur = true;
            dim1 = a1;
            dim2 = a0;
        } else if ((i < (a2+1)*width) && (i >= ((a1+1)*width))) { // Focused / no blur
            isBlur = false;
        } else if ((i < (a3+1)*width) && (i >= ((a2+1)*width))) { // Gradient Near Gaussian blur
            isGradient = 2;
            sigma = sigma_near;
            isBlur = true;
            dim1 = a3;
            dim2 = a2;
        } else {  // Near Gaussian blur
            isGradient = 0;
            sigma = sigma_near;
            isBlur = true;
        }

        //Check to see if blur should be applied, if no blur applied keep colors the same
        if (isBlur) {
            qBlue[i] = blur(blue, i, width, height, sigma, dim1, dim2, isGradient);
            qGreen[i] = blur(green, i, width, height, sigma, dim1, dim2, isGradient);
            qRed[i] = blur(red, i, width, height, sigma, dim1, dim2, isGradient);
        } else{
            qBlue[i] = blue[i];
            qGreen[i] = green[i];
            qRed[i] = red[i];
        }
    }

    /* Vertical convolution */
    for (int i{}; i < size; i++) {
        /** Define local variables:
         *      isGradient is a classifier for which gradient to use with 0 = no gradient, 1 = far gradient, and 2 = near gradient
         *      isBlur checks if the image should be blurred, dim1 and dim2 are assigned to a0,a1,a2,a3 depending on the type of blur, sigma assigns the sigma to use
         */
        int isGradient {};
        bool isBlur {false};
        int dim1 {};
        int dim2 {};
        float sigma {};

        if (i < (a0+1)*width) {  // Far Gaussian blur   // height: 0~a0, width: input.getWidth()
            isGradient = 0;
            sigma = sigma_far;
            isBlur = true;
        } else if ((i < (a1+1)*width) && (i >= ((a0+1)*width) )) {    // Gradient Far Gaussian blur
            isGradient = 1;
            sigma = sigma_far;
            isBlur = true;
            dim1 = a1;
            dim2 = a0;
        } else if ((i < (a2+1)*width) && (i >= ((a1+1)*width))) { // Focused / no blur
            isBlur = false;
        } else if ((i < (a3+1)*width) && (i >= ((a2+1)*width))) { // Gradient Near Gaussian blur
            isGradient = 2;
            sigma = sigma_near;
            isBlur = true;
            dim1 = a3;
            dim2 = a2;
        } else {  // Near Gaussian blur
            isGradient = 0;
            sigma = sigma_near;
            isBlur = true;
        }

        //Check to see if blur should be applied, if no blur applied keep colors the same
        int B{}, G{}, R{};
        if (isBlur) {
            B = ceil(blur(qBlue, i, width, height, sigma, dim1, dim2, isGradient));
            G = ceil(blur(qGreen, i, width, height, sigma, dim1, dim2, isGradient));
            R = ceil(blur(qRed, i, width, height, sigma, dim1, dim2, isGradient));
            cout << "isBlur at " << i << endl;
        } else{
            B = static_cast<int>(blue[i]);
            G = static_cast<int>(green[i]);
            R = static_cast<int>(red[i]);
        }
        int color = (0xff) << 24 | (R & 0xff) << 16 | (G & 0xff) << 8 | (B & 0xff);
        outputPixels[i]=color;
    }


    /***********************************************************************
     * output
    * **********************************************************************/
    env->ReleaseIntArrayElements(inputPixels_, pixels, 0);
    env->ReleaseIntArrayElements(outputPixels_, outputPixels, 0);

    // Deleting Pointers to prevent memory leaks
    delete[] red;
    delete[] green;
    delete[] blue;

    delete[] qBlue;
    delete[] qGreen;
    delete[] qRed;

    return 0;
}
extern "C"
JNIEXPORT jint JNICALL
Java_edu_asu_ame_meteor_speedytiltshift2022_SpeedyTiltShift_tiltshiftneonnative(JNIEnv *env,
                                                                                jclass instance,
                                                                                jintArray inputPixels_,
                                                                                jintArray outputPixels_,
                                                                                jint width,
                                                                                jint height,
                                                                                jfloat sigma_far,
                                                                                jfloat sigma_near,
                                                                                jint a0, jint a1,
                                                                                jint a2, jint a3) {
    jint *pixels = env->GetIntArrayElements(inputPixels_, NULL);
    jint *outputPixels = env->GetIntArrayElements(outputPixels_, NULL);

    for (int j=0;j<height;j++){
        for (int i=0;i<width;i++) {
            int B = pixels[j*width+i]%0x100;
            int G = (pixels[j*width+i]>>8)%0x100;
            int R = (pixels[j*width+i]>>16)%0x100;
            int A = 0xff;
            R=0;
            int color = (A & 0xff) << 24 | (R & 0xff) << 16 | (G & 0xff) << 8 | (B & 0xff);

            outputPixels[j*width+i]=color;
        }
    }

    env->ReleaseIntArrayElements(inputPixels_, pixels, 0);
    env->ReleaseIntArrayElements(outputPixels_, outputPixels, 0);
    return 0;
}
/***********************************************************************
* Gaussian Blur methods implementation
* Blur(int[] pixelsOut, int currIndex, int width)
* **********************************************************************/
void kernel(double* blurKernel,int r, float sigma) {
    double denom = sqrt(1.0/(2*M_PI*sigma*sigma));
    for (int k{}; k < 2*r+1; k++) {
        blurKernel[k] = ( denom * exp((-(k-r)*(k-r))/(2.0*sigma*sigma) ));
    }
}

double conv(double* pixel, int index,int width, int height, double* blurKernel, int r) {
    int x = index % width;
    int y = index / width;
    float temp = 0;
    for (int j {-r}; j <= r; j++) {
        if (y+j < 0 || y+j >= height) {  // edge cases
            temp += blurKernel[j+r] * pixel[(y-j)*width+x];   // symmetric padding
        } else {
            temp += blurKernel[j+r] * pixel[(y+j)*width+x];
        }
    }
    return temp;
}

double blur(double* pixels, int currIndex, int width, int height, float sigma, int dim1, int dim2, int isGradient) {
    float newSig;
    // Check to see which gradient calculation should be done, isGradient = 1 is far blur, isGradient = 2 is near blur
    if (isGradient == 1){
        newSig = sigma * ((dim1 - (static_cast<float> (currIndex)) / width)/(dim1 - dim2)); // height = a1 - a0
    } else if (isGradient == 2) {
        newSig = sigma * (((static_cast<float> (currIndex)) / width - dim2)/(dim1 - dim2)); // height = a3 - a2
    } else {
        newSig = sigma;
    }

    int r = ceil(2*newSig);

    /* Define far blurred filter */
    double* blurKernel = new double[2*r+1]; // on the heap
    kernel(blurKernel, r, newSig);

    /* Convolution */
    double pixel {};
    if (newSig > 0.6){
        pixel = conv(pixels, currIndex, width, height, blurKernel, r);
    } else {
        pixel = (pixels[currIndex]);
    }

    // delete blurKernel pointer to prevent memory leak
    delete[] blurKernel;

    return pixel;
}