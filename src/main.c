#include "int_array_compression.h"

int main()
{
    /* USAGE DEMO */

    // Original readings
    int8_t readings[11] = {5, 6, 6, 6, 10, 12, 12, 10, 10, 10, -36};
    uint8_t readings_nBytes = 11;
    // Decoded readings
    int8_t readings_decoded[11];
    uint8_t readings_decoded_nBytes = 0;
    // Bit buffer, where the compressed encodings are stored.
    // In our case it's an Iridium packet comprised of 59 bytes.
    uint8_t bitBuffer[59];
    uint8_t bitBuffer_nBytes = 59;
    uint32_t bitBuffer_nBits = 0;

    // Take the data from the readings and encode it into the bit buffer
    bitBuffer_reset(bitBuffer, bitBuffer_nBytes); // Reset now, enjoy later
    encodeReadings(readings, readings_nBytes, bitBuffer, &bitBuffer_nBits);

    // Take the bits from the bit buffer and decode the original readings
    decodeReadings(readings_decoded, &readings_decoded_nBytes, bitBuffer, bitBuffer_nBits);

    return 0;
}
