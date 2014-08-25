#ifndef BRR_H
#define BRR_H

#define SKIP_SAFE_MALLOC
#include "common.h"
//#include <stdbool.h>

// Global variables for prediction of filter
extern pcm_t p1, p2;
// Buffer for a single BRR data block (9 bytes)
extern u8 BRR[9];

void print_note_info(const unsigned int loopsize, const unsigned int samplerate);

void print_loop_info(unsigned int loopcount, pcm_t oldp1[], pcm_t oldp2[]);

void generate_wave_file(FILE *outwav, unsigned int samplerate, pcm_t *buffer, size_t k);

int get_brr_prediction(u8 filter, pcm_t p1, pcm_t p2);

void decodeBRR(pcm_t *out);

void apply_gauss_filter(pcm_t *buffer, size_t length);
#undef SKIP_SAFE_MALLOC
#endif