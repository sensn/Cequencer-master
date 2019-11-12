#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
/*
 * some good values for block size and count
 */
#define BLOCK_SIZE 8192
#define BLOCK_COUNT 20
 /*
  * function prototypes
  */
static void CALLBACK waveOutProc(HWAVEOUT, UINT, DWORD, DWORD, DWORD);
static WAVEHDR* allocateBlocks(int size, int count);
static void freeBlocks(WAVEHDR* blockArray);
static void writeAudio(HWAVEOUT hWaveOut, LPSTR data, int size);
/*
 * module level variables
 */
static CRITICAL_SECTION waveCriticalSection;
static WAVEHDR* waveBlocks;
static volatile int waveFreeBlockCount;
static int waveCurrentBlock;
int s_main(char* fpath)
{
	HWAVEOUT hWaveOut; /* device handle */
	HANDLE hFile;/* file handle */
	WAVEFORMATEX wfx; /* look this up in your documentation */
	char buffer[1024]; /* intermediate buffer for reading */
	int i;
	/*
	 * quick argument check
	 */
	/*if (margc != 2) {
		fprintf(stderr, "usage: %s <filename>\n", margv[0]);
		ExitProcess(1);*/
	//}
	/*
	 * initialise the module variables
	 */
	waveBlocks = allocateBlocks(BLOCK_SIZE, BLOCK_COUNT);
	waveFreeBlockCount = BLOCK_COUNT;
	waveCurrentBlock = 0;
	InitializeCriticalSection(&waveCriticalSection);
	/*
	 * try and open the file
	 */
	printf("%s", fpath[0]);
	if ((hFile = CreateFile(
		//"C:\\Users\\ATN_70\\Desktop\\C2_Ausbildung-master\\C2_Ausbildung\\snare.wav",
		fpath[0],
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		0,
		NULL
	)) == INVALID_HANDLE_VALUE) {
		printf(stderr, "%s: unable to open file '%s'\n");
		ExitProcess(1);
	}
	/*
	 * set up the WAVEFORMATEX structure.
	 */
	wfx.nSamplesPerSec = 44100; /* sample rate */
	wfx.wBitsPerSample = 16; /* sample size */
	wfx.nChannels = 2; /* channels*/
	wfx.cbSize = 0; /* size of _extra_ info */
	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nBlockAlign = (wfx.wBitsPerSample * wfx.nChannels) >> 3;
	wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;
	/*
	 * try to open the default wave device. WAVE_MAPPER is
	 * a constant defined in mmsystem.h, it always points to the
	 * default wave device on the system (some people have 2 or
	 * more sound cards).
	 */
	if (waveOutOpen(
		&hWaveOut,
		WAVE_MAPPER,
		&wfx,
		(DWORD_PTR)waveOutProc,
		(DWORD_PTR)& waveFreeBlockCount,
		CALLBACK_FUNCTION
	) != MMSYSERR_NOERROR) {
		fprintf(stderr, "%s: unable to open wave mapper device\n");
		ExitProcess(1);
	}
	/*
	 * playback loop
	 */
	while (1) {
		DWORD readBytes;
		if (!ReadFile(hFile, buffer, sizeof(buffer), &readBytes, NULL))
			break;
		if (readBytes == 0)
			break;
		if (readBytes < sizeof(buffer)) {
			printf("at end of buffer\n");
			memset(buffer + readBytes, 0, sizeof(buffer) - readBytes);
			printf("after memcpy\n");
		}
		writeAudio(hWaveOut, buffer, sizeof(buffer));
	}
	/*
	 * wait for all blocks to complete
	 */
	while (waveFreeBlockCount < BLOCK_COUNT)
		Sleep(10);
	/*
	 * unprepare any blocks that are still prepared
	 */
	for (i = 0; i < waveFreeBlockCount; i++)
		if (waveBlocks[i].dwFlags & WHDR_PREPARED)
			waveOutUnprepareHeader(hWaveOut, &waveBlocks[i], sizeof(WAVEHDR));
	DeleteCriticalSection(&waveCriticalSection);
	freeBlocks(waveBlocks);
	waveOutClose(hWaveOut);
	CloseHandle(hFile);
	return 0;
}

////

void writeAudioBlock(HWAVEOUT hWaveOut, LPSTR block, DWORD size)
{
	WAVEHDR header;
	/*
	 * initialise the block header with the size
	 * and pointer.
	 */
	ZeroMemory(&header, sizeof(WAVEHDR));
	header.dwBufferLength = size;
	header.lpData = block;
	/*
	 * prepare the block for playback
	 */
	waveOutPrepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
	/*
	 * write the block to the device. waveOutWrite returns immediately
	 * unless a synchronous driver is used (not often).
	 */
	waveOutWrite(hWaveOut, &header, sizeof(WAVEHDR));
	/*
	 * wait a while for the block to play then start trying
	 * to unprepare the header. this will fail until the block has
	 * played.
	 */
	Sleep(500);
	while (waveOutUnprepareHeader(
		hWaveOut,
		&header,
		sizeof(WAVEHDR)
	) == WAVERR_STILLPLAYING)
		Sleep(100);
}
////
LPSTR loadAudioBlock(const char* filename, DWORD* blockSize)
{
	HANDLE hFile = INVALID_HANDLE_VALUE;
	DWORD size = 0;
	DWORD readBytes = 0;
	void* block = NULL;
	/*
	 * open the file
	 */
	if ((hFile = CreateFile(
		filename,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		0,
		NULL
	)) == INVALID_HANDLE_VALUE)
		return NULL;
	/*
	 * get it's size, allocate memory and read the file
	 * into memory. don't use this on large files!
	 */
	do {
		if ((size = GetFileSize(hFile, NULL)) == 0)
			break;
		if ((block = HeapAlloc(GetProcessHeap(), 0, size)) == NULL)
			break;
		ReadFile(hFile, block, size, &readBytes, NULL);
	} while (0);
	CloseHandle(hFile);
	*blockSize = size;
	return (LPSTR)block;
}
/////

static void CALLBACK waveOutProc(
	HWAVEOUT hWaveOut,
	UINT uMsg,
	DWORD dwInstance,
	DWORD dwParam1,
	DWORD dwParam2
)
{
	/*
	 * pointer to free block counter
	 */
	int* freeBlockCounter = (int*)dwInstance;
	/*
	 * ignore calls that occur due to openining and closing the
	 * device.
	 */
	if (uMsg != WOM_DONE)
		return;
	EnterCriticalSection(&waveCriticalSection);
	(*freeBlockCounter)++;
	LeaveCriticalSection(&waveCriticalSection);
}


WAVEHDR* allocateBlocks(int size, int count)
{
	unsigned char* buffer;
	int i;
	WAVEHDR* blocks;
	DWORD totalBufferSize = (size + sizeof(WAVEHDR)) * count;
	/*
	 * allocate memory for the entire set in one go
	 */
	if ((buffer = HeapAlloc(
		GetProcessHeap(),
		HEAP_ZERO_MEMORY,
		totalBufferSize
	)) == NULL) {
		fprintf(stderr, "Memory allocation error\n");
		ExitProcess(1);
	}
	/*
	 * and set up the pointers to each bit
	 */
	blocks = (WAVEHDR*)buffer;
	buffer += sizeof(WAVEHDR) * count;
	for (i = 0; i < count; i++) {
		blocks[i].dwBufferLength = size;
		blocks[i].lpData = buffer;
		buffer += size;
	}
	return blocks;
}
void freeBlocks(WAVEHDR* blockArray)
{
	/*
	 * and this is why allocateBlocks works the way it does
	 */
	HeapFree(GetProcessHeap(), 0, blockArray);
}



void writeAudio(HWAVEOUT hWaveOut, LPSTR data, int size)
{
	WAVEHDR* current;
	int remain;
	current = &waveBlocks[waveCurrentBlock];
	while (size > 0) {
		/*
		 * first make sure the header we're going to use is unprepared
		 */
		if (current->dwFlags & WHDR_PREPARED)
			waveOutUnprepareHeader(hWaveOut, current, sizeof(WAVEHDR));
		if (size < (int)(BLOCK_SIZE - current->dwUser)) {
			memcpy(current->lpData + current->dwUser, data, size);
			current->dwUser += size;
			break;
		}
		remain = BLOCK_SIZE - current->dwUser;
		memcpy(current->lpData + current->dwUser, data, remain);
		size -= remain;
		data += remain;
		current->dwBufferLength = BLOCK_SIZE;
		waveOutPrepareHeader(hWaveOut, current, sizeof(WAVEHDR));
		waveOutWrite(hWaveOut, current, sizeof(WAVEHDR));
		EnterCriticalSection(&waveCriticalSection);
		waveFreeBlockCount--;
		LeaveCriticalSection(&waveCriticalSection);
		/*
		 * wait for a block to become free
		 */
		while (!waveFreeBlockCount)
			Sleep(10);
		/*
		 * point to the next block
		 */
		waveCurrentBlock++;
		waveCurrentBlock %= BLOCK_COUNT;
		current = &waveBlocks[waveCurrentBlock];
		current->dwUser = 0;
	}
}
