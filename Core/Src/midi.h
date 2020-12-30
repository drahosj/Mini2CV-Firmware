/*
 * midi.h
 *
 *  Created on: Dec 23, 2020
 *      Author: jake
 */

#ifndef SRC_MIDI_H_
#define SRC_MIDI_H_

void midibuf_append(char c);
void midi_parse(void);
void midi_recv(char c);

#endif /* SRC_MIDI_H_ */
