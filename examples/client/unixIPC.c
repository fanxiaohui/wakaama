//
// zengliang: this process is used to receive data from other sensor-process and report to lwm2m server,
// if this process crashed, and restart, can still receive data from other process.
//

#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "sensorData.h"

#define LWM2M_SENSOR_REPORT_SOCK  "/var/run/lwm2m.sock"   //filename must be absolute path, but file not already exist
#define FIRMWARE_UPDATE_SOCK  "/var/run/firmwareUpdate.sock"
#define MAX_CONNECT_NUM  8     //allow max 8 process pass data to lwm2mclient,which report to lwm2mServer
#define MAX_PACKET_SIZE 1024


extern int convertJsonToSensorData(const char* jsonData, InstanceData* sensorData);
extern void destroySensorData(InstanceData* sensorData);
extern void saveSensorDataToLocal(const InstanceData *sensorData);

int createUnixSocket()
{
    int fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if(fd < 0)
    {
        perror("create unix socket");
        exit(EXIT_FAILURE);
    }

    unlink(LWM2M_SENSOR_REPORT_SOCK);

    struct sockaddr_un server;
    server.sun_family = AF_UNIX;
    strcpy(server.sun_path, LWM2M_SENSOR_REPORT_SOCK);

    if (bind(fd, (struct sockaddr *) &server, sizeof(server)))
    {
        perror("bind unix socket");
        exit(EXIT_FAILURE);
    }

    fprintf(stdout,"create unix Socket ok: %s \n", server.sun_path);
    fflush(stdout);//need explicit flush stdout,  otherwise miss the log

    return fd;
}


char* receiveIpcData(int fd, struct sockaddr_un* peer)
{
    static char buffer[MAX_PACKET_SIZE];
    memset(buffer, 0, sizeof(buffer));

    //struct sockaddr_un peer;
    socklen_t addrLen = sizeof(struct sockaddr_un);//must initilize addrLen,otherwise will cause recvfrom error occasionally
    int numBytes = recvfrom(fd, buffer, MAX_PACKET_SIZE, 0, (struct sockaddr *)peer, &addrLen);
    if(numBytes > 0)
    {
        fprintf(stdout, "recvfrom %d bytes: %s \n", numBytes, buffer);
        fflush(stdout);
    }
    else
    {
        fprintf(stdout, "recvfrom error=%d :%s  \n", errno, strerror(errno));
        fflush(stdout);
    }

    return (numBytes > 0) ? buffer : NULL;
}


void send_Dgram_FirmwareUpdate(const int ipcfd, const char* buffer)
{
    struct sockaddr_un peer;
    memset(&peer, 0, sizeof(peer));
    peer.sun_family = AF_UNIX;
    strcpy(peer.sun_path, FIRMWARE_UPDATE_SOCK);

    //even if peer not run, this program still ok.
    int numBytes = sendto(ipcfd, buffer, strlen(buffer), 0, (struct sockaddr *)&peer, sizeof(struct sockaddr_un));

    if(numBytes != strlen(buffer))
    {
        fprintf(stdout, "send_Dgram_FirmwareUpdate error=%d : %s \n",errno,  strerror(errno));
    }
    else {
        fprintf(stdout, "send_Dgram_FirmwareUpdate ok.\n");
    }
    fflush(stdout);
}


int fromFirmwareProcess(const struct sockaddr_un* peer)
{
    return (strcmp(peer->sun_path,FIRMWARE_UPDATE_SOCK) == 0);
}