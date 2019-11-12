// by Doug Cox Apr 15, 2014 http://jdmcox.com
// using Visual Studio 2008 (I had to run Visual Studio as administrator to run this in debug mode)
// you should right-click on volume icon on Windows task bar, select Playback devices, select the Default Device, and in its Properties select Advanced, and then check the box or boxes in Exclusive Mode (exclusive mode needs this and it doesn't matter in sharing mode)
// Use Unicode Character Set
//		if (hr = pAudioClient->GetMixFormat(&internalwaveformat) != S_OK) goto Exit; // Windows automatically converts to and from its internal wave format
//		GetMixFormat is never necessary: (uses *internalwaveformat = &InternalWaveFormat.format; WAVEFORMATEXTENSIBLE InternalWaveFormat;
//		GUID G = ((WAVEFORMATEXTENSIBLE*)internalwaveformat)->SubFormat; // Data 1 is 1 for int, and 3 for float
//		WORD V = ((WAVEFORMATEXTENSIBLE*)internalwaveformat)->Samples.wValidBitsPerSample;
//		DWORD S = ((WAVEFORMATEXTENSIBLE*)internalwaveformat)->dwChannelMask;
#include <stdio.h>
#include <mmDeviceapi.h>
#include <audioclient.h>
#include <audiopolicy.h>
#include <avrt.h> // add avrt.lib to Project Properties, Linker, Input, Additional Dependencies

#define REFTIMES_PER_SEC  10000000.0 // 10,000,000
#define REFTIMES_PER_MILLISEC  10000 // 10,000
#define SAFE_RELEASE(punk)  if ((punk) != NULL) { (punk)->Release(); (punk) = NULL; }

const CLSID CLSID_MMDeviceEnumerator = __uuidof(IMMDeviceEnumerator)
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator)
const IID IID_IAudioClient = __uuidof(IAudioClient)
const IID IID_IAudioRenderClient = __uuidof(IAudioRenderClient)

int sw_main()
{
	BOOL exclusivemode, eventcallback;
	DWORD x, fileSize, BufferSize, dwBytesRead, taskIndex = 0;
	DWORD subchunksize, nextchunk, WavePtr, WaveLength, WaveEnd;
	UINT32 nFramesInFile, nFramesInBuffer, nBytesInBuffer, nFramesPlayed, nFramesPlaying, nFramesAvailable, nBytesAvailable;
	BYTE* pData = NULL, * WaveBuf = NULL;
	TCHAR WaveFile[256];
	char Format[] = "16 bits  44.1 kHz  2 channels\n\r";
	HANDLE hEvent = NULL;
	HANDLE hFile = NULL, hTask = NULL;
	HRESULT hr;
	REFERENCE_TIME hnsPeriod = 0;
	IMMDeviceEnumerator* pEnumerator = NULL;
	IMMDevice* pDevice = NULL;
	IAudioClient* pAudioClient = NULL;
	IAudioRenderClient* pAudioRenderClient = NULL;
	WAVEFORMATEXTENSIBLE WaveFormat;
	WAVEFORMATEX* waveformat, * internalwaveformat;

	fputs("Enter WAVE file name (presuming it's in this folder): ", stdout);
	fgetws(WaveFile, 256, stdin); // fgetws because it's compiled as Unicode
	for (x = 0; WaveFile[x] != 0; x++)
		;
	WaveFile[x - 1] = 0; // clear bad char
	// add .wav if necessary
	if ((WaveFile[x - 4] != '.') || (WaveFile[x - 3] != 'a') || (WaveFile[x - 2] != 'v')) {
		WaveFile[x - 1] = '.';
		WaveFile[x] = 'w';
		WaveFile[x + 1] = 'a';
		WaveFile[x + 2] = 'v';
		WaveFile[x + 3] = 0;
	}
	hFile = CreateFile(WaveFile, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile != INVALID_HANDLE_VALUE) {
		if (fileSize = GetFileSize(hFile, NULL)) {
			BufferSize = fileSize;
			WaveBuf = (BYTE*)VirtualAlloc(NULL, BufferSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			ReadFile(hFile, WaveBuf, fileSize, &dwBytesRead, NULL);
			if (*(DWORD*)& WaveBuf[8] == 0x45564157) {// "WAVE"
				if ((*(WORD*)& WaveBuf[20] == 1) || (*(WORD*)& WaveBuf[20] == 3) || (*(WORD*)& WaveBuf[20] == 0xFFFE)) {// PCM or WAVE_FORMAT_IEEE_FLOAT or WAVEFORMATEXTENSIBLE
					subchunksize = *(DWORD*)& WaveBuf[16];
					nextchunk = subchunksize + 20;
					if (*(DWORD*)& WaveBuf[nextchunk] == 0x74636166)// "fact"
						nextchunk += 12; // ignore fact chunk
					if (*(DWORD*)& WaveBuf[nextchunk] == 0x61746164) {// "data"
						WaveLength = *(DWORD*)& WaveBuf[nextchunk + 4];
						WavePtr = nextchunk + 8;
						WaveEnd = WavePtr + WaveLength;
						WaveFormat.Format.wFormatTag = *(WORD*)& WaveBuf[20];
						WaveFormat.Format.nChannels = *(WORD*)& WaveBuf[22];
						WaveFormat.Format.nSamplesPerSec = *(DWORD*)& WaveBuf[24];
						WaveFormat.Format.nAvgBytesPerSec = *(DWORD*)& WaveBuf[28];
						WaveFormat.Format.nBlockAlign = *(WORD*)& WaveBuf[32];
						WaveFormat.Format.wBitsPerSample = *(WORD*)& WaveBuf[34];
						if ((WaveFormat.Format.wFormatTag != WAVE_FORMAT_EXTENSIBLE))
							WaveFormat.Format.cbSize = 0;
						else {
							WaveFormat.Format.cbSize = 22; // sizeof(WAVEFORMATEXTENSIBLE);
							WaveFormat.Samples.wValidBitsPerSample = *(WORD*)& WaveBuf[38];
							WaveFormat.dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
							WaveFormat.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
						}
					}
					else goto Exit;
				}
				else goto Exit;
			}
			else goto Exit;
		}
		else goto Exit;
		CloseHandle(hFile);
		hFile = NULL;
	}
	else {
		fputs("file not found\r\npress Enter to exit", stdout);
		fgetc(stdin);
		goto Exit;
	}

	CoInitialize(NULL);
	if (hr = CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)& pEnumerator) != S_OK) goto Exit;
	if (hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice) != S_OK) goto Exit;
	if (hr = pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)& pAudioClient) != S_OK) goto Exit;

	if (hr = pAudioClient->GetMixFormat(&internalwaveformat) != S_OK) goto Exit; // Windows automatically converts to and from its internal wave format

	waveformat = &WaveFormat.Format;
	Format[0] = (waveformat->wBitsPerSample / 10) + '0';
	Format[1] = (waveformat->wBitsPerSample % 10) + '0';
	if (waveformat->nSamplesPerSec == 48000) {
		Format[9] = '4';
		Format[10] = '8';
		Format[11] = '.';
		Format[11] = '.';
		Format[12] = '0';
	}
	Format[19] = waveformat->nChannels + '0';
	if (waveformat->nSamplesPerSec != internalwaveformat->nSamplesPerSec)
	{
		if (waveformat->nSamplesPerSec == 44100)
			fputs("Your soundcard Samples/Second isn't set at 44.1kHz  Press Enter to Exit\n", stdout);
		else if (waveformat->nSamplesPerSec == 48000)
			fputs("Your soundcard Samples/Second isn't set at 48kHz  Press Enter to Exit\n", stdout);
		//MessageBox(NULL, TEXT(""), TEXT(""), MB_OK);
		fgetc(stdin);
		goto Exit;
	}

	fputs("Use Exclusive Mode? (Y/N): ", stdout);
	while (true) {
		x = fgetc(stdin);
		if ((x == 0x79) || (x == 0x59)) // y or Y
			exclusivemode = true;
		else
			exclusivemode = false;
		break;
	};
	x = fgetc(stdin); // clear bad char
	fputs("Use Event Callback? (Y/N): ", stdout);
	while (true) {
		x = fgetc(stdin);
		if ((x == 0x79) || (x == 0x59)) // y or Y
			eventcallback = true;
		else
			eventcallback = false;
		break;
	};
	x = fgetc(stdin); // clear bad char

	if (exclusivemode) {
		if (hr = pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, waveformat, NULL) != S_OK) {
			goto Exit;
		}
		if (hr = pAudioClient->GetDevicePeriod(NULL, &hnsPeriod) != S_OK) goto Exit;
		if (eventcallback) {
			if ((hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_EXCLUSIVE, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, hnsPeriod, hnsPeriod, waveformat, NULL)) != S_OK) {
				if (hr == 0x88890019) { // AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED
					if (hr = pAudioClient->GetBufferSize(&nFramesInBuffer) != S_OK) goto Exit;
					hnsPeriod = (REFERENCE_TIME)((REFTIMES_PER_SEC * nFramesInBuffer / waveformat->nSamplesPerSec) + 0.5);
					if (hr = pAudioClient->Release() != S_OK) goto Exit;
					if (hr = pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)& pAudioClient) != S_OK) goto Exit;
					if ((hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_EXCLUSIVE, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, hnsPeriod, hnsPeriod, waveformat, NULL)) != S_OK) goto Exit;
				}
				else { // shouldn't happen
					if (hr = pAudioClient->Release() != S_OK) goto Exit;
					if (hr = pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)& pAudioClient) != S_OK) goto Exit;
					if ((hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_EXCLUSIVE, 0, hnsPeriod, 0, waveformat, NULL)) != S_OK) goto Exit;
					eventcallback = false;
					fputs("Not using Event Callback\n\r", stdout);
				}
			}
		}
		else { // not eventcallback
			if (hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_EXCLUSIVE, 0, hnsPeriod, 0, waveformat, NULL) != S_OK) goto Exit;
		}
	} // end of if (exclusivemode)

	else { // if shared mode
//		if (hr = pAudioClient->GetMixFormat(&internalwaveformat) != S_OK) goto Exit; // Windows automatically converts to and from its internal wave format
//		if ((hr = pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, waveformat, &internalwaveformat)) != S_OK) {
//			if (hr != S_FALSE) { // S_FALSE would mean &internalwaveformat was good, so using it
//				goto Exit;
//			}
//		} NOT NECESSARY
		if (hr = pAudioClient->GetDevicePeriod(&hnsPeriod, NULL) != S_OK) goto Exit;
		if (eventcallback) {
			if (hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, 0, 0, waveformat, NULL) != S_OK) goto Exit;
		}
		else {
			if (hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, hnsPeriod, 0, waveformat, NULL) != S_OK) goto Exit;
		}
	}

	if (hr = pAudioClient->GetBufferSize(&nFramesInBuffer) != S_OK) goto Exit;
	fputs(Format, stdout);
	nFramesInFile = WaveLength / waveformat->nBlockAlign;
	if (eventcallback) {
		hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		if (hr = pAudioClient->SetEventHandle(hEvent) != S_OK) goto Exit;
	}
	if (hr = pAudioClient->GetService(IID_IAudioRenderClient, (void**)& pAudioRenderClient) != S_OK) goto Exit;
	nBytesInBuffer = nFramesInBuffer * waveformat->nBlockAlign;
	if (hr = pAudioRenderClient->GetBuffer(nFramesInBuffer, &pData) != S_OK) goto Exit;
	memset(pData, 0, nFramesInBuffer);
	if (pAudioRenderClient->ReleaseBuffer(nFramesInBuffer, 0) != S_OK) goto Exit;
	if (exclusivemode) {
		if ((hTask = AvSetMmThreadCharacteristics(TEXT("Pro Audio"), &taskIndex)) == NULL) goto Exit;
	}
	if (hr = pAudioClient->Start() != S_OK) goto preExit;

	if (eventcallback) {
		if (exclusivemode) {
			for (nFramesPlayed = 0; nFramesPlayed < nFramesInFile; nFramesPlayed += nFramesInBuffer) {
				if (WAIT_OBJECT_0 != WaitForSingleObject(hEvent, 2000)) goto preExit;
				if (hr = pAudioRenderClient->GetBuffer(nFramesInBuffer, &pData) != S_OK) goto preExit;
				nBytesAvailable = nFramesInBuffer * waveformat->nBlockAlign; // this is needed because WavePtr is a DWORD, not a BYTE
				memcpy(pData, &WaveBuf[WavePtr], nBytesAvailable);
				WavePtr += nBytesAvailable;
				if (pAudioRenderClient->ReleaseBuffer(nFramesInBuffer, 0) != S_OK) goto preExit;
			}
		}
		else { // shared mode
			for (nFramesPlayed = 0; nFramesPlayed < nFramesInFile; nFramesPlayed += nFramesAvailable) {
				if (WAIT_OBJECT_0 != WaitForSingleObject(hEvent, 2000)) goto preExit;
				if (hr = pAudioClient->GetCurrentPadding(&nFramesPlaying) != S_OK) goto preExit;
				nFramesAvailable = nFramesInBuffer - nFramesPlaying;
				if (nFramesAvailable > (nFramesInFile - nFramesPlayed))
					nFramesAvailable = nFramesInFile - nFramesPlayed;
				if (hr = pAudioRenderClient->GetBuffer(nFramesAvailable, &pData) != S_OK) goto preExit;
				nBytesAvailable = nFramesAvailable * waveformat->nBlockAlign;
				memcpy(pData, &WaveBuf[WavePtr], nBytesAvailable);
				WavePtr += nBytesAvailable;
				if (hr = pAudioRenderClient->ReleaseBuffer(nFramesAvailable, 0) != S_OK) goto preExit;
			}
		}
	}
	else { // not using event callback
		for (nFramesPlayed = 0; nFramesPlayed < nFramesInFile; nFramesPlayed += nFramesAvailable) {
			if (!exclusivemode)
				Sleep((DWORD)(hnsPeriod / REFTIMES_PER_MILLISEC / 2));
			if (hr = pAudioClient->GetCurrentPadding(&nFramesPlaying) != S_OK) goto preExit;
			nFramesAvailable = nFramesInBuffer - nFramesPlaying;
			if (nFramesAvailable > (nFramesInFile - nFramesPlayed))
				nFramesAvailable = nFramesInFile - nFramesPlayed;
			if (hr = pAudioRenderClient->GetBuffer(nFramesAvailable, &pData) != S_OK) goto preExit;
			nBytesAvailable = nFramesAvailable * waveformat->nBlockAlign;
			memcpy(pData, &WaveBuf[WavePtr], nBytesAvailable);
			WavePtr += nBytesAvailable;
			if (hr = pAudioRenderClient->ReleaseBuffer(nFramesAvailable, 0) != S_OK) goto preExit;
		}
		Sleep((DWORD)(hnsPeriod / REFTIMES_PER_MILLISEC));
	}
preExit:
	pAudioClient->Stop();
Exit:
	if (hFile)
		CloseHandle(hFile);
	if (hEvent)
		CloseHandle(hEvent);
	if (hTask)
		AvRevertMmThreadCharacteristics(hTask);
	if (WaveBuf)
		VirtualFree(WaveBuf, 0, MEM_RELEASE);
	if (pData)
		VirtualFree(pData, 0, MEM_RELEASE);
	SAFE_RELEASE(pEnumerator)
		SAFE_RELEASE(pDevice)
		SAFE_RELEASE(pAudioClient)
		SAFE_RELEASE(pAudioRenderClient)
		return 0;
}