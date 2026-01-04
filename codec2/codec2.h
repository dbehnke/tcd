/*---------------------------------------------------------------------------*\

  FILE........: codec2.h
  AUTHOR......: David Rowe (Modified for tcd integration)
  
  Codec 2 C++ Wrapper for Codec2-mod library.

\*---------------------------------------------------------------------------*/

#ifndef __CODEC2__
#define  __CODEC2__

extern "C" {
#include "codec2_mod.h"
}

#define CODEC2_MODE_3200 	0
#define CODEC2_MODE_1600 	2

class CCodec2
{
public:
	CCodec2(bool is_3200);
	~CCodec2();
	void codec2_encode(unsigned char *bits, const short *speech_in);
	void codec2_decode(short *speech_out, const unsigned char *bits);
	int  codec2_samples_per_frame();
	int  codec2_bits_per_frame();

private:
    codec2_t m_c2;
    bool m_is_3200;
};

#endif
