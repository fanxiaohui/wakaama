//
// Created by lianzeng on 18-6-23.
//
#include "liblwm2m.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "sensorData.h"
#include "unixIPC.h"

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

#define RES_ID_V_STATE      0   //0-idle,1-driving,2-charging
#define RES_ID_V_SPEED      1   //vehicle speed
#define RES_ID_V_RPM        2   //engine speed
#define RES_ID_V_TIMESTAMP  3   //sample time
#define RES_ID_V_FAULT_NUM  4
#define RES_ID_V_ENG_LOAD   5
#define RES_ID_V_ENG_FUEL_RATE  6
#define RES_ID_V_ENG_COOL_TEMPERATURE  7
#define RES_ID_V_TOTAL_HOURS   8
#define RES_ID_TRIP_DISTANCE  9
#define RES_ID_BATTERY_VOLTAGE 10
#define RES_ID_EMERGE_BRAKE  11
#define RES_ID_OBD_BLE_ADDRESS   12


//#define RES_NUM_VEHICLE     13
#define OBD_BLE_ADDRESS_LEN  17 //XX:XX:XX:XX:XX:XX

typedef InstanceData ObdData;

extern int writeFile(const char* filename,  const char* data);

static uint8_t fetchValueById(const ObdData *locDataP,
                              lwm2m_data_t *dataP)
{
    //-------------------------------------------------------------------- JH --
    for(int i = 0; i<locDataP->resNum; i++)
    {
        if(dataP->id == locDataP->resValues[i].resId)
        {
            lwm2m_data_encode_string(locDataP->resValues[i].value, dataP);
            return COAP_205_CONTENT;
        }

    }

    return  COAP_404_NOT_FOUND;
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
        *numResourceId  = RES_NUM_VEHICLE;//all resId is readble
        *tlvArrayP = lwm2m_data_new(*numResourceId);
        if (*tlvArrayP == NULL) return COAP_500_INTERNAL_SERVER_ERROR;

        // init readable resource id's
        for (i = 0 ; i < *numResourceId ; i++)
        {
            (*tlvArrayP)[i].id = locDataP->resValues[i].resId;
        }
    }

    for (i = 0 ; i < *numResourceId ; i++)
    {
        result = fetchValueById(locDataP, (*tlvArrayP) + i);
        if (result != COAP_205_CONTENT) break;
    }

    return result;
}

static uint8_t obdAddr_write(uint16_t instanceId,
                                  int numData,
                                  lwm2m_data_t * dataArray,
                                  lwm2m_object_t * objectP)
{

    uint8_t result = COAP_500_INTERNAL_SERVER_ERROR;
    ObdData * data = (ObdData*)(objectP->userData);

    // this is a single instance object
    if (instanceId != 0 || numData != 1)
    {
        return COAP_404_NOT_FOUND;
    }

    if(dataArray == NULL) return COAP_500_INTERNAL_SERVER_ERROR;


    int length = 0;
    switch (dataArray[0].id)
	{
	case RES_ID_OBD_BLE_ADDRESS:

		if(dataArray->value.asBuffer.buffer) {

			if (dataArray->value.asBuffer.length >= OBD_BLE_ADDRESS_LEN) {
				memset(data->resValues[RES_ID_OBD_BLE_ADDRESS].value,  0,  MAX_VALUE_LENGTH_PER_RESOURCE);
				strncpy(data->resValues[RES_ID_OBD_BLE_ADDRESS].value, dataArray->value.asBuffer.buffer, OBD_BLE_ADDRESS_LEN);
				data->resValues[RES_ID_OBD_BLE_ADDRESS].value[OBD_BLE_ADDRESS_LEN] = '\0';

				fprintf(stdout,"obdBle addr=%s \n", data->resValues[RES_ID_OBD_BLE_ADDRESS].value);

				//send_Dgram(g_fdIpc, OBD_DAEMON_SOCK , data->resValues[RES_ID_OBD_BLE_ADDRESS].value);
				//write obd address to file
				if(writeFile(OBD_BLUE_ADDR_FILE, data->resValues[RES_ID_OBD_BLE_ADDRESS].value))
					result = COAP_204_CHANGED;
				else
					result = COAP_500_INTERNAL_SERVER_ERROR;
			}
			else {
				fprintf(stdout, "invalid obd address:%s",dataArray->value.asBuffer.buffer);
				result = COAP_400_BAD_REQUEST;
			}
		}else{
			fprintf(stdout, "empty obd address \n");
			result = COAP_400_BAD_REQUEST;
		}
		break;

	default:
		result = COAP_405_METHOD_NOT_ALLOWED;
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
    }
#endif
}

static void setResourceValue(const ResourceValue* rv, ObdData* obdData)
{
  for(int i = 0; i<obdData->resNum; i++)
  {
      if(rv->resId == obdData->resValues[i].resId)
      {
          strncpy(obdData->resValues[i].value, rv->value, MAX_VALUE_LENGTH_PER_RESOURCE);
          return;
      }

  }

  fprintf(stderr,"error,not support resId=%d \n", rv->resId);

}

void update_vehicle_measurement(const ObjectData* sensorData, lwm2m_context_t* context)
{
    assert(sensorData->objId == LWM2M_VEHICLE_OBJECT_ID && sensorData->instNum == 1);

    lwm2m_object_t* Obj = (lwm2m_object_t*)LWM2M_LIST_FIND(context->objectList,sensorData->objId);
    if(Obj != NULL)
    {
        ObdData* obdData = (ObdData*)Obj->userData;
        const InstanceData* instData = &sensorData->data[0];

        for(int i = 0; i < instData->resNum; i++)//note:here can't directly copy sensorData->data[0] to Obj->userData, since userData's resId can't be changed after initialize
        {
            setResourceValue(&instData->resValues[i], obdData);
        }

        markSensorValueChangedToTrigLaterReport(sensorData->objId, context);
    }

}

static void initialResourceIds(ObdData* obdData)
{
    obdData->instId = 0;
    obdData->resNum = RES_NUM_VEHICLE;
    obdData->resValues[0].resId = RES_ID_V_STATE;
    obdData->resValues[1].resId = RES_ID_V_SPEED;
    obdData->resValues[2].resId = RES_ID_V_RPM;
    obdData->resValues[3].resId = RES_ID_V_TIMESTAMP;
    obdData->resValues[4].resId = RES_ID_V_FAULT_NUM;
    obdData->resValues[5].resId = RES_ID_V_ENG_LOAD;
    obdData->resValues[6].resId = RES_ID_V_ENG_FUEL_RATE;
    obdData->resValues[7].resId = RES_ID_V_ENG_COOL_TEMPERATURE;
    obdData->resValues[8].resId = RES_ID_V_TOTAL_HOURS;
    obdData->resValues[9].resId = RES_ID_TRIP_DISTANCE;
    obdData->resValues[10].resId = RES_ID_BATTERY_VOLTAGE;
    obdData->resValues[11].resId = RES_ID_EMERGE_BRAKE;
    obdData->resValues[12].resId = RES_ID_OBD_BLE_ADDRESS;
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
        vehicleObj->instanceList = (lwm2m_list_t *)lwm2m_malloc(sizeof(lwm2m_list_t));//TODO: for singleInstance object,instanceList is no use actually.
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
        vehicleObj->writeFunc   =  obdAddr_write;
        vehicleObj->readFunc    = prv_vehicle_read;
        vehicleObj->userData    = lwm2m_malloc(sizeof(ObdData));//for single instance object, lwm2m_object_t.userData is used to store sensor data;

        // initialize private data structure containing the needed variables
        if (NULL != vehicleObj->userData)
        {
            memset(vehicleObj->userData, 0, sizeof(ObdData));
            initialResourceIds(vehicleObj->userData);

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
