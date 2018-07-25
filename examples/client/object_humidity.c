//
// Created by impact on 18-7-25.
//

#ifdef LWM2M_CLIENT_MODE

#include <liblwm2m.h>
#include <memory.h>

#define RES_ID_HUMIDITY_SERIAL_NUM     0
#define RES_ID_HUMIDITY_VALUE          1
#define RES_ID_HUMIDITY_BATTERY        2


const int humidity_resIdList[3] = {RES_ID_HUMIDITY_SERIAL_NUM, RES_ID_HUMIDITY_VALUE, RES_ID_HUMIDITY_BATTERY};


#endif