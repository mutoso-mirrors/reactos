/*
 * PROJECT:         ReactOS Boot Loader (FreeLDR)
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            boot/freeldr/freeldr/reactos/archwsup.c
 * PURPOSE:         Routines for ARC Hardware Tree and Configuration Data
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include <freeldr.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

PCONFIGURATION_COMPONENT_DATA FldrArcHwTreeRoot;

/* FUNCTIONS ******************************************************************/

PVOID
NTAPI
FldrpHwHeapAlloc(IN SIZE_T Size)
{
    PVOID Buffer;

    /* Allocate memory from generic bootloader heap */
    Buffer = MmHeapAlloc(Size);
    return Buffer;
}

VOID
NTAPI
FldrSetIdentifier(IN PCONFIGURATION_COMPONENT_DATA ComponentData,
                  IN PCHAR IdentifierString)
{
    SIZE_T IdentifierLength;
    PCONFIGURATION_COMPONENT Component = &ComponentData->ComponentEntry;
    PCHAR Identifier;

    /* Allocate memory for the identifier */
    IdentifierLength = strlen(IdentifierString) + 1;
    Identifier = MmHeapAlloc(IdentifierLength);
    if (!Identifier) return;

    /* Copy the identifier */
    RtlCopyMemory(Identifier, IdentifierString, IdentifierLength);

    /* Set component information */
    Component->IdentifierLength = (ULONG)IdentifierLength;
    Component->Identifier = Identifier;
}

VOID
NTAPI
FldrCreateSystemKey(OUT PCONFIGURATION_COMPONENT_DATA *SystemNode)
{
    PCONFIGURATION_COMPONENT Component;

    /* Allocate the root */
    FldrArcHwTreeRoot = MmHeapAlloc(sizeof(CONFIGURATION_COMPONENT_DATA));
    if (!FldrArcHwTreeRoot) return;

    /* Set it up */
    Component = &FldrArcHwTreeRoot->ComponentEntry;
    Component->Class = SystemClass;
    Component->Type = MaximumType;
    Component->ConfigurationDataLength = 0;
    Component->Identifier = 0;
    Component->IdentifierLength = 0;
    Component->Flags = 0;
    Component->Version = 0;
    Component->Revision = 0;
    Component->Key = 0;
    Component->AffinityMask = 0xFFFFFFFF;

    /* Return the node */
    *SystemNode = FldrArcHwTreeRoot;
}

VOID
NTAPI
FldrLinkToParent(IN PCONFIGURATION_COMPONENT_DATA Parent,
                 IN PCONFIGURATION_COMPONENT_DATA Child)
{
    PCONFIGURATION_COMPONENT_DATA Sibling;

    /* Get the first sibling */
    Sibling = Parent->Child;

    /* If no sibling exists, then we are the first child */
    if (!Sibling)
    {
        /* Link us in */
        Parent->Child = Child;
    }
    else
    {
        /* Loop each sibling */
        do
        {
            /* This is now the parent */
            Parent = Sibling;
        } while ((Sibling = Sibling->Sibling));

        /* Found the lowest sibling; mark us as its sibling too */
        Parent->Sibling = Child;
    }
}

VOID
NTAPI
FldrCreateComponentKey(IN PCONFIGURATION_COMPONENT_DATA SystemNode,
                       IN CONFIGURATION_CLASS Class,
                       IN CONFIGURATION_TYPE Type,
                       IN IDENTIFIER_FLAG Flags,
                       IN ULONG Key,
                       IN ULONG Affinity,
                       IN PCHAR IdentifierString,
                       IN PCM_PARTIAL_RESOURCE_LIST ResourceList,
                       IN ULONG Size,
                       OUT PCONFIGURATION_COMPONENT_DATA *ComponentKey)
{
    PCONFIGURATION_COMPONENT_DATA ComponentData;
    PCONFIGURATION_COMPONENT Component;

    /* Allocate the node for this component */
    ComponentData = MmHeapAlloc(sizeof(CONFIGURATION_COMPONENT_DATA));
    if (!ComponentData) return;

    /* Now save our parent */
    ComponentData->Parent = SystemNode;

    /* Link us to the parent */
    if (SystemNode)
        FldrLinkToParent(SystemNode, ComponentData);

    /* Set us up */
    Component = &ComponentData->ComponentEntry;
    Component->Class = Class;
    Component->Type = Type;
    Component->Flags = Flags;
    Component->Key = Key;
    Component->AffinityMask = Affinity;

    /* Set identifier */
    if (IdentifierString)
        FldrSetIdentifier(ComponentData, IdentifierString);

    /* Set configuration data */
    if (ResourceList)
    {
        ComponentData->ConfigurationData = ResourceList;
        ComponentData->ComponentEntry.ConfigurationDataLength = Size;
    }

    /* Return the child */
    *ComponentKey = ComponentData;
}

