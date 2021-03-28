#include "int_array_compression.h"

// Reads from the bit buffer, writes out the readings and the number of retrieved readings
void decodeReadings(int8_t readings[], uint8_t* readings_ctr, uint8_t bitBuffer[], uint32_t bitBuffer_size)
{
    bitBuffer_reset(readings, 8);

    *readings_ctr = 0;
    uint32_t bitBuffer_ctr = 0;
    uint8_t codeMask; // Current code mask retrieved from the bit buffer
    uint8_t diffRange_nBits; // Number of bits which represent the difference range
    int8_t value; // Current value retrieved from the bit buffer

    // Read the first value
    readings[0] = bitBuffer_readBits(bitBuffer, bitBuffer_ctr, 8);
    bitBuffer_ctr += 8;
    *readings_ctr += 1;

    while(bitBuffer_ctr < bitBuffer_size)
    {
        // Read the next code mask
        codeMask = bitBuffer_readBits(bitBuffer, bitBuffer_ctr, 2);
        bitBuffer_ctr += 2;

        // Difference
        if(codeMask == 0)
        {
            // Read the difference code mask
            // 00: nBits = 2, 01: nBits = 3, 10: nBits = 4, 11: nBits = 5
            diffRange_nBits = bitBuffer_readBits(bitBuffer, bitBuffer_ctr, 2) + 2;
            bitBuffer_ctr += 2;

            // Read the difference
            int8_t difference = bitBuffer_readBits(bitBuffer, bitBuffer_ctr, diffRange_nBits);
            bitBuffer_ctr += diffRange_nBits;

            // Extrapolate the difference
            // Check whether it's a negative difference:
            // Negative differences start with a 0, positive with a 1.
            if( ( (difference >> (diffRange_nBits-1) ) & 1) == 0 )
            {
                difference = ~difference;

                // Put nBits to 0 (from the MSB)
                difference <<= 8-diffRange_nBits;
                difference = ((uint8_t)difference) >> (8-diffRange_nBits); // If I don't cast, ones will be added from the MSB, not zeros
                difference--;

                // Change sign
                difference = (~difference) + 1;
            }
            else
            {
                difference--;
            }

            // Calculate the value
            readings[*readings_ctr] = readings[(*readings_ctr) - 1] + difference;
            *readings_ctr += 1;
        }
        // Repetitions
        else if(codeMask == 1)
        {
            uint8_t nRepetitions = bitBuffer_readBits(bitBuffer, bitBuffer_ctr, 3) + 1;
            bitBuffer_ctr += 3;
            value = readings[(*readings_ctr)-1];
            for(uint8_t rep_ctr = 0; rep_ctr < nRepetitions; rep_ctr++)
            {
                readings[*readings_ctr] = value;
                *readings_ctr += 1;
            }
        }
        // No compression
        else if(codeMask == 2)
        {
            readings[*readings_ctr] = bitBuffer_readBits(bitBuffer, bitBuffer_ctr, 8);
            bitBuffer_ctr += 8;
            *readings_ctr += 1;
        }
        // The end
        else if(codeMask == 3)
        {
            break;
        }
    }


    return;
}

// Reads from the readings, encodes the into the bit buffer and sets the n. of bits in the bit buffer
void encodeReadings(int8_t readings[], uint8_t readings_size, uint8_t bitBuffer[], uint32_t* bitBuffer_bitCtr)
{
    *bitBuffer_bitCtr = 0; // Number of bits written to the output buffer
    uint8_t value; // Current value being written to the output buffer
    uint8_t value_nBits; // Number of bits needed for value
    uint8_t codeMask; // Bit mask for the code
    // Write out the first value
    bitBuffer_writeBits(bitBuffer, *bitBuffer_bitCtr, readings[0], 8);
    *bitBuffer_bitCtr += 8;
    for(uint8_t readings_ctr = 1; readings_ctr < readings_size; readings_ctr++)
    {
        int8_t difference = readings[readings_ctr] - readings[readings_ctr-1];
        // Code 00: differences
        if(difference >= -30 && difference <= 30 && difference != 0)
        {
            // Set the code mask
            codeMask = 0;
            // Calculate the value and nBits
            if(difference < 0)
            {
                // Make the value positive
                value = (~difference) + 1;
            }
            else
            {
                value = difference;
            }
            value++;
            value_nBits = calculate_nBits(value);
            if(difference < 0)
            {
                value = ~value;
            }
            // Write out the difference code mask
            bitBuffer_writeBits(bitBuffer, *bitBuffer_bitCtr, codeMask, 2);
            *bitBuffer_bitCtr += 2;

            // Write out the difference range code mask
            // 00: nBits = 2, 01: nBits = 3, 10: nBits = 4, 11: nBits = 5
            codeMask = value_nBits - 2;
            bitBuffer_writeBits(bitBuffer, *bitBuffer_bitCtr, codeMask, 2);
            *bitBuffer_bitCtr += 2;

            // Write out the difference
            bitBuffer_writeBits(bitBuffer, *bitBuffer_bitCtr, value, value_nBits);
            *bitBuffer_bitCtr += value_nBits;
        }
        // Code 01: repetitions
        else if(difference == 0)
        {
            codeMask = 1;
            uint8_t repetitions = 1;
            while(readings[readings_ctr+1] - readings[readings_ctr] == 0
                  && readings_ctr < readings_size && repetitions <= 8)
            {
                // Let's get stuck here
                repetitions++;
                readings_ctr++;
            }

            // Write out the code mask and the value
            bitBuffer_writeBits(bitBuffer, *bitBuffer_bitCtr, codeMask, 2);
            *bitBuffer_bitCtr += 2;
            bitBuffer_writeBits(bitBuffer, *bitBuffer_bitCtr, repetitions-1, 3);
            *bitBuffer_bitCtr += 3;
        }
        // Code 10: no compression
        else if(difference <= -30 || difference >= 30)
        {
            // Write out the code mask
            codeMask = 2;
            bitBuffer_writeBits(bitBuffer, *bitBuffer_bitCtr, codeMask, 2);
            *bitBuffer_bitCtr += 2;

            // The number is stored in two's complement
            bitBuffer_writeBits(bitBuffer, *bitBuffer_bitCtr, readings[readings_ctr], 8);
            *bitBuffer_bitCtr += 8;
        }
    }
    // Code 11: the end, my friend
    codeMask = 3;
    bitBuffer_writeBits(bitBuffer, *bitBuffer_bitCtr, codeMask, 2);
    *bitBuffer_bitCtr += 2;

    return;
}

uint8_t calculate_nBits(uint8_t value)
{
    for (uint8_t shift_index = N_BITS_INT8-1; shift_index >= 0; shift_index--)
    {
        if(((value >> shift_index) & 1) == 1)
        {
            // Reason for +1: shift_index starts from 15, not 16.
            return (shift_index + 1);
        }
    }
    return N_BITS_INT8;
}

void bitBuffer_set_nBit(uint8_t buffer[], uint32_t nBit)
{
    // Count nBit from the MSB
    buffer[nBit/8] |= ( 1 << (7-(nBit%8)) );
    return;
}
void bitBuffer_reset_nBit(uint8_t buffer[], uint32_t nBit)
{
    buffer[nBit/8] &= ~( 1 << (7-(nBit%8)) );
    return;
}
uint8_t bitBuffer_read_nBit(uint8_t buffer[], uint32_t nBit)
{
    return ( (buffer[nBit/8] >> (7-(nBit%8))) & 1 ) != 0;
}

void bitBuffer_writeBits(uint8_t buffer[], uint32_t buffer_bitCtr, uint8_t value, uint8_t value_nBits)
{
    // If the value is 1 bit, don't shift      Until I reach the LSB
    for(int16_t value_bitShift = value_nBits-1; value_bitShift >= 0; value_bitShift--)
    {
        uint8_t current_bit = (value >> value_bitShift) & 1;
        if(current_bit == 1)
        {
            bitBuffer_set_nBit(buffer, buffer_bitCtr);
        }
        /*
        // Needed only if the buffer isn't all set to 0-values
        else
        {
            bitBuffer_reset_nBit(buffer, buffer_bitCtr);
        }
        */
        buffer_bitCtr++;
    }
    return;
}

uint8_t bitBuffer_readBits(uint8_t buffer[], uint32_t buffer_bitCtr, uint8_t value_nBits)
{
    uint8_t value = 0;
    for(int8_t shift_idx = value_nBits-1; shift_idx >= 0; shift_idx--)
    {
        uint8_t current_bit = bitBuffer_read_nBit(buffer, buffer_bitCtr);
        value |= (bitBuffer_read_nBit(buffer, buffer_bitCtr) << shift_idx);
        buffer_bitCtr++;
    }

    return value;
}

void bitBuffer_reset(uint8_t buffer[], uint8_t size)
{
    for(uint8_t ctr = 0; ctr < size; ctr++)
    {
        buffer[ctr] = 0;
    }
    return;
}
