struct {
	CRITICAL_SECTION csection;
	WAVEHDR* wblocks;
	HWAVEOUT device;
	HANDLE thread;
	int threadStop;
	int rate;
	int channels;
	int scount;
	int blockSize;
	int blockCount;
	int wfree;
	int wcurrent;
	int lastOutputL;
	int lastOutputR;
	bool render;
	short buffer[1024*4];
	int peakL;
	int peakR;
} WSTREAMER;



inline void mixer_render(short int *wave,int size)
{
	const int step=16;
	int i,n;

	if(!size) return;

	WSTREAMER.render=TRUE;

	EnterCriticalSection(&WSTREAMER.csection);

	if(emu&&MusicActive)
	{
		handle_gme_error(gme_play(emu,size,wave));

		WSTREAMER.lastOutputL=wave[size-2];
		WSTREAMER.lastOutputR=wave[size-1];
	}
	else
	{
		if(WSTREAMER.lastOutputL!=0||WSTREAMER.lastOutputL!=0)
		{
			for(i=0;i<size;i+=2)
			{
				if(WSTREAMER.lastOutputL<0)
				{
					WSTREAMER.lastOutputL+=step;

					if(WSTREAMER.lastOutputL>0) WSTREAMER.lastOutputL=0;
				}

				if(WSTREAMER.lastOutputL>0)
				{
					WSTREAMER.lastOutputL-=step;

					if(WSTREAMER.lastOutputL<0) WSTREAMER.lastOutputL=0;
				}

				if(WSTREAMER.lastOutputR<0)
				{
					WSTREAMER.lastOutputR+=step;

					if(WSTREAMER.lastOutputR>0) WSTREAMER.lastOutputR=0;
				}

				if(WSTREAMER.lastOutputR>0)
				{
					WSTREAMER.lastOutputR-=step;

					if(WSTREAMER.lastOutputR<0) WSTREAMER.lastOutputR=0;
				}

				wave[i+0]=WSTREAMER.lastOutputL;
				wave[i+1]=WSTREAMER.lastOutputR;
			}
		}
		else
		{
			for(i=0;i<size;++i) wave[i]=0;
		}
	}

	LeaveCriticalSection(&WSTREAMER.csection);

	WSTREAMER.render=false;
}





static void mixer_write(HWAVEOUT hWaveOut, LPSTR data, int size)
{
	WAVEHDR* current;
	int remain;
	
	current=&WSTREAMER.wblocks[WSTREAMER.wcurrent];
	
	while(size>0)
	{
		if(current->dwFlags&WHDR_PREPARED)
		{
			waveOutUnprepareHeader(hWaveOut,current,sizeof(WAVEHDR));
		}
		
		if(size<(int)(WSTREAMER.blockSize-current->dwUser))
		{
			memcpy(current->lpData+current->dwUser,data,size);
			current->dwUser+=size;
			break;
		}
		
		remain=WSTREAMER.blockSize-current->dwUser;
		memcpy(current->lpData+current->dwUser,data,remain);
		size-=remain;
		data+=remain;
		current->dwBufferLength=WSTREAMER.blockSize;
		
		waveOutPrepareHeader(hWaveOut,current,sizeof(WAVEHDR));
		waveOutWrite(hWaveOut,current,sizeof(WAVEHDR));
		
		EnterCriticalSection(&WSTREAMER.csection);
		WSTREAMER.wfree--;
		LeaveCriticalSection(&WSTREAMER.csection);
		
		while(!WSTREAMER.wfree) Sleep(10);
		
		WSTREAMER.wcurrent++;
		WSTREAMER.wcurrent%=WSTREAMER.blockCount;
		
		current=&WSTREAMER.wblocks[WSTREAMER.wcurrent];
		current->dwUser=0;
	}
}



static WAVEHDR* mixer_allocate(int size, int count)
{
	DWORD totalBufferSize=(size+sizeof(WAVEHDR))*count;
	unsigned char* buffer;
	WAVEHDR* blocks;
	int i;
	
	buffer=(unsigned char*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,totalBufferSize);

	if(!buffer) return NULL;
	
	blocks=(WAVEHDR*)buffer;
	buffer+=sizeof(WAVEHDR)*count;

	for(i=0;i<count;i++)
	{
		blocks[i].dwBufferLength=size;
		blocks[i].lpData=(char*)buffer;
		buffer+=size;
	}
	
	return blocks;
}




static void CALLBACK mixer_callback(HWAVEOUT hWaveOut,UINT uMsg,DWORD dwInstance,DWORD dwParam1,DWORD dwParam2)
{
	int* freeBlockCounter=(int*)dwInstance;
	
	if(uMsg!=WOM_DONE) return;
	
	EnterCriticalSection(&WSTREAMER.csection);

	(*freeBlockCounter)++;

	LeaveCriticalSection(&WSTREAMER.csection);
}



void waveout_update_thread(void*)
{
	short buffer[1024];
	int i,l,r;

	while(!WSTREAMER.threadStop)
	{
		mixer_render(buffer,1024);//samples*stereo
		mixer_write(WSTREAMER.device,(LPSTR)buffer,1024*2);//bytes

		WSTREAMER.peakL=0;
		WSTREAMER.peakR=0;

		for(i=0;i<1024;i+=2)
		{
			 l=abs(buffer[i+0]);
			 r=abs(buffer[i+1]);

			 if(l>WSTREAMER.peakL) WSTREAMER.peakL=l;
			 if(r>WSTREAMER.peakR) WSTREAMER.peakR=r;
		}

		memcpy(WSTREAMER.buffer,&WSTREAMER.buffer[1024],1024*3*sizeof(short));
		memcpy(&WSTREAMER.buffer[1024*3],buffer,1024*sizeof(short));

		tuner_analyze(WSTREAMER.buffer,44100,1024*4);//samples*stereo
	}

	WSTREAMER.threadStop=2;
}



bool waveout_init(HWND sWnd,int mrate,int bsize,int bcount)
{
	WAVEFORMATEX wfx;
	
	WSTREAMER.rate=mrate;
	
	WSTREAMER.blockSize=bsize;
	WSTREAMER.blockCount=bcount;
	WSTREAMER.channels=2;
	WSTREAMER.lastOutputL=0;
	WSTREAMER.lastOutputR=0;
	WSTREAMER.render=false;

	memset(WSTREAMER.buffer,0,sizeof(WSTREAMER.buffer));

	InitializeCriticalSection(&WSTREAMER.csection);
	
	wfx.nSamplesPerSec=WSTREAMER.rate;
	wfx.wBitsPerSample=16;
	wfx.nChannels=WSTREAMER.channels;
	wfx.cbSize=0;
	wfx.wFormatTag=WAVE_FORMAT_PCM;
	wfx.nBlockAlign=(wfx.wBitsPerSample*wfx.nChannels)>>3;
	wfx.nAvgBytesPerSec=wfx.nBlockAlign*wfx.nSamplesPerSec;
	
	if(waveOutOpen(&WSTREAMER.device,WAVE_MAPPER,&wfx,(unsigned long)mixer_callback,(unsigned long)&WSTREAMER.wfree,CALLBACK_FUNCTION)!=MMSYSERR_NOERROR)
	{
		return false;
	}
	
	WSTREAMER.wblocks=mixer_allocate(WSTREAMER.blockSize,WSTREAMER.blockCount);
	WSTREAMER.wfree=WSTREAMER.blockCount;
	WSTREAMER.wcurrent=0;
	
	WSTREAMER.scount=0;
	
	DWORD dw=NULL;
	WSTREAMER.threadStop=0;
	WSTREAMER.thread=CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)waveout_update_thread,0,0,&dw);

	if(!WSTREAMER.thread) return false;
	
	SetThreadPriority(WSTREAMER.thread,THREAD_PRIORITY_HIGHEST);
	
	return true;
}



void waveout_shut(void)
{
	int i;
	
	WSTREAMER.threadStop=1;

	while(WSTREAMER.threadStop!=2) Sleep(10);
	
	while(WSTREAMER.wfree<WSTREAMER.blockCount) Sleep(10);
	
	for(i=0;i<WSTREAMER.wfree;i++)
	{
		if(WSTREAMER.wblocks[i].dwFlags&WHDR_PREPARED)
		{
			waveOutUnprepareHeader(WSTREAMER.device,&WSTREAMER.wblocks[i],sizeof(WAVEHDR));
		}
	}
	
	DeleteCriticalSection(&WSTREAMER.csection);

	HeapFree(GetProcessHeap(),0,WSTREAMER.wblocks);

	waveOutClose(WSTREAMER.device);
}