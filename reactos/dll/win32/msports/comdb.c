/*
 * PROJECT:     Ports installer library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll\win32\msports\comdb.c
 * PURPOSE:     COM port database
 * COPYRIGHT:   Copyright 2011 Eric Kohl
 */

#include <windows.h>
#include <msports.h>

#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(msports);

#define BITS_PER_BYTE 8

typedef struct _COMDB
{
    HKEY hKey;
} COMDB, *PCOMDB;


LONG
WINAPI
ComDBClaimNextFreePort(IN HCOMDB hComDB,
                       OUT LPDWORD ComNumber)
{
    PCOMDB pComDB;
    DWORD dwBitIndex;
    DWORD dwByteIndex;
    DWORD dwSize;
    DWORD dwType;
    PBYTE pBitmap = NULL;
    BYTE cMask;
    LONG lError;

    TRACE("ComDBClaimNextFreePort(%p %p)\n", hComDB, ComNumber);

    if (hComDB == INVALID_HANDLE_VALUE ||
        hComDB == NULL ||
        ComNumber == NULL)
        return ERROR_INVALID_PARAMETER;

    pComDB = (PCOMDB)hComDB;

    /* Get the required bitmap size */
    lError = RegQueryValueExW(pComDB->hKey,
                              L"ComDB",
                              NULL,
                              &dwType,
                              NULL,
                              &dwSize);
    if (lError != ERROR_SUCCESS)
        return lError;

    /* Allocate the bitmap */
    pBitmap = HeapAlloc(GetProcessHeap(),
                        HEAP_ZERO_MEMORY,
                        dwSize);
    if (pBitmap == NULL)
    {
        ERR("Failed to allocate the bitmap!\n");
        return lError;
    }

    /* Read the bitmap */
    lError = RegQueryValueExW(pComDB->hKey,
                              L"ComDB",
                              NULL,
                              &dwType,
                              pBitmap,
                              &dwSize);
    if (lError != ERROR_SUCCESS)
        goto done;

    lError = ERROR_INVALID_PARAMETER;
    for (dwBitIndex = 0; dwBitIndex < (dwSize * BITS_PER_BYTE); dwBitIndex++)
    {
        /* Calculate the byte index and a mask for the affected bit */
        dwByteIndex = dwBitIndex / BITS_PER_BYTE;
        cMask = 1 << (dwBitIndex % BITS_PER_BYTE);

        if ((pBitmap[dwByteIndex] & cMask) == 0)
        {
            pBitmap[dwByteIndex] |= cMask;
            *ComNumber = dwBitIndex + 1;
             lError = ERROR_SUCCESS;
             break;
        }
    }

    /* Save the bitmap if it was modified */
    if (lError == ERROR_SUCCESS)
    {
        lError = RegSetValueExW(pComDB->hKey,
                                L"ComDB",
                                0,
                                REG_BINARY,
                                pBitmap,
                                dwSize);
    }

done:;
    if (pBitmap != NULL)
        HeapFree(GetProcessHeap(), 0, pBitmap);

    return lError;
}


LONG
WINAPI
ComDBClaimPort(IN HCOMDB hComDB,
               IN DWORD ComNumber,
               IN BOOL ForceClaim,
               OUT PBOOL Forced)
{
    PCOMDB pComDB;
    DWORD dwBitIndex;
    DWORD dwByteIndex;
    DWORD dwType;
    DWORD dwSize;
    PBYTE pBitmap = NULL;
    BYTE cMask;
    LONG lError;

    TRACE("ComDBClaimPort(%p %lu)\n", hComDB, ComNumber);

    if (hComDB == INVALID_HANDLE_VALUE ||
        hComDB == NULL ||
        ComNumber == 0 ||
        ComNumber > COMDB_MAX_PORTS_ARBITRATED)
        return ERROR_INVALID_PARAMETER;

    pComDB = (PCOMDB)hComDB;

    /* Get the required bitmap size */
    lError = RegQueryValueExW(pComDB->hKey,
                              L"ComDB",
                              NULL,
                              &dwType,
                              NULL,
                              &dwSize);
    if (lError != ERROR_SUCCESS)
        return lError;

    /* Allocate the bitmap */
    pBitmap = HeapAlloc(GetProcessHeap(),
                        HEAP_ZERO_MEMORY,
                        dwSize);
    if (pBitmap == NULL)
    {
        ERR("Failed to allocate the bitmap!\n");
        return lError;
    }

    /* Read the bitmap */
    lError = RegQueryValueExW(pComDB->hKey,
                              L"ComDB",
                              NULL,
                              &dwType,
                              pBitmap,
                              &dwSize);
    if (lError != ERROR_SUCCESS)
        goto done;

    /* Get the bit index */
    dwBitIndex = ComNumber - 1;

    /* Check if the bit to set fits into the bitmap */
    if (dwBitIndex >= (dwSize * BITS_PER_BYTE))
    {
        FIXME("Resize the bitmap\n");

        lError = ERROR_INVALID_PARAMETER;
        goto done;
    }

    /* Calculate the byte index and a mask for the affected bit */
    dwByteIndex = dwBitIndex / BITS_PER_BYTE;
    cMask = 1 << (dwBitIndex % BITS_PER_BYTE);

    lError = ERROR_SHARING_VIOLATION;

    /* Check if the bit is not set */
    if ((pBitmap[dwByteIndex] & cMask) == 0)
    {
        /* Set the bit */
        pBitmap[dwByteIndex] |= cMask;
        lError = ERROR_SUCCESS;
    }

    /* Save the bitmap if it was modified */
    if (lError == ERROR_SUCCESS)
    {
        lError = RegSetValueExW(pComDB->hKey,
                                L"ComDB",
                                0,
                                REG_BINARY,
                                pBitmap,
                                dwSize);
    }

done:
    if (pBitmap != NULL)
        HeapFree(GetProcessHeap(), 0, pBitmap);

    return lError;
}


LONG
WINAPI
ComDBClose(IN HCOMDB hComDB)
{
    PCOMDB pComDB;

    TRACE("ComDBClose(%p)\n", hComDB);

    if (hComDB == HCOMDB_INVALID_HANDLE_VALUE || hComDB == NULL)
        return ERROR_INVALID_PARAMETER;

    pComDB = (PCOMDB)hComDB;

    /* Close the registry key */
    if (pComDB->hKey != NULL)
        RegCloseKey(pComDB->hKey);

    /* Release the database */
    HeapFree(GetProcessHeap(), 0, pComDB);

    return ERROR_SUCCESS;
}


LONG
WINAPI
ComDBGetCurrentPortUsage(IN HCOMDB hComDB,
                         OUT PBYTE Buffer,
                         IN DWORD BufferSize,
                         IN DWORD ReportType,
                         OUT LPDWORD MaxPortsReported)
{
    FIXME("ComDBGetCurrentPortUsage(%p)\n", hComDB);
    return ERROR_CALL_NOT_IMPLEMENTED;
}


LONG
WINAPI
ComDBOpen(OUT HCOMDB *phComDB)
{
    PCOMDB pComDB;
    DWORD dwDisposition;
    DWORD dwType;
    DWORD dwSize;
    PBYTE pBitmap;
    LONG lError;

    TRACE("ComDBOpen(%p)\n", phComDB);

    /* Allocate a new database */
    pComDB = HeapAlloc(GetProcessHeap(),
                       HEAP_ZERO_MEMORY,
                       sizeof(COMDB));
    if (pComDB == NULL)
    {
        ERR("Failed to allocate the database!\n");
        return ERROR_ACCESS_DENIED;
    }

    /* Create or open the database key */
    lError = RegCreateKeyExW(HKEY_LOCAL_MACHINE,
                             L"System\\CurrentControlSet\\Control\\COM Name Arbiter",
                             0,
                             NULL,
                             0,
                             KEY_ALL_ACCESS,
                             NULL,
                             &pComDB->hKey,
                             &dwDisposition);
    if (lError != ERROR_SUCCESS)
        goto done;

    /* Get the required bitmap size */
    lError = RegQueryValueExW(pComDB->hKey,
                              L"ComDB",
                              NULL,
                              &dwType,
                              NULL,
                              &dwSize);
    if (lError == ERROR_FILE_NOT_FOUND)
    {
        /* Allocate a new bitmap */
        dwSize = COMDB_MIN_PORTS_ARBITRATED / BITS_PER_BYTE;
        pBitmap = HeapAlloc(GetProcessHeap(),
                            HEAP_ZERO_MEMORY,
                            dwSize);
        if (pBitmap == NULL)
        {
            ERR("Failed to allocate the bitmap!\n");
            lError = ERROR_ACCESS_DENIED;
            goto done;
        }

        /* Write the bitmap to the registry */
        lError = RegSetValueExW(pComDB->hKey,
                                L"ComDB",
                                0,
                                REG_BINARY,
                                pBitmap,
                                dwSize);

        HeapFree(GetProcessHeap(), 0, pBitmap);
    }

done:;
    if (lError != ERROR_SUCCESS)
    {
        /* Clean up in case of failure */
        if (pComDB->hKey != NULL)
            RegCloseKey(pComDB->hKey);

        HeapFree(GetProcessHeap(), 0, pComDB);

        *phComDB = HCOMDB_INVALID_HANDLE_VALUE;
    }
    else
    {
        /* Return the database handle */
        *phComDB = (HCOMDB)pComDB;
    }

    TRACE("done (Error %lu)\n", lError);

    return lError;
}


LONG
WINAPI
ComDBReleasePort(IN HCOMDB hComDB,
                 IN DWORD ComNumber)
{
    PCOMDB pComDB;
    DWORD dwByteIndex;
    DWORD dwBitIndex;
    DWORD dwType;
    DWORD dwSize;
    PBYTE pBitmap = NULL;
    BYTE cMask;
    LONG lError;

    TRACE("ComDBReleasePort(%p %lu)\n", hComDB, ComNumber);

    if (hComDB == INVALID_HANDLE_VALUE ||
        ComNumber == 0 ||
        ComNumber > COMDB_MAX_PORTS_ARBITRATED)
        return ERROR_INVALID_PARAMETER;

    pComDB = (PCOMDB)hComDB;

    /* Get the required bitmap size */
    lError = RegQueryValueExW(pComDB->hKey,
                              L"ComDB",
                              NULL,
                              &dwType,
                              NULL,
                              &dwSize);
    if (lError != ERROR_SUCCESS)
        return lError;

    /* Allocate the bitmap */
    pBitmap = HeapAlloc(GetProcessHeap(),
                        HEAP_ZERO_MEMORY,
                        dwSize);
    if (pBitmap == NULL)
    {
        ERR("Failed to allocate the bitmap!\n");
        return lError;
    }

    /* Read the bitmap */
    lError = RegQueryValueExW(pComDB->hKey,
                              L"ComDB",
                              NULL,
                              &dwType,
                              pBitmap,
                              &dwSize);
    if (lError != ERROR_SUCCESS)
        goto done;

    /* Get the bit index */
    dwBitIndex = ComNumber - 1;

    /* Check if the bit to set fits into the bitmap */
    if (dwBitIndex >= (dwSize * BITS_PER_BYTE))
    {
        lError = ERROR_INVALID_PARAMETER;
        goto done;
    }

    /* Calculate the byte index and a mask for the affected bit */
    dwByteIndex = dwBitIndex / BITS_PER_BYTE;
    cMask = 1 << (dwBitIndex % BITS_PER_BYTE);

    /* Release the port */
    pBitmap[dwByteIndex] &= ~cMask;

    lError = RegSetValueExW(pComDB->hKey,
                            L"ComDB",
                            0,
                            REG_BINARY,
                            pBitmap,
                            dwSize);

done:;
    if (pBitmap != NULL)
        HeapFree(GetProcessHeap(), 0, pBitmap);

    return lError;
}


LONG
WINAPI
ComDBResizeDatabase(IN HCOMDB hComDB,
                    IN DWORD NewSize)
{
    FIXME("ComDBResizeDatabase(%p %lu)\n", hComDB, NewSize);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/* EOF */