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

#include "opencv2/imgproc/imgproc.hpp"

#define MAX_COLOR 255
using namespace std;
using namespace cv;


typedef IplImage* ImageType;
typedef uint8_t PixelType;



/**
 * Returns a reference to the pixel (row, col) of the image
 */
PixelType& get_px(const ImageType image, int row, int col)
{
	int step = image->widthStep;
	PixelType& px = (PixelType&)(image->imageData[row * step + col]);

	return px;
}

void invert_colors(ImageType &image){
	int height = image->height;
	int width  = image->width;
	
	for (int i = 0; i < height; ++i) {
		for (int j = 0; j < width; ++j) {
				PixelType &val=get_px(image,i,j);
				val = MAX_COLOR - val;
		}
	}
}

void apply_watershed(Mat original_image,Mat image, Mat markerMask,
	Mat &wshed){
	
	Mat imgGray;
	Mat markers(markerMask.size(), CV_32S);
	
	vector<vector<Point> > contours;
    vector<Vec4i> hierarchy;
            
    findContours(markerMask, contours, hierarchy, CV_RETR_CCOMP, 
    CV_CHAIN_APPROX_SIMPLE);
	
	
    markers = Scalar::all(0);
   
    cvtColor(markerMask, imgGray, CV_GRAY2BGR);
    
    int idx = 0;
    int compCount=0;
    for( ; idx >= 0; idx = hierarchy[idx][0], compCount++ )
		drawContours(markers, contours, idx, Scalar::all(compCount+1),
		 -1, 8, hierarchy, INT_MAX);

	cvtColor(original_image, imgGray, CV_GRAY2BGR);
	
    vector<Vec3b> colorTab;
    for(int  i = 0; i < compCount; i++ )
    {
		int b = theRNG().uniform(0, 255);
        int g = theRNG().uniform(0, 255);
        int r = theRNG().uniform(0, 255);
                
        colorTab.push_back(Vec3b((uchar)b, (uchar)g, (uchar)r));
        }

        double t = (double)getTickCount();
            
        Mat nimg;   
            
        cvtColor(image, nimg, CV_GRAY2BGR);

        watershed(nimg,markers);
        wshed = Mat(markerMask.size(), CV_8UC3);
            
        // paint the watershed image
        int i,j;
        for( i = 0; i < markers.rows; i++ )
        {
			for( j = 0; j < markers.cols; j++ )
            {
					
				int idx = markers.at<int>(i,j);
				if( idx == -1 )
					wshed.at<Vec3b>(i,j) = Vec3b(255,255,255);
				else if( idx <= 0 || idx > compCount )
					wshed.at<Vec3b>(i,j) = Vec3b(0,0,0);
				else
					wshed.at<Vec3b>(i,j) = colorTab[idx - 1];
				}
               
		}

        wshed = wshed*0.5 + imgGray*0.5;
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


void generate_external_seeds(ImageType &image)
{
	int height = image->height;
	int width  = image->width;
	
	
	int T=5;
	for (int i = 0; i < height; ++i) {
		for(int k=0;k<T;k++){
			PixelType &val=get_px(image,i,k);
			val = MAX_COLOR;
			
			PixelType &val2=get_px(image,i,width-k-1);
			val2 = MAX_COLOR;
		}
	}
	
	for (int j = 0; j < width; ++j) {
			for(int k=0;k<T;k++){
				PixelType &val=get_px(image,k,j);
				val = MAX_COLOR;
				
				PixelType &val2=get_px(image,height-k-1,j);
				val2 = MAX_COLOR;
			}
	}
	
	
}
/**
 * Main function to perform the WBC nucleus segmentation
 */
void wbc_nucleus_segmentation(const ImageType image, Mat &wshed)
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
	
	// compute the gradient from Is
	 cvMorphologyEx(toggle_image, gradient_image_8U, NULL, NULL, MORPH_GRADIENT, 5);
	 invert_colors(gradient_image_8U);
	// compute an erosion on Is to discard small residues;
	// 6: compute the watershed transform using Ib as markers 
	generate_external_seeds(threshold_image);
	
	cvNamedWindow("threshold_image", 1);
	cvShowImage("threshold_image", threshold_image);
	cvMoveWindow("threshold_image", 10, 40);

	apply_watershed(Mat(image),Mat(gradient_image_8U),Mat(threshold_image),wshed);

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
	
	
	Mat wshed;
	
	wbc_nucleus_segmentation(image,wshed);
	
	IplImage result =wshed;
	cvNamedWindow("After Toggle and Gradient", 1);
	cvShowImage("After Toggle and Gradient", &result);
	cvMoveWindow("After Toggle and Gradient", 10, 40);

	
	
	cvWaitKey(-1);

	cvDestroyWindow("Original");
	cvDestroyWindow("After Toggle");
	cvReleaseImage(&image);
	//cvReleaseImage(&result);
	
	return 0;
}
	
