#pragma once

namespace Natives
{
    void* /* UObject* */ (__fastcall* StaticConstructObject_Internal)(
        void* Class, // UObject*
        void* InOuter, // UObject*
        FName Name,
        int SetFlags,
        int InternalSetFlags,
        void* Template, // UObject*
        bool bCopyTransientsFromClassDefaults,
        void* InstanceGraph, // FObjectInstancingGraph*
        bool bAssumeTemplateIsArchetype
        );

    void* /* UObject* */ (__fastcall* StaticFindObject)(
        void* Class, // UObject*
        void* InOuter, // UObject*
        const wchar_t* Name,
        bool ExactClass
        );

    void(__fastcall* FMemoryFree)(
        void* a1
        );
    void* (__fastcall* FMemoryRealloc)(
        void* Original,
        SIZE_T Count,
        uint32 Alignment
        );

    void* (__fastcall* FMemoryMalloc)(
        SIZE_T Count,
        uint32 Alignment
        );

    void(__fastcall* ProcessEvent)(
        void* Object,
        void* Function,
        void* Parms
        );
}

namespace FMemory
{
    void Free(void* a1)
    {
        Natives::FMemoryFree(a1);
    }

    void* Realloc(void* Original, SIZE_T Count)
    {
        return Natives::FMemoryRealloc(Original, Count, 0);
    }

    void* Malloc(SIZE_T Count)
    {
        return Natives::FMemoryMalloc(Count, 0);
    }
}

void* ConstructObject(void* Class, void* InOuter)
{
    return Natives::StaticConstructObject_Internal(Class, InOuter, 0, 0, 0, nullptr, false, nullptr, false);
}

void* FindObject(const wchar_t* Name)
{
    return Natives::StaticFindObject(nullptr, nullptr, Name, false);
}

void ProcessEvent(void* obj, void* func, void* parms)
{
    return Natives::ProcessEvent(obj, func, parms);
}