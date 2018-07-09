//
// Created by lianzeng on 18-6-23.
//
#include "liblwm2m.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "sensorData.h"

/*
 *  Object     |      | Multiple  |     | Description                   |
 *  Name       |  ID  | Instances |Mand.|                               |
 *-------------+------+-----------+-----+-------------------------------+
 *  vehicle   | 1111  |    No     |  No |            |
 *
 *  Resources:
 *  Name        | ID  | Oper.|Instances|Mand.|  Type   | Range | Units | Description                                                                      |
 * -------------+-----+------+---------+-----+---------+-------+-------+----------------------------------------------------------------------------------+
 *  state       |  0  |R(ead)| Single  | Yes | int     |       |       | state of the vehicle. 0: idle 1: driving 2: charging 3: limp-home 4-15: reserved for future use |
 *  rpm         |  1  |R(ead)| Single  | Yes | Float   |       |  rpm  | engine speed read from obd interface |
 *  speed       |  2  |  R   | Single  | Yes | Float   |       |  km/h | vehicle speed read from obd interface |
 *  Timestamp   |  3  |  R   | Single  | Yes | Time    |       |   s   | The timestamp when the location measurement was performed.                       |
 *              |     |      |         |     |         |       |       |                              |
 */
#ifdef LWM2M_CLIENT_MODE

#define RES_ID_M_STATE      0
#define RES_ID_M_SPEED      1
#define RES_ID_M_RPM        2
#define RES_ID_M_TIMESTAMP  3

typedef  struct{
    char rpm[10];//engine speed
    float speed; //vehicle speed
    unsigned long timestamp;
    char   state[20];//0-idle,1-driving,2-charging
}ObdData;

static uint8_t prv_res2tlv(const ObdData* locDataP,
                           lwm2m_data_t* dataP)
{
    //-------------------------------------------------------------------- JH --
    uint8_t ret = COAP_205_CONTENT;
    switch (dataP->id)     // location resourceId
    {
        case RES_ID_M_STATE:
            lwm2m_data_encode_string(locDataP->state, dataP);
            break;
        case RES_ID_M_RPM:
            lwm2m_data_encode_string(locDataP->rpm, dataP);
            break;
        case RES_ID_M_SPEED:
            lwm2m_data_encode_float(locDataP->speed, dataP);
            break;
        case RES_ID_M_TIMESTAMP:
            lwm2m_data_encode_int(locDataP->timestamp, dataP);
            break;
        default:
            ret = COAP_404_NOT_FOUND;
            break;
    }

    return ret;
}

static uint8_t prv_vehicle_read(uint16_t instanceId,
                                 int*  numResourceId,
                                 lwm2m_data_t** tlvArrayP,
                                 lwm2m_object_t*  objectP)
{
//-------------------------------------------------------------------- JH --
    int     i;
    uint8_t result = COAP_500_INTERNAL_SERVER_ERROR;
    ObdData* locDataP = (ObdData*)(objectP->userData);

    // defined as single instance object!
    if (instanceId != 0) return COAP_404_NOT_FOUND;

    if (*numResourceId == 0)     // full object, readable resources!
    {
        uint16_t readResIds[] = {
                RES_ID_M_STATE,
                RES_ID_M_RPM,
                RES_ID_M_SPEED,
                RES_ID_M_TIMESTAMP
        }; // readable resources!

        *numResourceId  = sizeof(readResIds)/sizeof(readResIds[0]);
        *tlvArrayP = lwm2m_data_new(*numResourceId);
        if (*tlvArrayP == NULL) return COAP_500_INTERNAL_SERVER_ERROR;

        // init readable resource id's
        for (i = 0 ; i < *numResourceId ; i++)
        {
            (*tlvArrayP)[i].id = readResIds[i];
        }
    }

    for (i = 0 ; i < *numResourceId ; i++)
    {
        result = prv_res2tlv (locDataP, (*tlvArrayP)+i);
        if (result != COAP_205_CONTENT) break;
    }

    return result;
}

void display_vehicle_object(lwm2m_object_t * object)
{
#ifdef WITH_LOGS
    ObdData * data = (ObdData *)object->userData;
    fprintf(stdout, "  /%u: vehicle object:\r\n", object->objID);
    if (NULL != data)
    {
        fprintf(stdout, "  state: %s,  rpm: %s, speed: %.6f, timestamp: %lu \r\n",
                data->state,data->rpm, data->speed, data->timestamp);
    }
#endif
}

static void setResourceValue(const ResourceValue* rv, ObdData* obdData)
{
  switch(rv->resId)
  {
      case RES_ID_M_STATE:
          strncpy(obdData->state, rv->value, sizeof(obdData->state));
          break;
      default:
          fprintf(stderr,"error,not support resId=%d \n", rv->resId);
          break;
  }
}

void update_vehicle_measurement(const ObjectData* sensorData, lwm2m_context_t* context)
{
    assert(sensorData->objId == LWM2M_VEHICLE_OBJECT_ID && sensorData->instNum == 1);

    lwm2m_object_t* Obj = (lwm2m_object_t*)LWM2M_LIST_FIND(context->objectList,sensorData->objId);
    if(Obj != NULL)
    {
        ObdData* obdData = (ObdData*)Obj->userData;
        const InstanceData* instData = &sensorData->data[0];

        for(int i = 0; i < instData->resNum; i++)
        {
            setResourceValue(&instData->resValues[i], obdData);
        }
        //obdData->state = sensorData->state;
        //obdData->rpm = sensorData->rpm;
        //obdData->speed = sensorData->speed;
        obdData->timestamp = lwm2m_gettime();

    }

    lwm2m_uri_t urip = {LWM2M_URI_FLAG_OBJECT_ID, sensorData->objId, 0, 0};
    lwm2m_resource_value_changed(context, &urip);
}

lwm2m_object_t * create_object_vehicle(void)
{
    //-------------------------------------------------------------------- JH --


    lwm2m_object_t * vehicleObj = (lwm2m_object_t *)lwm2m_malloc(sizeof(lwm2m_object_t));
    if (NULL != vehicleObj)
    {
        memset(vehicleObj, 0, sizeof(lwm2m_object_t));

        // It assigns its unique ID
        vehicleObj->objID = LWM2M_VEHICLE_OBJECT_ID;

        // and its unique instance
        vehicleObj->instanceList = (lwm2m_list_t *)lwm2m_malloc(sizeof(lwm2m_list_t));
        if (NULL != vehicleObj->instanceList)
        {
            memset(vehicleObj->instanceList, 0, sizeof(lwm2m_list_t));
        }
        else
        {
            lwm2m_free(vehicleObj);
            return NULL;
        }

        // And the private function that will access the object.
        // Those function will be called when a read query is made by the server.
        // In fact the library don't need to know the resources of the object, only the server does.
        //
        vehicleObj->readFunc    = prv_vehicle_read;
        vehicleObj->userData    = lwm2m_malloc(sizeof(ObdData));

        // initialize private data structure containing the needed variables
        if (NULL != vehicleObj->userData)
        {
            memset(vehicleObj->userData, 0, sizeof(ObdData));
        }
        else
        {
            lwm2m_free(vehicleObj);
            vehicleObj = NULL;
        }
    }

    return vehicleObj;
}

void free_object_vehicle(lwm2m_object_t * object)
{
    lwm2m_list_free(object->instanceList);
    lwm2m_free(object->userData);
    lwm2m_free(object);
}

#endif