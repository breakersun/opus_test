/*
 * Copyright (c) 2018-2019, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ======== display.c ========
 */
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <string.h>
#include <time.h>

/* TI-Drivers Header files */
#include <ti/drivers/GPIO.h>

/* Display Header files */
#include <ti/display/Display.h>
#include <ti/display/DisplayUart.h>
#include <ti/display/DisplayExt.h>
#include <ti/display/AnsiColor.h>

/* Board Header files */
#include "Board.h"

/* Example GrLib image */
#include "splash_image.h"

#include "opus_multistream.h"
#include "opus.h"
#include "opus_private.h"
#include "test_opus_common.h"

#ifdef VALGRIND
#include <valgrind/memcheck.h>
#define VG_UNDEF(x,y) VALGRIND_MAKE_MEM_UNDEFINED((x),(y))
#define VG_CHECK(x,y) VALGRIND_CHECK_MEM_IS_DEFINED((x),(y))
#else
#define VG_UNDEF(x,y)
#define VG_CHECK(x,y)
#endif

Display_Handle hSerial;

opus_int32 *null_int_ptr = (opus_int32 *)NULL;
opus_uint32 *null_uint_ptr = (opus_uint32 *)NULL;

static const opus_int32 opus_rates[5] = {48000,24000,16000,12000,8000};

opus_int32 test_dec_api(void)
{
   opus_uint32 dec_final_range;
   OpusDecoder *dec;
   OpusDecoder *dec2;
   opus_int32 i,j,cfgs;
   unsigned char packet[1276];
#ifndef DISABLE_FLOAT_API
   float fbuf[960*2];
#endif
   short sbuf[960*2];
   int c,err;

   cfgs=0;
   /*First test invalid configurations which should fail*/
   Display_printf(hSerial, 0, 0,"\n  Decoder basic API tests\n");
   Display_printf(hSerial, 0, 0,"  ---------------------------------------------------\n");
   for(c=0;c<4;c++)
   {
      i=opus_decoder_get_size(c);
      if(((c==1||c==2)&&(i<=2048||i>1<<16))||((c!=1&&c!=2)&&i!=0))test_failed();
      Display_printf(hSerial, 0, 0,"    opus_decoder_get_size(%d)=%d ...............%s OK.\n",c,i,i>0?"":"....");
      cfgs++;
   }

   /*Test with unsupported sample rates*/
   for(c=0;c<4;c++)
   {
      for(i=-7;i<=96000;i++)
      {
         int fs;
         if((i==8000||i==12000||i==16000||i==24000||i==48000)&&(c==1||c==2))continue;
         switch(i)
         {
           case(-5):fs=-8000;break;
           case(-6):fs=INT32_MAX;break;
           case(-7):fs=INT32_MIN;break;
           default:fs=i;
         }
         err = OPUS_OK;
         VG_UNDEF(&err,sizeof(err));
         dec = opus_decoder_create(fs, c, &err);
         if(err!=OPUS_BAD_ARG || dec!=NULL)test_failed();
         cfgs++;
         dec = opus_decoder_create(fs, c, 0);
         if(dec!=NULL)test_failed();
         cfgs++;
         dec=malloc(opus_decoder_get_size(2));
         if(dec==NULL)test_failed();
         err = opus_decoder_init(dec,fs,c);
         if(err!=OPUS_BAD_ARG)test_failed();
         cfgs++;
         free(dec);
      }
   }

   VG_UNDEF(&err,sizeof(err));
   dec = opus_decoder_create(48000, 2, &err);
   if(err!=OPUS_OK || dec==NULL)test_failed();
   VG_CHECK(dec,opus_decoder_get_size(2));
   cfgs++;

   Display_printf(hSerial, 0, 0,"    opus_decoder_create() ........................ OK.\n");
   Display_printf(hSerial, 0, 0,"    opus_decoder_init() .......................... OK.\n");

   err=opus_decoder_ctl(dec, OPUS_GET_FINAL_RANGE(null_uint_ptr));
   if(err != OPUS_BAD_ARG)test_failed();
   VG_UNDEF(&dec_final_range,sizeof(dec_final_range));
   err=opus_decoder_ctl(dec, OPUS_GET_FINAL_RANGE(&dec_final_range));
   if(err!=OPUS_OK)test_failed();
   VG_CHECK(&dec_final_range,sizeof(dec_final_range));
   Display_printf(hSerial, 0, 0,"    OPUS_GET_FINAL_RANGE ......................... OK.\n");
   cfgs++;

   err=opus_decoder_ctl(dec,OPUS_UNIMPLEMENTED);
   if(err!=OPUS_UNIMPLEMENTED)test_failed();
   Display_printf(hSerial, 0, 0,"    OPUS_UNIMPLEMENTED ........................... OK.\n");
   cfgs++;

   err=opus_decoder_ctl(dec, OPUS_GET_BANDWIDTH(null_int_ptr));
   if(err != OPUS_BAD_ARG)test_failed();
   VG_UNDEF(&i,sizeof(i));
   err=opus_decoder_ctl(dec, OPUS_GET_BANDWIDTH(&i));
   if(err != OPUS_OK || i!=0)test_failed();
   Display_printf(hSerial, 0, 0,"    OPUS_GET_BANDWIDTH ........................... OK.\n");
   cfgs++;

   err=opus_decoder_ctl(dec, OPUS_GET_SAMPLE_RATE(null_int_ptr));
   if(err != OPUS_BAD_ARG)test_failed();
   VG_UNDEF(&i,sizeof(i));
   err=opus_decoder_ctl(dec, OPUS_GET_SAMPLE_RATE(&i));
   if(err != OPUS_OK || i!=48000)test_failed();
   Display_printf(hSerial, 0, 0,"    OPUS_GET_SAMPLE_RATE ......................... OK.\n");
   cfgs++;

   /*GET_PITCH has different execution paths depending on the previously decoded frame.*/
   err=opus_decoder_ctl(dec, OPUS_GET_PITCH(null_int_ptr));
   if(err!=OPUS_BAD_ARG)test_failed();
   cfgs++;
   VG_UNDEF(&i,sizeof(i));
   err=opus_decoder_ctl(dec, OPUS_GET_PITCH(&i));
   if(err != OPUS_OK || i>0 || i<-1)test_failed();
   cfgs++;
   VG_UNDEF(packet,sizeof(packet));
   packet[0]=63<<2;packet[1]=packet[2]=0;
   if(opus_decode(dec, packet, 3, sbuf, 960, 0)!=960)test_failed();
   cfgs++;
   Display_printf(hSerial, 0, 0,"    opus_decode ......................... OK.\n");
//   VG_UNDEF(&i,sizeof(i));
//   err=opus_decoder_ctl(dec, OPUS_GET_PITCH(&i));
//   if(err != OPUS_OK || i>0 || i<-1)test_failed();
//   cfgs++;
//   packet[0]=1;
//   if(opus_decode(dec, packet, 1, sbuf, 960, 0)!=960)test_failed();
//   cfgs++;
//   VG_UNDEF(&i,sizeof(i));
//   err=opus_decoder_ctl(dec, OPUS_GET_PITCH(&i));
//   if(err != OPUS_OK || i>0 || i<-1)test_failed();
//   cfgs++;
//   Display_printf(hSerial, 0, 0,"    OPUS_GET_PITCH ............................... OK.\n");
//
//   err=opus_decoder_ctl(dec, OPUS_GET_LAST_PACKET_DURATION(null_int_ptr));
//   if(err != OPUS_BAD_ARG)test_failed();
//   VG_UNDEF(&i,sizeof(i));
//   err=opus_decoder_ctl(dec, OPUS_GET_LAST_PACKET_DURATION(&i));
//   if(err != OPUS_OK || i!=960)test_failed();
//   cfgs++;
//   Display_printf(hSerial, 0, 0,"    OPUS_GET_LAST_PACKET_DURATION ................ OK.\n");
//
//   VG_UNDEF(&i,sizeof(i));
//   err=opus_decoder_ctl(dec, OPUS_GET_GAIN(&i));
//   VG_CHECK(&i,sizeof(i));
//   if(err != OPUS_OK || i!=0)test_failed();
//   cfgs++;
//   err=opus_decoder_ctl(dec, OPUS_GET_GAIN(null_int_ptr));
//   if(err != OPUS_BAD_ARG)test_failed();
//   cfgs++;
//   err=opus_decoder_ctl(dec, OPUS_SET_GAIN(-32769));
//   if(err != OPUS_BAD_ARG)test_failed();
//   cfgs++;
//   err=opus_decoder_ctl(dec, OPUS_SET_GAIN(32768));
//   if(err != OPUS_BAD_ARG)test_failed();
//   cfgs++;
//   err=opus_decoder_ctl(dec, OPUS_SET_GAIN(-15));
//   if(err != OPUS_OK)test_failed();
//   cfgs++;
//   VG_UNDEF(&i,sizeof(i));
//   err=opus_decoder_ctl(dec, OPUS_GET_GAIN(&i));
//   VG_CHECK(&i,sizeof(i));
//   if(err != OPUS_OK || i!=-15)test_failed();
//   cfgs++;
//   Display_printf(hSerial, 0, 0,"    OPUS_SET_GAIN ................................ OK.\n");
//   Display_printf(hSerial, 0, 0,"    OPUS_GET_GAIN ................................ OK.\n");
//
//   /*Reset the decoder*/
//   dec2=malloc(opus_decoder_get_size(2));
//   memcpy(dec2,dec,opus_decoder_get_size(2));
//   if(opus_decoder_ctl(dec, OPUS_RESET_STATE)!=OPUS_OK)test_failed();
//   if(memcmp(dec2,dec,opus_decoder_get_size(2))==0)test_failed();
//   free(dec2);
//   Display_printf(hSerial, 0, 0,"    OPUS_RESET_STATE ............................. OK.\n");
//   cfgs++;
//
//   VG_UNDEF(packet,sizeof(packet));
//   packet[0]=0;
//   if(opus_decoder_get_nb_samples(dec,packet,1)!=480)test_failed();
//   if(opus_packet_get_nb_samples(packet,1,48000)!=480)test_failed();
//   if(opus_packet_get_nb_samples(packet,1,96000)!=960)test_failed();
//   if(opus_packet_get_nb_samples(packet,1,32000)!=320)test_failed();
//   if(opus_packet_get_nb_samples(packet,1,8000)!=80)test_failed();
//   packet[0]=3;
//   if(opus_packet_get_nb_samples(packet,1,24000)!=OPUS_INVALID_PACKET)test_failed();
//   packet[0]=(63<<2)|3;
//   packet[1]=63;
//   if(opus_packet_get_nb_samples(packet,0,24000)!=OPUS_BAD_ARG)test_failed();
//   if(opus_packet_get_nb_samples(packet,2,48000)!=OPUS_INVALID_PACKET)test_failed();
//   if(opus_decoder_get_nb_samples(dec,packet,2)!=OPUS_INVALID_PACKET)test_failed();
//   Display_printf(hSerial, 0, 0,"    opus_{packet,decoder}_get_nb_samples() ....... OK.\n");
//   cfgs+=9;
//
//   if(OPUS_BAD_ARG!=opus_packet_get_nb_frames(packet,0))test_failed();
//   for(i=0;i<256;i++) {
//     int l1res[4]={1,2,2,OPUS_INVALID_PACKET};
//     packet[0]=i;
//     if(l1res[packet[0]&3]!=opus_packet_get_nb_frames(packet,1))test_failed();
//     cfgs++;
//     for(j=0;j<256;j++) {
//       packet[1]=j;
//       if(((packet[0]&3)!=3?l1res[packet[0]&3]:packet[1]&63)!=opus_packet_get_nb_frames(packet,2))test_failed();
//       cfgs++;
//     }
//   }
//   Display_printf(hSerial, 0, 0,"    opus_packet_get_nb_frames() .................. OK.\n");
//
//   for(i=0;i<256;i++) {
//     int bw;
//     packet[0]=i;
//     bw=packet[0]>>4;
//     bw=OPUS_BANDWIDTH_NARROWBAND+(((((bw&7)*9)&(63-(bw&8)))+2+12*((bw&8)!=0))>>4);
//     if(bw!=opus_packet_get_bandwidth(packet))test_failed();
//     cfgs++;
//   }
//   Display_printf(hSerial, 0, 0,"    opus_packet_get_bandwidth() .................. OK.\n");
//
//   for(i=0;i<256;i++) {
//     int fp3s,rate;
//     packet[0]=i;
//     fp3s=packet[0]>>3;
//     fp3s=((((3-(fp3s&3))*13&119)+9)>>2)*((fp3s>13)*(3-((fp3s&3)==3))+1)*25;
//     for(rate=0;rate<5;rate++) {
//       if((opus_rates[rate]*3/fp3s)!=opus_packet_get_samples_per_frame(packet,opus_rates[rate]))test_failed();
//       cfgs++;
//     }
//   }
//   Display_printf(hSerial, 0, 0,"    opus_packet_get_samples_per_frame() .......... OK.\n");
//
//   packet[0]=(63<<2)+3;
//   packet[1]=49;
//   for(j=2;j<51;j++)packet[j]=0;
//   VG_UNDEF(sbuf,sizeof(sbuf));
//   if(opus_decode(dec, packet, 51, sbuf, 960, 0)!=OPUS_INVALID_PACKET)test_failed();
//   cfgs++;
//   packet[0]=(63<<2);
//   packet[1]=packet[2]=0;
//   if(opus_decode(dec, packet, -1, sbuf, 960, 0)!=OPUS_BAD_ARG)test_failed();
//   cfgs++;
//   if(opus_decode(dec, packet, 3, sbuf, 60, 0)!=OPUS_BUFFER_TOO_SMALL)test_failed();
//   cfgs++;
//   if(opus_decode(dec, packet, 3, sbuf, 480, 0)!=OPUS_BUFFER_TOO_SMALL)test_failed();
//   cfgs++;
//   if(opus_decode(dec, packet, 3, sbuf, 960, 0)!=960)test_failed();
//   cfgs++;
//   Display_printf(hSerial, 0, 0,"    opus_decode() ................................ OK.\n");
//#ifndef DISABLE_FLOAT_API
//   VG_UNDEF(fbuf,sizeof(fbuf));
//   if(opus_decode_float(dec, packet, 3, fbuf, 960, 0)!=960)test_failed();
//   cfgs++;
//   Display_printf(hSerial, 0, 0,"    opus_decode_float() .......................... OK.\n");
//#endif
//
//#if 0
//   /*These tests are disabled because the library crashes with null states*/
//   if(opus_decoder_ctl(0,OPUS_RESET_STATE)         !=OPUS_INVALID_STATE)test_failed();
//   if(opus_decoder_init(0,48000,1)                 !=OPUS_INVALID_STATE)test_failed();
//   if(opus_decode(0,packet,1,outbuf,2880,0)        !=OPUS_INVALID_STATE)test_failed();
//   if(opus_decode_float(0,packet,1,0,2880,0)       !=OPUS_INVALID_STATE)test_failed();
//   if(opus_decoder_get_nb_samples(0,packet,1)      !=OPUS_INVALID_STATE)test_failed();
//   if(opus_packet_get_nb_frames(NULL,1)            !=OPUS_BAD_ARG)test_failed();
//   if(opus_packet_get_bandwidth(NULL)              !=OPUS_BAD_ARG)test_failed();
//   if(opus_packet_get_samples_per_frame(NULL,48000)!=OPUS_BAD_ARG)test_failed();
//#endif
//   opus_decoder_destroy(dec);
//   cfgs++;
//   Display_printf(hSerial, 0, 0,"                   All decoder interface tests passed\n");
//   Display_printf(hSerial, 0, 0,"                             (%6d API invocations)\n",cfgs);
//   return cfgs;
}

/*
 *  ======== mainThread ========
 */
void *mainThread(void *arg0)
{
    unsigned int ledPinValue;
    unsigned int loopCount = 0;
    char * oversion;

    GPIO_init();
    Display_init();

    GPIO_write(Board_GPIO_LED0, Board_GPIO_LED_ON);

    /* Initialize display and try to open both UART and LCD types of display. */
    Display_Params params;
    Display_Params_init(&params);
    params.lineClearMode = DISPLAY_CLEAR_BOTH;

    /* Open both an available LCD display and an UART display.
     * Whether the open call is successful depends on what is present in the
     * Display_config[] array of the board file.
     */
    Display_Handle hLcd = Display_open(Display_Type_LCD, &params);
    hSerial = Display_open(Display_Type_UART, &params);

    if (hSerial == NULL && hLcd == NULL) {
        /* Failed to open a display */
        while (1) {}
    }

    /* Check if the selected Display type was found and successfully opened */
    if (hSerial) {
        Display_printf(hSerial, 0, 0, "Hello Serial!");

        oversion = (char *)opus_get_version_string();
        Display_printf(hSerial, 0, 0, oversion);

        test_dec_api();
    }
    else
    {
        /* Print unavail message on LCD. Note it's not necessary to
         * check for NULL handles with Display APIs, so just assume hLcd works.
         */
        Display_printf(hLcd, 4, 0, "Serial display");
        Display_printf(hLcd, 5, 0, "not present");
        sleep(1);
    }

    /* Check if the selected Display type was found and successfully opened */
    if (hLcd) {
        Display_printf(hLcd, 5, 3, "Hello LCD!");

        /* Wait a while so text can be viewed. */
        sleep(3);

        /*
         * Use the GrLib extension to get the GraphicsLib context object of the
         * LCD, if it is supported by the display type.
         */
        Graphics_Context *context = DisplayExt_getGraphicsContext(hLcd);

        /* It's possible that no compatible display is available. */
        if (context) {
            /* Calculate X-Y coordinate to center the splash image */
            int splashX = (Graphics_getDisplayWidth(context) - Graphics_getImageWidth(&splashImage)) / 2;
            int splashY = (Graphics_getDisplayHeight(context) - Graphics_getImageHeight(&splashImage)) / 2;

            /* Draw splash */
            Graphics_drawImage(context, &splashImage, splashX, splashY);
            Graphics_flushBuffer(context);
        }
        else {
            /* Not all displays have a GraphicsLib back-end */
            Display_printf(hLcd, 0, 0, "Display driver");
            Display_printf(hLcd, 1, 0, "is not");
            Display_printf(hLcd, 2, 0, "GrLib capable!");
        }

        /* Wait for a bit, then clear */
        sleep(3);
        Display_clear(hLcd);
    }
    else
    {
        Display_printf(hSerial, 1, 0, "LCD display not present");
        sleep(1);
    }

    char *serialLedOn = "On";
    char *serialLedOff = "Off";

    /* If serial display can handle ANSI colors, use colored strings instead.
     *
     * You configure DisplayUart to use the ANSI variant by choosing what
     * function pointer list it should use. Ex:
     *
     * const Display_Config Display_config[] = {
     *   {
     *      .fxnTablePtr = &DisplayUartAnsi_fxnTable, // Alt: DisplayUartMin_
     *      ...
     */
    if (Display_getType(hSerial) & Display_Type_ANSI)
    {
        serialLedOn = ANSI_COLOR(FG_GREEN, ATTR_BOLD) "On" ANSI_COLOR(ATTR_RESET);
        serialLedOff = ANSI_COLOR(FG_RED, ATTR_UNDERLINE) "Off" ANSI_COLOR(ATTR_RESET);
    }

    /* Loop forever, alternating LED state and Display output. */
    while (1) {
        sleep(1);
        /* Toggle LED */
        GPIO_toggle(Board_GPIO_LED0);
    }
}
