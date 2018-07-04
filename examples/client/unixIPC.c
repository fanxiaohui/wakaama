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

#define UNIX_IPC_NAME  "/var/lwm2mReport_ipc"   //filename must be absolute path, but file not already exist
#define MAX_CONNECT_NUM  8     //allow max 8 process pass data to lwm2mclient,which report to lwm2mServer
#define MAX_PACKET_SIZE 1024

int createUnixSocket()
{
    int fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if(fd < 0)
    {
        perror("create unix socket");
        exit(EXIT_FAILURE);
    }

    unlink(UNIX_IPC_NAME);

    struct sockaddr_un server;
    server.sun_family = AF_UNIX;
    strcpy(server.sun_path, UNIX_IPC_NAME);

    if (bind(fd, (struct sockaddr *) &server, sizeof(server)))
    {
        perror("bind unix socket");
        exit(EXIT_FAILURE);
    }

    fprintf(stdout,"create unix Socket ok: %s \n", server.sun_path);
    fflush(stdout);//need explicit flush stdout,  otherwise miss the log

    return fd;
}


void processIpcData(int fd)
{
    static char buffer[MAX_PACKET_SIZE];
    memset(buffer, 0, sizeof(buffer));

    struct sockaddr_un client;
    socklen_t addrLen;
    int numBytes = recvfrom(fd, buffer, MAX_PACKET_SIZE, 0, (struct sockaddr *)&client, &addrLen);
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
}