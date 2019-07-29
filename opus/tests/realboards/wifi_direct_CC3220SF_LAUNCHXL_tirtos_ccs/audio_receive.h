#ifndef _AUDIO_RECEIVE_H_
#define _AUDIO_RECEIVE_H_

extern void Audio_Receive_Init(void);
extern unsigned int checksum(const unsigned char *data, unsigned int len);

#endif
