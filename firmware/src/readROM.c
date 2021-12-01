#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// example shim file, could be QSPI etc
uint32_t readFileData(uint8_t* address, uint8_t* data, uint32_t length) {
    // In this particular case, we know that the address is a ram address.
    memcpy(data, address, length);
    return length;
}
