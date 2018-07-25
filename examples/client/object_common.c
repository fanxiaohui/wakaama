//
// Created by zengliang on 18-7-25.
//

#ifdef LWM2M_CLIENT_MODE

#include <liblwm2m.h>
#include <memory.h>
#include <assert.h>
#include <internals.h>
#include "sensorData.h"
#include "object_common.h"

static const ResourceIDList g_resouceIdList[] =
{
        {LWM2M_AIR_QUALITY_PM_25_OBJECT_ID, pm25_resIdList,     elementsOf(pm25_resIdList)},
        {LWM2M_HUMIDITY_OBJECT_ID,          humidity_resIdList, elementsOf(humidity_resIdList)},
};


static const ResourceIDList* getResourceIdList(const int objId)
{
    for(int i = 0; i< elementsOf(g_resouceIdList); i++)
    {
        if(g_resouceIdList[i].objId == objId)
        {
            return &g_resouceIdList[i];
        }

    }
    return NULL;
}

static uint8_t fetchValueById(const InstanceData *locDataP,
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


static int getReadableResouceNum(const int objId)
{
    const ResourceIDList* resIdList = getResourceIdList(objId);
    if(resIdList)
        return resIdList->resNum;
    else
    {
        LOG_ARG("not found objId=%d \n",objId);
        return 0;
    }

}

//object read function must implement interface: lwm2m_read_callback_t(uint16_t instanceId, int * numDataP, lwm2m_data_t ** dataArrayP, lwm2m_object_t * objectP);
static uint8_t object_read_uniqInstance(uint16_t instanceId,
                                int*  numResourceId,
                                lwm2m_data_t** tlvArrayP,
                                lwm2m_object_t*  objectP
                                )
{
//-------------------------------------------------------------------- JH --
    int     i;
    uint8_t result = COAP_500_INTERNAL_SERVER_ERROR;
    InstanceData* locDataP = (InstanceData*)(objectP->userData);//for uniq instance , visit objectP->userData


    if (instanceId != 0) return COAP_404_NOT_FOUND; // defined as single instance object!

    if (*numResourceId == 0)     // full object, readable resources!
    {
        *numResourceId  = getReadableResouceNum(objectP->objID);
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

static void initialResourceIds(const int objId,const int instId, InstanceData* instData)
{
    const ResourceIDList* resIdList = getResourceIdList(objId);

    assert(resIdList != NULL);

    instData->instId = instId;
    instData->resNum = resIdList->resNum;
    for(int i = 0; i < resIdList->resNum; i++)
    {
        instData->resValues[i].resId = resIdList->resId[i];
    }

}

static void setResourceValue(const ResourceValue* rv, InstanceData* instanceData)
{
    for(int i = 0; i<instanceData->resNum; i++)
    {
        if(rv->resId == instanceData->resValues[i].resId)
        {
            strncpy(instanceData->resValues[i].value, rv->value, MAX_VALUE_LENGTH_PER_RESOURCE);
            return;
        }

    }

    LOG_ARG("error,not support resId=%d \n", rv->resId);

}

void update_Object_measurement(const ObjectData* sensorData, lwm2m_context_t* context)
{
    assert(sensorData->instNum == 1);

    lwm2m_object_t* Obj = (lwm2m_object_t*)LWM2M_LIST_FIND(context->objectList,sensorData->objId);
    if(Obj != NULL)
    {
        InstanceData* instanceData = (InstanceData*)Obj->userData;
        const InstanceData* instData = &sensorData->data[0];

        for(int i = 0; i < instData->resNum; i++)//note:here can't directly copy sensorData->data[0] to Obj->userData, since userData's resId can't be changed after initialize
        {
            setResourceValue(&instData->resValues[i], instanceData);
        }

        markSensorValueChangedToTrigLaterReport(sensorData->objId, context);
    }
    else
        LOG_ARG("not found objId=%d \n", sensorData->objId);

}

lwm2m_object_t * create_object_with_uniqInstance(const int objId)
{
    //-------------------------------------------------------------------- JH --


    lwm2m_object_t * Obj = (lwm2m_object_t *)lwm2m_malloc(sizeof(lwm2m_object_t));
    if (NULL != Obj)
    {
        memset(Obj, 0, sizeof(lwm2m_object_t));


        Obj->objID = objId;


        // and its unique instance
        Obj->instanceList = (lwm2m_list_t *)lwm2m_malloc(sizeof(lwm2m_list_t));
        if (NULL != Obj->instanceList)
        {
            memset(Obj->instanceList, 0, sizeof(lwm2m_list_t));
        }
        else
        {
            lwm2m_free(Obj);
            return NULL;
        }

        // And the private function that will access the object.
        // Those function will be called when a read query is made by the server.
        // In fact the library don't need to know the resources of the object, only the server does.
        //
        Obj->readFunc    = object_read_uniqInstance;
        Obj->userData    = lwm2m_malloc(sizeof(InstanceData));//for single instance object, lwm2m_object_t.userData is used to store sensor data;

        // initialize private data structure containing the needed variables
        if (NULL != Obj->userData)
        {
            memset(Obj->userData, 0, sizeof(InstanceData));
            initialResourceIds(objId, 0, Obj->userData);

        }
        else
        {
            lwm2m_free(Obj);
            Obj = NULL;
        }
    }

    return Obj;
}


void free_object(lwm2m_object_t * object)
{
    if(object->instanceList)
        lwm2m_list_free(object->instanceList);
    lwm2m_free(object->userData);
    lwm2m_free(object);
}


#endif