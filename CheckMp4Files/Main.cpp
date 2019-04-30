//----------------------------------------------------------------------------------------------
// Main.cpp
//----------------------------------------------------------------------------------------------
#include "Stdafx.h"

HRESULT ParseFiles(LPCWSTR, CH264AtomParser*);
HRESULT ParseFile(LPCWSTR, CH264AtomParser*);
HRESULT ParseNalu(CH264AtomParser*, const DWORD);

#define USE_DIRECTORY			0
#define COUNT_TO_START_PARSE	0
#define PARSE_NALUNIT			1

#if USE_DIRECTORY
#define INPUT_DIRECTORY L"C:\\"
#else
#define INPUT_FILE L"input.mp4"
#if PARSE_NALUNIT
#define CREATE_RAW_H264_DATA 1
#define USE_3BYTES_STARTCODE 1
#define OUTPUT_FILE L"output.h264"
#endif
#endif

int g_iFileCount = 0;

void WriteSpsPps(CMFWriteFile&, const int, const BYTE*, const DWORD);

inline bool ends_with(std::wstring const & value, std::wstring const & ending){

	if(ending.size() > value.size())
		return false;

	return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

void main(){

	CH264AtomParser* pH264AtomParser = NULL;

	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

	if(SUCCEEDED(hr)){

		hr = MFStartup(MF_VERSION, MFSTARTUP_LITE);

		if(SUCCEEDED(hr)){

			pH264AtomParser = new (std::nothrow)CH264AtomParser;

			if(pH264AtomParser != NULL){

#if USE_DIRECTORY
				hr = ParseFiles(INPUT_DIRECTORY, pH264AtomParser);
#else
				hr = ParseFile(INPUT_FILE, pH264AtomParser);
#endif
				TRACE((L"Total file count : %d", g_iFileCount));
				TRACE((L"-----------------------------------------------------------------------"));
			}

			LOG_HRESULT(MFShutdown());
		}

		CoUninitialize();
	}

	SAFE_DELETE(pH264AtomParser);
}

HRESULT ParseFiles(LPCWSTR wszDirectory, CH264AtomParser* pH264AtomParser){

	HRESULT hr = S_OK;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATA ffd;
	wstring wszCurrentDir = wszDirectory;
	wstring wszSearchDir = wszCurrentDir + L"*";

	hFile = FindFirstFile(wszSearchDir.c_str(), &ffd);

	if(hFile == INVALID_HANDLE_VALUE)
		return hr;

	do{

		if((wcsncmp(L".", ffd.cFileName, 1) != 0) && (wcsncmp(L"..", ffd.cFileName, 2) != 0)){

			if(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){

				wstring wszNewDirectory = wszDirectory;
				wszNewDirectory += ffd.cFileName;
				wszNewDirectory += L"\\";
				ParseFiles(wszNewDirectory.c_str(), pH264AtomParser);
			}
			else{

				wstring wszFile = wszCurrentDir + ffd.cFileName;

				if(ends_with(wszFile, L".mp4"))
					ParseFile(wszFile.c_str(), pH264AtomParser);
			}
		}
	}
	while(FindNextFile(hFile, &ffd) != 0);

	return hr;
}

HRESULT ParseFile(LPCWSTR wszMp4File, CH264AtomParser* pH264AtomParser){

	HRESULT hr = S_OK;

	g_iFileCount++;

#if USE_DIRECTORY
	if(g_iFileCount < COUNT_TO_START_PARSE)
		return hr;
#endif

	TRACE((L"-----------------------------------------------------------------------"));
	TRACE((L"%s", wszMp4File));

	pH264AtomParser->Delete();
	IF_FAILED_RETURN(pH264AtomParser->Initialize(wszMp4File));

	if(FAILED(hr = pH264AtomParser->ParseMp4())){

		TRACE((L"File ERROR : %s", wszMp4File));
	}
	else{

		DWORD dwTrackId;
		IF_FAILED_RETURN(pH264AtomParser->GetFirstVideoStream(&dwTrackId));
		MFTraceTimeString(pH264AtomParser->GetDuration(dwTrackId));

#if PARSE_NALUNIT
		ParseNalu(pH264AtomParser, dwTrackId);
#endif
	}

	TRACE((L"Count = %d", g_iFileCount));

	Sleep(10);

	return hr;
}

HRESULT ParseNalu(CH264AtomParser* pH264AtomParser, const DWORD dwTrackId){

	HRESULT hr = S_OK;
	CH264NaluParser cH264NaluParser;
	int iNaluLenghtSize;
	CMFBuffer pVideoBuffer;
	CMFBuffer pNalUnitBuffer;
	BYTE* pVideoData = NULL;
	DWORD dwBufferSize;
	LONGLONG llTime = 0;
	int iSubSliceCount;
	DWORD dwParsed;
	DWORD dwSampleCount = 0;
	DWORD dwSliceCount = 0;

	const DWORD H264_BUFFER_SIZE = 262144;

#ifdef CREATE_RAW_H264_DATA
	CMFWriteFile cMFWriteFileRaw;
	cMFWriteFileRaw.MFCreateFile(OUTPUT_FILE);
#endif

	iNaluLenghtSize = pH264AtomParser->GetNaluLenghtSize();
	cH264NaluParser.SetNaluLenghtSize(iNaluLenghtSize);

	try{

		IF_FAILED_THROW(pH264AtomParser->GetVideoConfigDescriptor(dwTrackId, &pVideoData, &dwBufferSize));
		IF_FAILED_THROW(cH264NaluParser.ParseVideoConfigDescriptor(pVideoData, dwBufferSize));

#ifdef CREATE_RAW_H264_DATA
		WriteSpsPps(cMFWriteFileRaw, iNaluLenghtSize, pVideoData, dwBufferSize);
#endif

		IF_FAILED_THROW(pVideoBuffer.Initialize(H264_BUFFER_SIZE));
		IF_FAILED_THROW(pNalUnitBuffer.Initialize(H264_BUFFER_SIZE));

		while(pH264AtomParser->GetNextSample(dwTrackId, &pVideoData, &dwBufferSize, &llTime) == S_OK){

			dwSampleCount++;

			IF_FAILED_THROW(pVideoBuffer.Reserve(dwBufferSize));
			memcpy(pVideoBuffer.GetStartBuffer(), pVideoData, dwBufferSize);
			IF_FAILED_THROW(pVideoBuffer.SetEndPosition(dwBufferSize));

#ifdef CREATE_RAW_H264_DATA
#if USE_3BYTES_STARTCODE
			if(iNaluLenghtSize == 4)
				cMFWriteFileRaw.MFWriteFileDataStartCode3(pVideoData + 4, dwBufferSize);
			else
				cMFWriteFileRaw.MFWriteFileDataStartCode3(pVideoData + 2, dwBufferSize);
#else
			if(iNaluLenghtSize == 4)
				cMFWriteFileRaw.MFWriteFileDataStartCode4(pVideoData + 4, dwBufferSize);
			else
				cMFWriteFileRaw.MFWriteFileDataStartCode4(pVideoData + 2, dwBufferSize);
#endif
#endif

			pNalUnitBuffer.Reset();
			iSubSliceCount = 0;

			do{

				if(iSubSliceCount == 0){

					IF_FAILED_THROW(pNalUnitBuffer.Reserve(pVideoBuffer.GetBufferSize()));
					memcpy(pNalUnitBuffer.GetStartBuffer(), pVideoBuffer.GetStartBuffer(), pVideoBuffer.GetBufferSize());
					IF_FAILED_THROW(pNalUnitBuffer.SetEndPosition(pVideoBuffer.GetBufferSize()));
				}

				IF_FAILED_THROW(cH264NaluParser.ParseNaluHeader(pVideoBuffer, &dwParsed));

				if(cH264NaluParser.IsNalUnitCodedSlice()){

					iSubSliceCount++;
					dwSliceCount++;

					if(pVideoBuffer.GetBufferSize() == 0){

						pNalUnitBuffer.SetStartPositionAtBeginning();
					}
					else{

						// Sub-slices
						IF_FAILED_THROW(pNalUnitBuffer.SetStartPosition(dwParsed));
					}
				}
				else{

					if(iSubSliceCount > 0){

						// Can be NAL_UNIT_FILLER_DATA after sub-slices
						pNalUnitBuffer.SetStartPositionAtBeginning();

						// We assume sub-slices are contiguous, so skip others
						pVideoBuffer.Reset();
					}
					else{

						pNalUnitBuffer.Reset();
					}
				}

				if(hr == S_FALSE){

					// S_FALSE means slice is corrupted. Just clear previous frames presentation, sometimes it's ok to continue
					hr = S_OK;
				}
				else if(pVideoBuffer.GetBufferSize() != 0){

					// Some slice contains SEI message and sub-slices, continue parsing
					hr = S_FALSE;
				}
			}
			while(hr == S_FALSE);
		}
	}
	catch(HRESULT){}

#ifdef CREATE_RAW_H264_DATA
	cMFWriteFileRaw.CloseFile();
#endif

	TRACE((L"ParseNalu : track %u - sample %u - slices %s - resolution %lux%lu",
		pH264AtomParser->GetTrackSampleSize(dwTrackId),
		dwSampleCount,
		(dwSliceCount != dwSampleCount ? L"true" : L"false"),
		cH264NaluParser.GetWidth(),
		cH264NaluParser.GetHeight()
	));

	return hr;
}

void WriteSpsPps(CMFWriteFile& cMFWriteFileRaw, const int iNaluLenghtSize, const BYTE* pVideoData, const DWORD dwBufferSize){

	DWORD dwNaluSize = 0;
	const BYTE* pData = NULL;
	DWORD dwLastSize;

	if(iNaluLenghtSize == 4){

		dwNaluSize = MAKE_DWORD(pVideoData);

		assert(dwNaluSize);
		assert((dwNaluSize + 8) < dwBufferSize);
	}
	else{

		dwNaluSize = pVideoData[0] << 8;
		dwNaluSize |= pVideoData[1];

		assert(dwNaluSize);
		assert((dwNaluSize + 4) < dwBufferSize);
	}

#if USE_3BYTES_STARTCODE
	if(iNaluLenghtSize == 4)
		cMFWriteFileRaw.MFWriteFileDataStartCode3(pVideoData + 4, dwNaluSize);
	else
		cMFWriteFileRaw.MFWriteFileDataStartCode3(pVideoData + 2, dwNaluSize);
#else
	if(iNaluLenghtSize == 4)
		cMFWriteFileRaw.MFWriteFileDataStartCode4(pVideoData + 4, dwNaluSize);
	else
		cMFWriteFileRaw.MFWriteFileDataStartCode4(pVideoData + 2, dwNaluSize);
#endif

	pData = pVideoData + dwNaluSize;
	pData += (iNaluLenghtSize == 4 ? 8 : 4);

	dwLastSize = (dwBufferSize - dwNaluSize) - (iNaluLenghtSize == 4 ? 8 : 4);

#if USE_3BYTES_STARTCODE
	if(iNaluLenghtSize == 4)
		cMFWriteFileRaw.MFWriteFileDataStartCode3(pData, dwLastSize);
	else
		cMFWriteFileRaw.MFWriteFileDataStartCode3(pData, dwLastSize);
#else
	if(iNaluLenghtSize == 4)
		cMFWriteFileRaw.MFWriteFileDataStartCode4(pData, dwLastSize);
	else
		cMFWriteFileRaw.MFWriteFileDataStartCode4(pData, dwLastSize);
#endif
}