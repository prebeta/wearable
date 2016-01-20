#include <iostream>                        // std::cout
#include <cstdio>
#include <sstream>
#include <fstream>
#include <dirent.h>
#include <string.h>
#include <opencv2/core/core.hpp>           // cv::Mat
#include <opencv2/highgui/highgui.hpp>     // cv::imread()
#include <opencv2/imgproc/imgproc.hpp>     // cv::Canny()

using namespace std;
using namespace cv;

string get_colors(Mat filtered);
string compare_templates(Mat bkgmask);
string get_pattern(Mat filtered);
string int_to_hex(int i);

int main(int argc, char *argv[]){
	Mat src, can, blurred, background, filtered, cropped;
	string article, path, info_color, pattern;
	std::vector<Vec3d> bkg_colors;
	int i, j, count, bgcount = 0;
	double d;
	Vec3d white, color;
	white[0] = 255;
	white[1] = 255;
	white[2] = 255;
	if (argc != 2){
		cout << "Wrong number of parameters" << endl;
		return -1;
	}
	cout << "Loading original image: " << argv[1] << endl;
	src = imread(argv[1], CV_LOAD_IMAGE_COLOR);
	while (src.rows > 800 || src.cols > 800){
		resize(src, src, Size(src.cols/2, src.rows/2));
	}
	if(! src.data ){
		cout <<  "Could not open or find the image" << endl ;
		return -1;
	}
	cout << "Removing background in image" << endl;
	background = Mat::zeros(src.rows, src.cols, CV_8UC3);
	blur(src, blurred, Size(3, 3));
	for(i = 0; i < blurred.rows; i+=40) {
		color = blurred.at<Vec3b>(Point(0, i));
		if(std::find(bkg_colors.begin(), bkg_colors.end(), color) == bkg_colors.end()){
			bkg_colors.push_back(color);
		}
		color = blurred.at<Vec3b>(Point(blurred.cols-1, i));
		if(std::find(bkg_colors.begin(), bkg_colors.end(), color) == bkg_colors.end()){
			bkg_colors.push_back(color);
		}
	}
	for(j = 1; j < blurred.cols-1; j+=20) {
		color = blurred.at<Vec3b>(Point(j, 0));
		if(std::find(bkg_colors.begin(), bkg_colors.end(), color) == bkg_colors.end()){
			bkg_colors.push_back(color);
		}
		color = blurred.at<Vec3b>(Point(j, blurred.rows-1));
		if(std::find(bkg_colors.begin(), bkg_colors.end(), color) == bkg_colors.end()){
			bkg_colors.push_back(color);
		}
	}
	for(i = 0; i < blurred.rows; i++){
		for(j = 0; j < blurred.cols; j++){
			color = blurred.at<Vec3b>(Point(j, i));
			count = 0;
			while(count < bkg_colors.size()){
				if(d = norm(color - bkg_colors.at(count)) < 20){
					background.at<Vec3b>(Point(j, i)) = white;
					bgcount++;
				}
				count++;
			}
		}
	}

	/*Creating background mask*/

	Canny( background, can, 50, 100, 3);
	blur(can, can, Size(3, 3));
	background = Mat::zeros(src.rows, src.cols, CV_8UC3);
	bitwise_not(background, background);
	
	int largest_contour_index=0;
	double largest_area=0;
	cv::Rect bounding_rect;
	vector<vector<Point>> contours;
	vector<Vec4i> hierarchy;
	findContours(can, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE);
	for(i = 0; i < contours.size(); i++){
		double a=contourArea(contours[i], false);
		if(a>largest_area){
			largest_area=a;
			largest_contour_index=i;
			bounding_rect=boundingRect(contours[i]);
		}
	}
	Scalar scolor(0,0,0);
	drawContours(background, contours, largest_contour_index, scolor, CV_FILLED, 8);
	Mat edge = Mat::zeros(src.rows,src.cols, CV_8UC3);
	bitwise_not(edge, edge);
	drawContours(edge, contours, largest_contour_index, scolor, 2, 8, hierarchy);
	cv::bitwise_or(background, src, filtered);
	cropped = filtered(bounding_rect);

	article = compare_templates(background(bounding_rect));
	info_color = get_colors(cropped);
	pattern = get_pattern(cropped);
	path = "./processed/";
	path += argv[1];
	cout << endl << "Clothing category: " << article << endl;
	cout << "Colors: " << info_color << endl;
	cout << "Pattern: " << pattern << endl;
	imwrite("_template.jpg", background(bounding_rect));		//creating templates
	imshow("Original Image", src);
	imshow("Removed Background", filtered);
	imshow("Mask", background(bounding_rect)); 
	waitKey(0);                                          // Wait for a keystroke in the window
	return 0;
}

string get_colors(Mat filtered){
	std::vector<Vec3d> colors;
	std::vector<double> percents;
	int i, j, k, l, q, wtc = 0;
	double d;
	std::ostringstream res;

	Vec3d white, color;
	white[0] = 255;
	white[1] = 255;
	white[2] = 255;
	int medrows = filtered.rows/2;
	int medcols = filtered.cols/2;

	for(i = 0; i < medrows; i++) {
		for(j = 0; j < medcols; j++) {
			for(k = 0; k < 4; k++) {
				if(k == 0) {
					color = filtered.at<Vec3b>(Point(medcols-j, medrows-i));
				} else if(k == 1) {
					color = filtered.at<Vec3b>(Point(medcols-j, medrows+i));
				} else if(k == 2) {
					color = filtered.at<Vec3b>(Point(medcols+j, medrows+i));	
				} else {
					color = filtered.at<Vec3b>(Point(medcols+j, medrows-i));	
				}
				if(color != white){
					l = 0;
					q = 1;
					while(q && l < colors.size()){
						if(d = norm(color - colors.at(l)) < 80){
							percents.at(l)++;
							q = 0;
						}
						l++;
					}
					if (q){
						percents.push_back(1);
						colors.push_back(color);
					}
				} else {
					wtc++;
				}
			}
		}
	}
	for(i = 0; i < colors.size(); i++){
		color = colors.at(i); 	
		if ((percents.at(i) /= (medrows*medcols*4 - wtc)) > 0.1){
			res << "{"<< int_to_hex((int)color[2]) << int_to_hex((int)color[1]) << int_to_hex((int)color[0]) << ":" << percents.at(i) <<"}";
		}
	}
	return res.str();
}


string compare_templates(Mat bkgmask){
	cout << "Comparing contours" << endl;
	std::vector<string> names;
	std::vector<double> diffs;
	Mat temp, resized;
	int i, j, index;
	double count, min, terat, rat, divi;
	DIR *dir;
	string dir_name = ".\\templates\\";
	//string dir_name = "./templates/";
	string img_name;
	string path;
	struct dirent *ent;
	Vec3b white, color;
	white[0] = 255;
	white[1] = 255;
	white[2] = 255;
	rat = (double)bkgmask.rows/(double)bkgmask.cols;

	if((dir = opendir(dir_name.c_str())) != NULL) {
		while ((ent = readdir (dir)) != NULL){
			img_name = ent->d_name;
			path = dir_name + img_name;
			temp = imread(path, CV_LOAD_IMAGE_COLOR);
			if(temp.data){
				terat = (double)temp.rows/(double)temp.cols;
				divi = terat/rat;
				names.push_back(img_name);
				if(divi < 1.2 && divi > 0.8){
					resize(bkgmask, resized, temp.size());
					count = 0;
					for(i = 0; i < resized.rows; i++) {
						for(j = 0; j < resized.cols; j++) {
							if((color = temp.at<Vec3b>(Point(j, i))) != resized.at<Vec3b>(Point(j, i))){
									count++;
							}
						}
					}
					count /= (temp.rows*temp.cols);
					diffs.push_back(count);
					//cout << img_name << " : " <<  count << endl;
				} else {
					diffs.push_back(0.99);
				}
			}
		}
		closedir(dir);
	} else {
		cout << "Templates not found" << endl;
	}
	min = 1;
	for(i = 0; i < diffs.size(); i++){
		if(diffs.at(i) < min){
			min = diffs.at(i);
			index = i;
		}
	}
	path = names.at(index);
	return path.substr(0, path.find('_'));
}

string get_pattern(Mat filtered){
	Mat can;
	string ret = "None";
	int i, j, theta;
	Canny(filtered, can, 50, 200, 3);
	vector<Vec2f> lines;
	vector<int> thet;
	vector<int> counts;
	HoughLines(can, lines, 1, CV_PI/180, 145, 0, 0 );
	for(i = 0; i < lines.size(); i++ ){
		theta = (int)(lines[i][1]*5);
		j = 0;
		while(j < thet.size() && thet[j] != theta){
			j++;
		}
		if(j == thet.size()){
			thet.push_back(theta);
			counts.push_back(1);
		} else {
			counts[j]++;
		}
	}
	for (i = 0; i < counts.size(); i++){
		if(counts[i] > 5){
			if(!ret.compare("striped") || !ret.compare("plaid/checkered")){
				ret = "plaid/checkered";
			} else {
				ret = "striped";
			}
		}
	}
	//imshow("Contours", can);
	return ret;
}

string int_to_hex(int i){
        std::stringstream stream;
        stream << std::hex << i;
    string ret = stream.str();
    if (ret.length()==1){
        ret= "0"+ret;
    }
        return ret;
}