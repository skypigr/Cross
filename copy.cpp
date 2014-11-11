#include <stdio.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <assert.h>

#if defined (__unix__) && !defined(UNIX)
# define UNIX
#endif
#if defined (__linux__) && !defined(LINUX)
# define LINUX
#endif

#ifdef WIN32
#include <windows.h>
#define	FSEEK	_fseeki64	
#define FTELL	_ftelli64
#define SLEEP	Sleep
#endif

#ifdef LINUX
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#define	FSEEK	fseek	
#define FTELL	ftell
#define SLEEP(x)	usleep(x)
#endif

using namespace std;

static char gBuffer[1024*256];

double psttime(time_t end, time_t begin)
{

#ifdef WIN32
	return static_cast<double>(end - begin) / CLOCKS_PER_SEC;
#else
	return static_cast<double>(end - begin) / CLOCKS_PER_SEC * 1000 ;
#endif
}

void ShowPercent(int percent)
{
	cout<<setw(3)<<right<<setfill(' ')<<percent<<"%";
}

void ShowProcessBar(int width,int percent)
{
	int sz = width +3;
	int dlsz = static_cast<int>( (double)width * percent / 100.);
	char *p = new char[sz];
	char *begin;
	memset(p,0,sz);
	
	begin = p;
	*p++ = '[';
	
	if(dlsz)
	{
		for (int i = 0; i < dlsz - 1; i++)
			*p++ = '=';
		*p++ = '>';
	}

	while (p-1  - begin < width)
		*p++ = ' ';
	*p++ = ']';
	
	cout<<begin;
	delete []begin;

}

void ShowSpeed(double kbs)
{
	char *csp[]={"K/s","M/s","G/s"}, *sp;

	if( (int)kbs >> 10 < 1)
		sp = csp[0];	
	else if((int)kbs >> 20 < 1)
	{			
		kbs /=(1<<10);
		sp = csp[1];
	}
	else
	{
		kbs /=(1<<20);
		sp = csp[2];
	}

	cout<<fixed<<setprecision(2)<<setw(8)<<setfill(' ')<<right<<kbs<<sp;

}

void ShowTime(double t)
{
	//the final output should like [========>] 1,212,784    247K/s   in 5.9s
	cout<<"  in ";
	cout<<fixed<<setprecision(1)<<t<<"s";
}

void PrintMsg(int width, int percent, double speed, double time)
{	
	//assert(percent>=0);
	printf("\r");

	ShowPercent(percent);
	ShowProcessBar(width,percent);
	ShowSpeed(speed);
	ShowTime(time);

	fflush(stdout);
}


long long  init(const char *src, const char *des)
{
	long long nFileLen = 0L;
	FILE *pSrc, *pDes;

#ifdef WIN32
	fopen_s(&pSrc,src,"rb"); //win32
#else
	pSrc = fopen(src,"rb");
#endif

	if (pSrc)
	{			
		FSEEK(pSrc,0,SEEK_END);	
		nFileLen = FTELL(pSrc);
	}
	else
	{
		cout<<"Cannot Open File: "<<src<<endl;
		return -1;
	}

		
#ifdef WIN32
	fopen_s(&pDes,des,"w");//win32
#else
	pDes = fopen(des,"w");
#endif
	

	if (pDes)
	{
		fflush(pDes);
		fclose(pDes);
	}
	else
	{
		cout<<"Cannot Open File: "<<src<<endl;
		return -1;
	}
	
	return nFileLen;
}

int read(FILE* pread, FILE* pwrite, long long begin, int num)
{

	char* pBuffer = gBuffer;

	int readsize = 0;
	if (pread)
	{
		//fpos_t pos = static_cast<fpos_t> (begin);
		FSEEK(pread,begin,0);
		//fsetpos(pread, &pos);
		readsize = fread(pBuffer, 1, num, pread);
		fflush(pread);
		//fclose(pFile);
	}


	if (pwrite)
	{
		fwrite(pBuffer, 1, readsize, pwrite);
		fflush(pwrite);
		//fclose(pFile2);
	}

	return readsize;
}


int main(int argc, char* argv[])
{

	time_t begin, end;
	begin = clock();
	end = clock();

	const char * SRC_FILE = argv[1];
	const char * DEST_FILE = argv[2];
	int MaxSpeed = 0;

	if (argc > 3)
		MaxSpeed = atoi( argv[3]+12);

	int bufszie = 4096*64;	
	long long filesize = init(SRC_FILE, DEST_FILE);

#ifdef _DEBUG
	assert(filesize>0);
#endif
	if(filesize<0) 
		return -1;

	int readsize = 0;
	long long totalsize = 0;

	FILE* pread, *pwrite;
#ifdef WIN32
	fopen_s(&pread,SRC_FILE, "rb");	
	fopen_s(&pwrite,DEST_FILE, "ab+");	
	
#else
	pread = fopen(SRC_FILE, "rb");	
	pwrite = fopen(DEST_FILE, "ab+");
#endif
			
	double speed = 0;
	long long count = 0;
	double lasttime = 0, pstime;
	int percent;

		
	begin = clock();
	readsize = read(pread, pwrite, 0, bufszie);
	totalsize += readsize;

	while (readsize)
	{
		count++;
		if(count>10)
		{
			end = clock();
			speed = (double)totalsize / psttime(end,begin) /1024.;	

			if(speed > MaxSpeed  && MaxSpeed )
			{
				SLEEP(100);
				continue;
			}
		}
		else
			end = clock();

	
		double invt = psttime(end,begin);
		if (invt - lasttime > 0.2)
			invt = 1;
		else
			invt = 0;

		if (invt  && begin != end)
		{
			//speed
			speed = (double)totalsize / psttime(end,begin) /1024.;
			pstime = psttime(end,begin);
			percent = static_cast<int>( (totalsize / (double)filesize) * 100);
			PrintMsg(50,percent,speed,pstime);

			lasttime = psttime(end,begin);	
		}
		

		readsize = read(pread, pwrite, totalsize, bufszie);
		totalsize += readsize;
		if(readsize < bufszie) 
			break;
		
	}
	speed = (double)totalsize / psttime(end,begin)/1024.;
	pstime = psttime(end,begin);
	percent = static_cast<int>( (totalsize / (double)filesize) * 100);
	PrintMsg(50,percent,speed,pstime);

	cout<<endl;
	cout << "time is :" << psttime(end,begin) << endl;


	fclose(pread);
	fclose(pwrite);
	return 0;
}
