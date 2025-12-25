#pragma once

#include "UnrealContainers.h"
using namespace UC;

#define SimpleMessageBox(text) MessageBoxA(NULL, text, "FortCmd", MB_OK)
#define SimpleMessageBoxW(text) MessageBoxW(NULL, text, L"FortCmd", MB_OK)

#define INDEX_NONE -1

typedef signed char int8;
typedef signed short int16;
typedef signed int int32;
typedef signed long long int64;
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;

// Hardcoded to 26.30 rn
namespace Offsets
{
    namespace UStruct
    {
        int32 ChildProperties = 0x50;
    }
}

#define ENUM_CLASS_FLAGS(Enum) \
	inline constexpr Enum& operator|=(Enum& Lhs, Enum Rhs) { return Lhs = (Enum)((__underlying_type(Enum))Lhs | (__underlying_type(Enum))Rhs); } \
	inline constexpr Enum& operator&=(Enum& Lhs, Enum Rhs) { return Lhs = (Enum)((__underlying_type(Enum))Lhs & (__underlying_type(Enum))Rhs); } \
	inline constexpr Enum& operator^=(Enum& Lhs, Enum Rhs) { return Lhs = (Enum)((__underlying_type(Enum))Lhs ^ (__underlying_type(Enum))Rhs); } \
	inline constexpr Enum  operator| (Enum  Lhs, Enum Rhs) { return (Enum)((__underlying_type(Enum))Lhs | (__underlying_type(Enum))Rhs); } \
	inline constexpr Enum  operator& (Enum  Lhs, Enum Rhs) { return (Enum)((__underlying_type(Enum))Lhs & (__underlying_type(Enum))Rhs); } \
	inline constexpr Enum  operator^ (Enum  Lhs, Enum Rhs) { return (Enum)((__underlying_type(Enum))Lhs ^ (__underlying_type(Enum))Rhs); } \
	inline constexpr bool  operator! (Enum  E)             { return !(__underlying_type(Enum))E; } \
	inline constexpr Enum  operator~ (Enum  E)             { return (Enum)~(__underlying_type(Enum))E; }

enum EObjectFlags
{
    RF_NoFlags = 0x00000000,
    RF_Public = 0x00000001,
    RF_Standalone = 0x00000002,
    RF_MarkAsNative = 0x00000004,
    RF_Transactional = 0x00000008,
    RF_ClassDefaultObject = 0x00000010,
    RF_ArchetypeObject = 0x00000020,
    RF_Transient = 0x00000040,
    RF_MarkAsRootSet = 0x00000080,
    RF_TagGarbageTemp = 0x00000100,
    RF_NeedInitialization = 0x00000200,
    RF_NeedLoad = 0x00000400,
    RF_KeepForCooker = 0x00000800,
    RF_NeedPostLoad = 0x00001000,
    RF_NeedPostLoadSubobjects = 0x00002000,
    RF_NewerVersionExists = 0x00004000,
    RF_BeginDestroyed = 0x00008000,
    RF_FinishDestroyed = 0x00010000,
    RF_BeingRegenerated = 0x00020000,
    RF_DefaultSubObject = 0x00040000,
    RF_WasLoaded = 0x00080000,
    RF_TextExportTransient = 0x00100000,
    RF_LoadCompleted = 0x00200000,
    RF_InheritableComponentTemplate = 0x00400000,
    RF_DuplicateTransient = 0x00800000,
    RF_StrongRefOnFrame = 0x01000000,
    RF_NonPIEDuplicateTransient = 0x02000000,
    RF_Dynamic = 0x04000000,
    RF_WillBeLoaded = 0x08000000,
    RF_HasExternalPackage = 0x10000000,
    RF_PendingKill = 0x20000000,
    RF_Garbage = 0x40000000,
    RF_AllocatedInSharedPage = 0x80000000,
};
ENUM_CLASS_FLAGS(EObjectFlags);

struct FName
{
    int32 a1;
    // int32 a2;

    std::string ToString();
};

struct FUObjectItem
{
    struct UObject* Object;
    int32 Flags;
    int32 ClusterRootIndex;
    int32 SerialNumber;
};

struct FChunkedFixedUObjectArray
{
    enum
    {
        NumElementsPerChunk = 64 * 1024,
    };

    FUObjectItem** Objects;
    FUObjectItem* PreAllocatedObjects;
    int32 MaxElements;
    int32 NumElements;
    int32 MaxChunks;
    int32 NumChunks;

    FORCEINLINE int32 Num() const
    {
        return NumElements;
    }

    FORCEINLINE bool IsValidIndex(int32 Index) const
    {
        return Index < Num() && Index >= 0;
    }

    FORCEINLINE struct UObject* Get(int32 Index)
    {
        const uint32 ChunkIndex = (uint32)Index / NumElementsPerChunk;
        const uint32 WithinChunkIndex = (uint32)Index % NumElementsPerChunk;
        if (!IsValidIndex(Index)) return nullptr;
        if (!(ChunkIndex < (uint32)NumChunks)) return nullptr;
        if (!(Index < MaxElements)) return nullptr;
        FUObjectItem* Chunk = Objects[ChunkIndex];
        if (!Chunk) return nullptr;
        return (Chunk + WithinChunkIndex)->Object;
    }
};

struct UObject
{
    void** VTable;
    EObjectFlags ObjectFlags;
    int32 InternalIndex;
    UObject* /*UClass*/ ClassPrivate;
    FName NamePrivate;
    UObject* OuterPrivate;

    std::string GetName()
    {
        return NamePrivate.ToString();
    }

    static inline FChunkedFixedUObjectArray* Objects = nullptr;

    static inline UObject* (*StaticFindObject)(UObject* Class, UObject* Outer, const wchar_t* Name, bool ExactClass) = nullptr;
    static inline void (*ProcessEventNative)(UObject*, UObject*, void*) = nullptr;

    template <typename T = UObject*>
    T& GetChild(int32 Offset)
    {
        return *(T*)(int64(this) + Offset);
    }

    static UObject* FindObject(const wchar_t* Name, UObject* Class = nullptr, bool ExactClass = false, UObject* Outer = nullptr)
    {
        if (!StaticFindObject)
        {
            auto Addr = Memcury::Scanner::FindPattern("48 89 74 24 ? 48 89 7C 24 ? 55 41 54 41 55 41 56 41 57 48 8B EC 48 83 EC 60 4C 8B E9 48 8D 4D").Get();

            if (!Addr)
            {
                SimpleMessageBox("Couldn't find StaticFindObject");
                return nullptr;
            }

            StaticFindObject = decltype(StaticFindObject)(Addr);
        }

        return StaticFindObject(Class, Outer, Name, ExactClass);
    }

    void ProcessEvent(UObject* Function, void* Args = nullptr)
    {
        if (!ProcessEventNative)
        {
            auto Addr = Memcury::Scanner::FindPattern("40 55 56 57 41 54 41 55 41 56 41 57 48 81 EC 20 01 00 00 48 8D 6C 24 ? 48 89 9D ? ? ? ? 48 8B 05").Get();

            if (!Addr)
            {
                SimpleMessageBox("Couldn't find ProcessEvent");
                return;
            }

            ProcessEventNative = decltype(ProcessEventNative)(Addr);
        }

        ProcessEventNative(this, Function, Args);
    }
};

std::string FName::ToString()
{
    static auto Lib = UObject::FindObject(L"/Script/Engine.Default__KismetStringLibrary");
    static auto Func = UObject::FindObject(L"/Script/Engine.KismetStringLibrary.Conv_NameToString");
    struct {
        FName InName;
        FString ReturnValue;
    } args {*this};
    Lib->ProcessEvent(Func, &args);
    auto ret = args.ReturnValue.ToString();
    args.ReturnValue.Free();
    return ret;
}
