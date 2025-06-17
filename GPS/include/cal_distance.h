#ifndef GEO_UTILS_H
#define GEO_UTILS_H

namespace GeoUtils {
    double haversine_distance(double lat1, double lon1, double lat2, double lon2);
    double bearing(double lat1, double lon1, double lat2, double lon2);
}

#endif

