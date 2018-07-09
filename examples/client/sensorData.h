//
// Created by impact on 18-7-5.
//

#ifndef LWM2MCLIENT_JSONDATA_H
#define LWM2MCLIENT_JSONDATA_H


#include <memory.h>
#include "liblwm2m.h"
#include <stdlib.h>

#define  MAX_INSTANCE_PER_OBJ    12
#define  MAX_RESOURCE_PER_INSTANCE  20
#define  MAX_VALUE_LENGTH_PER_RESOURCE   50
#define  elementsOf(array) (sizeof(array)/sizeof(array[0]))
#define  ENSURE_END_WITH_NULL_CHAR(array) {array[elementsOf(array)-1] = '\0';}

typedef struct {
    int resId;
    char value[MAX_VALUE_LENGTH_PER_RESOURCE];
}ResourceValue;


typedef struct{
    int instId;
    int resNum;
    ResourceValue resValues[MAX_RESOURCE_PER_INSTANCE];
}InstanceData;

typedef struct{
    int objId;
    int instNum;
    InstanceData data[MAX_INSTANCE_PER_OBJ];
}ObjectData;


typedef void (*FunctionUpdateMeasure)(const ObjectData* , lwm2m_context_t* );

typedef struct{
    int objId;
    FunctionUpdateMeasure  pfun;
}ObjIdFuncMap;


static inline void appendResourceIdValue(const char* resId, const char *resV, InstanceData *instData)
{
    int resNum = instData->resNum;
    if(resNum < MAX_RESOURCE_PER_INSTANCE)
    {
        instData->resValues[resNum].resId = atoi(resId);//use strtol() to replace atoi()
        strncpy(instData->resValues[resNum].value, resV, MAX_VALUE_LENGTH_PER_RESOURCE);
        ENSURE_END_WITH_NULL_CHAR(instData->resValues[resNum].value);
        instData->resNum += 1;
    }
}



#endif //LWM2MCLIENT_JSONDATA_H
