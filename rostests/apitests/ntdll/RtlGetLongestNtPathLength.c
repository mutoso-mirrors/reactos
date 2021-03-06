/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for RtlGetLongestNtPathLength
 * PROGRAMMER:      Thomas Faber <thfabba@gmx.de>
 */

#define WIN32_NO_STATUS
#include <wine/test.h>
#include <ndk/rtlfuncs.h>

/*
ULONG
NTAPI
RtlGetLongestNtPathLength(VOID);
*/

START_TEST(RtlGetLongestNtPathLength)
{
    ULONG Length;

    Length = RtlGetLongestNtPathLength();
    ok(Length == 269, "Length = %lu\n", Length);
}
