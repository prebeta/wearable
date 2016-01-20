// Usage: ./app input.jpg
#include "opencv2/opencv.hpp"
#include <string>

using namespace cv;
using namespace std;

class WatershedSegmenter{
private:
    cv::Mat markers;
public:
    void setMarkers(cv::Mat& markerImage)
    {
        markerImage.convertTo(markers, CV_32S);
    }

    cv::Mat process(cv::Mat &image)
    {
        cv::watershed(image, markers);
        markers.convertTo(markers,CV_8U);
        return markers;
    }
};


int main(int argc, char* argv[])
{
    cv::Mat image = cv::imread(argv[1]);
    cv::Mat binary;// = cv::imread(argv[2], 0);
    cv::cvtColor(image, binary, CV_BGR2GRAY);
    cv::threshold(binary, binary, 100, 255, THRESH_BINARY);

    imshow("originalimage", image);
    imshow("originalbinary", binary);

    // Eliminate noise and smaller objects
    cv::Mat fg;
    cv::erode(binary,fg,cv::Mat(),cv::Point(-1,-1),2);
    imshow("fg", fg);

    // Identify image pixels without objects
    cv::Mat bg;
    cv::dilate(binary,bg,cv::Mat(),cv::Point(-1,-1),3);
    cv::threshold(bg,bg,1, 128,cv::THRESH_BINARY_INV);
    imshow("bg", bg);

    // Create markers image
    cv::Mat markers(binary.size(),CV_8U,cv::Scalar(0));
    markers= fg+bg;
    imshow("markers", markers);

    // Create watershed segmentation object
    WatershedSegmenter segmenter;
    segmenter.setMarkers(markers);

    cv::Mat result = segmenter.process(image);
    result.convertTo(result,CV_8U);
    imshow("final_result", result);

    cv::waitKey(0);

    return 0;
}