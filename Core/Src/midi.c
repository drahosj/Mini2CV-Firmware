/*
 * midi.c
 *
 *  Created on: Dec 23, 2020
 *      Author: jake
 */

#include <string.h>
#include <stdio.h>

#include "main.h"
#include "dac.h"

#define MIDI_CH 0x00

#define NOTE_ON (0x90 | MIDI_CH)
#define NOTE_OFF (0x80 | MIDI_CH)

static void note_off(char, char);
static void note_on(char, char);

enum MIDI_STATE {
	STATE_IDLE,
	STATE_AWAIT_NOTE_NUM,
	STATE_AWAIT_VELOCITY,
};

static int state = 0;
static int note_num = 0;
static int status = 0;
static int note = -1;

/* State machine
 *
 *
 *                                                  |--------------- Perform Note On/ Note Off action <------------------|
 *                                                  v                                                                    |
 * --------                                ------------------                            ------------------              |
 * | IDLE | --- (NOTE_ON || NOTE_OFF) ---> | AWAIT_NOTE_NUM | --- (non-Status byte) ---> | AWAIT_VELOCITY | ---> (non-Status byte)
 * --------  |                             ------------------  |                         ------------------  |
 *    ^      |                                      ^          |                                             |
 *    |      |                                      |     (Status byte)                                 (Status byte)
 *    |-(Other byte)                                |          |                                             |
 *    |                                             |          |                                             |
 *    |                                   (NOTE_ON || NOTE_OFF)|                                             |
 *    |                                             |          |                                             |
 *    |--------------------- (Other Status byte) -------------------------------------------------------------
 */

void midi_recv(char c)
{
	switch (state) {

	/* Awaiting status byte */
	case STATE_IDLE:
		if ((c == NOTE_ON) || (c == NOTE_OFF)) {
			/* Received status byte of NOTE ON or NOTE OFF */
			status = c;
			state = STATE_AWAIT_NOTE_NUM;
		} else {
			/* Received any other byte */
			state = STATE_IDLE;
		}
		break;

	/* NOTE ON|OFF status byte received, waiting for note number */
	case STATE_AWAIT_NOTE_NUM:
		if (c & 0x80) {
			printf("Unexpected status %02X while awaiting note number!\n", c);
			/* Received new status byte before other message finished */
			if ((c == NOTE_ON) || (c == NOTE_OFF))  {
				/* Restart NOTE ON|OFF state */
				status = c;
				state = STATE_AWAIT_NOTE_NUM;
			} else {
				/* Reset to idle state */
				state = STATE_IDLE;
			}
		} else {
			/* Not a new status byte. Continue NOTE ON|OFF message */
			note_num = c;
			state = STATE_AWAIT_VELOCITY;
		}
		break;

	/* Awaiting final byte of NOTE ON|OFF */
	case STATE_AWAIT_VELOCITY:
		if (c & 0x80) {
			printf("Unexpected status %02X while awaiting velocity!\n", c);
			/* Received new status byte before other message finished */
			if ((c == NOTE_ON) || (c == NOTE_OFF))  {
				/* Restart NOTE ON|OFF state */
				status = c;
				state = STATE_AWAIT_NOTE_NUM;
			} else {
				/* Reset to idle state */
				state = STATE_IDLE;
			}
		} else {
			if ((c == 0x00) || (status == NOTE_OFF)) {
				note_off(note_num, c);
				state = STATE_AWAIT_NOTE_NUM;
			} else {
				note_on(note_num, c);
				state = STATE_AWAIT_NOTE_NUM;
			}
		}
		break;
	default:
		state = STATE_IDLE;
	}
}

static void note_off(char num, char vel)
{
	/* Only turn off if note_off is the last note_on */
	if (num == note) {
		HAL_GPIO_WritePin(GATE_GPIO_Port, GATE_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
	}
}

static void note_on(char num, char vel)
{
	note = num;
	HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_1, DAC_ALIGN_12B_R, (num - 36) * 51);
	HAL_DAC_Start(&hdac1, DAC_CHANNEL_1);
	HAL_GPIO_WritePin(GATE_GPIO_Port, GATE_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
}


