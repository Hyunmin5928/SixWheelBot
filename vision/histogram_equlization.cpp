#include <opencv2/opencv.hpp>
#include <vector>
using namespace cv;

// RGB 채널별 퍼센타일 스트레칭
Mat percentileStretchRGB(const Mat& src,
                         float lo_pct = 0.02f,
                         float hi_pct = 0.98f)
{
    std::vector<Mat> ch;
    split(src, ch);  // B, G, R

    int total = src.rows * src.cols;
    int lo_idx = int(total * lo_pct);
    int hi_idx = int(total * hi_pct);

    for (int c = 0; c < 3; ++c) {
        // 1) 1채널을 1D 벡터로 복사
        Mat flat = ch[c].reshape(1, total);
        std::vector<uchar> vec;
        vec.assign(flat.datastart, flat.dataend);

        // 2) 정렬해서 하위/상위 임계값 추출
        std::nth_element(vec.begin(), vec.begin() + lo_idx, vec.end());
        uchar lo = vec[lo_idx];
        std::nth_element(vec.begin(), vec.begin() + hi_idx, vec.end());
        uchar hi = vec[hi_idx];

        if (hi <= lo) continue;  // 예외 처리

        // 3) 스트레칭 계수 계산
        double alpha = 255.0 / double(hi - lo);
        double beta  = -lo * alpha;

        // 4) 채널에 적용
        ch[c].convertTo(ch[c], ch[c].type(), alpha, beta);
    }

    Mat dst;
    merge(ch, dst);
    return dst;
}


// Y 채널 퍼센타일 스트레칭
void stretchY(Mat& Y, float lo_pct, float hi_pct) {
    int total = Y.rows * Y.cols;
    int lo_idx = int(total * lo_pct);
    int hi_idx = int(total * hi_pct);

    // 1채널을 1D 벡터로
    Mat flat = Y.reshape(1, total);
    std::vector<uchar> v;
    v.assign(flat.datastart, flat.dataend);

    // 하위/상위 값 찾기
    std::nth_element(v.begin(), v.begin()+lo_idx, v.end());
    uchar lo = v[lo_idx];
    std::nth_element(v.begin(), v.begin()+hi_idx, v.end());
    uchar hi = v[hi_idx];
    if (hi<=lo) return;

    // 선형 스트레칭 계수
    double α = 255.0/(hi-lo), β = -lo*α;
    Y.convertTo(Y, Y.type(), α, β);
}

// 감마 보정
void gammaCorrectY(Mat& Y, double gamma) {
    Mat lut(1, 256, CV_8U);
    for(int i=0;i<256;i++)
        lut.at<uchar>(i) = cv::saturate_cast<uchar>(
            pow(i/255.0, gamma)*255.0
        );
    LUT(Y, lut, Y);
}

int main(int argc, char** argv) {
    if (argc != 5) {
        printf("Usage: %s <input.jpg>\n", argv[0]);
        return -1;
    }
    for (int i = 1; i <= 4; ++i) {
        Mat src = imread(argv[i], IMREAD_COLOR);
        if (src.empty()) {
            fprintf(stderr, "이미지를 열 수 없습니다: %s\n", argv[i]);
            continue;
        }

        // 1) BGR → YCrCb
        Mat ycrcb;
        cvtColor(src, ycrcb, COLOR_BGR2YCrCb);
        std::vector<Mat> ch(3);
        split(ycrcb, ch);

        // 2) Y 채널만 스트레칭 & 감마
        stretchY(ch[0], 0.02f, 0.98f);
        gammaCorrectY(ch[0], 0.8);  // γ<1 : 대비↑

        // 3) 합치고 → BGR
        merge(ch, ycrcb);
        Mat dst;
        cvtColor(ycrcb, dst, COLOR_YCrCb2BGR);

        // 3) 저장
        char buf[256];
        sprintf(buf, "output/enhanced_%d.jpg", i);
        imwrite(buf, dst);
    }
    printf("화이트밸런스 + 스트레칭 결과 저장 완료.\n");
    return 0;
}
