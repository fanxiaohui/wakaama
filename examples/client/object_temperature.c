//
// Created by zengliang on 18-7-10.
//


#include <liblwm2m.h>
#include "sensorData.h"

#ifdef LWM2M_CLIENT_MODE

#define RES_ID_SENSOR_INTERVAL   5524
#define RES_ID_SENSOR_VALUE      5700
#define RES_ID_SENSOR_BATTERY    5800
#define RES_ID_TIMESTAMP         6021

#define RES_NUM                  4


typedef struct _prv_instance_
{
    /*
     * The first two are mandatories and represent the pointer to the next instance and the ID of this one. The rest
     * is the instance scope user data
     */
    struct _prv_instance_ * next;   // matches lwm2m_list_t::next
    uint16_t instID;               // matches lwm2m_list_t::id

    InstanceData userData;
} prv_instance_t;


static uint8_t fetchValueById(const InstanceData *locDataP, lwm2m_data_t *dataP)
{
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

static uint8_t resourceValues_read(uint16_t instanceId,
                                   int *numDataP,
                                   lwm2m_data_t **dataArrayP,
                                   lwm2m_object_t *objectP)
{

    uint8_t result = COAP_404_NOT_FOUND;
    prv_instance_t *  targetP = (prv_instance_t *)lwm2m_list_find(objectP->instanceList, instanceId);
    if (NULL == targetP) return result;

    if (*numDataP == 0)
    {
        *numDataP = RES_NUM;
        *dataArrayP = lwm2m_data_new(RES_NUM);
        if (*dataArrayP == NULL) return COAP_500_INTERNAL_SERVER_ERROR;
        for(int i = 0; i<RES_NUM; i++ ) {
            (*dataArrayP)[i].id = targetP->userData.resValues[i].resId;
        }
    }

    for (int i = 0 ; i < *numDataP ; i++)
    {
        result = fetchValueById(&targetP->userData, (*dataArrayP) + i);
        if (result != COAP_205_CONTENT) break;
    }

    return result;
}

static void initialResourceIds(InstanceData* instanceData)
{
    instanceData->resNum = RES_NUM;
    instanceData->resValues[0].resId = RES_ID_SENSOR_VALUE;
    instanceData->resValues[1].resId = RES_ID_SENSOR_BATTERY;
    instanceData->resValues[2].resId = RES_ID_TIMESTAMP;
    instanceData->resValues[3].resId = RES_ID_SENSOR_INTERVAL;
}

lwm2m_object_t * create_temperature_object(void)
{

    lwm2m_object_t * obj = (lwm2m_object_t *)lwm2m_malloc(sizeof(lwm2m_object_t));

    if (NULL != obj)
    {
        memset(obj, 0, sizeof(lwm2m_object_t));

        obj->objID = LWM2M_TEMPERATURE_OBJECT_ID;

        for (int i=0 ; i < MAX_INSTANCE_PER_OBJ ; i++)
        {
            prv_instance_t *  instance = (prv_instance_t *)lwm2m_malloc(sizeof(prv_instance_t));
            if (NULL == instance)
            {
                return NULL; //TODO:  free obj
            }
            memset(instance, 0, sizeof(prv_instance_t));
            instance->instID = i;
            instance->userData.instId = i;
            initialResourceIds(&instance->userData);

            obj->instanceList = LWM2M_LIST_ADD(obj->instanceList, instance);
        }
        /*
         * From a single instance object, two more functions are available.
         * - The first one (createFunc) create a new instance and filled it with the provided informations. If an ID is
         *   provided a check is done for verifying his disponibility, or a new one is generated.
         * - The other one (deleteFunc) delete an instance by removing it from the instance list (and freeing the memory
         *   allocated to it)
         */
        obj->readFunc = resourceValues_read;
        //obj->discoverFunc = prv_discover;
        //obj->writeFunc = prv_write;
        //obj->executeFunc = prv_exec;
        //obj->createFunc = prv_create;
        //obj->deleteFunc = prv_delete;
    }

    return obj;
}


void free_object_temperature(lwm2m_object_t * object)
{
    lwm2m_list_free(object->instanceList);
    if (object->userData != NULL)
    {
        lwm2m_free(object->userData);
        object->userData = NULL;
    }
    lwm2m_free(object);
}


#endif
