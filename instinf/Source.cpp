/*
* Source.cpp
* author:
* This code is an reverse research of devcon64.exe. 
* ALL RIGHTS ARE RESERVED BY MicroSoft.
* 
* Description
* This demo, for install an inf file on windows10-x64.
* The inf file is a config file for windows drivers.So you can install driver by right click on this file.
* But, sometimes may want to install driver programmly, that's why this piece of code were created.
*/
#include <Windows.h>
#include <setupapi.h >
#include <newdev.h>
#include <tchar.h>
#include <stdio.h>
#pragma comment(lib, "Setupapi.lib")
#pragma comment(lib, "Newdev.lib")

/*
* An function pointer for Win API UpdateDriverForPlugAndPlayDevicesW
*/
typedef BOOL(*pfnUpdateDriverForPlugAndPlayDevices)(HWND, LPCTSTR, LPCTSTR, DWORD, PBOOL);

int main(int argc, char** argv) {
	LPCTSTR lpszInfPath = TEXT("path_to_inf_file.inf");
	TCHAR szBuffer[MAX_PATH] = { 0x0 };
	GetFullPathName(lpszInfPath, MAX_PATH, szBuffer, NULL);

	GUID guid;
	TCHAR szClassName[32] = { 0x0 };
	BOOL bRet = SetupDiGetINFClass(szBuffer, &guid, szClassName, 32, 0);
	if (!bRet) {
		return -1;
	}

	HDEVINFO hDevInfo = SetupDiCreateDeviceInfoList(&guid, NULL);
	if (INVALID_HANDLE_VALUE == hDevInfo) {
		return -1;
	}

	SP_DEVINFO_DATA sp{ 0 };
	sp.cbSize = sizeof(sp);
	bRet = SetupDiCreateDeviceInfo(hDevInfo, szClassName, &guid, NULL, NULL, DICD_GENERATE_ID, &sp);
	if (!bRet) {
		// int Error = GetLastError();
		SetupDiDestroyDeviceInfoList(hDevInfo);
		return -1;
	}

	/*
	* if you install inf file manually, the command may: devcon64.exe install xxx.inf HardwareId
	* The value , of HardwareId, is coming from DeviceList.NTAMD64 in xxx.inf, like this:
	* [DeviceList.NTAMD64]
	* %lyVirtualDevice.DeviceDesc64%=lyVD,lyVirtualDeviceId
	* So, the 'HardwareId' is [lyVirtualDeviceId]
	*/
	LPCTSTR lpszDeviceName = TEXT("HardwareID");
	TCHAR szHardWareId[MAX_PATH] = { 0x0 };
	_tcscpy_s(szHardWareId, lpszDeviceName);
	size_t cbDeviceNameLen = lstrlenW(lpszDeviceName) * sizeof(TCHAR);
	bRet = SetupDiSetDeviceRegistryProperty(hDevInfo, &sp, SPDRP_HARDWAREID, (const BYTE*)lpszDeviceName, (DWORD)cbDeviceNameLen);
	if (!bRet) {
		// int Error = GetLastError();
		SetupDiDestroyDeviceInfoList(hDevInfo);
		return -1;
	}

	bRet = SetupDiCallClassInstaller(0x19, hDevInfo, &sp);
	if (!bRet) {
		// int Error = GetLastError();
		SetupDiDestroyDeviceInfoList(hDevInfo);
		return -1;
	}


	HMODULE hNewDevDll = LoadLibrary(TEXT("newdev.dll"));
	if (NULL == hNewDevDll) {
		// error
		return -1;
	}
	pfnUpdateDriverForPlugAndPlayDevices func =
		(pfnUpdateDriverForPlugAndPlayDevices)GetProcAddress(hNewDevDll, "UpdateDriverForPlugAndPlayDevicesW");
	if (NULL == func) {
		// error
		FreeLibrary(hNewDevDll);
		return -1;
	}
	BOOL bRequireReboot = FALSE;
	bRet = func(NULL, szHardWareId, szBuffer, INSTALLFLAG_FORCE, &bRequireReboot);
	if (!bRet) {
		int error = GetLastError();
		printf("UpdateDriverForPlugAndPlayDevicesW Failed with:0x%08x\n", error);
	}
	FreeLibrary(hNewDevDll);
	SetupDiDestroyDeviceInfoList(hDevInfo);
	return 0;
}