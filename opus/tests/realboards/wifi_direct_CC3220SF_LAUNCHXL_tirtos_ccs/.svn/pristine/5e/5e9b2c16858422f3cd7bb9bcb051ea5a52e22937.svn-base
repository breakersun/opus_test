
#ifndef _P2P_H_
#define _P2P_H_


#define P2P_GROUP_OWNER_ENABLE (1)
#define P2P_GROUP_CLIENT_ENABLE (2)

typedef struct UDPSocket
{
    int iSockDesc;              //Socket FD
    struct SlSockAddrIn_t Server;  //Address to Bind - INADDR_ANY for Local IP
    struct SlSockAddrIn_t Client;  //Client Address or Multicast Group
    int iServerLength;
    int iClientLength;
}tUDPSocket;

extern tUDPSocket g_udpSocket;
extern unsigned char g_p2pWorkMode;

extern void p2p_init(void);

#endif



