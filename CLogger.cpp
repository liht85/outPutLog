#include "Logger.h"
#include <time.h>
#include <sys/timeb.h>
#include <vector>
#include <ImageHlp.h>
#include <Shlobj.h>
#pragma comment(lib,"imagehlp.lib")
#pragma comment( lib, "shell32.lib")

BOOL  GetAppDataPath(char *pszDefaultDir) {

	char szDocument[MAX_PATH] = { 0 };
	LPITEMIDLIST pidl = NULL;
	SHGetSpecialFolderLocation(NULL, CSIDL_LOCAL_APPDATA, &pidl);
	if (pidl && SHGetPathFromIDListA(pidl, szDocument))
	{
		return (0 != GetShortPathNameA(szDocument, pszDefaultDir, _MAX_PATH));
	}
	return FALSE;
}

CLogger::CLogger()
{
	strMutexName = "Mutex_Name_2018.08.23";
	InitializeCriticalSection(&m_cs);
	m_hMutex = CreateMutexA(NULL, FALSE, strMutexName.c_str());
	m_strFolderName = "Lenovo\\LenovoWrapper";
	time(&curTime);
	localtime_s(&timeInfo, &curTime);
	BOOL bRet = GetAppDataPath(szFilePath);
	if (!bRet)
	{
		MessageBoxA(NULL, "GetAppDataPath_ERR", "TIPS", NULL);
	}
	strcat_s(szFilePath, sizeof(szFilePath) / sizeof(szFilePath[0]), "\\");
	strcat_s(szFilePath, sizeof(szFilePath) / sizeof(szFilePath[0]), m_strFolderName.c_str());
	strcat_s(szFilePath, sizeof(szFilePath) / sizeof(szFilePath[0]), "\\");
	snprintf(szFileName, sizeof(szFileName), "%s%d%02d%02d.log", szFilePath, timeInfo.tm_year + 1900, timeInfo.tm_mon + 1, timeInfo.tm_mday);

	bRet = MakeSureDirectoryPathExists(szFilePath);	//  文件夹不存在 则创建  路径格式必须 以 \\  结尾 如：“  d:\\ddd1\\log\\ ”   
	if (!bRet)
	{
		MessageBoxA(NULL, "MakeSureDirectoryPathExists", "TIPS", NULL);
	}

}
CLogger::~CLogger()
{
	DeleteCriticalSection(&m_cs);
}

void CLogger::StartMutex(HANDLE hMutex)
{
	if (hMutex != NULL)
	{
		WaitForSingleObject(hMutex, INFINITE);
	}
}
void CLogger::StopMutex(HANDLE hMutex)
{
	if (hMutex != NULL)
	{
		ReleaseMutex(hMutex);
	}
}

void CLogger::SetLogSwitch(bool bswitch)
{
	m_Switch = bswitch;
}

void CLogger::traceEX(BOOL bTAG, char *pFile, int iLineNumb, char *fmt, ...)
{
	if (!m_Switch)
	{
		return;
	}
	EnterCriticalSection(&m_cs);
	StartMutex(m_hMutex);
	string strLog;
	int nWritten = 0;
	char szHead[_MAX_PATH] = { 0 };
	size_t nLength = 0;
	std::vector<char> vBuffer;
	if (bTAG)
	{
		snprintf(szHead, sizeof(szHead), "[INF] [%d-%d] [%d%02d%02d/%02d:%02d:%02d][%-25s- %04d]", GetCurrentProcessId(), GetCurrentThreadId(),
			timeInfo.tm_year + 1900, timeInfo.tm_mon + 1, timeInfo.tm_mday,
			timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec, pFile, iLineNumb);
	}
	else
	{
		snprintf(szHead, sizeof(szHead), "[ERR] [%d-%d] [%d%02d%02d/%02d:%02d:%02d][%-25s- %04d]", GetCurrentProcessId(), GetCurrentThreadId(),
			timeInfo.tm_year + 1900, timeInfo.tm_mon + 1, timeInfo.tm_mday,
			timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec, pFile, iLineNumb);
	}

	if (nullptr == pLogHandle)
	{
		int i = 0;
		fopen_s(&pLogHandle, szFileName, "a+");
		while (nullptr == pLogHandle)
		{
			fopen_s(&pLogHandle, szFileName, "a+");
			Sleep(50);
		}
	}

	fprintf(pLogHandle, "%s\t", szHead);
//	fflush(pLogHandle);
	va_list args;
	va_start(args, fmt);

	nLength = _vscprintf(fmt, args)+2;  // 获取格式化字符串长度
	vBuffer.resize(nLength, '\0');	
	nWritten = _vsnprintf_s(&vBuffer[0], vBuffer.size(),nLength, fmt, args);
	va_end(args);
	if (nWritten > 0)
	{
		strLog = &vBuffer[0];
	}
	fprintf(pLogHandle, "%s\n", strLog.c_str());
	fflush(pLogHandle);
	if (pLogHandle)
	{
		fclose(pLogHandle);
		pLogHandle = nullptr;
	}
	LeaveCriticalSection(&m_cs);
	StopMutex(m_hMutex);
	return;
}