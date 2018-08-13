
#include "sensorData.h"

Lwm2mObjDataType lwm2m_data_type_map[] =
{
  {LWM2M_VEHICLE_OBJECT_ID,     RES_NUM_VEHICLE,    {{0,'s'},{1,'f'},{2,'f'},{3,'i'},{4,'i'},{5,'f'},{6,'f'},{7,'f'},{8,'f'},{9,'f'},{10,'f'},{11,'i'},{12,'s'}} },
  {LWM2M_TEMPERATURE_OBJECT_ID, RES_NUM_TEMPERATURE,{{5519,'f'},{5520,'f'},{5524,'i'},{5527,'s'},{5601,'f'},{5602,'f'},{5700,'f'},{5701,'s'},{5800,'f'},{5906,'s'},{5907,'s'},{6021,'i'}} },
};

static void data_encode(const char* value,const char type,  lwm2m_data_t *dataP)
{
  double valuef;
  int64_t    valuei;

  switch(type)
  {
  case 'f':
	  sscanf(value, "%lf", &valuef);
	  lwm2m_data_encode_float(valuef, dataP);
	  break;
  case 'i':
	  sscanf(value, "%ld", &valuei);
	  lwm2m_data_encode_int(valuei, dataP);
	  break;
  case 's':
	  lwm2m_data_encode_string(value, dataP);
	  break;
  default:
	  fprintf(stderr, "wrong type %c \n", type);
	  break;

  }
}

void lwm2m_data_encode_common(const int objId,const int resId,const char* value, lwm2m_data_t *dataP)
{
  for(int i = 0; i< elementsOf(lwm2m_data_type_map); i++)
  {
	  if(lwm2m_data_type_map[i].ojbId == objId)
	  {
		  for(int j = 0;j < lwm2m_data_type_map[i].resNum; j++)
		  {
			  if(lwm2m_data_type_map[i].types[j].resId == resId)
			  {
				  char type = lwm2m_data_type_map[i].types[j].dataType;
				  data_encode(value, type, dataP);
				  return;
			  }
		  }
	  }
  }

  fprintf(stderr,"not found objId:%d,resId:%d \n", objId, resId);
}

