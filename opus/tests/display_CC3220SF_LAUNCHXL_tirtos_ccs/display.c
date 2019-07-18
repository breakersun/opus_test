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

#define PACKETSIZE 512
#define CHANNELS 2
#define FRAMESIZE 128

opus_int32 test_dec_api(void)
{
    OpusDecoder *decoder;
    int result;
    int error;

    unsigned char *in = malloc(PACKETSIZE);
    opus_int16 *out = malloc(FRAMESIZE*CHANNELS*sizeof(*out));

    Display_printf(hSerial, 0, 0,"\n  Decoder basic API tests\n");

    Display_printf(hSerial, 0, 0, "  Checking for padding overflow... ");
    if (!in || !out) {
        Display_printf(hSerial, 0, 0, "FAIL (out of memory)\n");
        return -1;
    }
    in[0] = 0xff;
    in[1] = 0x41;
    memset(in + 2, 0xff, PACKETSIZE - 3);
    in[PACKETSIZE-1] = 0x0b;

    decoder = opus_decoder_create(48000, CHANNELS, &error);
    result = opus_decode(decoder, in, PACKETSIZE, out, FRAMESIZE, 0);
    opus_decoder_destroy(decoder);

    free(in);
    free(out);
    Display_printf(hSerial, 0, 0,"\n  Decoder basic API tests end\n");
}


opus_int32 test_enc_api(void)
{
    opus_uint32 enc_final_range;
    OpusEncoder *enc;
    opus_int32 i,j;
    int c,err,cfgs;

    unsigned char *in = malloc(PACKETSIZE);
    opus_int16 *out = malloc(FRAMESIZE*CHANNELS*sizeof(*out));

    if (!in || !out) {
        Display_printf(hSerial, 0, 0, "FAIL (out of memory)\n");
        return -1;
    }

    in[0] = 0xff;
    in[1] = 0x41;
    memset(in + 2, 0xff, PACKETSIZE - 3);
    in[PACKETSIZE-1] = 0x0b;

    enc = opus_encoder_create(16000, 2, OPUS_APPLICATION_VOIP, NULL);
    opus_encode(enc, in, PACKETSIZE, out, FRAMESIZE*CHANNELS*sizeof(*out));
    opus_encoder_destroy(enc);
    free(in);
    free(out);

    Display_printf(hSerial, 0, 0,"    opus_encode() ................................ OK.\n");
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

        //test_dec_api();
        test_enc_api();
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
