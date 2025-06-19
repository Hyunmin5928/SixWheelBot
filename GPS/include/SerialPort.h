#pragma once
#include <termios.h>

class CSerialPort
{
public:
    CSerialPort();
    ~CSerialPort();

    bool Open(const char* portName, int baudRate);
//    bool Open(const char *devicePath, speed_t baudRate);

    void Close();

    int Read(char* buffer, int maxLen);
    int Write(const char* buffer, int len);

    void Flush();
    bool SetTimeout(int readTimeout); // ms 단위
    int CountReadBuff();
    bool ReadLine(char* line, int maxLen);

private:
    int _fd;  // File descriptor
};
