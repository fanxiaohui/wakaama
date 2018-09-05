/*
 * sqliteUtils.h
 *
 *  Created on: Sep 4, 2018
 *      Author: impact
 */

#ifndef EXAMPLES_CLIENT_SQLITEUTILS_H_
#define EXAMPLES_CLIENT_SQLITEUTILS_H_

#include "sensorData.h"

struct ObjectData;

extern void openDbAndCreateTable();
extern void close_db();
extern bool excute_db(const char* sqlCmd);
extern void persistToDb(const ObjectData *sensorData);

typedef void (*pFunInsertDb)(const ObjectData *sensorData);

typedef struct{
	int objId;
	const char* tableName;
	pFunInsertDb insertFun;
}DbTableList;

#endif /* EXAMPLES_CLIENT_SQLITEUTILS_H_ */
