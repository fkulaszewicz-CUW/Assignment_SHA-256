#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
using namespace std;

// Initialize hash values:
// first 32 bits of the fractional parts of the square roots of the first 8 primes 2..19
unsigned int h0 = 0x6a09e667, h1 = 0xbb67ae85, h2 = 0x3c6ef372, h3 = 0xa54ff53a, h4 = 0x510e527f, h5 = 0x9b05688c, h6 = 0x1f83d9ab, h7 = 0x5be0cd19;

// Initialize array of round constants:
// first 32 bits of the fractional parts of the cube roots of the first 64 primes 2..311
unsigned int k[64] = {0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
                      0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
                      0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
                      0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
                      0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
                      0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
                      0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
                      0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

////////////////////// Message schedule array helpers //////////////////////
unsigned int RightROT(unsigned int bits, int moveBitSpaces)
{
    return ((bits >> moveBitSpaces) | (bits << (32 - moveBitSpaces)));
}

unsigned int RightSHFT(unsigned int bits, int moveBitSpaces)
{
    return (bits >> moveBitSpaces);
}

unsigned int s0(unsigned int bits)
{
    return RightROT(bits, 7) ^ RightROT(bits, 18) ^ RightSHFT(bits, 3);
}

unsigned int s1(unsigned int bits)
{
    return RightROT(bits, 17) ^ RightROT(bits, 19) ^ RightSHFT(bits, 10);
}

unsigned int S0(unsigned int a)
{
    return RightROT(a, 2) ^ RightROT(a, 13) ^ RightROT(a, 22);
}

unsigned int S1(unsigned int e)
{
    return RightROT(e, 6) ^ RightROT(e, 11) ^ RightROT(e, 25);
}

unsigned int CH(unsigned int e, unsigned int f, unsigned int g)
{
    return (e & f) ^ ((~e) & g);
}

unsigned int MAJ(unsigned int a, unsigned int b, unsigned int c)
{
    return (a & b) ^ (a & c) ^ (b & c);
}
////////////////////// Message schedule array helpers //////////////////////

int main()
{
    //////////////////// INITIALIZE /////////////////
    // Read BookOfMarkFileToHash and store in message
    ifstream myfile("../BookOfMarkFileToHash.txt");
    stringstream fileText;
    fileText << myfile.rdbuf();
    string message = fileText.str();

    // Begin with the original message of length L bits:
    int strLength = message.length();
    int padLength = 64 - (strLength % 64);
    if (padLength < 9)
    {
        padLength += 64;
    }
    int messageLength = (strLength + padLength);
    //////////////////// INITIALIZE /////////////////

    //////////////////// PADDING ////////////////////
    // Resize message to a multiple of 512 bits:
    message.resize(messageLength);

    // Append a single '1' bit
    int oneBitStart = strLength;
    message[oneBitStart++] = 0x80;
    while (oneBitStart < messageLength)
    {
        // Append K '0' bits, where K is the minimum number >= 0
        // such that (L + 1 + K + 64) is a multiple of 512:
        message[oneBitStart++] = 0x00;
    }

    // Append L as a 64-bit big-endian integer, making the total post-processed length a multiple of 512 bits
    // such that the bits in the message are: <original message of length L> 1 <K zeros> <L as 64 bit integer>,
    // (the number of bits will be a multiple of 512):
    unsigned long bigEndianInt = strLength * 8;
    int bitShift = 56;
    for (int i = 8; i > 0; i--)
    {
        message[messageLength - i] = bigEndianInt >> bitShift;
        bitShift -= 8;
    }
    //////////////////// PADDING ////////////////////

    //////////////////// PROCESSING /////////////////
    // Break message into 512-bit chunks:
    int numOfChunks = messageLength / 64;

    // For each chunk:
    for (int i = 0, block = 0; i < numOfChunks; i++, block++)
    {
        // Create a 64-entry message schedule array w[0..63] of 32-bit words
        // (The initial values in w[0..63] don't matter, so many implementations zero them here):
        unsigned int w[64] = {0};

        // Copy chunk into first 16 words w[0..15] of the message schedule array:
        for (int i = 0, j = 0; i < 16; i++, j += 4)
        {
            w[i] = ((message[j + (block * 64)] & 0xff) << 24) | ((message[j + 1 + (block * 64)] & 0xff) << 16) 
                | ((message[j + 2 + (block * 64)] & 0xff) << 8) | ((message[j + 3 + (block * 64)] & 0xff));
        }

        // Extend the first 16 words into the remaining 48 words w[16..63] of the message schedule array:
        for (int i = 16; i < 64; i++)
        {
            w[i] = w[i - 16] + s0(w[i - 15]) + w[i - 7] + s1(w[i - 2]);
        }

        // Initialize working variables to current hash value:
        unsigned int a = h0, b = h1, c = h2, d = h3, e = h4, f = h5, g = h6, h = h7;

        // Compression function main loop:
        for (int i = 0; i < 64; i++)
        {
            unsigned int temp1 = h + S1(e) + CH(e, f, g) + k[i] + w[i];
            unsigned int temp2 = S0(a) + MAJ(a, b, c);
            h = g;
            g = f;
            f = e;
            e = d + temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 + temp2;
        }

        h0 = h0 + a;
        h1 = h1 + b;
        h2 = h2 + c;
        h3 = h3 + d;
        h4 = h4 + e;
        h5 = h5 + f;
        h6 = h6 + g;
        h7 = h7 + h;
    }

    // Produce the final hash value (big-endian)
    // ensuring leading zeros are preserved:
    stringstream hash;
    hash << setfill('0') << setw(8) << hex << h0;
    hash << setfill('0') << setw(8) << hex << h1;
    hash << setfill('0') << setw(8) << hex << h2;
    hash << setfill('0') << setw(8) << hex << h3;
    hash << setfill('0') << setw(8) << hex << h4;
    hash << setfill('0') << setw(8) << hex << h5;
    hash << setfill('0') << setw(8) << hex << h6;
    hash << setfill('0') << setw(8) << hex << h7;
    string finalHash = hash.str();
    cout << finalHash << endl;
    //////////////////// PROCESSING /////////////////
}