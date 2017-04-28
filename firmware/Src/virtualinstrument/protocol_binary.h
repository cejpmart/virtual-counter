/*
 * protocol_binary.h
 *
 *  Created on: Apr 18, 2017
 *      Author: minexew
 */

#ifndef VIRTUALINSTRUMENT_PROTOCOL_BINARY_H_
#define VIRTUALINSTRUMENT_PROTOCOL_BINARY_H_

void protocolBinaryInit(const char* device_version);
void protocolBinaryHandle(const uint8_t* data, size_t length);

#endif /* VIRTUALINSTRUMENT_PROTOCOL_BINARY_H_ */
