#pragma once

#include "controller.h"
#include <librobosub/robosub.h>
#include <librobosub/networktcp.h>
#include <signal.h>
#include <opencv2/opencv.hpp>
#include <opencv2/ximgproc.hpp>
#include <mutex>

extern bool running;
extern bool refresh;
extern Controller* controller1;
extern Controller* controller2;
extern long controllerTime;
extern mutex drawLock;

extern const char* NETWORK_HOST;