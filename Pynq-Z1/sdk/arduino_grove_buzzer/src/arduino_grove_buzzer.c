/******************************************************************************
 *  Copyright (c) 2016, Xilinx, Inc.
 *  All rights reserved.
 * 
 *  Redistribution and use in source and binary forms, with or without 
 *  modification, are permitted provided that the following conditions are met:
 *
 *  1.  Redistributions of source code must retain the above copyright notice, 
 *     this list of conditions and the following disclaimer.
 *
 *  2.  Redistributions in binary form must reproduce the above copyright 
 *      notice, this list of conditions and the following disclaimer in the 
 *      documentation and/or other materials provided with the distribution.
 *
 *  3.  Neither the name of the copyright holder nor the names of its 
 *      contributors may be used to endorse or promote products derived from 
 *      this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 *  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
 *  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR 
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 *  OR BUSINESS INTERRUPTION). HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
 *  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *****************************************************************************/
/******************************************************************************
 *
 *
 * @file arduino_grove_buzzer.c
 *
 * IOP code (MicroBlaze) for grove buzzer.
 * The grove buzzer has to be connected to an arduino interface 
 * via a shield socket.
 * Grove Buzzer is write only, and has simple one-bit GPIO interface.
 * Hardware version 1.2.
 * http://www.seeedstudio.com/wiki/Grove_-_Buzzer
 *
 * <pre>
 * MODIFICATION HISTORY:
 *
 * Ver   Who  Date     Changes
 * ----- --- ------- -----------------------------------------------
 * 1.00a pp  04/13/16 release
 * 1.00d yrq 07/26/16 separate pmod and arduino
 *
 * </pre>
 *
 *****************************************************************************/
 
#include "xparameters.h"
#include "arduino.h"
#include "xgpio.h"

/*
 * Passed parameters in MAILBOX_WRITE_CMD
 * bits 31:16 => period in us
 * bits 15:1 => number of times to generate
 * bit 0 => 0=tone generation, 1=melody demo
 */
#define SPEAKER_CHANNEL 1
// The driver instance for GPIO Devices
XGpio pb_speaker;

void generateTone(int PeriodInUS) {
    // turn-ON speaker
    XGpio_DiscreteWrite(&pb_speaker, SPEAKER_CHANNEL, 1);
    delay_us(PeriodInUS>>1);
    // turn-OFF speaker
    XGpio_DiscreteWrite(&pb_speaker, SPEAKER_CHANNEL, 0);
    delay_us(PeriodInUS>>1);
}

void playTone(int tone, int duration) { 
    // tone is in us delay
    long i;
    for (i = 0; i < duration * 1000L; i += tone * 2) {
        generateTone(tone*2);
    }
}

void playNote(char note, int duration) {

    char names[] = { 'c', 'd', 'e', 'f', 'g', 'a', 'b', 'C', 'D' };
    int tones[] = { 1915, 1700, 1519, 1432, 1275, 1136, 1014, 956, 1702 };
    int i;

    // play the tone corresponding to the note name
    for (i = 0; i < 8; i++) {
        if (names[i] == note) {
          playTone(tones[i], duration);
        }
    }
}

void melody_demo(void) {
    // The number of notes
    int length = 15;
    char notes[] = "ccggaagffeeddc ";
    int beats[] = { 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 4 };
    int tempo = 300;
    int i;

    for(i = 0; i < length; i++) {
        if(notes[i] == ' ') {
            delay_ms(beats[i] * tempo);
        } else {
            playNote(notes[i], beats[i] * tempo);
        }
        // Delay between notes
        delay_ms(tempo / 2);
    }
}

int main(void) {
    int i=0;
    int PeriodInUS = 0x55;
    int numberoftimes = 10;
    u32 cmd;

    arduino_init(0,0,0,0);
    /* 
     * Configuring IO Switch to connect GPIO
     * bit-0 will be controlled by the software to drive the speaker
     * Buzzer is connected to bit[0] of the Channel 1 of AXI GPIO instance
     */
    config_arduino_switch(A_GPIO, A_GPIO, A_GPIO, 
                          A_GPIO, A_GPIO, A_GPIO,
                          D_GPIO, D_GPIO, D_GPIO, D_GPIO, D_GPIO,
                          D_GPIO, D_GPIO, D_GPIO, D_GPIO,
                          D_GPIO, D_GPIO, D_GPIO, D_GPIO);
    XGpio_Initialize(&pb_speaker, XPAR_GPIO_0_DEVICE_ID);
    XGpio_SetDataDirection(&pb_speaker, SPEAKER_CHANNEL, 0x0);
    // initially keep it OFF
    XGpio_DiscreteWrite(&pb_speaker, 1, 0);

    while(1){
        while(MAILBOX_CMD_ADDR==0);
        cmd = MAILBOX_CMD_ADDR;
        if(cmd & 0x1)
            melody_demo();
        else {
            // period in us
            PeriodInUS = (cmd >> 16) & 0x0ffff;
            // number of times
            numberoftimes = (cmd >> 1) & 0x07fff;
            for(i=0; i<numberoftimes; i++)
                generateTone(PeriodInUS);
        }
        MAILBOX_CMD_ADDR = 0x0;
    }
    return 0;

}