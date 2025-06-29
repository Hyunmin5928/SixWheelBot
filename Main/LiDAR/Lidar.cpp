#include "Lidar.h"

Lidar::Lidar(){
    lidar_setup();
}

Lidar::~Lidar(){
    lidar.turnOff();
    lidar.disconnecting();
}

int Lidar::lidar_setup(){

    std::string port;
    ydlidar::os_init();

    std::map<std::string, std::string> ports =
        ydlidar::lidarPortList();
    std::map<std::string, std::string>::iterator it;

    if (ports.size() == 1)
    {
        port = ports.begin()->second;
    }
    else
    {
        int id = 0;

        for (it = ports.begin(); it != ports.end(); it++)
        {
        printf("[%d] %s %s\n", id, it->first.c_str(), it->second.c_str());
        id++;
        }

        if (ports.empty()){
        printf("Not Lidar was detected. Please enter the lidar serial port:");
        std::cin >> port;
        }
        else {
            while (ydlidar::os_isOk())
            {
                printf("Please select the lidar port:");
                std::string number;
                std::cin >> number;

                if ((size_t)atoi(number.c_str()) >= ports.size())
                {
                continue;
                }

                it = ports.begin();
                id = atoi(number.c_str());

                while (id)
                {
                id--;
                it++;
                }

                port = it->second;
                break;
            }
        }
    }

    int baudrate = 128000;
    bool isSingleChannel = true;

    float frequency = 4.0f; // 기존 6.0

    if(!ydlidar::os_isOk())
        return 0;

    lidar.setlidaropt(LidarPropSerialPort, port.c_str(), port.size());
    /// ignore array
    std::string ignore_array;
    ignore_array.clear();
    lidar.setlidaropt(LidarPropIgnoreArray, ignore_array.c_str(),
                        ignore_array.size());

    //////////////////////int property/////////////////
    /// lidar baudrate
    lidar.setlidaropt(LidarPropSerialBaudrate, &baudrate, sizeof(int));
    int optval = TYPE_TRIANGLE;
    lidar.setlidaropt(LidarPropLidarType, &optval, sizeof(int));
    optval = YDLIDAR_TYPE_SERIAL;
    lidar.setlidaropt(LidarPropDeviceType, &optval, sizeof(int));
    /// sample rate
    optval = 5;
    lidar.setlidaropt(LidarPropSampleRate, &optval, sizeof(int));
    /// abnormal count
    optval = 4;
    lidar.setlidaropt(LidarPropAbnormalCheckCount, &optval, sizeof(int));
    /// Intenstiy bit count
    optval = 10;
    lidar.setlidaropt(LidarPropIntenstiyBit, &optval, sizeof(int));

    //////////////////////bool property/////////////////
    /// fixed angle resolution
    bool b_optvalue = true;    // 기존 false
    lidar.setlidaropt(LidarPropFixedResolution, &b_optvalue, sizeof(bool));
    /// rotate 180
    b_optvalue = false;
    lidar.setlidaropt(LidarPropReversion, &b_optvalue, sizeof(bool));
    /// Counterclockwise
    b_optvalue = false;
    lidar.setlidaropt(LidarPropInverted, &b_optvalue, sizeof(bool));
    b_optvalue = true;
    lidar.setlidaropt(LidarPropAutoReconnect, &b_optvalue, sizeof(bool));
    /// one-way communication
    b_optvalue = true;  // 기존 false
    lidar.setlidaropt(LidarPropSingleChannel, &isSingleChannel, sizeof(bool));
    /// intensity
    b_optvalue = false;
    lidar.setlidaropt(LidarPropIntenstiy, &b_optvalue, sizeof(bool));
    /// Motor DTR
    b_optvalue = true;
    lidar.setlidaropt(LidarPropSupportMotorDtrCtrl, &b_optvalue, sizeof(bool));
    /// HeartBeat
    b_optvalue = false;
    lidar.setlidaropt(LidarPropSupportHeartBeat, &b_optvalue, sizeof(bool));

    //////////////////////float property/////////////////
    /// unit: °
    float f_optvalue = 180.0f;
    lidar.setlidaropt(LidarPropMaxAngle, &f_optvalue, sizeof(float));
    f_optvalue = -180.0f;
    lidar.setlidaropt(LidarPropMinAngle, &f_optvalue, sizeof(float));
    f_optvalue = 64.f;
    lidar.setlidaropt(LidarPropMaxRange, &f_optvalue, sizeof(float));
    f_optvalue = 0.05f;
    lidar.setlidaropt(LidarPropMinRange, &f_optvalue, sizeof(float));
    lidar.setlidaropt(LidarPropScanFrequency, &frequency, sizeof(float));
    lidar.enableGlassNoise(false);
    lidar.enableSunNoise(false);
    lidar.setBottomPriority(true);

    uint32_t t = getms();
    int c=0;
    bool ret=lidar.initialize();
    if (!ret)
    {
        fprintf(stderr, "Fail to initialize %s\n", lidar.DescribeError());
        fflush(stderr);
        return -1;
    }

    return 0;
}

// void Lidar::scan_oneCycle() {
//     lidar.turnOn();
//     LaserPoint nearPoint = {0.0f, 800.0f, 0.0f}; // 초기값
//     if (ydlidar::os_isOk()) {
//         if (lidar.doProcessSimple(scanData)) {
//             // std::cout << "scanData.points.size(): " << scanData.points.size() << std::endl;
//             usableData.clear();
//             for (const auto& p : scanData.points) {
//                 std::printf("raw angle: %.2f, raw range: %.5f\n", p.angle * 180.0 / M_PI, p.range);
//                 LaserPoint temp = {p.angle * 180.0 / M_PI, p.range * 1000.0, p.intensity};
//                 usableData.push_back(temp);
//             }
//             std::printf("nearpoint : angle %.02f  range %.02f\n", nearPoint.angle, nearPoint.range);
//         } else {
//             fprintf(stderr, "Failed to get Lidar Data\n");
//             fflush(stderr);
//         }
//     }
// }

void Lidar::scan_oneCycle(){
    LaserPoint nearPoint = {0.0f, 800.0f, 0.0f}; // 초기값
    if(ydlidar::os_isOk()){
        if(lidar.doProcessSimple(scanData)){
            usableData.clear();
            for(size_t i=0; i<scanData.points.size(); i++){
                const LaserPoint &p = scanData.points.at(i);
                LaserPoint temp={p.angle*180.0/M_PI, p.range*1000.0, p.intensity};
                usableData.push_back(temp);
            }
            // std::printf("usableData : %.02f",usableData.at(0).angle);
        }
        else{
            fprintf(stderr, "Failed to get Lidar Data\n");
            fflush(stderr);
        }
        std::cout<<"\n";
    }
}

std::vector<LaserPoint> Lidar::get_scanData(){
    return usableData;
}

LaserPoint Lidar::get_nearPoint(){
    LaserPoint nearPoint = {0.0f, 800.0f, 0.0f};
    float minRange = 800.0f;
    for (const auto& point : usableData) {
        if (point.range < minRange && 
            point.range > 0.0f && 
            point.angle < 60.0f && 
            point.angle> -60.0f) 
        {
            minRange = point.range;
            nearPoint = point;
        }
    }
    // std::printf("nearpoint : angle %.02f  range %.02f", nearPoint.angle, nearPoint.range);
    return nearPoint;
}

// int main(){
//     Lidar lidar;
//     lidar.scan_oneCycle();
//     lidar.get_scanData();
//     lidar.get_nearPoint();
// }

bool Lidar::turnOn() {
    return lidar.turnOn();  // CYdLidar의 turnOn 호출
}

bool Lidar::turnOff() {
    return lidar.turnOff(); // CYdLidar의 turnOff 호출
}