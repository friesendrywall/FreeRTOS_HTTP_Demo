#ifndef RAND_H
#define RAND_H

BaseType_t xApplicationGetRandomNumber(uint32_t *pulNumber);
uint32_t ulApplicationGetNextSequenceNumber(uint32_t ulSourceAddress,
    uint16_t usSourcePort,
    uint32_t ulDestinationAddress,
    uint16_t usDestinationPort);

#endif