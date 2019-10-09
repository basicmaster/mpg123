#include <syn123.h>
#include <out123.h>

#include "compat.h"

static long block_rate(long rate, double speed, double factor, off_t bi)
{
	static long maxrate = -1;
	if(maxrate < 0)
		maxrate = syn123_resample_maxrate();
	double foutrate = (double)rate/(speed*pow(factor, bi));
	long outrate = foutrate > maxrate ? maxrate : (long)foutrate;
	return outrate > 0 ? outrate : 1;
}

int main(int argc, char **argv)
{
	int ret = 0;
	if(argc < 7)
	{
		fprintf( stderr
		,	"Usage: %s <rate> <freq> <block> <speed> <speedfactor> <smooth>"
			" [duration [outfile]]\n"
		,	argv[0] );
		return 1;
	}
	long rate = atol(argv[1]);
	double freq = atof(argv[2]);
	long block = atol(argv[3]);
	double speed  = atof(argv[4]);
	double factor = atof(argv[5]);
	if(factor < 1e-10)
	{
		fprintf(stderr, "Funny. Give me a speed factor properly > 0!\n");
		return -6;
	}
	int smooth = atoi(argv[6]);
	double duration = argc > 7 ? atof(argv[7]) : 5;
	// Trivially ensuring that it's > 0, playing a bit longer than required.
	// Could cut things in the end, if I really care.
	off_t out_limit = (off_t)(duration*rate)/block+1;
	char *outfile = argc > 8 ? argv[8] : NULL;
	syn123_handle *syn = syn123_new(rate, 1, MPG123_ENC_FLOAT_32, 0, NULL);
	if(!syn)
		return -1;
	out123_handle *out = out123_new();
	if(!out)
		return -2;
	if(out123_open(out, outfile ? "wav" : NULL, outfile))
		return -3;
	if(out123_start(out, rate, 1, MPG123_ENC_FLOAT_32))
		return -3;
	if(syn123_setup_waves(syn, 1, NULL, &freq, NULL, NULL, NULL))
		return -4;
	off_t bi = 0;
	// Biggest output rate to plan for.
	long firstrate = block_rate(rate, speed, factor, 0);
	long lastrate  = block_rate(rate, speed, factor, (out_limit-1));
	long maxrate = firstrate > lastrate ? firstrate : lastrate;
	long minrate = firstrate < lastrate ? firstrate : lastrate;
	fprintf(stderr, "rate %ld to [%ld;%ld]\n", rate, minrate, maxrate);
	size_t maxoutblock = syn123_resample_count( rate, maxrate
	,	syn123_resample_incount(rate, maxrate, block) );
	size_t maxinblock  = syn123_resample_incount(rate, minrate, block);
	//fprintf(stderr, "maxinblock %zu, maxoutblock %zu\n", maxinblock, maxoutblock);
	if(!maxoutblock || !maxinblock)
		return -7;
	float *outbuf = malloc(sizeof(float)*maxoutblock);
	float *inbuf = malloc(sizeof(float)*maxinblock);
	if(!outbuf || !inbuf)
		return -13;

	off_t intotal  = 0;
	off_t outtotal = 0;
	while(bi < out_limit)
	{
		long outrate = block_rate(rate, speed, factor, bi);
		fprintf(stderr, "block %"OFF_P" rate %ld\n", (off_p)bi, outrate);
		if(syn123_setup_resample(syn, rate, outrate, 1, 0, smooth))
		{
			ret = -11;
			break;
		}
		// Determine how many input samples to feed to get block output samples.
		ssize_t inblock = syn123_resample_inexpect(syn, block);
		if(inblock <= 0 || inblock > maxinblock)
		{
			fprintf(stderr, "bad inblock: %zd\n", inblock);
			ret = -15;
			break;
		}
		ssize_t outblock = syn123_resample_expect(syn, inblock);
		fprintf(stderr, "in %zu out %zu\n", inblock, outblock);
		if(outblock <= 0 || outblock > maxoutblock || outblock < block)
		{
			fprintf(stderr, "bad outblock: %zd\n", outblock);
			ret = -16;
			break;
		}
		size_t bytes = syn123_read(syn, inbuf, sizeof(float)*inblock);
		if(bytes != sizeof(float)*inblock)
		{
			ret = -14;
			break;
		}
		size_t outsamples = syn123_resample(syn, outbuf, inbuf, inblock);
		if(outsamples != outblock)
		{
			fprintf( stderr, "output prediction wrong in block %"
				PRIiMAX": %zu != %zu\n", bi, outsamples, outblock );
			ret = -17;
			break;
		}
		intotal  += inblock;
		outtotal += outblock;
		//fprintf( stderr, "ratio: %f %"OFF_P":%"OFF_P" to %"OFF_P":%"OFF_P"\n"
		//,	(double)intotal/outtotal, (off_p)intotal
		//,	(off_p)syn123_resample_intotal(rate, outrate, outtotal)
		//,	(off_p)outtotal, (off_p)syn123_resample_total(rate, outrate, intotal) );
		syn123_amp(outbuf, MPG123_ENC_FLOAT_32, outsamples, syn123_db2lin(-12), 0, NULL, NULL);
		if(out123_play(out, outbuf, sizeof(float)*outsamples) != sizeof(float)*outsamples)
		{
			ret = -10;
			break;
		}
#if 0
		float spacer[100];
		if(outsamples)
		{
			for(unsigned int i=0; i<sizeof(spacer)/sizeof(float); ++i)
				spacer[i] = outbuf[outsamples-1];
		}
		else
			memset(spacer, 0, sizeof(spacer));
		out123_play(out, spacer, sizeof(spacer));
#endif
		++bi;
	}
	free(outbuf);
	free(inbuf);
	out123_del(out);
	syn123_del(syn);
	return ret;
}
