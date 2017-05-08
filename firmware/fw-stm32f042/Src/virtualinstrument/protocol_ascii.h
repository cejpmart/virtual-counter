/*
 * protocol_ascii.h
 *
 *  Created on: Mar 3, 2017
 *      Author: minexew
 */

#ifndef VIRTUALINSTRUMENT_PROTOCOL_ASCII_H_
#define VIRTUALINSTRUMENT_PROTOCOL_ASCII_H_

void protocolAsciiInit();
void protocolAsciiHandle(const uint8_t* data, size_t length);

#endif /* VIRTUALINSTRUMENT_PROTOCOL_ASCII_H_ */
