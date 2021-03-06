#include "stdafx.h"
#include "sysinv.h"
#include "smbios.h"

#include <ObjBase.h>

// 7.2.2 System � Wake-up Type 
static LPCTSTR WAKE_UP_TYPE[] = {
	_T("Reserved"),						// 0x00 Reserved
	_T("Other"),						// 0x01 Other
	_T("Unknown"),						// 0x02 Unknown
	_T("APM Timer"),					// 0x03 APM Timer
	_T("Modem Ring"),					// 0x04 Modem Ring
	_T("LAN Remote"),					// 0x05 LAN Remote
	_T("Power Switch"),					// 0x06 Power Switch
	_T("PCI PME#"),						// 0x07 PCI PME#
	_T("AC Power Restored")				// 0x08 AC Power Restored
};

PNODE GetSystemDetail()
{
	TCHAR hostname[MAX_COMPUTERNAME_LENGTH + 1];
	DWORD bufferSize = MAX_COMPUTERNAME_LENGTH + 1;
	SYSTEM_INFO systemInfo;
	PRAW_SMBIOS_DATA smbios = NULL;
	PSMBIOS_STRUCT_HEADER smHeader = NULL;
	SYSTEMTIME systemTime;
	TCHAR szSystemTime[MAX_PATH + 1];
	DWORD cursor = 0;
	LPTSTR pszBuffer = NULL;
	BOOL isWow64;

	PNODE node = node_alloc(L"System", 0);

	// Get host name
	GetComputerName(hostname, &bufferSize);
	node_att_set(node, L"Hostname", hostname, NAFLG_KEY);

	// Get time stamp (Universal full format eg. 2009-06-15T20:45:30Z)
	GetSystemTime(&systemTime);
	if (cursor = GetDateFormat(LOCALE_SYSTEM_DEFAULT, 0, &systemTime, DATE_FORMAT, szSystemTime, MAX_PATH + 1)) {
		szSystemTime[cursor - 1] = 'T';
		if (GetTimeFormat(LOCALE_SYSTEM_DEFAULT, 0, &systemTime, TIME_FORMAT, &szSystemTime[cursor], MAX_PATH + 1)) {
			node_att_set(node, _T("Timestamp"), szSystemTime, NAFLG_FMT_DATETIME);
		}
	}

	// Get system info
	GetNativeSystemInfo(&systemInfo);
	
	// System architecture
	switch(systemInfo.wProcessorArchitecture) {
	case PROCESSOR_ARCHITECTURE_INTEL:
		node_att_set(node, L"Architecture", L"x86", 0);
		break;

	case PROCESSOR_ARCHITECTURE_AMD64:
		node_att_set(node, L"Architecture", L"x86_64", 0);
		break;

	case PROCESSOR_ARCHITECTURE_ARM:
		node_att_set(node, L"Architecture", L"ARM", 0);
		break;

	case PROCESSOR_ARCHITECTURE_IA64:
		node_att_set(node, L"Architecture", L"IA64", 0);
		break;

	default:
		node_att_set(node, L"Architecture", L"Unknown", 0);
	}

	// Make sure 64bit agent is run on 64bit systems
#ifndef _WIN64
	if(IsWow64Process(GetCurrentProcess(), &isWow64) && isWow64)
		SetError(ERR_WARN, 0, _T("Using the 32bit agent version on a 64bit system may return inaccurate results. Please use the 64bit version."));
#endif

	// SMBIOS Info (Type 1)
	smbios = GetSmbiosData();
	if (NULL != (smHeader = GetNextStructureOfType(NULL, SMB_TABLE_SYSTEM))) {
		// 0x04 Manufacturer
		pszBuffer = GetSmbiosString(smHeader, BYTE_AT_OFFSET(smHeader, 0x04));
		node_att_set(node, _T("Manufacturer"), pszBuffer, 0);
		LocalFree(pszBuffer);

		// 0x05 Product Name
		pszBuffer = GetSmbiosString(smHeader, BYTE_AT_OFFSET(smHeader, 0x05));
		node_att_set(node, _T("Product"), pszBuffer, 0);
		LocalFree(pszBuffer);

		// 0x06 Version String
		pszBuffer = GetSmbiosString(smHeader, BYTE_AT_OFFSET(smHeader, 0x06));
		node_att_set(node, _T("Version"), pszBuffer, 0);
		LocalFree(pszBuffer);

		// 0x07 Serial Number
		pszBuffer = GetSmbiosString(smHeader, BYTE_AT_OFFSET(smHeader, 0x07));
		node_att_set(node, _T("SerialNumber"), pszBuffer, 0);
		LocalFree(pszBuffer);

		// SMBIOS v2.1+
		if (smHeader->Length < 0x19) goto smbios_parsed;

		// 0x08 UUID Byte Array
		pszBuffer = (LPTSTR)LocalAlloc(LPTR, sizeof(TCHAR) * 40);
		StringFromGUID2(VAL_AT_OFFET(GUID, smHeader, 0x08), pszBuffer, 40);
		node_att_set(node, _T("Uuid"), pszBuffer, NAFLG_FMT_GUID);
		LocalFree(pszBuffer);

		// 0x18 Wake-up Type
		node_att_set(node, _T("WakeUpType"), SAFE_INDEX(WAKE_UP_TYPE, BYTE_AT_OFFSET(smHeader, 0x18)), 0);

		// v2.4+
		if (smHeader->Length < 0x1B) goto smbios_parsed;
		// 0x19 SKU Number
		pszBuffer = GetSmbiosString(smHeader, BYTE_AT_OFFSET(smHeader, 0x19));
		node_att_set(node, _T("SkuNumber"), pszBuffer, 0);
		LocalFree(pszBuffer);

		// 0x1A Family
		pszBuffer = GetSmbiosString(smHeader, BYTE_AT_OFFSET(smHeader, 0x1A));
		node_att_set(node, _T("Family"), pszBuffer, 0);
		LocalFree(pszBuffer);
	}

smbios_parsed:

	return node;
}