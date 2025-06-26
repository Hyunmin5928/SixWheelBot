#pragma once
#include "SafeQueue.hpp"

void vision_thread(SafeQueue<float>& dir_queue);
static void recvall(int sock, void* buf, size_t len);
static void sendall(int sock, const void* buf, size_t len);