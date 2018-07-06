//
// Created by impact on 18-7-6.
//

#include <stdio.h>
#include <liblwm2m.h>
#include "sensorData.h"

extern void update_vehicle_measurement(const SensorData* sensorData, lwm2m_context_t* context);


void printSensorData(const SensorData *sensorData)
{
    fprintf(stdout,"objId=%d,  instId=%d \n", sensorData->objId, sensorData->instId);
    for(int i = 0; i< sensorData->resNum; i++)
    {
        fprintf(stdout, "resId=%d, resV= %s \n", sensorData->resValues[i].resId, sensorData->resValues[i].value);
    }
}



void saveSensorDataToLocal(const SensorData *sensorData, lwm2m_context_t* context)
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