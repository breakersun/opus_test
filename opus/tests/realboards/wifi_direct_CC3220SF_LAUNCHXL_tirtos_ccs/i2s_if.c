
#include <stdint.h>
#include <stddef.h>
#include <stdint.h>

/* POSIX Header files */
#include <pthread.h>
#include <semaphore.h>

/* Driver Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/I2S.h>

/* Example/Board Header files */
#include "Board.h"
#include "AudioCodec.h"
#include "stdbool.h"
#include "i2s_if.h"
#include "simplelink.h"
#include "p2p.h"

#define THREADSTACKSIZE   2048

/* The higher the sampling frequency, the less time we have to process the data, but the higher the sound quality. */
#define SAMPLE_RATE                     16000//44100   /* Supported values: 8kHz, 16kHz, 32kHz and 44.1kHz */
#define INPUT_OPTION                    AudioCodec_MIC_LINE_IN
#define OUTPUT_OPTION                   AudioCodec_SPEAKER_HP

/* The more storage space we have, the more delay we have, but the more time we have to process the data. */
#define NUMBUFS         32      /* Total number of buffers to loop through */

/* Semaphore used to indicate that data must be processed */
sem_t semDataReadyForTreatment;

/* Lists containing transactions. Each transaction is in turn in these three lists */
List_List i2sReadList;
List_List treatmentList;
List_List i2sWriteList;

/* Buffers containing the data: written by read-interface, modified by treatment, and read by write-interface */
static uint8_t buf[NUMBUFS][BUFSIZE];
static uint8_t* i2sBufList[NUMBUFS];

/* Transactions will successively be part of the i2sReadList, the treatmentList and the i2sWriteList */
I2S_Transaction i2sTransaction[NUMBUFS];
I2S_Transaction *i2sTransactionList[NUMBUFS];

I2S_Handle i2sHandle;
bool g_playBack = true;

static void errCallbackFxn(I2S_Handle handle, int_fast16_t status, I2S_Transaction *transactionPtr) {
    /*
     * The content of this callback is executed if an I2S error occurs
     */
    I2S_stopClocks(handle);
    I2S_close(handle);
}

static void writeCallbackFxn(I2S_Handle handle, int_fast16_t status, I2S_Transaction *transactionPtr) {
    /*
     * The content of this callback is executed every time a write-transaction is started
     */

    /* We must consider the previous transaction (the current one is not over)  */
    I2S_Transaction *transactionFinished = (I2S_Transaction*)List_prev(&transactionPtr->queueElement);

    if(transactionFinished != NULL){
        /* Remove the finished transaction from the write queue and
         * feed the read queue (we do not need anymore the data of this transaction)*/
        List_remove(&i2sWriteList, (List_Elem*)transactionFinished);
        if(true == g_playBack)
        {
            List_put(&i2sReadList, (List_Elem*)transactionFinished);
        }else{
            List_put(&treatmentList, (List_Elem*)transactionFinished);
        }

        /* We do not need to queue transaction here: treatment-function takes care of this :) */
    }
}

static void readCallbackFxn(I2S_Handle handle, int_fast16_t status, I2S_Transaction *transactionPtr) {
    /*
     * The content of this callback is executed every time a read-transaction is started
     */

    /* We must consider the previous transaction (the current one is not over) */
    I2S_Transaction *transactionFinished = (I2S_Transaction*)List_prev(&transactionPtr->queueElement);

    if(transactionFinished != NULL){

        /* The finished transaction contains data that must be treated */
        List_remove(&i2sReadList, (List_Elem*)transactionFinished);
        List_put(&treatmentList, (List_Elem*)transactionFinished);

        /* Start the treatment of the data */
        sem_post(&semDataReadyForTreatment);

        /* We do not need to queue transaction here: writeCallbackFxn takes care of this :) */
    }
}

void I2s_Init(void)
{
    /*
     * Initialize TLV320AIC3254 Codec on Audio BP
     */
    uint8_t status;

    I2S_init();
    status = AudioCodec_open();
    if( AudioCodec_STATUS_SUCCESS != status)
    {
        /* Error Initializing codec */
        while(1);
    }

    /* Configure Codec */
    status =  AudioCodec_config(AudioCodec_TI_3254, AudioCodec_16_BIT,
                                SAMPLE_RATE, AudioCodec_STEREO, AudioCodec_SPEAKER_HP,
                                AudioCodec_MIC_LINE_IN);
    if( AudioCodec_STATUS_SUCCESS != status)
    {
        /* Error Initializing codec */
        while(1);
    }

    /* Volume control */
    AudioCodec_speakerVolCtrl(AudioCodec_TI_3254, AudioCodec_SPEAKER_HP, 75);
    AudioCodec_micVolCtrl(AudioCodec_TI_3254, AudioCodec_MIC_ONBOARD, 75);

    /*
     * Prepare the semaphore
     */
    int retc = sem_init(&semDataReadyForTreatment, 0, 0);
    if (retc == -1) {
        while (1);
    }

    /*
     *  Open the I2S driver
     */
    I2S_Params i2sParams;
    I2S_Params_init(&i2sParams);
    i2sParams.samplingFrequency     = SAMPLE_RATE;                       /* Sampling Freq */
    i2sParams.memorySlotLength      = I2S_MEMORY_LENGTH_16BITS;          /* Memory slot length */
    i2sParams.moduleRole            = I2S_MASTER;                        /* Master / Slave selection */
    i2sParams.trueI2sFormat         = (bool)true;                        /* Activate true I2S format */
    i2sParams.invertWS              = (bool)true;                        /* WS inversion */
    i2sParams.isMSBFirst            = (bool)true;                        /* Endianness selection */
    i2sParams.isDMAUnused           = (bool)false;                       /* Selection between DMA and CPU transmissions */
    i2sParams.samplingEdge          = I2S_SAMPLING_EDGE_RISING;          /* Sampling edge */
    i2sParams.beforeWordPadding     = 0;                                 /* Before sample padding */
    i2sParams.bitsPerWord           = 16;                                /* Bits/Sample */
    i2sParams.afterWordPadding      = 0;                                 /* After sample padding */
    i2sParams.fixedBufferLength     = BUFSIZE;                           /* Fixed Buffer Length */
    i2sParams.SD0Use                = I2S_SD0_OUTPUT;                    /* SD0Use */
    i2sParams.SD1Use                = I2S_SD1_INPUT;                     /* SD1Use */
    i2sParams.SD0Channels           = I2S_CHANNELS_STEREO;               /* Channels activated on SD0 */
    i2sParams.SD1Channels           = I2S_CHANNELS_STEREO;               /* Channels activated on SD1 */
    i2sParams.phaseType             = I2S_PHASE_TYPE_DUAL;               /* Phase type */
    i2sParams.startUpDelay          = 0;                                 /* Start up delay */
    i2sParams.MCLKDivider           = 40;                                /* MCLK divider */
    i2sParams.readCallback          = readCallbackFxn;                   /* Read callback */
    i2sParams.writeCallback         = writeCallbackFxn;                  /* Write callback */
    i2sParams.errorCallback         = errCallbackFxn;                    /* Error callback */
    i2sParams.custom                = NULL;                              /* customParams */

    i2sHandle = I2S_open(Board_I2S0, &i2sParams);
    if (i2sHandle == NULL) {
        /* Error Opening the I2S driver */
        while(1);
    }

    /*
     * Initialize the queues and the I2S transactions
     */
    List_clearList(&i2sReadList);
    List_clearList(&treatmentList);
    List_clearList(&i2sWriteList);

    uint8_t k;
    /* Half the transactions are initially stored in the read queue */
    for(k = 0; k < NUMBUFS/2; k++) {
        i2sBufList[k] = &buf[k][0];
        i2sTransactionList[k] = &i2sTransaction[k];
        I2S_Transaction_init(i2sTransactionList[k]);
        i2sTransactionList[k]->bufPtr  = i2sBufList[k];
        i2sTransactionList[k]->bufSize = BUFSIZE;
        List_put(&i2sReadList, (List_Elem*)i2sTransactionList[k]);
    }

    /* The second half of the transactions is stored in the write queue */
    for(k = NUMBUFS/2; k < NUMBUFS; k++) {
        i2sBufList[k] = &buf[k][0];
        i2sTransactionList[k] = &i2sTransaction[k];
        I2S_Transaction_init(i2sTransactionList[k]);
        i2sTransactionList[k]->bufPtr  = i2sBufList[k];
        i2sTransactionList[k]->bufSize = BUFSIZE;
        List_put(&i2sWriteList, (List_Elem*)i2sTransactionList[k]);
    }

    I2S_setReadQueueHead(i2sHandle,  (I2S_Transaction*) List_head(&i2sReadList));
    I2S_setWriteQueueHead(i2sHandle, (I2S_Transaction*) List_head(&i2sWriteList));

    /*
     * Start I2S streaming
     */
    I2S_startClocks(i2sHandle);
//    I2S_startRead(i2sHandle);
//    I2S_startWrite(i2sHandle);
}

