//*****************************************************************************
// audio_in.c
//
//
//*****************************************************************************
#include "pthread.h"
#include "stdbool.h"
#include "simplelink.h"
#include "p2p.h"
#include "wlan.h"
#include "uart_term.h"
#include "netcfg.h"
#include "network_if.h"
#include "unistd.h"
#include "audio_send.h"
#include "audio_receive.h"
#include "sl_socket.h"
#include "stdbool.h"
#include "i2s_if.h"
#include "List.h"
#include <ti/drivers/I2S.h>

#include "opus_multistream.h"
#include "opus.h"
#include "opus_private.h"
#include "test_opus_common.h"

opus_int32 test_dec_api(void);
opus_int32 test_enc_api(void);



static unsigned char packet[1276];
static short sbuf[960 * 2];

static void* Audio_Send_Thread( void *pvParameters )
{
    long retc = -1;
    opus_int32 ret = 0;

//    test_dec_api();
//    test_enc_api();

    int err = OPUS_OK;
    OpusEncoder *enc;
    enc = opus_encoder_create(16000, 2, OPUS_APPLICATION_VOIP, &err);
    if (err != OPUS_OK || enc == NULL)
        UART_PRINT("opus_encoder_create fail\n");
    UART_PRINT("opus_encoder_create OK\n");

    /* note : fire I2S read/write process immediately after bring up I2S module */
    I2S_startRead(i2sHandle);
    I2S_startWrite(i2sHandle);

    I2S_Transaction* transactionToTreat;
    while(1)
    {
        /* Wait for transaction ready for treatment */
        retc = sem_wait(&semDataReadyForTreatment);
        if (retc == -1) {
            while (1);
        }

        transactionToTreat = (I2S_Transaction*) List_head(&treatmentList);

//        memcpy();
        ret = opus_encode(enc,
                          (opus_int16 *)(transactionToTreat->bufPtr),
                          transactionToTreat->bufSize / (2 * sizeof(opus_int16)),
                          packet,
                          sizeof(packet));
//        UART_PRINT("transactionToTreat->bufSize : %d\n",
//                   transactionToTreat->bufSize);

//        ret = opus_encode(enc, sbuf, 960, packet, sizeof(packet));

        if (ret < 1 || ret > (opus_int32)sizeof(packet)) {
            UART_PRINT("opus_encode fail 0x%02x\n", ret);
            while(1);
        }

        if(transactionToTreat != NULL)
        {
            /* Place in the write-list the transaction we just treated */
            List_remove(&treatmentList, (List_Elem*)transactionToTreat);
            if(true == g_playBack)
            {
                List_put(&i2sWriteList, (List_Elem*)transactionToTreat);
            }
            else
            {
                retc = sl_SendTo(g_udpSocket.iSockDesc,
                                  (char*)(transactionToTreat->bufSize),
                                  BUFSIZE,
                                  0,
                                  (struct SlSockAddr_t*)&(g_udpSocket.Client),
                                  sizeof(g_udpSocket.Client));
                List_put(&i2sReadList, (List_Elem*)transactionToTreat);
                if(retc < 0)
                {
                    Report("Unable to send data\n\r");
                }

            }
        }
    }
    pthread_exit(0);
    return NULL;
}

void Audio_Send_Init(void)
{
    pthread_t thread;
    pthread_attr_t pAttrs;
    struct sched_param priParam;
    int retc;
    int detachState;

    /* Set priority and stack size attributes */
    pthread_attr_init(&pAttrs);
    priParam.sched_priority = 8;

    detachState = PTHREAD_CREATE_DETACHED;
    retc = pthread_attr_setdetachstate(&pAttrs, detachState);
    if(retc != 0)
    {
       /* pthread_attr_setdetachstate() failed */
       Report("pthread_attr_setdetachstate() failed. \r\n");
       while(1);
    }

    pthread_attr_setschedparam(&pAttrs, &priParam);

    retc |= pthread_attr_setstacksize(&pAttrs, 1024 * 10);
    if(retc != 0)
    {
       /* pthread_attr_setstacksize() failed */
       Report("pthread_attr_setstacksize() failed. \r\n");
       while(1);
    }

    retc = pthread_create(&thread, &pAttrs, Audio_Send_Thread, NULL);
    if(retc != 0)
    {
       /* pthread_create() failed */
       Report("Audio_Send_Thread create failed. \r\n");
       while(1);
    }
}


