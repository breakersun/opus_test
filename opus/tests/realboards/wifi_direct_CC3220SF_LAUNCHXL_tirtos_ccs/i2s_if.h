#ifndef _I2S_IF_H_
#define _I2S_IF_H_

#include "stdbool.h"
#include "List.h"
#include "semaphore.h"

//sample rate:16KHz BUFSIZE:128;sample rate:48KHz BUFSIZE:384;
#define BUFSIZE         (128)     /* I2S buffer size */

/* Semaphore used to indicate that data must be processed */
extern sem_t semDataReadyForTreatment;

/* Lists containing transactions. Each transaction is in turn in these three lists */
extern List_List i2sReadList;
extern List_List treatmentList;
extern List_List i2sWriteList;
extern bool g_playBack;

extern void I2s_Init(void);

#endif

