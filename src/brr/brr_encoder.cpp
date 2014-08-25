#include <stdio.h>
#include <stdlib.h>
//#include <unistd.h>
//#include <getopt.h>
//#include <stdbool.h>
#include <string.h>
#include <math.h>
#include "common.h"
#include "brr.h"

#include <limits>
#define INFINITY (std::numeric_limits<int>::max())
 /*
static void print_instructions()
{
	printf(
		"\n*** BRR Tools 3.0 ***\n\n"
		"BRR encoder (c) 2013 Bregalad special tanks to Kode54\n"
		"Usage : brr_encoder [options] infile.wav outfile.brr\n"
		"Options :\n"
		"-a[ampl] adjust wave amplitude by a factor ampl (default : 1.0)\n"
		"-l(pos) enable looping flag in the encoded BRR sample (default : disabled)\n"
		"   If a number follows the -l flag, this is the input's loop point in samples.\n"
		"   The output will be resampled in a way so the looped part of the sample is\n"
		"   an integer # of BRR blocks.\n"
		"-f[0123] manually enable filters for BRR blocks (default : all enabled)\n"
		"-r[type][ratio] resample input stream, followed by resample ratio (0.0 to 4.0)\n"
		"  (lower means more samples at output, better quality but increased size,\n"
		"  higher means less smaples, worse quality but decreased size).\n"
		"-s[type][rate] automatically resample to get the specified samplerate\n"
		"-t[N] truncate the input wave to the the first N samples (ignoring\n"
		"  any sound data that follows)\n"
		"-w disable wrapping (encoded sample will be compatible with old SPC players)\n"
		"-g enable treble boost to compensate the gaussian filtering of SNES hardware\n"
		"\nResampling interpolation types :\n"
		"n : nearest neighboor, l : linear, s : sine, c : cubic, b : bandlimited\n\n"
		"Examples : brr_encoder -l432 -a0.8 -f01 -sc32000 in_sample.wav out_sample.brr\n"
		"           brr_encoder -l -f23 -rb0.84 -t19 in_sample.wav out_sample.brr\n"
	);
	exit(1);
}
  */
static u8 filter_at_loop = 0;
static pcm_t p1_at_loop, p2_at_loop;
static bool FIRen[4] = {true, true, true, true};	// Which BRR filters are enabled
static unsigned int FIRstats[4] = {0, 0, 0, 0};	// Statistincs on BRR filter usage
static bool wrap_en = true;
static char resample_type = 'l';					// Resampling type (n = nearest neighboor, l = linear, c = cubic, s = sine, b = bandlimited)

double sinc(const double x)
{
	if(x == 0.0)
		return 1.0;
	else
		return sin(PI * x) / (PI * x);
}

// Convert a block from PCM to BRR
// Returns the squared error between original data and encoded data
// If "is_end_point" is true, the predictions p1/p2 at loop are also used in caluclating the error (depending on filter at loop)

#define CLAMP_16(n) ( ((signed short)(n) != (n)) ? ((signed short)(0x7fff - ((n)>>24))) : (n) )

double ADPCMMash(unsigned int shiftamount, u8 filter, const pcm_t PCM_data[16], bool write, bool is_end_point)
{
	double d2=0.0;
	pcm_t l1 = p1;
	pcm_t l2 = p2;
	int step = 1<<shiftamount;

	int vlin, d, da, dp, c;

	for(int i=0; i<16; ++i)
	{
		/* make linear prediction for next sample */
		/*      vlin = (v0 * iCoef[0] + v1 * iCoef[1]) >> 8; */
		vlin = get_brr_prediction(filter, l1, l2) >> 1;
		d = ( PCM_data[i] >> 1 ) - vlin;		/* difference between linear prediction and current sample */
		da = abs( d );
		if ( wrap_en && da > 16384 && da < 32768 )
		{
			/* Take advantage of wrapping */
			d = d - 32768 * ( d >> 24 );
			if(write) printf("Caution : Wrapping was used.\n");
		}
		dp = d + (step << 2) + (step >> 2);
		c = 0;
		if (dp > 0)
		{
			if (step > 1)
				c = dp / ( step / 2 );
			else
				c = dp * 2;
			if (c > 15)
				c = 15;
		}
		c -= 8;
		dp = ( c << shiftamount ) >> 1;		/* quantized estimate of samp - vlin */
		/* edge case, if caller even wants to use it */
		if ( shiftamount > 12 )
			dp = ( dp >> 14 ) & ~0x7FF;
		c &= 0x0f;		/* mask to 4 bits */

		l2 = l1;			/* shift history */
		l1 = (pcm_t) ( CLAMP_16( vlin + dp ) * 2 );

		d = PCM_data[i] - l1;
		d2 += (double)d * d;		/* update square-error */

		if (write)					/* if we want output, put it in proper place */
			(BRR+1)[i >> 1] |= (i&1) ? c : c<<4;
	}

	if (is_end_point)
		switch(filter_at_loop)
		{	/* Also account for history points when looping is enabled & filters used */
			case 0:
			d2 /= 16.;
			break;

			/* Filter 1 */
			case 1:
			d = l1 - p1_at_loop;
			d2 += (double)d * d;
			d2 /= 17.;
			break;

			/* Filters 2 & 3 */
			default:
			d = l1 - p1_at_loop;
			d2 += (double)d * d;
			d = l2 - p2_at_loop;
			d2 += (double)d * d;
			d2 /= 18.;
		}
	else
		d2 /= 16.;

	if (write)
    {	/* when generating real output, we want to return these */
		p1 = l1;
		p2 = l2;

		BRR[0] = (shiftamount<<4)|(filter<<2);
		if(is_end_point)
				BRR[0] |= 1;						//Set the end bit if we're on the last block
    }
	return d2;
}

// Encode a ADPCM block using brute force over filters and shift amounts
void ADPCMBlockMash(const pcm_t PCM_data[16], bool is_loop_point, bool is_end_point)
{
	int smin, kmin;
	double dmin = INFINITY;

	for(int s=0; s<13; ++s)
		for(int k=0; k<4; ++k)
			if(FIRen[k])
			{
				double d = ADPCMMash(s, k, PCM_data, false, is_end_point);
				if (d < dmin)
				{
					kmin = k;		//Memorize the filter, shift values with smaller error
					dmin = d;
					smin = s;
				}
			}

	if(is_loop_point)
	{
		filter_at_loop = kmin;
		p1_at_loop = p1;
		p2_at_loop = p2;
	}
	ADPCMMash(smin, kmin, PCM_data, true, is_end_point);
	FIRstats[kmin]++;
}

static pcm_t *resample(pcm_t *samples, int samples_length, int out_length, char type)
{
	double ratio = (double)samples_length / (double)out_length;
	pcm_t *out = (short*)safe_malloc(2 * out_length);

	printf("Resampling by effective ratio of %f...\n", ratio);
	
	switch(type) {
	case 'n':								//No interpolation
		for(int i=0; i<out_length; ++i)
		{
			out[i] = samples[(int)floor(i*ratio)];
		}
		break;
	case 'l':								//Linear interpolation
		for(int i=0; i<out_length; ++i)
		{
			int a = (int)(i*ratio);		//Whole part of index
			double b = i*ratio-a;		//Fractional part of index
			if((a+1)==samples_length)
				out[i] = samples[a];	//This used only for the last sample
			else
				out[i] = (1-b)*samples[a]+b*samples[a+1];
		}
		break;
	case 's':								//Sine interpolation
		for(int i=0; i<out_length; ++i)
		{
			int a = (int)(i*ratio);
			double b = i*ratio-a;
			double c = (1.0-cos(b*PI))/2.0;
			if((a+1)==samples_length)
				out[i] = samples[a];	//This used only for the last sample
			else out[i] = (1-c)*samples[a]+c*samples[a+1];
		}
		break;
	case 'c':										//Cubic interpolation
		for(int i=0; i<out_length; ++i)
		{
			int a = (int)(i*ratio);

			short s0 = (a == 0) ? samples[0] : samples[a-1];
			short s1 = samples[a];
			short s2 = (a+1 >= samples_length) ? samples[samples_length-1] : samples[a+1];
			short s3 = (a+2 >= samples_length) ? samples[samples_length-1] : samples[a+2];

			double a0 = s3-s2-s0+s1;
			double a1 = s0-s1-a0;
			double a2 = s2-s0;
			double b = i*ratio-a;
			double b2 = b*b;
			double b3 = b2*b;
			out[i] = b3*a0 + b2*a1 + b*a2 + s1;
		}
		break;

	case 'b':									// Bandlimited interpolation
		// Antialisaing pre-filtering
		if(ratio > 1.0)
		{
			signed short *samples_antialiased = (short*)safe_malloc(2 * samples_length);

			#define FIR_ORDER (15)
			double fir_coefs[FIR_ORDER+1];
			// Compute FIR coefficients
			for(int k=0; k<=FIR_ORDER; ++k)
				fir_coefs[k] = sinc(k/ratio)/ratio;

			// Apply FIR filter to samples
			for(int i=0; i<samples_length; ++i)
			{
				double acc = samples[i] * fir_coefs[0];
				for(int k=FIR_ORDER; k>0; --k)
				{
					acc += fir_coefs[k] * ((i+k < samples_length) ? samples[i+k] : samples[samples_length-1]);
					acc += fir_coefs[k] * ((i-k >= 0) ? samples[i-k] : samples[0]);
				}
				samples_antialiased[i] = (pcm_t)acc;
			}

			free(samples);
			samples = samples_antialiased;
		}
		// Actual resampling using sinc interpolation
		for(int i=0; i<out_length; ++i)
		{
			double a = i*ratio;
			double acc = 0.0;
			for(int j=(int)a-FIR_ORDER; j<=(int)a+FIR_ORDER; ++j)
			{
				pcm_t sample;
				if(j >=0)
					if(j < samples_length)
						sample = samples[j];
					else
						sample = samples[samples_length-1];
				else
					sample = samples[0];
				
				acc += sample*sinc(a-j);
			}
			out[i] = (pcm_t)acc;
		}
		break;

	/*default :
		fprintf(stderr, "\nError : A valid interpolation algorithm must be chosen !\n");
		print_instructions();*/
	}
	// No longer need the non-resampled version of the sample
	free(samples);
	return out;
}

// This function applies a treble boosting filter that compensates the gauss lowpass filter
static pcm_t *treble_boost_filter(pcm_t *samples, int length)
{	// Tepples' coefficient multiplied by 0.6 to avoid overflow in most cases
	const double coefs[8] = {0.912962, -0.16199, -0.0153283, 0.0426783, -0.0372004, 0.023436, -0.0105816, 0.00250474};

	pcm_t *out = (short*)safe_malloc(length * 2);
	for(int i=0; i<length; ++i)
	{
		double acc = samples[i] * coefs[0];
		for(int k=7; k>0; --k)
		{
			acc += coefs[k] * ((i+k < length) ? samples[i+k] : samples[length-1]);
			acc += coefs[k] * ((i-k >= 0) ? samples[i-k] : samples[0]);
		}
		out[i] = acc;
	}
	free(samples);
	return out;
}
/*
int main(const int argc, char *const argv[])
{
	double ampl_adjust = 1.0;				// Adjusting amplitude
    double ratio = 1.0;					// Resampling factor (range ]0..4])
    char loop_flag = 0;						// = 0x02 if loop flag is active
    unsigned int target_samplerate = 0;		// Output sample rate (0 = don't change)
    bool fix_loop_en = false;				// True if fixed loop is activated
	unsigned int loop_start;				// Starting point of loop
	unsigned int truncate_len = 0;			// Point at which input wave will be truncated (if = 0, input wave is not truncated)
	bool treble_boost = false;

	int c;
	while((c = getopt(argc, argv, "a:l::f:wr:s:z:r:t:g")) != -1)
	{
		switch(c)
		{
			case 'a':
				ampl_adjust = atof(optarg);
				break;

			// Only specified filters are enabled
			case 'f':
				FIRen[0] = false;
				FIRen[1] = false;
				FIRen[2] = false;
				FIRen[3] = false;
				for(int i=0; i < strlen(optarg); ++i)
				{
					switch(optarg[i])
					{
						case '0' :
							FIRen[0] = true;
							break;
						case '1' :
							FIRen[1] = true;
							break;
						case '2' :
							FIRen[2] = true;
							break;
						case '3' :
							FIRen[3] = true;
							break;
						default:
							print_instructions();
					}
				}
				break;

			case 'w':
				wrap_en = false;
				break;

			case 'r':
				resample_type = optarg[0];
				ratio = atof(optarg+1);
				if(ratio <= 0.0 || ratio > 4.0)
					print_instructions();
				break;

			case 's':
				resample_type = optarg[0];
				target_samplerate = atoi(optarg+1);
				break;

			case 'l':
				loop_flag = 0x02;
				if(optarg)			// The argument to -l is facultative
				{
					loop_start = atoi(optarg);
					fix_loop_en = true;
				}
				break;
			
			case 't':
				truncate_len = atoi(optarg);
				break;

			case 'g':
				treble_boost = true;
				break;

			default :
				printf("Invalid command line syntax !\n");
				print_instructions();
		}
	}

	if(argc - optind != 2) print_instructions();
	char *inwav_path = argv[optind];			// Path of input and output files
	char *outbrr_path = argv[optind+1];

	FILE *inwav = fopen(inwav_path, "rb");
	if(!inwav)
	{
		fprintf(stderr, "Error : Can't open file %s for reading.\n", inwav_path);
		exit(1);
	}

	struct
	{
		char chunk_ID[4];				// Should be 'RIFF'
		u32 chunk_size;
		char wave_str[4];				// Should be 'WAVE'
		char sc1_id[4];					// Should be 'fmt '
		u32 sc1size;					// Should be at least 16
		u16 audio_format;				// Should be 1 for PCM
		u16 chans;						// 1 for mono, 2 for stereo, etc...
		u32 sample_rate;
		u32 byte_rate;
		u16 block_align;
		u16 bits_per_sample;
	}
	hdr;

	// Read header
	int err = fread(&hdr, 1, sizeof(hdr), inwav);
	// If they couldn't read the file (for example if it's too small)
	if(err != sizeof(hdr))
	{
		fprintf(stderr, "Error : Input file in incompatible format %d\n", err);
		exit(1);
	}

	// Read "RIFF" word
	if(strncmp(hdr.chunk_ID, "RIFF", 4))
	{
		fprintf(stderr, "Error : Input file in unsupported format : \"RIFF\" block missing.\n");
		exit(1);
	}
	// "WAVEfmt" letters
	if(strncmp(hdr.wave_str, "WAVEfmt ", 8))
	{
		fprintf(stderr, "Input file in unsupported format : \"WAVEfmt\" block missing !\n");
		exit(1);
	}

	//Size of sub-chunk1 (header) must be at least 16 and in PCM format
	if(hdr.sc1size < 0x10 || hdr.audio_format != 1)
	{
		fprintf(stderr, "Input file in unsupported format : file must be uncompressed PCM !\n");
		exit(1);
	}

	//Check how many channels
	if(hdr.chans != 1)
		printf("Input is multi-channel : Will automatically be converted to mono.\n");

	// Check for correctness of byte rate
	if(hdr.byte_rate != hdr.sample_rate*hdr.chans*hdr.bits_per_sample/8)
	{
		fprintf(stderr, "Byte rate in input file is set incorrectly.\n");
		exit(1);
	}

	//Read block align and bits per sample numbers
	if(hdr.block_align != hdr.bits_per_sample*hdr.chans/8)
	{
		fprintf(stderr, "Block align in input file is set incorrectly\n");
		exit(1);
	}
	fseek(inwav, hdr.sc1size-0x10, SEEK_CUR);			// nSkip possible longer header

	struct
	{
		char name[4];
		u32 size;
	}
	sub_hdr;
	while(true)
	{
		err = fread(&sub_hdr, 1, sizeof(sub_hdr), inwav);
		if(err != sizeof(sub_hdr))
		{
			fprintf(stderr, "End of file reached without finding a \"data\" chunk.\n");
			exit(1);
		}
		if(strncmp(sub_hdr.name, "data", 4))			// If there is anyother non-"data" block, skip it
			fseek(inwav, sub_hdr.size, SEEK_CUR);
		else
			break;
	}

	// Output buffer
	unsigned int samples_length = sub_hdr.size/hdr.block_align;
	// Optional truncation of input sample
	if(truncate_len && (truncate_len < samples_length))
		samples_length = truncate_len;

	pcm_t *samples = safe_malloc(2*samples_length);

	// Adjust amplitude in function of amount of channels
	ampl_adjust /= hdr.chans;
	switch (hdr.bits_per_sample)
	{
		signed int sample;
		case 8 :
			for(int i=0; i < samples_length; ++i)
			{
				unsigned char in8_chns[hdr.chans];
				fread(in8_chns, 1, hdr.chans, inwav);	// Read single sample on CHANS channels at a time
				sample = 0;
				for(int ch=0; ch < hdr.chans; ++ch)		// Average samples of all channels
					sample += in8_chns[ch]-0x80;
				samples[i] = (pcm_t)((sample<<8) * ampl_adjust);
			}
			break;

		case 16 :
			for(int i=0; i < samples_length; ++i)
			{
				signed short in16_chns[hdr.chans];
				fread(in16_chns, 2, hdr.chans, inwav);
				sample = 0; 
				for(int ch=0; ch < hdr.chans; ++ch)
					sample += in16_chns[ch];
				samples[i] = (pcm_t)(sample * ampl_adjust);
			}
			break;

		// If you encounter the error below, add your implementation for different # of bits
		default :
			fprintf(stderr, "Error : unsupported amount of bits per sample (8 or 16 are supported)\n");
			exit(1);
	}
	fclose(inwav);		// We're done with the input wave file

	unsigned int target_length;
	if(target_samplerate)						// Set resample factor if auto samplerate mode
		target_length = ((long long)samples_length * target_samplerate) /  hdr.sample_rate;
	else
		target_length = (int)(samples_length/ratio);

	unsigned int new_loopsize;
	if(fix_loop_en)
	{
		unsigned int loopsize = ((long long)(samples_length - loop_start) * target_length) / samples_length;
		// New loopsize is the multiple of 16 that comes after loopsize
		new_loopsize = ((loopsize + 15)/16)*16;
		// Adjust resampling
		target_length = ((long long)target_length * new_loopsize) / loopsize;
	}

	samples = resample(samples, samples_length, target_length, resample_type);
	samples_length = target_length;

	// Apply trebble boost filter (gussian lowpass compensation) if requested by user
	if(treble_boost) samples = treble_boost_filter(samples, samples_length);
	
	if ((samples_length % 16) != 0)
	{
		int padding = 16 - (samples_length % 16);
		printf(
			"The Amount of PCM samples isn't a multiple of 16 !\n"
			"The sample will be padded with %d zeroes at the beginning.\n"
		, padding);

		// Increase buffer size and add zeroes at beginning
		samples = realloc(samples, 2*(samples_length + padding));
		if(!samples)
		{
			fprintf(stderr, "Error : Can't allocate memory.\n");
			exit(1);
		}
		memmove(samples + padding, samples, 2*samples_length);
		samples_length += padding;

		do
			samples[--padding] = 0;
		while(padding > 0);
	}
	printf("Size of file to encode : %u samples = %u BRR blocks.\n", samples_length, samples_length/16);
	
	FILE *outbrr = fopen(outbrr_path, "wb");
	if(!outbrr)
	{
		fprintf(stderr, "Error : Can't open file %s for writing.\n", outbrr_path);
		exit(1);
	}

	bool initial_block = false;
	for (int i=0; i<16; ++i)					//Initialization needed if any of the first 16 samples isn't zero
		initial_block |= samples[i]!=0;

	if(initial_block)
	{		//Write initial BRR block
		const u8 initial_block[9] = {loop_flag, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
		fwrite(initial_block, 1, 9, outbrr);
		printf("Initial BRR block added.\n");
	}
	p1 = 0;
	p2 = 0;
	for (int n=0; n<samples_length; n+=16)
	{
		//Clear BRR buffer
		memset(BRR, 0, 9);
		//Encode BRR block, tell the encoder if we're at loop point (if loop is enabled), and if we're at end point
		ADPCMBlockMash(samples + n, fix_loop_en && (n == (samples_length - new_loopsize)), n == samples_length - 16);
		//Set the loop flag if needed
		BRR[0] |= loop_flag;
		fwrite(BRR, 9, 1, outbrr);
	}
	printf("Done !\n");

	if(fix_loop_en)
	{
		unsigned int k = samples_length - (initial_block ? new_loopsize - 16 : new_loopsize);
		printf("Position of the loop within the BRR sample : %u samples = %u BRR blocks.\n", k, k/16);
	}

	for(int i=0; i<4; i++)
		if (FIRstats[i]>0) printf("Filter %u used on %u blocks.\n", i, FIRstats[i]);

	fclose(outbrr);
	free(samples);
	return 0;
}
*/