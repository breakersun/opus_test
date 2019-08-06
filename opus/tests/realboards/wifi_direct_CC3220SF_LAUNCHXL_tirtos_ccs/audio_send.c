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

static void* Audio_Send_Thread( void *pvParameters )
{
    I2s_Init();
    long retc = -1;

//    test_dec_api();
//    test_enc_api();

//    int err = OPUS_OK;
//    OpusEncoder *enc;
//    enc = opus_encoder_create(48000, 2, OPUS_APPLICATION_VOIP, &err);
//    if (err != OPUS_OK || enc == NULL)
//    {
//        UART_PRINT("opus_encoder_create fail\n");
//    }
//    UART_PRINT("opus_encoder_create success\n");

    I2S_Transaction* transactionToTreat;
    while(1)
    {
        /* Wait for transaction ready for treatment */
        retc = sem_wait(&semDataReadyForTreatment);
        if (retc == -1) {
            while (1);
        }

        transactionToTreat = (I2S_Transaction*) List_head(&treatmentList);

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


