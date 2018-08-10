/*******************************************************************************
 * 
 * Copyright (c) 2014 Bosch Software Innovations GmbH, Germany.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * The Eclipse Distribution License is available at
 *    http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Bosch Software Innovations GmbH - Please refer to git log
 *    Pascal Rieux - Please refer to git log
 *    
 ******************************************************************************/
/*! \file
  LWM2M object "Location" implementation

  \author Joerg Hubschneider
*/

/*
 *  Object     |      | Multiple  |     | Description                   |
 *  Name       |  ID  | Instances |Mand.|                               |
 *-------------+------+-----------+-----+-------------------------------+
 *  Location   |   6  |    No     |  No |  see TS E.7 page 101          |
 *
 *  Resources:
 *  Name        | ID  | Oper.|Instances|Mand.|  Type   | Range | Units | Description                                                                      |
 * -------------+-----+------+---------+-----+---------+-------+-------+----------------------------------------------------------------------------------+
 *  Latitude    |  0  |  R   | Single  | Yes | Float   |       |  Deg  | The decimal notation of latitude  e.g. -  45.5723  [Worlds Geodetic System 1984].|
 *  Longitude   |  1  |  R   | Single  | Yes | Float   |       |  Deg  | The decimal notation of longitude e.g. - 153.21760 [Worlds Geodetic System 1984].|
 *  Altitude    |  2  |  R   | Single  | No  | Float   |       |   m   | The decimal notation of altitude in meters above sea level.                      |
 *  Radius      |  3  |  R   | Single  | No  | Float   |       |   m   | The value in the Radius Resource indicates the size in meters of a circular area |
 *              |     |      |         |     |         |       |       | around a point of geometry.                                                      |
 *  Velocity    |  4  |  R   | Single  | No  | Opaque  |       |   *   | The velocity of the device as defined in 3GPP 23.032 GAD specification(*).       |
 *              |     |      |         |     |         |       |       | This set of values may not be available if the device is static.                 |
 *              |     |      |         |     |         |       |       | opaque: see OMA_TS 6.3.2                                                         |
 *  Timestamp   |  5  |  R   | Single  | Yes | Time    |       |   s   | The timestamp when the location measurement was performed.                       |
 *  Speed       |  6  |  R   | Single  | No  | Float   |       |  m/s  | Speed is the time rate of change in position of a LwM2M Client without regard    |
 *              |     |      |         |     |         |       |       | for direction: the scalar component of velocity.                                 |
 */

#include "liblwm2m.h"
#include "sensorData.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <internals.h>

#ifdef LWM2M_CLIENT_MODE


// ---- private "object location" specific defines ----
// Resource Id's:
#define RES_M_LATITUDE     0
#define RES_M_LONGITUDE    1
#define RES_O_ALTITUDE     2
#define RES_O_RADIUS       3
#define RES_O_VELOCITY     4
#define RES_M_TIMESTAMP    5
#define RES_O_SPEED        6

//-----  3GPP TS 23.032 V11.0.0(2012-09) ---------
#define HORIZONTAL_VELOCITY                  0  // for Octet-1 upper half(..<<4)
#define HORIZONTAL_VELOCITY_VERTICAL         1  // set vertical direction bit!
#define HORIZONTAL_VELOCITY_WITH_UNCERTAINTY 2

#define VELOCITY_OCTETS                      5  // for HORIZONTAL_VELOCITY_WITH_UNCERTAINTY 

typedef struct
{
    float    latitude;
    float    longitude;
    float    altitude;
    float    radius;
    uint8_t  velocity   [VELOCITY_OCTETS];        //3GPP notation 1st step: HORIZONTAL_VELOCITY_WITH_UNCERTAINTY
    unsigned long timestamp;
    float    speed;
} location_data_t;

/**
implementation for all read-able resources
*/
static uint8_t prv_res2tlv(lwm2m_data_t* dataP,
                           location_data_t* locDataP)
{
    //-------------------------------------------------------------------- JH --
    uint8_t ret = COAP_205_CONTENT;  
    switch (dataP->id)     // location resourceId
    {
    case RES_M_LATITUDE:
        lwm2m_data_encode_float(locDataP->latitude, dataP);
        break;
    case RES_M_LONGITUDE:
        lwm2m_data_encode_float(locDataP->longitude, dataP);
        break;
    case RES_O_ALTITUDE:
        lwm2m_data_encode_float(locDataP->altitude, dataP);
        break;
    case RES_O_RADIUS:
        lwm2m_data_encode_float(locDataP->radius, dataP);
        break;
    case RES_O_VELOCITY:
        lwm2m_data_encode_string((const char*)locDataP->velocity, dataP);
        break;
    case RES_M_TIMESTAMP:
        lwm2m_data_encode_int(locDataP->timestamp, dataP);
        break;
    case RES_O_SPEED:
        lwm2m_data_encode_float(locDataP->speed, dataP);
        break;
    default:
        ret = COAP_404_NOT_FOUND;
        break;
    }
  
    return ret;
}


/**
  * Implementation (callback-) function of reading object resources. For whole
  * object, single resources or a sequence of resources
  * see 3GPP TS 23.032 V11.0.0(2012-09) page 23,24.
  * implemented for: HORIZONTAL_VELOCITY_WITH_UNCERTAINT
  * @param objInstId    in,     instances ID of the location object to read
  * @param numDataP     in/out, pointer to the number of resource to read. 0 is the
  *                             exception for all readable resource of object instance
  * @param tlvArrayP    in/out, TLV data sequence with initialized resource ID to read
  * @param objectP      in,     private location data structure
  */
static uint8_t prv_location_read(uint16_t instId,
                                 int*  numDataP,
                                 lwm2m_data_t** tlvArrayP,
                                 lwm2m_object_t*  objectP)
{   
    //-------------------------------------------------------------------- JH --
    int     i;
    uint8_t result = COAP_500_INTERNAL_SERVER_ERROR;
    location_data_t* locDataP = (location_data_t*)(objectP->userData);

    // defined as single instance object!
    if (instId != 0) return COAP_404_NOT_FOUND;

    if (*numDataP == 0)     // full object, readable resources!
    {
        uint16_t readResIds[] = {
                RES_M_LATITUDE,
                RES_M_LONGITUDE,
                RES_O_ALTITUDE,
                RES_O_RADIUS,
                RES_O_VELOCITY,
                RES_M_TIMESTAMP,
                RES_O_SPEED
        }; // readable resources!
        
        *numDataP  = sizeof(readResIds)/sizeof(uint16_t);
        *tlvArrayP = lwm2m_data_new(*numDataP);
        if (*tlvArrayP == NULL) return COAP_500_INTERNAL_SERVER_ERROR;
        
        // init readable resource id's
        for (i = 0 ; i < *numDataP ; i++)
        {
            (*tlvArrayP)[i].id = readResIds[i];
        }
    }
    
    for (i = 0 ; i < *numDataP ; i++)
    {
        result = prv_res2tlv ((*tlvArrayP)+i, locDataP);
        if (result!=COAP_205_CONTENT) break;
    }
    
    return result;
}

void display_location_object(lwm2m_object_t * object)
{
#ifdef WITH_LOGS
    location_data_t * data = (location_data_t *)object->userData;
    fprintf(stdout, "  /%u: Location object:\r\n", object->objID);
    if (NULL != data)
    {
        fprintf(stdout, "    latitude: %.6f, longitude: %.6f, altitude: %.6f, radius: %.6f, timestamp: %lu, speed: %.6f\r\n",
                data->latitude, data->longitude, data->altitude, data->radius, data->timestamp, data->speed);
    }
#endif
}

/**
  * Convenience function to set the velocity attributes.
  * see 3GPP TS 23.032 V11.0.0(2012-09) page 23,24.
  * implemented for: HORIZONTAL_VELOCITY_WITH_UNCERTAINTY
  * @param locationObj location object reference (to be casted!)
  * @param bearing          [Deg]  0 - 359    resolution: 1 degree
  * @param horizontalSpeed  [km/h] 1 - s^16-1 resolution: 1 km/h steps
  * @param speedUncertainty [km/h] 1-254      resolution: 1 km/h (255=undefined!)
  */
void location_setVelocity(lwm2m_object_t* locationObj,
                          uint16_t bearing,
                          uint16_t horizontalSpeed,
                          uint8_t speedUncertainty)
{
    //-------------------------------------------------------------------- JH --
    location_data_t* pData = locationObj->userData;
    pData->velocity[0] = HORIZONTAL_VELOCITY_WITH_UNCERTAINTY << 4;
    pData->velocity[0] = (bearing & 0x100) >> 8;
    pData->velocity[1] = (bearing & 0x0FF);
    pData->velocity[2] = horizontalSpeed >> 8;
    pData->velocity[3] = horizontalSpeed & 0xff;
    pData->velocity[4] = speedUncertainty;
}

/**
  * A convenience function to set the location coordinates with its timestamp.
  * @see testMe()
  * @param locationObj location object reference (to be casted!)
  * @param latitude  the second argument.
  * @param longitude the second argument.
  * @param altitude  the second argument.
  * @param timestamp the related timestamp. Seconds since 1970.
  */
static void update_gps_measurement(const location_data_t *gpsData, lwm2m_context_t *context)


{
    lwm2m_object_t* objectP = (lwm2m_object_t*)LWM2M_LIST_FIND(context->objectList,LWM2M_LOCATION_OBJECT_ID);
    memcpy(objectP->userData, gpsData, sizeof(location_data_t));
}


static void setResourceValue(const ResourceValue* rv, location_data_t* obdData)
{
    double tmp = 0.0;

    switch(rv->resId)
    {
        case RES_M_LATITUDE:
            utils_textToFloat((uint8_t *)rv->value,strlen(rv->value), &tmp);
            obdData->latitude = tmp;
            break;
        case RES_O_ALTITUDE:
            utils_textToFloat((uint8_t *)rv->value,strlen(rv->value), &tmp);
            obdData->altitude = tmp;
            break;
        case RES_M_LONGITUDE:
            utils_textToFloat((uint8_t *)rv->value,strlen(rv->value), &tmp);
            obdData->longitude = tmp;
            break;
        case RES_O_RADIUS:
            utils_textToFloat((uint8_t *)rv->value,strlen(rv->value), &tmp);
            obdData->radius = tmp;
            break;
        case RES_O_SPEED:
            utils_textToFloat((uint8_t *)rv->value,strlen(rv->value), &tmp);
            obdData->speed = tmp;
            break;
        case RES_M_TIMESTAMP:
            utils_textToInt((uint8_t*)rv->value,strlen(rv->value), &obdData->timestamp);
            break;
        default:
            fprintf(stderr,"gps resId=%d not supported \n", rv->resId);
            break;
    }
}

void update_location_measurement(const ObjectData* sensorData, lwm2m_context_t* context)
{
    //first, convert ObjectData to location_data_t
    assert(sensorData->objId == LWM2M_LOCATION_OBJECT_ID && sensorData->instNum == 1);

    lwm2m_object_t* Obj = (lwm2m_object_t*)LWM2M_LIST_FIND(context->objectList,sensorData->objId);
    if(Obj != NULL)
    {
        location_data_t* gpsData = (location_data_t*)Obj->userData;
        const InstanceData* instData = &sensorData->data[0];

        for(int i = 0; i < instData->resNum; i++)
        {
            setResourceValue(&instData->resValues[i], gpsData);
        }

        markSensorValueChangedToTrigLaterReport(sensorData->objId, context);
    }
}

/**
  * This function creates the LWM2M Location. 
  * @return gives back allocated LWM2M data object structure pointer. On error, 
  * NULL value is returned.
  */
lwm2m_object_t * get_object_location(void)
{
    //-------------------------------------------------------------------- JH --
    lwm2m_object_t * locationObj;

    locationObj = (lwm2m_object_t *)lwm2m_malloc(sizeof(lwm2m_object_t));
    if (NULL != locationObj)
    {
        memset(locationObj, 0, sizeof(lwm2m_object_t));

        // It assigns its unique ID
        // The 6 is the standard ID for the optional object "Location".
        locationObj->objID = LWM2M_LOCATION_OBJECT_ID;
        
        // and its unique instance
        locationObj->instanceList = (lwm2m_list_t *)lwm2m_malloc(sizeof(lwm2m_list_t));
        if (NULL != locationObj->instanceList)
        {
            memset(locationObj->instanceList, 0, sizeof(lwm2m_list_t));
        }
        else
        {
            lwm2m_free(locationObj);
            return NULL;
        }

        // And the private function that will access the object.
        // Those function will be called when a read query is made by the server.
        // In fact the library don't need to know the resources of the object, only the server does.
        //
        locationObj->readFunc    = prv_location_read;
        locationObj->userData    = lwm2m_malloc(sizeof(location_data_t));

        // initialize private data structure containing the needed variables
        if (NULL != locationObj->userData)
        {
            location_data_t* data = (location_data_t*)locationObj->userData;
            data->latitude    = 30.163539;  // hangzhou
            data->longitude   = 120.09172;
            data->altitude    = 8495.0000;
            data->radius      = 0.0;
            location_setVelocity(locationObj, 0, 0, 255); // 255: speedUncertainty not supported!
            data->timestamp   = time(NULL);
            data->speed       = 0.0;
        }
        else
        {
            lwm2m_free(locationObj);
            locationObj = NULL;
        }
    }
    
    return locationObj;
}

void free_object_location(lwm2m_object_t * object)
{
    lwm2m_list_free(object->instanceList);
    lwm2m_free(object->userData);
    lwm2m_free(object);
}


void stub_updateLocation(lwm2m_context_t *context)
{
    static float latitude = 30.163539;
    latitude += 0.1;
    static float longitude = 120.09172;
    longitude += 0.1;


    location_data_t gpsData;
    memset(&gpsData, 0, sizeof(gpsData));
    gpsData.latitude = latitude;
    gpsData.longitude = longitude;
    gpsData.altitude = 30;
    gpsData.radius = 0;
    gpsData.speed = 80;
    gpsData.timestamp = lwm2m_gettime();

    update_gps_measurement(&gpsData, context);

    markSensorValueChangedToTrigLaterReport(LWM2M_LOCATION_OBJECT_ID, context);
}

#endif  //LWM2M_CLIENT_MODE
