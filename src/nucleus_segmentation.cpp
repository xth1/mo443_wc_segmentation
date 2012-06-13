/**
 * WBC Nucleus Semgmentation Routines
 * 04-Jun-2012
 * Murilo Adriano Vasconcelos <murilo@clever-lang.org>
 * Thiago da Silva Arruda <>
 */
 
#include <cv.h>
#include <cxcore.h>
#include <highgui.h>
#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <vector>

using namespace std;

typedef IplImage* ImageType;
typedef uint8_t PixelType;

const PixelType MAX_COLOR = 255;

/**
 * Returns a reference to the pixel (row, col) of the image
 */
PixelType& get_px(const ImageType image, int row, int col)
{
	int step = image->widthStep;
	PixelType& px = (PixelType&)(image->imageData[row * step + col]);

	return px;
}

void invert_colors(ImageType& image){
	int height = image->height;
	int width  = image->width;
	
	for (int i = 0; i < height; ++i) {
		for (int j = 0; j < width; ++j) {
			PixelType& val = get_px(image, i, j);
			val = MAX_COLOR - val;
		}
	}
}

/**
 * Performs the Scale-space Toggle Simplification
 */
ImageType scale_space_toggle_simplification(ImageType image, int k = 7)
{
	int raw_element[] = {0, -2, -2, -2, -2, -2, -2, -2, -2};

	// Create a 3x3 scaled (scale=2) structuring element
	IplConvKernel* element = 
		cvCreateStructuringElementEx(3, 3, 1, 1, 
			CV_SHAPE_CUSTOM, 
			raw_element
		);
	
	ImageType psi1 = cvCreateImage(cvGetSize(image), IPL_DEPTH_8U, 1);
	ImageType psi2 = cvCreateImage(cvGetSize(image), IPL_DEPTH_8U, 1);
	
	
	// Psi1 is the image dilated k times by the element
	// similarly, psi2 is the image eroded k times by the structuring element
	cvDilate(image, psi1, element, k);
	cvErode(image, psi2, element, k);
	
	// This is where we store the resulting image
	ImageType result = cvCreateImage(cvGetSize(image), IPL_DEPTH_8U, 1);
	
	int height = image->height;
	int width  = image->width;
	
	
	// Eq. 5
	for (int i = 0; i < height; ++i) {
		for (int j = 0; j < width; ++j) {
			PixelType psi1_val  = get_px(psi1, i, j);
			PixelType psi2_val  = get_px(psi2, i, j);
			PixelType image_val = get_px(image, i, j);
			PixelType& res_val  = get_px(result, i, j);
			
			if (psi1_val - image_val < image_val - psi2_val) {
				res_val = psi1_val;
			}
			else if (psi1_val - image_val == image_val - psi2_val) {
				res_val = image_val;
			}
			else {
				res_val = psi2_val;
			}
		}
	}
	
	// Freeing resources
	cvReleaseImage(&psi1);
	cvReleaseImage(&psi2);
	
	return result;
}

/**
 * Main function to perform the WBC nucleus segmentation
 */
ImageType wbc_nucleus_segmentation(const ImageType image)
{
	ImageType threshold_image 
		= cvCreateImage(cvGetSize(image), IPL_DEPTH_8U, 1);
	
	ImageType gradient_image_16S
		=cvCreateImage(cvGetSize(image), IPL_DEPTH_16S, 1);	
	
	ImageType gradient_image_8U=
		cvCreateImage(cvGetSize(image), IPL_DEPTH_8U, 1);
	
	ImageType toggle_image;

	// Create a binary image, 'threshold_image', by thresholding;
	cvThreshold(image, threshold_image, 90, 255, CV_THRESH_BINARY_INV);
	
	// Create a simplified image, simplified_image, applying the Scale-space
	//	Toggle Operator in 'threshold_image';
	toggle_image = scale_space_toggle_simplification(image);
	
	// compute the gradient from Is, using Sobel operator
	// it use first derivate in horizontal and vertical directions
	//  and a 3 x 3 kernel
	cvSobel(toggle_image, gradient_image_16S, 1, 1, 3);
	 
	//convert gradient image to adequate scale and invert its color
	cvConvertScaleAbs(gradient_image_16S, gradient_image_8U, 1, 0);
	invert_colors(gradient_image_8U);
	 
	// compute an erosion on Is to discard small residues;
	// 6: compute the watershed transform using Ib as markers 
	return gradient_image_8U;	
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
	
	ImageType toggle = wbc_nucleus_segmentation(image);
	
	cvNamedWindow("Original", 1);
	cvShowImage("Original", image);
	cvMoveWindow("Original", 10, 40);
	
	cvNamedWindow("After Toggle and Gradient", 1);
	cvShowImage("After Toggle and Gradient", toggle);
	cvMoveWindow("After Toggle and Gradient", 10, 40);

	
	
	cvWaitKey(-1);

	cvDestroyWindow("Original");
	cvDestroyWindow("After Toggle");
	cvReleaseImage(&image);
	cvReleaseImage(&toggle);
	
	return 0;
}
	
