#include "cal_distance.h"
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


#define DEG2RAD(deg) ((deg) * M_PI / 180.0)
#define RAD2DEG(rad) ((rad) * 180.0 / M_PI)

namespace GeoUtils {

    const double R = 6371000.0;  // 지구 반지름 (단위: m)

    double haversine_distance(double lat1, double lon1, double lat2, double lon2) {
        double phi1 = DEG2RAD(lat1);
        double phi2 = DEG2RAD(lat2);
        double d_phi = DEG2RAD(lat2 - lat1);
        double d_lambda = DEG2RAD(lon2 - lon1);

        double a = std::sin(d_phi / 2) * std::sin(d_phi / 2) +
                   std::cos(phi1) * std::cos(phi2) *
                   std::sin(d_lambda / 2) * std::sin(d_lambda / 2);

        double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1 - a));

        return R * c;
    }

    double bearing(double lat1, double lon1, double lat2, double lon2) {
        double phi1 = DEG2RAD(lat1);
        double phi2 = DEG2RAD(lat2);
        double delta_lambda = DEG2RAD(lon2 - lon1);

        double y = std::sin(delta_lambda) * std::cos(phi2);
        double x = std::cos(phi1) * std::sin(phi2) -
                   std::sin(phi1) * std::cos(phi2) * std::cos(delta_lambda);

        double theta = std::atan2(y, x);
        return fmod((RAD2DEG(theta) + 360.0), 360.0);  // 0~360도 범위
    }

}

