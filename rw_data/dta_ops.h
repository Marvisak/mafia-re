#pragma once

#include "main.h"

void DeCrypt(uchar* buffer, uint len, uint key1, uint key2);
int Decompression(uchar* buffer, uchar* dest, uint len);
int WavDecompression(uchar* buffer, uchar* dest, uint len, S_WAVHEADER* wav_header);