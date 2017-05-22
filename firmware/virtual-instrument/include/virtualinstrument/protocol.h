/*
 * protocol.h
 *
 *  Created on: Oct 10, 2016
 *      Author: Martin Cejp
 */

#ifndef VIRTUALINSTRUMENT_PROTOCOL_H_
#define VIRTUALINSTRUMENT_PROTOCOL_H_

#include <stddef.h>
#include <stdint.h>

void protocolInit(uint16_t board_id, uint16_t instrument_version, uint32_t f_cpu);
void protocolDataIn(const uint8_t* data, size_t length);
void protocolProcess(void);

#endif /* VIRTUALINSTRUMENT_PROTOCOL_H_ */
