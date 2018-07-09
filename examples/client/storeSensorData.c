//
// Created by impact on 18-7-6.
//

#include <stdio.h>
#include <liblwm2m.h>
#include "sensorData.h"

extern void update_vehicle_measurement(const ObjectData* sensorData, lwm2m_context_t* context);


void printSensorData(const ObjectData *sensorData)
{
    fprintf(stdout,"objId=%d \n", sensorData->objId);
    for(int i = 0; i< sensorData->instNum; i++)
    {
        const InstanceData* instData = &sensorData->data[i];
        fprintf(stdout, "instId=%d \n", instData->instId);

        for(int j = 0; j< instData->resNum; j++)
        {
            fprintf(stdout,"resId=%d, resValue=%s \n", instData->resValues[j].resId, instData->resValues[j].value);
        }
    }
    fflush(stdout);
}



void saveSensorDataToLocal(const ObjectData *sensorData, lwm2m_context_t* context)
{
    static ObjIdFuncMap map[] =
    {
        {LWM2M_VEHICLE_OBJECT_ID, update_vehicle_measurement},

    };


    if(sensorData == NULL) return;

    printSensorData(sensorData);

    for(int i = 0; i < elementsOf(map); i++)
    {
        if(sensorData->objId == map[i].objId)
        {
            map[i].pfun(sensorData, context);
            return ;
        }
    }

    fprintf(stderr, "error, not support objId=%d currently \n", sensorData->objId);
    fflush(stderr);

}