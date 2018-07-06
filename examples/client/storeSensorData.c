//
// Created by impact on 18-7-6.
//

#include <stdio.h>
#include "sensorData.h"

void printSensorData(const SensorData *sensorData)
{
    fprintf(stdout,"objId=%d,  instId=%d \n", sensorData->objId, sensorData->instId);
    for(int i = 0; i< sensorData->resNum; i++)
    {
        fprintf(stdout, "resId=%d, resV= %s \n", sensorData->resValues[i].resId, sensorData->resValues[i].value);
    }
}



void saveSensorDataToLocal(const SensorData *sensorData)
{
    if(sensorData == NULL) return;
    printSensorData(sensorData);


}