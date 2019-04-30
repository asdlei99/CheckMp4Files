//----------------------------------------------------------------------------------------------
// MFFile.h
//----------------------------------------------------------------------------------------------
#ifndef MFFILE_H
#define MFFILE_H

class CMFWriteFile{

public:

	CMFWriteFile() : m_hFile(INVALID_HANDLE_VALUE){}
	~CMFWriteFile(){ CLOSE_HANDLE_IF(m_hFile); }

	BOOL MFCreateFile(const WCHAR* wszFile){

		BOOL bRet = FALSE;
		CLOSE_HANDLE_IF(m_hFile);

		m_hFile = CreateFile(wszFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

		if(m_hFile == INVALID_HANDLE_VALUE){
			IF_ERROR_RETURN(bRet);
		}

		SetLastError(0);

		return bRet = TRUE;
	}

	BOOL MFWriteFileData(const BYTE* pData, const DWORD dwLength){

		if(m_hFile == INVALID_HANDLE_VALUE)
			return FALSE;

		DWORD dwWritten;

		if(!WriteFile(m_hFile, (LPCVOID)pData, (DWORD)dwLength, &dwWritten, 0) || dwWritten != dwLength)
			return FALSE;

		return TRUE;
	}

	BOOL MFWriteFileDataStartCode4(const BYTE* pData, const DWORD dwLength){

		if(m_hFile == INVALID_HANDLE_VALUE)
			return FALSE;

		DWORD dwWritten;

		BYTE btStartCode[4] = {0x00, 0x00, 0x00, 0x01};

		if(!WriteFile(m_hFile, (LPCVOID)btStartCode, 4, &dwWritten, 0) || dwWritten != 4)
			return FALSE;

		if(!WriteFile(m_hFile, (LPCVOID)pData, (DWORD)dwLength, &dwWritten, 0) || dwWritten != dwLength)
			return FALSE;

		return TRUE;
	}

	BOOL MFWriteFileDataStartCode3(const BYTE* pData, const DWORD dwLength){

		if(m_hFile == INVALID_HANDLE_VALUE)
			return FALSE;

		DWORD dwWritten;

		BYTE btStartCode[3] = {0x00, 0x00, 0x01};

		if(!WriteFile(m_hFile, (LPCVOID)btStartCode, 3, &dwWritten, 0) || dwWritten != 3)
			return FALSE;

		if(!WriteFile(m_hFile, (LPCVOID)pData, (DWORD)dwLength, &dwWritten, 0) || dwWritten != dwLength)
			return FALSE;

		return TRUE;
	}

	void CloseFile(){ CLOSE_HANDLE_IF(m_hFile); }

private:

	HANDLE m_hFile;
};

#endif