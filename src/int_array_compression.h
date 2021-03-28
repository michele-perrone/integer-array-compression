#include <inttypes.h>

#define N_OF_BITS_INT16 16
#define N_BITS_INT8 8

// Helper functions
void bitBuffer_reset(uint8_t buffer[], uint8_t size);
uint8_t calculate_nBits(uint8_t value);
void bitBuffer_set_nBit(uint8_t buffer[], uint32_t nBit);
void bitBuffer_reset_nBit(uint8_t buffer[], uint32_t nBit);
uint8_t bitBuffer_read_nBit(uint8_t buffer[], uint32_t nBit);
void bitBuffer_writeBits(uint8_t buffer[], uint32_t buffer_bitCtr, uint8_t value, uint8_t value_nBits);
uint8_t bitBuffer_readBits(uint8_t buffer[], uint32_t buffer_bitCtr, uint8_t value_nBits);

// The actual algorithm
void encodeReadings(int8_t readings[], uint8_t readings_size, uint8_t bitBuffer[], uint32_t* bitBuffer_bitCtr);
void decodeReadings(int8_t readings[], uint8_t* readings_ctr, uint8_t bitBuffer[], uint32_t bitBuffer_size);
