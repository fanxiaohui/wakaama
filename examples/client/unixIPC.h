/*
 * unixIPC.h
 *
 *  Created on: Aug 8, 2018
 *      Author: impact
 */

#ifndef EXAMPLES_CLIENT_UNIXIPC_H_
#define EXAMPLES_CLIENT_UNIXIPC_H_

#define LWM2M_SENSOR_REPORT_SOCK  "/var/run/lwm2m.sock"   //filename must be absolute path, other process send data to lwm2mclient via this sock
#define FIRMWARE_UPDATE_SOCK      "/var/run/firmwareUpdate.sock" //semd download and upgrade cmd to firmware process
#define OBD_REPORT_SOCK           "/var/run/obdreport.sock"  //obd report data to lwm2mclient via this sock
//#define OBD_DAEMON_SOCK          "/var/run/obddaemon.sock"   //
#define OBD_BLUE_ADDR             "/appstore/lwm2m/obdAddr.txt"


extern int g_fdIpc;
extern void send_Dgram(const int ipcfd, const char* peerSock, const char* data);


#endif /* EXAMPLES_CLIENT_UNIXIPC_H_ */
