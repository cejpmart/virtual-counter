/*
 * protocol_scpi.h
 *
 *  Created on: Mar 1, 2017
 *      Author: Martin Cejp
 */

#ifndef VIRTUALINSTRUMENT_PROTOCOL_SCPI_H_
#define VIRTUALINSTRUMENT_PROTOCOL_SCPI_H_

void protocolScpiInit();
void protocolScpiHandle(const uint8_t* data, size_t length);

#endif /* VIRTUALINSTRUMENT_PROTOCOL_SCPI_H_ */
