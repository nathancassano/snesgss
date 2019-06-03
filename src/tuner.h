/* this code is C version of Java 5KTuner by John Montgomery, CC-BY 2008 */



#include <float.h>



int tuner_note;
int tuner_detune;
int *tuner_a=NULL;
int tuner_a_len;
bool tuner_active;



const double tuner_freqs[]={ 174.61,164.81,155.56,146.83,138.59,130.81,123.47,116.54,110.00,103.83,98.00,92.50,87.31,82.41,77.78 };
const char* tuner_note_names[]={ "F","E","D#","D","C#","C","B","A#","A","G#","G","F#","F","E","D#" };



void tuner_init(void)
{
	tuner_a=NULL;
	tuner_a_len=0;
	tuner_note=0;
	tuner_detune=0;
	tuner_active=false;
}



void tuner_shut(void)
{
	if(tuner_a)
	{
		free(tuner_a);
		tuner_a=NULL;
	}
}



void tuner_set_active(bool active)
{
	tuner_active=active;
}



const char* tuner_get_note_name(void)
{
	if(tuner_note>=0) return tuner_note_names[tuner_note]; else return NULL;
}



int tuner_get_detune(void)
{
	return tuner_detune;
}



void tuner_analyze(short *wave,int rate,int len)
{
	double diff,prevDiff,dx,prevDx,maxDiff,frequency,dist,minDist,matchFreq,prevFreq,nextFreq;
	int i,j,ptr,sampleLen,value,detune;

	if(!tuner_active) return;
	
	len>>=1;

	prevDiff=0;
	prevDx=0;
	maxDiff=0;
	sampleLen=0;

	if(!tuner_a)
	{
		tuner_a=(int*)malloc(len*sizeof(int));
		tuner_a_len=len;
	}

	if(tuner_a_len<len)
	{
		tuner_a=(int*)realloc(tuner_a,len*sizeof(int));
		tuner_a_len=len;
	}

	ptr=0;

	for(i=0;i<len;++i)
	{
		tuner_a[i]=wave[ptr+0]+wave[ptr+1];
		ptr+=2;
	}

	for(i=0;i<len;++i)
	{
		diff=0;

		for(j=0;j<len;++j) if(i+j<len) diff+=abs(tuner_a[j]-tuner_a[i+j]); else diff+=abs(tuner_a[j]);

		dx=prevDiff-diff;

		if(dx<0&&prevDx>0&&diff<(0.5*maxDiff))
		{
			sampleLen=i-1;
			break;
		}

		prevDx=dx;
		prevDiff=diff;

		if(diff>maxDiff) maxDiff=diff;
	}

	if(sampleLen>0)
	{
		frequency=(double)rate/(double)sampleLen;

		while(frequency< 82.41) frequency=2.0*frequency;
		while(frequency>164.81) frequency=0.5*frequency;

		minDist=DBL_MAX;
		tuner_note=-1;

		for(i=0;i<sizeof(tuner_freqs)/sizeof(double);++i)
		{
			dist=abs(tuner_freqs[i]-frequency);

			if(dist<minDist)
			{
				minDist=dist;
				tuner_note=i;
			}
		}

		tuner_detune=0;
		matchFreq=tuner_freqs[tuner_note];

		if(frequency<matchFreq)
		{
			prevFreq=tuner_freqs[tuner_note+1];
			tuner_detune=(int)(-100*(frequency-matchFreq)/(prevFreq-matchFreq));
		}
		else
		{
			nextFreq=tuner_freqs[tuner_note-1];
			tuner_detune=(int)(100*(frequency-matchFreq)/(nextFreq-matchFreq));
		}
	}
	else
	{
		tuner_note=-1;
		tuner_detune=0;
	}
}