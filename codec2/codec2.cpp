/*---------------------------------------------------------------------------*\

  FILE........: codec2.cpp
  AUTHOR......: David Rowe (Modified for tcd integration)
  
  Codec 2 C++ Wrapper for Codec2-mod library.

\*---------------------------------------------------------------------------*/

#include <iostream>
#include <cstring>
#include "codec2.h"

CCodec2::CCodec2(bool is_3200) : m_is_3200(is_3200)
{
    if (!m_is_3200) {
        std::cerr << "CRITICAL WARNING: Codec2 1600 mode (Half-Rate) is NOT supported by this Codec2-mod integration." << std::endl;
        std::cerr << "Only 3200 mode (M17) is available. 1600 encoding/decoding will fail." << std::endl;
    }
    
    // Initialize the C struct (embedded in class, so no malloc needed)
    // codec2_init expects a pointer to an already allocated struct (on stack or heap)
    // defined in codec2_internal.h and exposed via codec2_mod.h
    codec2_init(&m_c2);
}

CCodec2::~CCodec2()
{
   // codec2_mod does not seem to have a destroy/free function 
   // because it uses fixed size buffers and no dynamic allocation (if allocated on stack)
   // but verifying... original C code used mallocs but this mod seems to optimize for static/stack.
   // Correct. "codec2_init" resets state. No "codec2_destroy".
}

int CCodec2::codec2_bits_per_frame()
{
    return 64; // BYTES_PER_FRAME * 8
}

int CCodec2::codec2_samples_per_frame()
{
    // Codec2-mod is hardcoded for 3200 mode (160 samples / 20ms) in most contexts relative to M17
    // codec2_internal.h: SAMPLES_PER_FRAME (2 * N_SAMP) = 160
    return 160;
}

void CCodec2::codec2_encode(unsigned char *bits, const short *speech_in)
{
    if (!m_is_3200) return;
    
    // Codec2-mod API: codec2_encode(codec2_t *c2, uint8_t *bits, const int16_t *speech)
    ::codec2_encode(&m_c2, bits, speech_in);
}

void CCodec2::codec2_decode(short *speech_out, const unsigned char *bits)
{
    if (!m_is_3200) {
        std::memset(speech_out, 0, 320 * sizeof(short)); // Silence for 1600 mode attempts
        return;
    }

    // Codec2-mod API: codec2_decode(codec2_t *c2, int16_t *speech, const uint8_t *bits)
    ::codec2_decode(&m_c2, speech_out, bits);
}
