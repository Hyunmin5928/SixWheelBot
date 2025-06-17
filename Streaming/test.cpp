#include <opencv2/opencv.hpp>
using namespace cv;

int main(){
    std::string pipeline =
      "souphttpsrc location=http://localhost:8080/stream ! "
      "multipartdemux ! "
      "jpegdec ! "
      "videoconvert ! "
      "appsink";

    VideoCapture cap(pipeline, cv::CAP_GSTREAMER);
    if(!cap.isOpened()){
        std::cerr<<"스트림 열기 실패\n";
        return -1;
    }
    Mat frame;
    while(cap.read(frame)){
        imshow("MJPEG Stream", frame);
        if(waitKey(1)==27) break;
    }
    return 0;
}
