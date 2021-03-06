/*******************************************************************************
 *
 * Copyright (c) 2013, 2014 Intel Corporation and others.
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
 *    Julien Vermillard - initial implementation
 *    Fabien Fleutot - Please refer to git log
 *    David Navarro, Intel Corporation - Please refer to git log
 *    Bosch Software Innovations GmbH - Please refer to git log
 *    Pascal Rieux - Please refer to git log
 *    Gregory Lemercier - Please refer to git log
 *    
 *******************************************************************************/

/*
 * This object is single instance only, and provide firmware upgrade functionality.
 * Object ID is 5.
 */

/*
 * resources:
 * 0 package                   write
 * 1 package url               write,read
 * 2 update                    exec
 * 3 state                     read
 * 5 update result             read
 * 6 package name              read
 * 7 package version           read
 * 8 update protocol support   read
 * 9 update delivery method    read
 */

#include "liblwm2m.h"
#include "sensorData.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <liblwm2m.h>
#include <assert.h>
#include <internals.h>

// ---- private object "Firmware" specific defines ----
// Resource Id's:
#define RES_M_PACKAGE                   0
#define RES_M_PACKAGE_URI               1
#define RES_M_UPDATE                    2
#define RES_M_STATE                     3
#define RES_M_UPDATE_RESULT             5
#define RES_O_PKG_NAME                  6
#define RES_O_PKG_VERSION               7
#define RES_O_UPDATE_PROTOCOL           8
#define RES_M_UPDATE_METHOD             9



const static int g_readableResId[7] = {RES_M_PACKAGE_URI,RES_M_STATE,RES_M_UPDATE_RESULT,RES_O_PKG_NAME,RES_O_PKG_VERSION,RES_O_UPDATE_PROTOCOL,RES_M_UPDATE_METHOD};

#define LWM2M_FIRMWARE_PROTOCOL_NUM     4
#define LWM2M_FIRMWARE_PROTOCOL_NULL    ((uint8_t)-1)

#define DELIVER_METHOD_PULL_ONLY    0   //client fetch by url
#define DELIVER_METHOD_PUSH_ONLY    1   //lwm2mserver transfer file to client
#define DELIVER_METHOD_PUSH_PULL    2

#define FIRMWARE_UPDATE_PROTOCOL_COAP  0
#define FIRMWARE_UPDATE_PROTOCOL_COAPS  1
#define FIRMWARE_UPDATE_PROTOCOL_HTTP11  2
#define FIRMWARE_UPDATE_PROTOCOL_HTTPS11  3


#define STATE_IDLE          0   //before downloading or after succesfull update
#define STATE_DOWNLOADING   1   //after receive url from server
#define STATE_DOWNLOAD_FINISHED    2   //after downloaded finished, how about download failed ?
#define STATE_UPDATING      3  //after receive update cmd from server

#define UPDATE_RESULT_INITIAL   0
#define UPDATE_RESULT_SUCCESFULL   1
#define UPDATE_RESULT_NOT_ENOUGH_FLASH_SPACE   2
#define UPDATE_RESULT_NOT_ENOUGH_RAM   3
#define UPDATE_RESULT_CONNECTION_LOST_DURING_DOWNLOADING  4
#define UPDATE_RESULT_INTEGRRITY_CHECK_FAIL  5
#define UPDATE_RESULT_UNSUPPORT_PACKAGE_TYPE  6
#define UPDATE_RESULT_INVALID_URL    7
#define UPDATE_RESULT_FIRMWARE_UPDATE_FAILED  8
#define UPDATE_RESULT_UNSUPPORT_PROTOCOL   9


#define  MAX_URL_LENGTH   256 //maybe include other info, such as md5sum of new package

extern int g_fdIpc;
extern void send_Dgram_FirmwareUpdate(const int ipcfd, const char* buffer);

typedef struct
{
    uint8_t state;
    uint8_t result;
    char pkg_url[MAX_URL_LENGTH]; //max 256 char length
    char pkg_name[256];
    char pkg_version[256];
    uint8_t protocol_support[LWM2M_FIRMWARE_PROTOCOL_NUM];
    uint8_t delivery_method;
} firmware_data_t;

void update_firmwareState(const char* cmdResp, lwm2m_context_t* context)//update state when receive resp from firmwareUpdateProcess
{
    fprintf(stdout,"cmdResp: %s ",cmdResp);
    lwm2m_object_t* objectP = (lwm2m_object_t*)LWM2M_LIST_FIND(context->objectList, LWM2M_FIRMWARE_UPDATE_OBJECT_ID);
    if(objectP) {
        firmware_data_t *data = (firmware_data_t *) (objectP->userData);
        int oldState = data->state;
        switch(oldState)
        {
            case STATE_DOWNLOADING:
                if(strcmp(cmdResp,"DL_OK") == 0) {
                    data->state = STATE_DOWNLOAD_FINISHED;
                }else if(strcmp(cmdResp,"DL_FAIL") == 0){
                    data->state = STATE_IDLE;//TODO:confirm with impact
                }else{
                    fprintf(stderr,"wrong resp");
                }

                break;
            case STATE_UPDATING:
                if(strcmp(cmdResp,"UP_OK") == 0){
                    data->state = STATE_IDLE;
                    data->result = UPDATE_RESULT_SUCCESFULL;
                }else if(strcmp(cmdResp,"UP_FAIL") == 0){
                    data->result = UPDATE_RESULT_FIRMWARE_UPDATE_FAILED;
                }else{
                    fprintf(stderr,"wrong resp");
                }
                break;
            default:
                fprintf(stdout, "wrong resp");
                break;
        }

        fprintf(stdout,"oldState =%d,  newState=%d ",oldState, data->state);
        fflush(stdout);
    }
}

static uint8_t prv_firmware_read(uint16_t instanceId,
                                 int * numDataP,
                                 lwm2m_data_t ** dataArrayP,
                                 lwm2m_object_t * objectP)
{

    uint8_t result;
    firmware_data_t * data = (firmware_data_t*)(objectP->userData);

    // this is a single instance object
    if (instanceId != 0)
    {
        return COAP_404_NOT_FOUND;
    }

    // is the server asking for the full object ?
    if (*numDataP == 0)
    {
        *numDataP = elementsOf(g_readableResId);
        *dataArrayP = lwm2m_data_new(*numDataP);
        if (*dataArrayP == NULL) return COAP_500_INTERNAL_SERVER_ERROR;
        for(int i = 0; i<*numDataP; i++)
        (*dataArrayP)[i].id = g_readableResId[i];

    }

    int i = 0;
    do
    {
        switch ((*dataArrayP)[i].id)
        {
        case RES_M_PACKAGE:
        case RES_M_UPDATE:
            result = COAP_405_METHOD_NOT_ALLOWED;
            break;

        case RES_M_STATE:
            // firmware update state (int)
            lwm2m_data_encode_int(data->state, *dataArrayP + i);
            result = COAP_205_CONTENT;
            break;

        case RES_M_UPDATE_RESULT:
            lwm2m_data_encode_int(data->result, *dataArrayP + i);
            result = COAP_205_CONTENT;
            break;
        case RES_M_PACKAGE_URI:
            lwm2m_data_encode_string(data->pkg_url, *dataArrayP + i);
            result = COAP_205_CONTENT;
            break;
        case RES_O_PKG_NAME:
            lwm2m_data_encode_string(data->pkg_name, *dataArrayP + i);
            result = COAP_205_CONTENT;
            break;

        case RES_O_PKG_VERSION:
            lwm2m_data_encode_string(data->pkg_version, *dataArrayP + i);
            result = COAP_205_CONTENT;
            break;

        case RES_O_UPDATE_PROTOCOL:
        {
            int ri;
            int num = 0;
            lwm2m_data_t* subTlvP = NULL;

            while ((num < LWM2M_FIRMWARE_PROTOCOL_NUM) &&
                    (data->protocol_support[num] != LWM2M_FIRMWARE_PROTOCOL_NULL))
                num++;

            if (num) {
                subTlvP = lwm2m_data_new(num);
                for (ri = 0; ri<num; ri++)
                {
                    subTlvP[ri].id = ri;
                    lwm2m_data_encode_int(data->protocol_support[ri], subTlvP + ri);
                }
            } else {
                /* If no protocol is provided, use CoAP as default (per spec) */
                num = 1;
                subTlvP = lwm2m_data_new(num);
                subTlvP[0].id = 0;
                lwm2m_data_encode_int(0, subTlvP);
            }
            lwm2m_data_encode_instances(subTlvP, num, *dataArrayP + i);
            result = COAP_205_CONTENT;
            break;
        }

        case RES_M_UPDATE_METHOD:
            lwm2m_data_encode_int(data->delivery_method, *dataArrayP + i);
            result = COAP_205_CONTENT;
            break;

        default:
            result = COAP_404_NOT_FOUND;
        }

        i++;
    } while (i < *numDataP && result == COAP_205_CONTENT);

    return result;
}

static int checkUrlScheme(const char* url)
{
    //if url invalid, data->result = UPDATE_RESULT_INVALID_URL;
    static const char* sample = "http://x.x.x.x/";
    const int minlen = strlen(sample);

    if(strlen(url) > minlen)
    {
        if(strstr(url, "http") != NULL)
            return 1;

    }

    return 0;
}

static uint8_t prv_firmware_write(uint16_t instanceId,
                                  int numData,
                                  lwm2m_data_t * dataArray,
                                  lwm2m_object_t * objectP)
{

    uint8_t result = COAP_500_INTERNAL_SERVER_ERROR;
    firmware_data_t * data = (firmware_data_t*)(objectP->userData);

    // this is a single instance object
    if (instanceId != 0)
    {
        return COAP_404_NOT_FOUND;
    }

    if(dataArray == NULL) return COAP_500_INTERNAL_SERVER_ERROR;

    int i = 0;
    int length = 0;
    do
    {
        switch (dataArray[i].id)
        {
        case RES_M_PACKAGE:
            // inline firmware binary
            result = COAP_204_CHANGED;
            break;

        case RES_M_PACKAGE_URI:
            // URL for download the firmware
            if((dataArray->type ==LWM2M_TYPE_STRING|| dataArray->type == LWM2M_TYPE_OPAQUE)  && dataArray->value.asBuffer.buffer) {
                memset(data->pkg_url,0,sizeof(data->pkg_url));
                length = (dataArray->value.asBuffer.length < MAX_URL_LENGTH) ? dataArray->value.asBuffer.length
                                                                             : MAX_URL_LENGTH;//must use dataArray->value.asBuffer.length to restrict since dataArray->value.asBuffer.buffer is not teminate with null.
                strncpy(data->pkg_url, dataArray->value.asBuffer.buffer, length);//if length == 0, strncpy do nothing.
                ENSURE_END_WITH_NULL_CHAR(data->pkg_url);
                fprintf(stdout,"pkgurl=%s \n", data->pkg_url);
                if (checkUrlScheme(data->pkg_url)) {
                    data->state = STATE_DOWNLOADING;
                    data->result = UPDATE_RESULT_INITIAL;
                    result = COAP_204_CHANGED;
                    send_Dgram_FirmwareUpdate(g_fdIpc, data->pkg_url);
                } else if (data->pkg_url[0] == '\0') {  //empty string
                    data->state = STATE_IDLE;
                    data->result = UPDATE_RESULT_INITIAL;
                    result = COAP_204_CHANGED;
                } else {
                	fprintf(stdout,"invalid URL:%s",dataArray->value.asBuffer.buffer);
                    result = COAP_400_BAD_REQUEST;
                }
            }else{
            	fprintf(stderr, "wrong parse type(%d) for URL \n", dataArray->type);
            	result = COAP_400_BAD_REQUEST;
            }
            break;

        default:
            result = COAP_405_METHOD_NOT_ALLOWED;
        }

        i++;
    } while (i < numData && result == COAP_204_CHANGED);

    return result;
}

static uint8_t prv_firmware_execute(uint16_t instanceId,
                                    uint16_t resourceId,
                                    uint8_t * buffer,
                                    int length,
                                    lwm2m_object_t * objectP)
{
    firmware_data_t * data = (firmware_data_t*)(objectP->userData);

    // this is a single instance object
    if (instanceId != 0)
    {
        return COAP_404_NOT_FOUND;
    }

    if (length != 0) return COAP_400_BAD_REQUEST;

    // for execute callback, resId is always set.
    switch (resourceId)
    {
    case RES_M_UPDATE:
        if (data->state == STATE_DOWNLOAD_FINISHED)
        {
            fprintf(stdout,"\n\t Begin FIRMWARE UPDATE\r\n\n");
            data->state = STATE_UPDATING;
            data->result = UPDATE_RESULT_INITIAL;
            send_Dgram_FirmwareUpdate(g_fdIpc, "UPDATE");//TODO: update with paras,eg:date ?
            return COAP_204_CHANGED;
        }
        else
        {

            fprintf(stdout,"\n\t can't excute update ,currState=%d \r\n\n", data->state);
            return COAP_400_BAD_REQUEST;
        }
    default:
        return COAP_405_METHOD_NOT_ALLOWED;
    }
}

void display_firmware_object(lwm2m_object_t * object)
{
#ifdef WITH_LOGS
    firmware_data_t * data = (firmware_data_t *)object->userData;
    fprintf(stdout, "  /%u: Firmware object:\r\n", object->objID);
    if (NULL != data)
    {
        fprintf(stdout, "    state: %u, result: %u\r\n", data->state,
                data->result);
    }
#endif
}

lwm2m_object_t * get_object_firmware(void)
{
    /*
     * The get_object_firmware function create the object itself and return a pointer to the structure that represent it.
     */
    lwm2m_object_t * firmwareObj;

    firmwareObj = (lwm2m_object_t *)lwm2m_malloc(sizeof(lwm2m_object_t));

    if (NULL != firmwareObj)
    {
        memset(firmwareObj, 0, sizeof(lwm2m_object_t));

        /*
         * It assigns its unique ID
         * The 5 is the standard ID for the optional object "Object firmware".
         */
        firmwareObj->objID = LWM2M_FIRMWARE_UPDATE_OBJECT_ID;

        /*
         * and its unique instance
         *
         */
        firmwareObj->instanceList = (lwm2m_list_t *)lwm2m_malloc(sizeof(lwm2m_list_t));
        if (NULL != firmwareObj->instanceList)
        {
            memset(firmwareObj->instanceList, 0, sizeof(lwm2m_list_t));
        }
        else
        {
            lwm2m_free(firmwareObj);
            return NULL;
        }

        /*
         * And the private function that will access the object.
         * Those function will be called when a read/write/execute query is made by the server. In fact the library don't need to
         * know the resources of the object, only the server does.
         */
        firmwareObj->readFunc    = prv_firmware_read;
        firmwareObj->writeFunc   = prv_firmware_write;
        firmwareObj->executeFunc = prv_firmware_execute;
        firmwareObj->userData    = lwm2m_malloc(sizeof(firmware_data_t));

        /*
         * Also some user data can be stored in the object with a private structure containing the needed variables
         */
        if (NULL != firmwareObj->userData)
        {
            firmware_data_t *data = (firmware_data_t*)(firmwareObj->userData);

            data->state = STATE_IDLE;
            data->result = UPDATE_RESULT_INITIAL;
            strcpy(data->pkg_name, "NokiaEdgeSensor");
            strcpy(data->pkg_version, "1.0");

            /* Only support CoAP based protocols */
            data->protocol_support[0] = FIRMWARE_UPDATE_PROTOCOL_HTTP11;
            data->protocol_support[1] = FIRMWARE_UPDATE_PROTOCOL_HTTPS11;
            data->protocol_support[2] = LWM2M_FIRMWARE_PROTOCOL_NULL;
            data->protocol_support[3] = LWM2M_FIRMWARE_PROTOCOL_NULL;


           data->delivery_method = DELIVER_METHOD_PULL_ONLY;
        }
        else
        {
            lwm2m_free(firmwareObj);
            firmwareObj = NULL;
        }
    }

    return firmwareObj;
}

void free_object_firmware(lwm2m_object_t * objectP)
{
    if (NULL != objectP->userData)
    {
        lwm2m_free(objectP->userData);
        objectP->userData = NULL;
    }
    if (NULL != objectP->instanceList)
    {
        lwm2m_free(objectP->instanceList);
        objectP->instanceList = NULL;
    }
    lwm2m_free(objectP);
}

