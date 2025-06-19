#include "SerialPort.h"
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <cstring>
#include <sys/ioctl.h>
#include <cstdio>

CSerialPort::CSerialPort() : _fd(-1) {}

CSerialPort::~CSerialPort() {
    Close();
}

bool CSerialPort::Open(const char* portName, int baudRate) {
    _fd = open(portName, O_RDWR | O_NOCTTY | O_NDELAY);
    if (_fd < 0) {
        perror("open");
        return false;
    }

    fcntl(_fd, F_SETFL, 0);

    struct termios tty;
    memset(&tty, 0, sizeof tty);
    if (tcgetattr(_fd, &tty) != 0) {
        perror("tcgetattr");
        return false;
    }

    speed_t speed;
    switch (baudRate) {
        case 9600: speed = B9600; break;
        case 115200: speed = B115200; break;
        default: speed = B9600; break;
    }

    cfsetospeed(&tty, speed);
    cfsetispeed(&tty, speed);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_iflag = IGNPAR;
    tty.c_oflag = 0;
    tty.c_lflag = 0;

    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 10; // 1초 타임아웃

    tcflush(_fd, TCIFLUSH);
    return (tcsetattr(_fd, TCSANOW, &tty) == 0);
}

void CSerialPort::Close() {
    if (_fd != -1) {
        close(_fd);
        _fd = -1;
    }
}

int CSerialPort::Read(char* buffer, int maxLen) {
    return read(_fd, buffer, maxLen);
}

int CSerialPort::Write(const char* buffer, int len) {
    return write(_fd, buffer, len);
}

void CSerialPort::Flush() {
    tcflush(_fd, TCIOFLUSH);
}

bool CSerialPort::SetTimeout(int readTimeout) {
    struct termios tty;
    if (tcgetattr(_fd, &tty) != 0) return false;
    tty.c_cc[VTIME] = readTimeout / 100; // 0.1s 단위
    return (tcsetattr(_fd, TCSANOW, &tty) == 0);
}

int CSerialPort::CountReadBuff() {
    int count;
    ioctl(_fd, FIONREAD, &count);
    return count;
}

bool CSerialPort::ReadLine(char* line, int maxLen) {
    int len = 0;
    char c;
    while (len < maxLen - 1) {
        int n = read(_fd, &c, 1);
        if (n <= 0) break;
        line[len++] = c;
        if (c == '\n') break;
    }
    line[len] = '\0';
    return len > 0;
}