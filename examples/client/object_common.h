//
// Created by impact on 18-7-25.
//

#ifndef LWM2MCLIENT_OBJECT_COMMON_H
#define LWM2MCLIENT_OBJECT_COMMON_H

#include <liblwm2m.h>

extern lwm2m_object_t * create_object_with_uniqInstance(const int objId);
extern void free_object(lwm2m_object_t * object);
extern void update_Object_measurement(const ObjectData* sensorData, lwm2m_context_t* context);



typedef struct {
    const int  objId;
    const int* resId;
    int        resNum;
}ResourceIDList;


extern const int pm25_resIdList[2];
extern const int humidity_resIdList[3];


#endif //LWM2MCLIENT_OBJECT_COMMON_H
