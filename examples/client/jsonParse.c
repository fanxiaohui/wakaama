//
// Created by impact on 18-7-5.
//

#include <liblwm2m.h>
#include <stdio.h>
#include "sensorData.h"
#include "cJSON.h"
/*
example:
  {
  "objId":3303,
  "instId":2,
  "resValues":[
   {
   "id":1,
   "v":"100"
   },
   {
   "id":2,
   "v":"102"
   },
   {
   "id":3
   "v":"103"
   }
   ]
  }
 *
 *
 *
 */

#define FAILURE   (0)
#define SUCCESS   (1)

SensorData* convertJsonToSensorData(const char * const jsonData)
{
    if(jsonData == NULL) return NULL;

    static SensorData sensorData;// static to avoid stackoverflow
    memset(&sensorData, 0, sizeof(sensorData));

    const cJSON *resValue = NULL;
    const cJSON *resValues = NULL;

    int status = SUCCESS;
    cJSON *result_json = cJSON_Parse(jsonData);
    if (result_json == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            fprintf(stderr, "Error parse json: %s\n", error_ptr);
        }
        status = FAILURE;
        goto end;
    }

    const cJSON * objId = cJSON_GetObjectItemCaseSensitive(result_json, "objId");
    const cJSON* instId = cJSON_GetObjectItemCaseSensitive(result_json, "instId");

    if (!cJSON_IsNumber(objId) || !cJSON_IsNumber(instId))
    {
        fprintf(stderr, "error: objId and instId must be a number \n");
        status = FAILURE;
        goto end;
    }

    sensorData.objId = objId->valueint;
    sensorData.instId = instId->valueint;


    resValues = cJSON_GetObjectItemCaseSensitive(result_json, "resValues");
    cJSON_ArrayForEach(resValue, resValues)
    {
        cJSON *resId = cJSON_GetObjectItemCaseSensitive(resValue, "id");
        cJSON *rvalue = cJSON_GetObjectItemCaseSensitive(resValue, "v");

        if (!cJSON_IsNumber(resId) || !cJSON_IsString(rvalue))
        {
            fprintf(stderr, "error: resId must be a number, rvalue must be string \n");
            status = FAILURE;
            goto end;
        }


        appendResourceIdValue(resId->valueint, rvalue->valuestring, &sensorData);

    }


    end:
    cJSON_Delete(result_json);
    return (status == SUCCESS) ? &sensorData : NULL;
}




