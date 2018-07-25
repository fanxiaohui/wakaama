//
// Created by zengliang on 18-7-25.
//


#ifdef LWM2M_CLIENT_MODE

#include <liblwm2m.h>
#include <memory.h>

#define RES_ID_PM25_BUS_IDENTIFY     0
#define RES_ID_PM25_QUALITY          1


const int pm25_resIdList[2] = {RES_ID_PM25_BUS_IDENTIFY, RES_ID_PM25_QUALITY};


#endif