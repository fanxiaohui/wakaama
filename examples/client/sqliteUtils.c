

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include "src_sql/sqlite3.h"
#include "sqliteUtils.h"
#include "liblwm2m.h"

#define RES_ID_V_SPEED      1   //vehicle speed,make sure same definition with file object_vehicle.c
#define RES_ID_V_RPM        2   //engine speed
#define RES_ID_V_TIMESTAMP  3   //sample time
#define TEMPERATURE_RES_ID_SENSOR_VALUE      5700 //make sure same definition with file object_temperature.c
#define TEMPERATURE_RES_ID_TIMESTAMP         6021
#define TEMPERATURE_RES_ID_SERIAL_NUM        5907

#define VEHICLE_TABLE_NAME  "vehicle"
#define TEMPERATURE_TABLE_NAME   "temperature"


static void insertVehicle(const ObjectData *sensorData)
{
	static char cmd[100];
	float speed = 0.00;
	float rpm = 0.00;
	int64_t timestamp =  0;
	int ret = 1 ;

	for(int i = 0; i < sensorData->data[0].resNum; i++)
	{
		const ResourceValue* rv = &sensorData->data[0].resValues[i];
		switch(rv->resId)
		{
		case RES_ID_V_SPEED:
			ret = sscanf(rv->value, "%f", &speed);
			break;
		case RES_ID_V_RPM:
			ret = sscanf(rv->value, "%f", &rpm);
			break;
		case RES_ID_V_TIMESTAMP:
			ret = sscanf(rv->value, "%ld", &timestamp);
			break;
		default: break;
		}

		if(ret != 1)
		{
			fprintf(stderr,"parseValuefailed(%d:%d) ",sensorData->objId,rv->resId);
			return;
		}
	}

	int len = sprintf(cmd, "insert into vehicle values(%.2f,%.2f,%ld);",speed,rpm,timestamp);
	cmd[len] = 0;
	if(!excute_db(cmd))
	{
		fprintf(stderr,"insertVehicleDb failed\n");
	}

}

static void insertTemperature(const ObjectData *sensorData)
{
	static char cmd[100];

	char macAddr[50];
	macAddr[49] = 0;
	int ret = 1 ;

	for(int j = 0; j < sensorData->instNum; j++)
	{
		const InstanceData* instData = &sensorData->data[j];
		int columnNum = 3;
		float value = 0.0;
		int64_t timestamp = 0;
		macAddr[0] = 0;

		for(int i = 0; i < instData->resNum; i++)
		{
			const ResourceValue* rv = &instData->resValues[i];
			switch(rv->resId)
			{
			case TEMPERATURE_RES_ID_SENSOR_VALUE:
				columnNum -= 1;
				ret = sscanf(rv->value, "%f", &value);
				break;
			case TEMPERATURE_RES_ID_TIMESTAMP:
				columnNum -= 1;
				ret = sscanf(rv->value, "%ld", &timestamp);
				break;
			case TEMPERATURE_RES_ID_SERIAL_NUM:
				columnNum -= 1;
				strcpy(macAddr, rv->value);
				break;
			default: break;
			}

			if(ret != 1)
			{
				fprintf(stderr,"parseValuefailed(%d:%d) ",sensorData->objId,rv->resId);
				return;
			}
		}
		if(columnNum != 0)//miss necessary data to insert db
		{
			fprintf(stderr,"missingData\n");
			continue;
		}

		int len = sprintf(cmd, "insert into temperature values(\"%s\",%.2f,%ld);",macAddr,value,timestamp);
		cmd[len] = 0;
		//printf("debug:%s\n",cmd);
		if(!excute_db(cmd))
		{
			fprintf(stderr,"insertTemperatureDb failed\n");
		}
	}
}

static DbTableList sqlTable[] =
{
		{LWM2M_VEHICLE_OBJECT_ID, VEHICLE_TABLE_NAME,&insertVehicle},
		{LWM2M_TEMPERATURE_OBJECT_ID, TEMPERATURE_TABLE_NAME,&insertTemperature},
};

static sqlite3 *g_db = NULL;



static bool open_db(const char* dbName)
{

	int rc = sqlite3_open(dbName, &g_db);//Open a connection to a new or existing SQLite database
	if(rc)
	{
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(g_db));
		sqlite3_close(g_db);
		return false;
	}
    fprintf(stdout, "open db %s success \n", dbName);
	return true;

}

void close_db()
{
	sqlite3_close(g_db);
}


static int callback(void *NotUsed, int argc, char **argv, char **azColName)
{
	for(int i=0; i<argc; i++){
		printf("%s = %s ", azColName[i], argv[i] ? argv[i] : "NULL");
	}
	printf("\n");
	return 0;

}



bool excute_db(const char* sqlCmd)
{
	char *zErrMsg = 0;
	int rc = sqlite3_exec(g_db,sqlCmd , callback, 0, &zErrMsg);
	if( rc != SQLITE_OK )
	{
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
		return false;
	}
	return true;
}

void openDbAndCreateTable()
{
	if(open_db("edge.db"))
	{
		//ensure the table name is same with sqlTable
		excute_db("create table vehicle(speed float, rpm float , timestamp integer);");
		excute_db("create table temperature(macAddr varchar(17),value float, timestamp integer);");
	}
}

void persistToDb(const ObjectData *sensorData)//new data will append to db;
{
  for(int i = 0 ;i < sizeof(sqlTable)/sizeof(sqlTable[0]); i++)
  {
	  if(sensorData->objId == sqlTable[i].objId)
	  {
		  if(sqlTable[i].insertFun)
		  {
			  sqlTable[i].insertFun(sensorData);
		  }
	  }
  }
}















