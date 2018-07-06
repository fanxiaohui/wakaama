//
// Created by impact on 18-7-5.
//

#ifndef LWM2MCLIENT_JSONDATA_H
#define LWM2MCLIENT_JSONDATA_H


#include <memory.h>

#define  MAX_RESOURCE_PER_INSTANCE  20
#define  MAX_VALUE_LENGTH_PER_RESOURCE   50
#define  elementsOf(array) (sizeof(array)/sizeof(array[0]))
#define  ENSURE_END_WITH_NULL_CHAR(array) {array[elementsOf(array)-1] = '\0';}

typedef struct {
    int resId;
    char value[MAX_VALUE_LENGTH_PER_RESOURCE];
}ResourceValue;


typedef struct{
    int objId;
    int instId;
    int resNum;
    ResourceValue resValues[MAX_RESOURCE_PER_INSTANCE];
}SensorData;



static inline void appendResourceIdValue(int resId, const char *resV, SensorData *sensorData)
{
    int resNum = sensorData->resNum;
    if(resNum < MAX_RESOURCE_PER_INSTANCE)
    {
        sensorData->resValues[resNum].resId = resId;
        strncpy(sensorData->resValues[resNum].value, resV, MAX_VALUE_LENGTH_PER_RESOURCE);
        ENSURE_END_WITH_NULL_CHAR(sensorData->resValues[resNum].value);
        sensorData->resNum += 1;
    }
}



#endif //LWM2MCLIENT_JSONDATA_H
