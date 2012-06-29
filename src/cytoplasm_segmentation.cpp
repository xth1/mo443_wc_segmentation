/**
 * WBC Cytoplasm Semgmentation Routines
 * 04-Jun-2012
 * Murilo Adriano Vasconcelos <murilo@clever-lang.org>
 * Thiago da Silva Arruda <>
 */
 
#include <cv.h>
#include <cxcore.h>
#include <highgui.h>
#include <iostream>
#include "opencv2/imgproc/imgproc.hpp"

using namespace std;
using namespace cv;

typedef IplImage* ImageType;
typedef uint8_t PixelType;

ImageType cytoplasm_segmentation(const ImageType image)
{
	ImageType img = cvCreateImage(cvGetSize(image), IPL_DEPTH_8U, 1);
	
	cvCopy(image, img);
	
	cvEqualizeHist(img, img);
	cvThreshold(image, img, 200, 255, THRESH_BINARY_INV);
	
	IplConvKernel* element = 
		cvCreateStructuringElementEx(91, 91, 45, 45, MORPH_ELLIPSE);
		
	cvErode(img, img, element);
	cvDilate(img, img, element);
	
	//cvMorphologyEx(img, img, NULL, element, MORPH_OPEN);	
	
	return img;
}

int main(int argc, char* argv[])
{
	if (argc != 2) {
		cerr << "Must pass the path for the image as first argument." << endl;
		return 1;
	}

	ImageType image = cvLoadImage(argv[1], 0);

	if (!image) {
		cerr << "The image " << argv[1] << " couldn't be loaded." << endl;
		return 2;
	}

	cvNamedWindow("Original", 1);
	cvShowImage("Original", image);
	cvMoveWindow("Original", 10, 40);

	ImageType seg = cytoplasm_segmentation(image);

	cvNamedWindow("Seg", 1);
	cvShowImage("Seg", seg);
	cvMoveWindow("Seg", 10, 300);

	cvWaitKey(-1);

	cvDestroyWindow("Original");
	cvDestroyWindow("Seg");
	cvReleaseImage(&seg);
	cvReleaseImage(&image);

	return 0;
}