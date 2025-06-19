#include <Python.h>
#include <iostream>
#include "GPS_parse.h"
#include <unistd.h>
#include "logger.h"
#include <vector>
#include <tuple>

// 1. C++ 벡터 선언 (전역 또는 지역)
std::vector<std::tuple<double, double, int>> routeList;  // (lat, lon, turnType)
std::vector<std::tuple<double, double, int>> parsed_coords;

int main(){
GPS gps_device;
sGPS gps;
init_logger("gps.log");


bool G_flag = false;
while(!G_flag){
G_flag = gps_device.GetGPSdata(&gps);}
    if (G_flag) {
        std::cout << "[✅] GPS 데이터 수신 성공!" << std::endl;
        Py_Initialize();

        // Python 모듈 import
        PyObject* pName = PyUnicode_DecodeFSDefault("call_map_api");
        PyObject* pModule = PyImport_Import(pName);
        Py_DECREF(pName);

        if (pModule) {
            // redirect_stdout_to_log() 호출 (로그 파일로 print 리디렉션)
            PyObject* pRedirectFunc = PyObject_GetAttrString(pModule, "redirect_stdout_to_log");
            if (PyCallable_Check(pRedirectFunc)) {
                PyObject* pRedirectResult = PyObject_CallObject(pRedirectFunc, NULL);
                Py_XDECREF(pRedirectResult);
            }
            Py_XDECREF(pRedirectFunc);

            // 1. fetch_route_data 호출
            PyObject* pFetchFunc = PyObject_GetAttrString(pModule, "fetch_route_data");

            if (PyCallable_Check(pFetchFunc)) {
                PyObject* pArgs = PyTuple_Pack(5,
                    PyFloat_FromDouble(gps.latitude),
                    PyFloat_FromDouble(gps.longitude),
                    PyFloat_FromDouble(37.337891000000255),
                    PyFloat_FromDouble(127.10951339999924),
                    PyUnicode_FromString("6uHPB650j41F9NmAfTKjs5DxEZ0eBcTC77dm55iX")
                );

                PyObject* pFetchResult = PyObject_CallObject(pFetchFunc, pArgs);
                Py_DECREF(pArgs);
                Py_DECREF(pFetchFunc);

                if (pFetchResult) {
                    // 2. parse_route_coords 호출
                    PyObject* pParseFunc = PyObject_GetAttrString(pModule, "parse_route_coords");

                    if (PyCallable_Check(pParseFunc)) {
                        PyObject* pParseArgs = PyTuple_Pack(1, pFetchResult);
                        PyObject* pParseResult = PyObject_CallObject(pParseFunc, pParseArgs);
                        Py_DECREF(pParseArgs);
                        Py_DECREF(pParseFunc);

                        if (pParseResult) {
                            std::cout << "✅ 파싱된 경로 정보 반환됨" << std::endl;

                            // unpack: (route_coords, crosswalk_coords, turn_points)
                            PyObject* route_coords = PyTuple_GetItem(pParseResult, 0);
                            PyObject* crosswalk_coords = PyTuple_GetItem(pParseResult, 1);
                            PyObject* turn_points = PyTuple_GetItem(pParseResult, 2);
                          

                            if (PyList_Check(route_coords)) {
                                Py_ssize_t len = PyList_Size(route_coords);
                                for (Py_ssize_t i = 0; i < len; ++i) {
                                    PyObject* tuple_item = PyList_GetItem(route_coords, i);  // Borrowed reference
                                    if (PyTuple_Check(tuple_item) && PyTuple_Size(tuple_item) == 3) {
                                        PyObject* latObj = PyTuple_GetItem(tuple_item, 0);
                                        PyObject* lonObj = PyTuple_GetItem(tuple_item, 1);
                                        PyObject* turnTypeObj = PyTuple_GetItem(tuple_item, 2);

                                        if (PyFloat_Check(latObj) && PyFloat_Check(lonObj)) {
                                            double lat = PyFloat_AsDouble(latObj);
                                            double lon = PyFloat_AsDouble(lonObj);

                                            int turnType = -1;  // 기본값: 없음
                                            if (turnTypeObj != Py_None && PyLong_Check(turnTypeObj)) {
                                                turnType = static_cast<int>(PyLong_AsLong(turnTypeObj));
                                            }

                                            parsed_coords.emplace_back(lat, lon, turnType);
                                        }
                                    }
                                }
                            }


                            // 3. print_route_with_turntypes(route_coords)
                            PyObject* pPrint1 = PyObject_GetAttrString(pModule, "print_route_with_turntypes");
                            if (PyCallable_Check(pPrint1)) {
                                PyObject* args1 = PyTuple_Pack(1, route_coords);
                                PyObject_CallObject(pPrint1, args1);
                                Py_DECREF(args1);
                            }
                            Py_XDECREF(pPrint1);

                            // 4. print_coords("횡단보도 좌표", crosswalk_coords)
                            PyObject* pPrint2 = PyObject_GetAttrString(pModule, "print_coords");
                            if (PyCallable_Check(pPrint2)) {
                                PyObject* label = PyUnicode_FromString("횡단보도 좌표");
                                PyObject* args2 = PyTuple_Pack(2, label, crosswalk_coords);
                                PyObject_CallObject(pPrint2, args2);
                                Py_DECREF(args2);
                                Py_DECREF(label);
                            }
                            Py_XDECREF(pPrint2);

                            // 5. print_turn_points(turn_points)
                            PyObject* pPrint3 = PyObject_GetAttrString(pModule, "print_turn_points");
                            if (PyCallable_Check(pPrint3)) {
                                PyObject* args3 = PyTuple_Pack(1, turn_points);
                                PyObject_CallObject(pPrint3, args3);
                                Py_DECREF(args3);
                            }
                            Py_XDECREF(pPrint3);

                            Py_DECREF(pParseResult);
                        } else {
                            PyErr_Print();
                        }

                    } else {
                        PyErr_Print();
                    }

                    Py_DECREF(pFetchResult);
                } else {
                    PyErr_Print();
                }
            } else {
                PyErr_Print();
            }

            Py_DECREF(pModule);
        } else {
            PyErr_Print();
        }

        Py_Finalize();

    } else {

        std::cout << "[❌] GPS 데이터 수신 대기 중..." << std::endl;

    }

    for (const auto& [lat, lon, turn] : parsed_coords) {
    std::cout << "위도: " << lat << ", 경도: " << lon
              << ", turnType: " << (turn >= 0 ? std::to_string(turn) : "없음") << "\n";
    }
    

close_logger();

return 0;
}