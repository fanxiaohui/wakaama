//
// Created by impact on 18-7-6.
//

#include <stdio.h>
#include <liblwm2m.h>
#include "sensorData.h"
#include "object_common.h"
#include "unixIPC.h"
#include "../../core/er-coap-13/er-coap-13.h"

extern void update_vehicle_measurement(const ObjectData* sensorData, lwm2m_context_t* context);
extern void update_temperature_measurement(const ObjectData* sensorData, lwm2m_context_t* context);
extern void update_location_measurement(const ObjectData* sensorData, lwm2m_context_t* context);
extern void update_Device_measurement(const ObjectData* sensorData, lwm2m_context_t* context);
extern void update_Connect_Monitor_measurement(const ObjectData* sensorData, lwm2m_context_t* context);
extern void send_Dgram(const int ipcfd, const char* peerSock, const char* data);
extern void sendToLwm2mServerIfHasGotReadRequest(lwm2m_context_t* contextP, const int objId);
extern uint8_t object_read(lwm2m_context_t * contextP, lwm2m_uri_t * uriP, lwm2m_media_type_t * formatP, uint8_t ** bufferP, size_t * lengthP);
extern uint8_t message_send(lwm2m_context_t * contextP,coap_packet_t * message, void * sessionH);
extern int g_fdIpc;

void printSensorData(const ObjectData *sensorData)
{
#ifdef WITH_LOGS
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
#endif
}

ReadRequestRecord g_readRequestRecord[] =   //need append if new objId added
{
		{1,false},{2,false},{3,false},{4,false},{5,false},{6,false},{7,false},
		{10244,false},{10255,false},{10256,false},{10257,false}
};

void changeReadRequestFlag(const int objId, const bool value)
{
	for(int i =0 ; i < elementsOf(g_readRequestRecord); i++)
	{
		if(g_readRequestRecord[i].objId == objId)
		{
			g_readRequestRecord[i].recvReadRequest = value;
			return ;
		}
	}

	fprintf(stdout, "warning: not found objId %d \n", objId);
}

#define Clear_READ_REQUEST(objId)  changeReadRequestFlag(objId, false);
#define SET_READ_REQUEST(objId)    changeReadRequestFlag(objId, true);

bool hasRecvReadRequest(const int objId)
{
	for(int i =0 ; i < elementsOf(g_readRequestRecord); i++)
	{
		if(g_readRequestRecord[i].objId == objId)
		{
			return g_readRequestRecord[i].recvReadRequest;
		}
	}

	return false;
}

static lwm2m_uri_t g_readUri = {0,INVALID_LWM2MID,INVALID_LWM2MID,INVALID_LWM2MID};//set value when recv read request,clear when resp to server
static const void* g_sessionH = NULL; //type is :connection_t or dtls_connection_t (#ifdef WITH_TINYDTLS)

void clearUri(lwm2m_uri_t* uri)
{
	uri->flag = 0;
	uri->objectId = INVALID_LWM2MID;
	uri->instanceId = INVALID_LWM2MID;
	uri->resourceId = INVALID_LWM2MID;
}


void saveSensorDataToLocal(const ObjectData *sensorData, lwm2m_context_t* context)
{
    static ObjIdFuncMap map[] =
    {
        {LWM2M_VEHICLE_OBJECT_ID,           update_vehicle_measurement},
        {LWM2M_TEMPERATURE_OBJECT_ID,       update_temperature_measurement},
        {LWM2M_LOCATION_OBJECT_ID,          update_location_measurement},
        {LWM2M_AIR_QUALITY_PM_25_OBJECT_ID, update_Object_measurement},
        {LWM2M_HUMIDITY_OBJECT_ID,          update_Object_measurement},
		{LWM2M_DEVICE_OBJECT_ID,            update_Device_measurement},
		{LWM2M_CONN_MONITOR_OBJECT_ID,      update_Connect_Monitor_measurement},

    };


    if(sensorData == NULL) return;

    printSensorData(sensorData);

    for(int i = 0; i < elementsOf(map); i++)
    {
        if(sensorData->objId == map[i].objId)
        {
            map[i].pfun(sensorData, context);//save to local first, then send to server
            sendToLwm2mServerIfHasGotReadRequest(context, sensorData->objId);//fetch data from local
            return ;
        }
    }

    fprintf(stderr, "error, not support objId=%d currently \n", sensorData->objId);
    fflush(stderr);

}

static void storeInfoOnReadRequestBegin(const lwm2m_uri_t * uriP, const void* sessionH)
{
	SET_READ_REQUEST(uriP->objectId);//so will report to server immediately
	//need local cache, since uriP will be freed.
	g_readUri = *uriP;//TODO: save uriP per object, rather than global;
	g_sessionH = sessionH;
}

static void clearInfoOnReadRequestEnd(const int objId)//after send needed data  to server
{
   	Clear_READ_REQUEST(objId);
   	clearUri(&g_readUri);
   	g_sessionH = NULL;
}

void forwardReadRequestToSensor(const lwm2m_uri_t * uriP, const void* sessionH)
{
	//{"3":{}}   or  {"3":{"1":{}}}   or  {"10255":{"0":{"5524":"","M":"R"}}}

  static char buff[100];
  //memset(buff, 0, sizeof(buff));

  if(uriP)
  {

	  storeInfoOnReadRequestBegin(uriP, sessionH);

	  int objId = uriP->objectId;
	  int len = 0;
	  len += sprintf(buff+len, "{\"%d\":{", objId);

	  if (LWM2M_URI_IS_SET_INSTANCE(uriP))
	  {
		  int instId = uriP->instanceId;
		  len += sprintf(buff+len, "\"%d\":{", instId);

		  if (LWM2M_URI_IS_SET_RESOURCE(uriP))
		  {
			  int resId = uriP->resourceId;
			  len += sprintf(buff+len, "\"%d\":\"\",\"M\":\"R\"", resId);
		  }
		  len += sprintf(buff+len, "}");
	  }

	  len += sprintf(buff+len, "}}");

	  buff[len] = 0;//end with null char

	  send_Dgram(g_fdIpc, LWM2M_KURA_SOCK, buff);

	  fprintf(stdout,"forwardReadRequestToSensor :%s \n",buff);
  }
}


void sendToLwm2mServerIfHasGotReadRequest(lwm2m_context_t* contextP, const int objId)
{//will send twice when recv read request, since the original procedure not change(history value) + latest value;

	if(hasRecvReadRequest(objId) && g_sessionH)
	{

		uint8_t * buffer = NULL;
		size_t length = 0;
		lwm2m_media_type_t format = LWM2M_CONTENT_JSON;

		uint8_t result = object_read(contextP, &g_readUri, &format, &buffer, &length);//read data from local, encode in json format
		if (COAP_205_CONTENT != result)	return;

		coap_packet_t message[1];

		coap_init_message(message, COAP_TYPE_NON, COAP_205_CONTENT, 0);//lwm2m based on coap
		coap_set_header_content_type(message, format);
		coap_set_payload(message, buffer, length);

		message->mid = contextP->nextMID++;
		(void)message_send(contextP, message, (void*)g_sessionH);

        if (buffer != NULL) lwm2m_free(buffer);
        clearInfoOnReadRequestEnd(objId);

	   	fprintf(stdout, "sendDataToReadRequest:%d \n",objId);
	}

}
