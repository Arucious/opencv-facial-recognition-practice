// HW04_Part2
//
// TODO:
// 1: Copy your scale and translate outline functions from Part 1 into this program
//     (Yes this is bad software engineering style to just make copies of code... deal with it)
// 2: In the main method, find the TODO Required portion and add code to scale/translate
// 3: Test with a shape image and face image.
//    That is, set the second command line argument to be an image with a face in it.
//    You can capture an image from one of the lab examples.
//
// Mac/Linux users: 
// This program also uses haarcascade_frontalface_alt.xml file for the Viola/Jones face detector
// So be sure to copy this file into the appropriate directory.

#include "stdafx.h"

#include <opencv2/opencv.hpp>
#include <string>
#include <vector>
#include <iostream>

using namespace cv;
using namespace std;

Mat originalImage;
Mat outlineImage;

//Required: Translate (move) an outline stored in a vector of points by adding an offset to 
//the coordinates of all points in the outline
void translateOutline(vector<Point>& outline, Point center);

//Required: Scale an outline by multiplying all coordinates of all points in the outline by a constant
void scaleOutline(vector<Point>& outline, double scale);

//Given: function to detect faces after normalizing the image
void detectFaces(Mat& image, CascadeClassifier& cascade, vector<Rect>& faces);

//Given: function to detect the largest red object in an image
bool findLargestRedObject(Mat& view, Point& location, vector<Point>& outline, int redThreshold);

//Given: a dummy function to pass to the slider bar to threshold the red object
void onTrackbar(int value, void* data);

//Given: Compute the area and center of a region bounded by an outline
void computeObjectAreaAndCenter(vector<Point>& outline, double& area, Point& center);

//Given: Draw an outline stored in a vector of points
void drawOutline(Mat& image, vector<Point>& outline);

int main(int argc, char* argv[])
{

	//Put a frame around a picture of a face
	if (argc <= 2)
	{
		cout << "Please provide a filename of an outline and an image" << endl;
		return 0;
	}

	string face_cascade_name = "haarcascade_frontalface_alt.xml";
	CascadeClassifier face_cascade;
	if (!face_cascade.load(face_cascade_name))
	{
		printf("--(!)Error loading\n"); return -1;
	}

	outlineImage = imread(argv[1]);
	originalImage = imread(argv[2]);
	if (outlineImage.empty() || originalImage.empty())
	{
		cout << "Unable to open images" << endl;
		return 0;
	}

	Mat displayImage(originalImage.rows, originalImage.cols, CV_8UC3);
	originalImage.copyTo(displayImage);
	vector<Point> outline;
	vector<Point> originalOutline;
	int redThreshold = 190;

	double scaleFactor = 1.0;
	Point2f translation(0, 0);

	namedWindow("Image Window", 1);
	createTrackbar("Red Threshold", "Image Window", &redThreshold, 255, onTrackbar, &originalOutline);

	while (1 == 1)
	{
		originalImage.copyTo(displayImage);
		std::vector<Rect> faces;
		detectFaces(originalImage, face_cascade, faces);
		Point outlineCenter;
		double outlineArea;

		if (originalOutline.size() > 0)
		{
			computeObjectAreaAndCenter(originalOutline, outlineArea, outlineCenter);

			for (int F = 0; F<faces.size(); F++)
			{
				//calculate the center of the faces returned by the face detector
				Point faceCenter(faces[F].x + faces[F].width / 2, faces[F].y + faces[F].height / 2);

				//make a copy that we can manipulate
				outline = originalOutline;

				translateOutline(originalOutline, faceCenter - outlineCenter); // centers outline around face
				double ratio = sqrt(faces[F].width * faces[F].height / outlineArea); // ratio to normalize the star-to-face measurements that seems to do the best job
				scaleOutline(originalOutline, ratio * 1.1); // automatically scales to the face size. 

				//draw the manipulated outline on the image
				drawOutline(displayImage, outline);
			}
		}

		imshow("Image Window", displayImage);
		char key = waitKey(33);
		if (key == 'q')
		{
			break;
		}
		if (key == ' ')
		{
			imwrite("Part2_result.png", displayImage);
		}
	}

	return 0;
}

void detectFaces(Mat& image, CascadeClassifier& face_cascade, vector<Rect>& faces)
{

	// http://docs.opencv.org/modules/objdetect/doc/cascade_classification.html
	// http://docs.opencv.org/doc/tutorials/objdetect/cascade_classifier/cascade_classifier.html


	Mat frame_gray;
	cvtColor(image, frame_gray, CV_BGR2GRAY);
	equalizeHist(frame_gray, frame_gray);

	//imshow("gray image", frame_gray);
	//-- Detect faces
	face_cascade.detectMultiScale(frame_gray, faces, 1.1, 2, 0 | CV_HAAR_SCALE_IMAGE, Size(30, 30));
}


void drawOutline(Mat& image, vector<Point>& outline)
{
	int numPoints = outline.size() - 1;
	for (int f = 0; f<numPoints; f++)
	{
		line(image, outline[f], outline[f + 1], Scalar(255, 0, 0), 3);
	}
}

void translateOutline(vector<Point>& outline, Point center)
{
	// just add the center offset to each point to translate it
	for (int i = 0; i < outline.size() - 1; i++) {
		outline[i] += center;
	}

}

void scaleOutline(vector<Point>& outline, double scale)
{
	Point center;
	double area;
	computeObjectAreaAndCenter(outline, area, center); // need old center for later
													   // scale each vector 
	for (int i = 0; i < outline.size() - 1; i++) {
		outline[i] *= scale;
	}
	Point newCenter;
	computeObjectAreaAndCenter(outline, area, newCenter);
	// offset is the difference between the old center and the new one
	// add offset back into the new outline to center it back where it firsts was
	for (int i = 0; i < outline.size() - 1; i++) {
		outline[i] += (center - newCenter);
	}
}

// Need to overload on the type of the point
void computeObjectAreaAndCenter(vector<Point>& outline, double& area, Point& center)
{
	// http://docs.opencv.org/modules/imgproc/doc/structural_analysis_and_shape_descriptors.html
	Moments objectProperties;
	objectProperties = moments(outline, false);

	area = objectProperties.m00;
	center.x = (objectProperties.m10 / area);
	center.y = (objectProperties.m01 / area);
}


bool findLargestRedObject(Mat& view, Point& location, vector<Point>& outline, int redThreshold)
{
	//allocate some images to store intermediate results
	vector<Mat> YCrCb;
	YCrCb.push_back(Mat(view.rows, view.cols, CV_8UC3));
	vector<Mat> justRed;
	justRed.push_back(Mat(view.rows, view.cols, CV_8UC1));
	vector<Mat> displayRed;
	displayRed.push_back(Mat(view.rows, view.cols, CV_8UC3));

	//Switch color spaces to YCrCb so we can detect red objects even if they are dark
	cvtColor(view, YCrCb[0], CV_BGR2YCrCb);

	//Pull out just the red channel
	int extractRed[6] = { 1,0, 1, 1, 1, 2 };
	mixChannels(&(YCrCb[0]), 1, &(justRed[0]), 1, extractRed, 1);

	// Threshold the red object (with the threshold from the slider)
	threshold(justRed[0], justRed[0], redThreshold, 255, CV_THRESH_BINARY);
	vector<vector<Point>> objectContours;
	vector<Vec4i> dummy;

	//Find all of the contiguous image regions
	findContours(justRed[0], objectContours, dummy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

	//find the largest object
	int largestArea(-1), largestIndex(-1);
	Point largestCenter;
	for (int i = 0; i<objectContours.size(); i++)
	{
		Point tempCenter;
		double tempArea;
		computeObjectAreaAndCenter(objectContours[i], tempArea, tempCenter);

		if (tempArea > largestArea)
		{
			largestArea = tempArea;
			largestIndex = i;
			largestCenter = tempCenter;
		}
	}
	location = largestCenter;
	if (largestIndex >= 0)
	{
		outline = objectContours[largestIndex];
	}

	//Construct an image for display that shows the red channel as gray
	mixChannels(&(YCrCb[0]), 1, &(displayRed[0]), 1, extractRed, 3);
	if (largestIndex >= 0)
	{
		//put a red circle around the red object
		circle(displayRed[0], largestCenter, std::min(double(view.cols) / 2, sqrt(largestArea)), Scalar(0, 0, 255), 1);
	}
	imshow("Just Red", displayRed[0]);


	if (largestIndex >= 0)
	{
		return true;
	}
	else
	{
		return false;
	}

}

void onTrackbar(int redThreshold, void* data)
{
	Point largestCenter;
	vector<Point>* largestOutline = (vector<Point>*)(data);
	findLargestRedObject(outlineImage, largestCenter, *largestOutline, redThreshold);
}