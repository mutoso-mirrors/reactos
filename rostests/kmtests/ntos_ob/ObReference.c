/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Object Referencing test
 * PROGRAMMER:      Thomas Faber <thfabba@gmx.de>
 */

#include <kmt_test.h>
#define NDEBUG
#include <debug.h>

#define CheckObject(Handle, Pointers, Handles) do                   \
{                                                                   \
    PUBLIC_OBJECT_BASIC_INFORMATION ObjectInfo;                     \
    Status = ZwQueryObject(Handle, ObjectBasicInformation,          \
                            &ObjectInfo, sizeof ObjectInfo, NULL);  \
    ok_eq_hex(Status, STATUS_SUCCESS);                              \
    ok_eq_ulong(ObjectInfo.PointerCount, Pointers);                 \
    ok_eq_ulong(ObjectInfo.HandleCount, Handles);                   \
} while (0)

static POBJECT_TYPE ObDirectoryObjectType;

static
VOID
TestReference(
    IN HANDLE Handle,
    IN PUNICODE_STRING Name OPTIONAL,
    IN PUNICODE_STRING NameUpper OPTIONAL,
    IN BOOLEAN CaseSensitive,
    IN ULONG AdditionalReferences,
    IN BOOLEAN Permanent)
{
    NTSTATUS Status;
    LONG_PTR Ret;
    PVOID Object = NULL;
    PVOID Object2 = NULL;
    PVOID Object3 = NULL;
    PVOID Object4 = NULL;

    CheckObject(Handle, 2LU + AdditionalReferences, 1LU);

    Status = ObReferenceObjectByHandle(Handle, DIRECTORY_ALL_ACCESS, NULL, KernelMode, &Object, NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok(Object != NULL, "ObReferenceObjectByHandle returned NULL object\n");
    CheckObject(Handle, 3LU + AdditionalReferences, 1LU);

    Status = ObReferenceObjectByHandle(Handle, DIRECTORY_ALL_ACCESS, NULL, KernelMode, &Object2, NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok(Object != NULL, "ObReferenceObjectByHandle returned NULL object\n");
    ok_eq_pointer(Object, Object2);
    CheckObject(Handle, 4LU + AdditionalReferences, 1LU);

    if (!skip(Object != NULL, "No object to reference!\n"))
    {
        Ret = ObReferenceObject(Object);
        ok_eq_longptr(Ret, (LONG_PTR)4 + AdditionalReferences);
        CheckObject(Handle, 5LU + AdditionalReferences, 1LU);

        Ret = ObReferenceObject(Object);
        ok_eq_longptr(Ret, (LONG_PTR)5 + AdditionalReferences);
        CheckObject(Handle, 6LU + AdditionalReferences, 1LU);

        Status = ObReferenceObjectByPointer(Object, DIRECTORY_ALL_ACCESS, NULL, KernelMode);
        ok_eq_hex(Status, STATUS_SUCCESS);
        CheckObject(Handle, 7LU + AdditionalReferences, 1LU);

        Status = ObReferenceObjectByPointer(Object, DIRECTORY_ALL_ACCESS, NULL, KernelMode);
        ok_eq_hex(Status, STATUS_SUCCESS);
        CheckObject(Handle, 8LU + AdditionalReferences, 1LU);

        Ret = ObDereferenceObject(Object);
        ok_eq_longptr(Ret, (LONG_PTR)6 + AdditionalReferences);
        CheckObject(Handle, 7LU + AdditionalReferences, 1LU);

        Ret = ObDereferenceObject(Object);
        ok_eq_longptr(Ret, (LONG_PTR)5 + AdditionalReferences);
        CheckObject(Handle, 6LU + AdditionalReferences, 1LU);

        Ret = ObDereferenceObject(Object);
        ok_eq_longptr(Ret, (LONG_PTR)4 + AdditionalReferences);
        CheckObject(Handle, 5LU + AdditionalReferences, 1LU);

        Ret = ObDereferenceObject(Object);
        ok_eq_longptr(Ret, (LONG_PTR)3 + AdditionalReferences);
        CheckObject(Handle, 4LU + AdditionalReferences, 1LU);
    }

    if (Name && !skip(ObDirectoryObjectType != NULL, "No directory object type\n"))
    {
        Status = ObReferenceObjectByName(Name, 0, NULL, DIRECTORY_ALL_ACCESS, ObDirectoryObjectType, KernelMode, NULL, &Object3);
        ok_eq_hex(Status, STATUS_SUCCESS);
        CheckObject(Handle, 5LU + AdditionalReferences, 1LU);
    }

    if (NameUpper && !skip(ObDirectoryObjectType != NULL, "No directory object type\n"))
    {
        Status = ObReferenceObjectByName(NameUpper, 0, NULL, DIRECTORY_ALL_ACCESS, ObDirectoryObjectType, KernelMode, NULL, &Object4);
        ok_eq_hex(Status, CaseSensitive ? STATUS_OBJECT_NAME_NOT_FOUND : STATUS_SUCCESS);
        CheckObject(Handle, 5LU + AdditionalReferences + !CaseSensitive, 1LU);
    }

    if (NameUpper && !skip(Object4 != NULL, "No object to dereference\n"))
    {
        Ret = ObDereferenceObject(Object4);
        ok_eq_longptr(Ret, (LONG_PTR)4 + AdditionalReferences);
        CheckObject(Handle, 5LU + AdditionalReferences, 1LU);
    }
    if (Name && !skip(Object3 != NULL, "No object to dereference\n"))
    {
        Ret = ObDereferenceObject(Object3);
        ok_eq_longptr(Ret, (LONG_PTR)3 + AdditionalReferences);
        CheckObject(Handle, 4LU + AdditionalReferences, 1LU);
    }
    if (!skip(Object2 != NULL, "No object to dereference\n"))
    {
        Ret = ObDereferenceObject(Object2);
        ok_eq_longptr(Ret, (LONG_PTR)2 + AdditionalReferences);
        CheckObject(Handle, 3LU + AdditionalReferences, 1LU);
    }
    if (!skip(Object != NULL, "No object to dereference\n"))
    {
        Ret = ObDereferenceObject(Object);
        ok_eq_longptr(Ret, (LONG_PTR)1 + AdditionalReferences);
        CheckObject(Handle, 2LU + AdditionalReferences, 1LU);
    }

    CheckObject(Handle, 2LU + AdditionalReferences, 1LU);

    if (Permanent)
    {
        Status = ZwMakeTemporaryObject(Handle);
        ok_eq_hex(Status, STATUS_SUCCESS);
        CheckObject(Handle, 2LU + AdditionalReferences, 1LU);

        Status = ZwMakeTemporaryObject(Handle);
        ok_eq_hex(Status, STATUS_SUCCESS);
        CheckObject(Handle, 2LU + AdditionalReferences, 1LU);
    }
}

START_TEST(ObReference)
{
    NTSTATUS Status;
    NTSTATUS ExceptionStatus;
    HANDLE DirectoryHandle = NULL;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING Name, *pName;
    UNICODE_STRING NameUpper, *pNameUpper;
    ULONG i;
    struct
    {
        PWSTR Name;
        ULONG Flags;
        ULONG AdditionalReferences;
    } Tests[] =
    {
        { NULL,                 0,                                                          0 },
        { NULL,                 OBJ_CASE_INSENSITIVE,                                       0 },
        { NULL,                 OBJ_KERNEL_HANDLE,                                          0 },
        { NULL,                 OBJ_PERMANENT,                                              0 },
        { NULL,                 OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,                       0 },
        { NULL,                 OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,                   0 },
        { NULL,                 OBJ_KERNEL_HANDLE | OBJ_PERMANENT,                          0 },
        { NULL,                 OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE | OBJ_PERMANENT,   0 },
        { L"\\YayDirectory0",   0,                                                          1 },
        { L"\\YayDirectory1",   OBJ_CASE_INSENSITIVE,                                       1 },
        { L"\\YayDirectory2",   OBJ_KERNEL_HANDLE,                                          1 },
        { L"\\YayDirectory3",   OBJ_PERMANENT,                                              1 },
        { L"\\YayDirectory4",   OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,                       1 },
        { L"\\YayDirectory5",   OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,                   1 },
        { L"\\YayDirectory6",   OBJ_KERNEL_HANDLE | OBJ_PERMANENT,                          1 },
        { L"\\YayDirectory7",   OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE | OBJ_PERMANENT,   1 },
    };
    HANDLE ObjectTypeHandle;

    /* ObReferenceObjectByName needs the object type... so get it... */
    RtlInitUnicodeString(&Name, L"\\ObjectTypes\\Directory");
    InitializeObjectAttributes(&ObjectAttributes, &Name, OBJ_KERNEL_HANDLE, NULL, NULL);
    Status = ObOpenObjectByName(&ObjectAttributes, NULL, KernelMode, NULL, 0, NULL, &ObjectTypeHandle);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok(ObjectTypeHandle != NULL, "ObjectTypeHandle = NULL\n");
    if (!skip(Status == STATUS_SUCCESS && ObjectTypeHandle, "No handle\n"))
    {
        Status = ObReferenceObjectByHandle(ObjectTypeHandle, 0, NULL, KernelMode, (PVOID)&ObDirectoryObjectType, NULL);
        ok_eq_hex(Status, STATUS_SUCCESS);
        ok(ObDirectoryObjectType != NULL, "ObDirectoryObjectType = NULL\n");
        Status = ZwClose(ObjectTypeHandle);
        ok_eq_hex(Status, STATUS_SUCCESS);
    }

    for (i = 0; i < sizeof Tests / sizeof Tests[0]; ++i)
    {
        DPRINT("Run %d\n", i);
        if (Tests[i].Name)
        {
            RtlInitUnicodeString(&Name, Tests[i].Name);
            pName = &Name;
            Status = RtlUpcaseUnicodeString(&NameUpper, &Name, TRUE);
            ok_eq_hex(Status, STATUS_SUCCESS);
            if (skip(Status == STATUS_SUCCESS, "No upper case name\n"))
                pNameUpper = NULL;
            else
                pNameUpper = &NameUpper;
        }
        else
        {
            pName = NULL;
            pNameUpper = NULL;
        }
        InitializeObjectAttributes(&ObjectAttributes, pName, Tests[i].Flags, NULL, NULL);
        Status = ZwCreateDirectoryObject(&DirectoryHandle, DIRECTORY_ALL_ACCESS, &ObjectAttributes);
        ok_eq_hex(Status, STATUS_SUCCESS);
        ok(DirectoryHandle != NULL, "DirectoryHandle = NULL\n");

        if (!skip(Status == STATUS_SUCCESS && DirectoryHandle, "Cannot proceed without an object"))
        {
            TestReference(DirectoryHandle, pName, pNameUpper, FALSE, Tests[i].AdditionalReferences, (Tests[i].Flags & OBJ_PERMANENT) != 0);
            /* try again for good measure */
            TestReference(DirectoryHandle, pName, pNameUpper, FALSE, Tests[i].AdditionalReferences, FALSE);

            Status = ZwClose(DirectoryHandle);
            ok_eq_hex(Status, STATUS_SUCCESS);
            DirectoryHandle = NULL;
        }

        if (pNameUpper)
            RtlFreeUnicodeString(pNameUpper);
    }

    if (DirectoryHandle)
    {
        Status = ZwClose(DirectoryHandle);
        ok_eq_hex(Status, STATUS_SUCCESS);
    }

    /* parameter tests */
    /* bugcheck at APC_LEVEL
    Status = ObReferenceObject(NULL);
    Status = ObReferenceObjectByPointer(NULL, 0, NULL, UserMode);
    Status = ObReferenceObjectByPointer(NULL, 0, NULL, KernelMode);*/

    ExceptionStatus = STATUS_SUCCESS;
    _SEH2_TRY {
        /* TODO: this belongs in an ObHandle test if we ever have one */
        /* NtClose must accept everything */
        DPRINT("Closing null handle (NtClose)\n");
        Status = NtClose(NULL);
        ok_eq_hex(Status, STATUS_INVALID_HANDLE);
        DPRINT("Closing null kernel handle (NtClose)\n");
        Status = NtClose((HANDLE)0x80000000);
        ok_eq_hex(Status, STATUS_INVALID_HANDLE);
        DPRINT("Closing -1 handle (NtClose)\n");
        Status = NtClose((HANDLE)0x7FFFFFFF);
        ok_eq_hex(Status, STATUS_INVALID_HANDLE);
        DPRINT("Closing -1 kernel handle (NtClose)\n");
        Status = NtClose((HANDLE)0xFFFFFFFF);
        ok_eq_hex(Status, STATUS_INVALID_HANDLE);
        DPRINT("Closing 123 handle (NtClose)\n");
        Status = NtClose((HANDLE)123);
        ok_eq_hex(Status, STATUS_INVALID_HANDLE);
        DPRINT("Closing 123 kernel handle (NtClose)\n");
        Status = NtClose((HANDLE)(123 | 0x80000000));
        ok_eq_hex(Status, STATUS_INVALID_HANDLE);

        /* ObCloseHandle with UserMode accepts everything */
        DPRINT("Closing null handle (ObCloseHandle, UserMode)\n");
        Status = ObCloseHandle(NULL, UserMode);
        ok_eq_hex(Status, STATUS_INVALID_HANDLE);
        DPRINT("Closing null kernel handle (ObCloseHandle, UserMode)\n");
        Status = ObCloseHandle((HANDLE)0x80000000, UserMode);
        ok_eq_hex(Status, STATUS_INVALID_HANDLE);
        DPRINT("Closing -1 handle (ObCloseHandle, UserMode)\n");
        Status = ObCloseHandle((HANDLE)0x7FFFFFFF, UserMode);
        ok_eq_hex(Status, STATUS_INVALID_HANDLE);
        DPRINT("Closing -1 kernel handle (ObCloseHandle, UserMode)\n");
        Status = ObCloseHandle((HANDLE)0xFFFFFFFF, UserMode);
        ok_eq_hex(Status, STATUS_INVALID_HANDLE);
        DPRINT("Closing 123 handle (ObCloseHandle, UserMode)\n");
        Status = ObCloseHandle((HANDLE)123, UserMode);
        ok_eq_hex(Status, STATUS_INVALID_HANDLE);
        DPRINT("Closing 123 kernel handle (ObCloseHandle, UserMode)\n");
        Status = ObCloseHandle((HANDLE)(123 | 0x80000000), UserMode);
        ok_eq_hex(Status, STATUS_INVALID_HANDLE);

        /* ZwClose only accepts 0 and -1 */
        DPRINT("Closing null handle (ZwClose)\n");
        Status = ZwClose(NULL);
        ok_eq_hex(Status, STATUS_INVALID_HANDLE);
        DPRINT("Closing null kernel handle (ZwClose)\n");
        Status = ZwClose((HANDLE)0x80000000);
        ok_eq_hex(Status, STATUS_INVALID_HANDLE);
        /* INVALID_KERNEL_HANDLE, 0x7FFFFFFF
        Status = ZwClose((HANDLE)0x7FFFFFFF);*/
        DPRINT("Closing -1 kernel handle (ZwClose)\n");
        Status = ZwClose((HANDLE)0xFFFFFFFF);
        ok_eq_hex(Status, STATUS_INVALID_HANDLE);
        /* INVALID_KERNEL_HANDLE, 0x7B, 1, 0, 0
        Status = ZwClose((HANDLE)123);
        Status = ZwClose((HANDLE)(123 | 0x80000000));*/

        /* ObCloseHandle with KernelMode accepts only 0 and -1 */
        DPRINT("Closing null handle (ObCloseHandle, KernelMode)\n");
        Status = ObCloseHandle(NULL, KernelMode);
        ok_eq_hex(Status, STATUS_INVALID_HANDLE);
        DPRINT("Closing null kernel handle (ObCloseHandle, KernelMode)\n");
        Status = ObCloseHandle((HANDLE)0x80000000, KernelMode);
        ok_eq_hex(Status, STATUS_INVALID_HANDLE);
        /* INVALID_KERNEL_HANDLE, 0x7FFFFFFF, 1, 0, 0
        Status = ObCloseHandle((HANDLE)0x7FFFFFFF, KernelMode);*/
        DPRINT("Closing -1 kernel handle (ObCloseHandle, KernelMode)\n");
        Status = ObCloseHandle((HANDLE)0xFFFFFFFF, KernelMode);
        ok_eq_hex(Status, STATUS_INVALID_HANDLE);
        /* INVALID_KERNEL_HANDLE, 0x7B, 1, 0, 0
        Status = ObCloseHandle((HANDLE)123, KernelMode);
        Status = ObCloseHandle((HANDLE)(123 | 0x80000000), KernelMode);*/
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        ExceptionStatus = _SEH2_GetExceptionCode();
    } _SEH2_END;
    ok_eq_hex(ExceptionStatus, STATUS_SUCCESS);

    if (ObDirectoryObjectType)
    {
        ObDereferenceObject(ObDirectoryObjectType);
        ObDirectoryObjectType = NULL;
    }
}
