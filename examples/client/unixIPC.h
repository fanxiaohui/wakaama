/*
 * unixIPC.h
 *
 *  Created on: Aug 8, 2018
 *      Author: impact
 */

#ifndef EXAMPLES_CLIENT_UNIXIPC_H_
#define EXAMPLES_CLIENT_UNIXIPC_H_

#define LWM2M_SENSOR_REPORT_SOCK  "/var/run/lwm2m.sock"   //filename must be absolute path, but file not already exist
#define FIRMWARE_UPDATE_SOCK      "/var/run/firmwareUpdate.sock"
#define OBD_REPORT_SOCK           "/var/run/obdreport.sock"


extern int g_fdIpc;
extern void send_Dgram(const int ipcfd, const char* peerSock, const char* data);


#endif /* EXAMPLES_CLIENT_UNIXIPC_H_ */
