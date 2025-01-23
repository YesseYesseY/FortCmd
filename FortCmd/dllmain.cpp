#include <Windows.h>
#include <stdlib.h>
#include <format>
#include <string>

#include "memcury.h"

#define ALLOC_CONSOLE

#define CheckAddr(Addr, FailMsg) if (!Addr) { BasicMessageBox(FailMsg); return 0; }

#ifdef ALLOC_CONSOLE
#define PRINT(str, ...) printf(str "\n", __VA_ARGS__)
#else
#define PRINT(str)
#endif

#define BasicMessageBox(Msg) MessageBoxA(NULL, Msg, "FortCmd", MB_OK)

#define KeyPressed(Key) (GetAsyncKeyState(VK_F9) & 1)

#define GET_OFFSETPTR(Type, Base, Offset) ((Type*)((uint64)Base + Offset))
#define GET_OFFSET(Type, Base, Offset) (*GET_OFFSETPTR(Type, Base, Offset))

typedef signed char int8;
typedef signed short int16;
typedef signed int int32;
typedef signed long long int64;
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;

typedef int64 FName;

namespace Offsets
{
    int32 UFieldNextOffset = 40;
    int32 ChildrenOffset = 56;
    int32 ClassPrivateOffset = 16;
    int32 NamePrivateOffset = 24;
    int32 FunctionFlagsOffset = 136;
    int32 ManualAutoCompleteListOffset = 48;
    int32 AutoCompleteMapPathsOffset = 64;
    int32 AutoCompleteCommandColorOffset = 96;
    int32 AutoCompleteCVarColorOffset = 100;
    int32 AutoCompleteFadedColorOffset = 104;
    int32 GameViewportClientOffset = 1832;
    int32 ViewportConsoleOffset = 64;
    int32 PropertyDataOffset = 112;
    int32 PropertyFlagsOffset = 56;
    int32 ConsoleObjectsOffset = 8;
}

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

void(__fastcall * FMemoryFree)(
    void* a1
    );
void* (__fastcall * FMemoryRealloc)(
    void* Original,
    SIZE_T Count,
    uint32 Alignment
    );

void* (__fastcall* FMemoryMalloc)(
    SIZE_T Count,
    uint32 Alignment
    );

void(__fastcall * ProcessEvent)(
    void* Object,
    void* Function,
    void* Parms
    );

template <typename T>
struct TArray
{
    T* Data;
    int32 ArrayNum;
    int32 ArrayMax;

    FORCEINLINE TArray()
    {
        Data = nullptr;
        ArrayNum = 0;
        ArrayMax = 0;
    }
    
    FORCEINLINE TArray(int32 size)
    {
        Data = (T*)FMemoryMalloc(sizeof(T) * size, 0);
        ArrayNum = 0;
        ArrayMax = sizeof(T);
    }

    FORCEINLINE int32 Num() const
    {
        return ArrayNum;
    }

    FORCEINLINE int32 Max()
    {
        return ArrayMax;
    }

    FORCEINLINE T* GetData()
    {
        return Data;
    }
    
    FORCEINLINE T& operator[](int32 Index)
    {
        return GetData()[Index];
    }

    FORCEINLINE void IncreaseSize(int32 amount)
    {
        Data = (T*)FMemoryRealloc(Data, sizeof(T) * (ArrayMax + amount), 0);
        ArrayMax += amount;
    }

    FORCEINLINE void Add(T Val)
    {
        if (ArrayMax <= ArrayNum)
        {
            IncreaseSize(1);
        }
        Data[ArrayNum++] = Val;
    }

    FORCEINLINE void Free()
    {
        if (Data)
            FMemoryFree(Data);
        ArrayNum = 0;
        ArrayMax = 0;
    }

};

struct FString
{
    TArray<wchar_t> Data;

    FORCEINLINE FString()
    {

    }

    FORCEINLINE FString(const wchar_t* Str)
    {
        int32 size = wcslen(Str) + 1;
        Data = TArray<wchar_t>(size);
        memcpy(Data.Data, Str, sizeof(wchar_t) * size);
        Data.ArrayNum = size;
        Data.ArrayMax = size;
    }

    FORCEINLINE int32 Len() const
    {
        return Data.Num() ? Data.Num() - 1 : 0;
    }

    FORCEINLINE TCHAR& operator[](int32 Index)
    {
        return Data.GetData()[Index];
    }

    // How this is implemented doesnt really matter, it's not gonna be used in the final version anyways
    FORCEINLINE std::string ToString()
    {
        auto wstr = std::wstring(Data.Data);
        return std::string(wstr.begin(), wstr.end());
    }

    FORCEINLINE FString& operator+=(const wchar_t* Str)
    {
        int32 oldlen = Len();
        int32 newsize = wcslen(Str) + 1;
        Data.IncreaseSize(newsize);
        memcpy(
            (void*)((uint64)Data.Data + (oldlen * sizeof(wchar_t))),
            Str,
            newsize * sizeof(wchar_t)
        );
        Data.ArrayNum = oldlen + newsize;
        return *this;
    }

    FORCEINLINE void Free()
    {
        Data.Free();
    }

    FORCEINLINE FString& operator+=(FString Str)
    {
        return *this += Str.Data.Data;
    }
};

template<int32 Size, uint32 Alignment>
struct alignas(Alignment) TAlignedBytes
{
    uint8 Pad[Size];
};

template<typename ElementType>
struct TTypeCompatibleBytes :
    public TAlignedBytes<
    sizeof(ElementType),
    alignof(ElementType)
    >
{
};

template <uint32 NumInlineElements, typename SecondaryAllocator = void>
struct TInlineAllocator
{
    template <typename ElementType>
    struct ForElementType
    {
        TTypeCompatibleBytes<ElementType> InlineData[NumInlineElements];
        ElementType* SecondaryData;
    };
};

struct TBitArray
{
    typedef TInlineAllocator<4>::ForElementType<uint32> AllocatorType;

    AllocatorType AllocatorInstance;
    int32         NumBits;
    int32         MaxBits;
};

template<typename ElementType>
union TSparseArrayElementOrFreeListLink
{
    /** If the element is allocated, its value is stored here. */
    ElementType ElementData;

    struct
    {
        /** If the element isn't allocated, this is a link to the previous element in the array's free list. */
        int32 PrevFreeIndex;

        /** If the element isn't allocated, this is a link to the next element in the array's free list. */
        int32 NextFreeIndex;
    };
};

template <typename T>
struct TSparseArray
{
    typedef TSparseArrayElementOrFreeListLink<
        TAlignedBytes<sizeof(T), alignof(T)>
    > FElementOrFreeListLink;
    TArray<FElementOrFreeListLink> Data;
    TBitArray AllocationFlags;
    int32 FirstFreeIndex;
    int32 NumFreeIndices;

    int32 Num() const
    {
        return Data.Num() - NumFreeIndices;
    }

    FElementOrFreeListLink& GetData(int32 Index)
    {
        return ((FElementOrFreeListLink*)Data.GetData())[Index];
    }

    T& operator[](int32 Index)
    {
        return *(T*)&GetData(Index).ElementData;
    }
};

template <typename InElementType>
struct TSetElement
{
    InElementType Value;
    mutable int32 HashNextId; // FSetElementId
    mutable int32 HashIndex;
};

template <typename T>
struct TSet
{
    TSparseArray<TSetElement<T>> Elements;
    mutable TInlineAllocator<1>::ForElementType<int32> Hash;
    mutable int32 HashSize;

    int32 Num() const
    {
        return Elements.Num();
    }

    FORCEINLINE T& operator[](int32 Index)
    {
        return Elements[Index].Value;
    }
};

template <typename T1, typename T2>
struct TPair
{
    T1 First;
    T2 Second;
};

template <typename KeyType, typename ValueType>
struct TMap
{
    TSet<TPair<KeyType, ValueType>> Pairs;

    FORCEINLINE int32 Num() const
    {
        return Pairs.Num();
    }

    FORCEINLINE TPair<KeyType, ValueType>& operator[](int32 Index)
    {
        return Pairs[Index];
    }
};

struct FUObjectItem
{
    void* Object;
    int32 Flags;
    int32 ClusterRootIndex;
    int32 SerialNumber;
};

struct ObjectArray
{
    FORCEINLINE int32 Num()
    {
        return GET_OFFSET(int32, this, 8);
    }
    FORCEINLINE void* GetObj(int32 Index)
    {
        return (*(FUObjectItem**)this)[Index].Object;
    }
};

static ObjectArray* GUObjectArray;

bool(__fastcall* FindPackagesInDirectory)(
    TArray<FString>& a1,
    const FString& a2
    );

#define INDEX_NONE -1

struct FAutoCompleteNode
{
    int32 IndexChar;
    TArray<int32> AutoCompleteListIndices;
    TArray<FAutoCompleteNode*> ChildNodes;

    FAutoCompleteNode()
    {
        IndexChar = INDEX_NONE;
    }

    FAutoCompleteNode(int32 NewChar)
    {
        IndexChar = NewChar;
    }
};

struct FColor
{
    union { struct { uint8 B, G, R, A; }; uint32 AlignmentDummy; };

    FORCEINLINE FColor(uint8 InR, uint8 InG, uint8 InB, uint8 InA = 255)
    {
        // put these into the body for proper ordering with INTEL vs non-INTEL_BYTE_ORDER
        R = InR;
        G = InG;
        B = InB;
        A = InA;
    }
};

struct FAutoCompleteCommand
{
    FString Command;
    FString Desc;
    FColor Color;

    FAutoCompleteCommand()
        : Color(180, 180, 180)
    {
    }
};

FString FNameToString(FName& Name)
{
    static void* KismetStringLibrary = StaticFindObject(nullptr, nullptr, L"/Script/Engine.KismetStringLibrary", false);
    if (!KismetStringLibrary)
    {
        BasicMessageBox("Didn't find KismetStringLibrary :(");
        return FString();
    }
    static void* Conv_NameToString = StaticFindObject(nullptr, nullptr, L"/Script/Engine.KismetStringLibrary.Conv_NameToString", false);
    if (!Conv_NameToString)
    {
        BasicMessageBox("Didn't find Conv_NameToString :(");
        return FString();
    }

    struct {
        FName InName;
        FString ReturnValue;
    } Parms { Name };

    ProcessEvent(KismetStringLibrary, Conv_NameToString, &Parms);
    return Parms.ReturnValue;
}

FString GetCPPType(void* Child)
{
#pragma region Hide the copy paste
#define CheckPropClass(Class) if (!Class) { BasicMessageBox("Didn't Find " #Class " :("); return L""; }
    static void* BoolPropertyClass = StaticFindObject(nullptr, nullptr, L"/Script/CoreUObject.BoolProperty", false);
    static void* ArrayPropertyClass = StaticFindObject(nullptr, nullptr, L"/Script/CoreUObject.ArrayProperty", false);
    static void* EnumPropertyClass = StaticFindObject(nullptr, nullptr, L"/Script/CoreUObject.EnumProperty", false);
    static void* NumericPropertyClass = StaticFindObject(nullptr, nullptr, L"/Script/CoreUObject.NumericProperty", false);
    static void* BytePropertyClass = StaticFindObject(nullptr, nullptr, L"/Script/CoreUObject.ByteProperty", false);
    static void* ObjectPropertyClass = StaticFindObject(nullptr, nullptr, L"/Script/CoreUObject.ObjectProperty", false);
    static void* ClassPropertyClass = StaticFindObject(nullptr, nullptr, L"/Script/CoreUObject.ClassProperty", false);
    static void* DelegatePropertyClass = StaticFindObject(nullptr, nullptr, L"/Script/CoreUObject.DelegateProperty", false);
    static void* DoublePropertyClass = StaticFindObject(nullptr, nullptr, L"/Script/CoreUObject.DoubleProperty", false);
    static void* FloatPropertyClass = StaticFindObject(nullptr, nullptr, L"/Script/CoreUObject.FloatProperty", false);
    static void* IntPropertyClass = StaticFindObject(nullptr, nullptr, L"/Script/CoreUObject.IntProperty", false);
    static void* Int16PropertyClass = StaticFindObject(nullptr, nullptr, L"/Script/CoreUObject.Int16Property", false);
    static void* Int64PropertyClass = StaticFindObject(nullptr, nullptr, L"/Script/CoreUObject.Int64Property", false);
    static void* Int8PropertyClass = StaticFindObject(nullptr, nullptr, L"/Script/CoreUObject.Int8Property", false);
    static void* InterfacePropertyClass = StaticFindObject(nullptr, nullptr, L"/Script/CoreUObject.InterfaceProperty", false);
    static void* LazyObjectPropertyClass = StaticFindObject(nullptr, nullptr, L"/Script/CoreUObject.LazyObjectProperty", false);
    static void* MapPropertyClass = StaticFindObject(nullptr, nullptr, L"/Script/CoreUObject.MapProperty", false);
    static void* MulticastDelegatePropertyClass = StaticFindObject(nullptr, nullptr, L"/Script/CoreUObject.MulticastDelegateProperty", false);
    static void* NamePropertyClass = StaticFindObject(nullptr, nullptr, L"/Script/CoreUObject.NameProperty", false);
    static void* SetPropertyClass = StaticFindObject(nullptr, nullptr, L"/Script/CoreUObject.SetProperty", false);
    static void* SoftObjectPropertyClass = StaticFindObject(nullptr, nullptr, L"/Script/CoreUObject.SoftObjectProperty", false);
    static void* SoftClassPropertyClass = StaticFindObject(nullptr, nullptr, L"/Script/CoreUObject.SoftClassProperty", false);
    static void* StrPropertyClass = StaticFindObject(nullptr, nullptr, L"/Script/CoreUObject.StrProperty", false);
    static void* StructPropertyClass = StaticFindObject(nullptr, nullptr, L"/Script/CoreUObject.StructProperty", false);
    static void* UInt16PropertyClass = StaticFindObject(nullptr, nullptr, L"/Script/CoreUObject.UInt16Property", false);
    static void* UInt32PropertyClass = StaticFindObject(nullptr, nullptr, L"/Script/CoreUObject.UInt32Property", false);
    static void* UInt64PropertyClass = StaticFindObject(nullptr, nullptr, L"/Script/CoreUObject.UInt64Property", false);
    static void* WeakObjectPropertyClass = StaticFindObject(nullptr, nullptr, L"/Script/CoreUObject.WeakObjectProperty", false);
    static void* TextPropertyClass = StaticFindObject(nullptr, nullptr, L"/Script/CoreUObject.TextProperty", false);
    CheckPropClass(BoolPropertyClass);
    CheckPropClass(ArrayPropertyClass);
    CheckPropClass(EnumPropertyClass);
    CheckPropClass(NumericPropertyClass);
    CheckPropClass(BytePropertyClass);
    CheckPropClass(ObjectPropertyClass);
    CheckPropClass(ClassPropertyClass);
    CheckPropClass(DelegatePropertyClass);
    CheckPropClass(DoublePropertyClass);
    CheckPropClass(FloatPropertyClass);
    CheckPropClass(IntPropertyClass);
    CheckPropClass(Int16PropertyClass);
    CheckPropClass(Int64PropertyClass);
    CheckPropClass(Int8PropertyClass);
    CheckPropClass(InterfacePropertyClass);
    CheckPropClass(LazyObjectPropertyClass);
    CheckPropClass(MapPropertyClass);
    CheckPropClass(MulticastDelegatePropertyClass);
    CheckPropClass(NamePropertyClass);
    CheckPropClass(SetPropertyClass);
    CheckPropClass(SoftObjectPropertyClass);
    CheckPropClass(SoftClassPropertyClass);
    CheckPropClass(StrPropertyClass);
    CheckPropClass(StructPropertyClass);
    CheckPropClass(UInt16PropertyClass);
    CheckPropClass(UInt32PropertyClass);
    CheckPropClass(UInt64PropertyClass);
    CheckPropClass(WeakObjectPropertyClass);
    CheckPropClass(TextPropertyClass);
#pragma endregion

    void* ClassPrivate = GET_OFFSET(void*, Child, Offsets::ClassPrivateOffset);
    if (ClassPrivate == BoolPropertyClass)
    {
        return L"bool";
    }
    else if (ClassPrivate == FloatPropertyClass)
    {
        return L"float";
    }
    else if (ClassPrivate == DoublePropertyClass)
    {
        return L"double";
    }
    else if (ClassPrivate == IntPropertyClass)
    {
        return L"int32";
    }
    else if (ClassPrivate == Int16PropertyClass)
    {
        return L"int16";
    }
    else if (ClassPrivate == Int64PropertyClass)
    {
        return L"int64";
    }
    else if (ClassPrivate == Int8PropertyClass)
    {
        return L"int8";
    }
    else if (ClassPrivate == UInt16PropertyClass)
    {
        return L"uint16";
    }
    else if (ClassPrivate == UInt32PropertyClass)
    {
        return L"uint32";
    }
    else if (ClassPrivate == UInt64PropertyClass)
    {
        return L"uint64";
    }
    else if (ClassPrivate == StrPropertyClass)
    {
        return L"FString";
    } 
    else if (ClassPrivate == NamePropertyClass)
    {
        return L"FName";
    }
    else if (ClassPrivate == TextPropertyClass)
    {
        return L"FText";
    }
    else if (ClassPrivate == ClassPropertyClass)
    {
        if (GET_OFFSET(uint64, Child, Offsets::PropertyFlagsOffset) & 0x0004000000000000)
        {
            void* PropertyClass = GET_OFFSET(void*, Child, Offsets::PropertyDataOffset);
            FString ret = L"TSubclassOf<U";
            ret += FNameToString(GET_OFFSET(FName, PropertyClass, Offsets::NamePrivateOffset));
            ret += L">";
            return ret;
        }
        else
        {
            return L"UClass*";
        }
    }
    else if (ClassPrivate == ObjectPropertyClass)
    {
        void* PropertyClass = GET_OFFSET(void*, Child, Offsets::PropertyDataOffset);
        FString ret = L"U";
        ret += FNameToString(GET_OFFSET(FName, PropertyClass, Offsets::NamePrivateOffset));
        ret += L"*";
        return ret;
    }
    else if (ClassPrivate == ArrayPropertyClass)
    {
        void* Inner = GET_OFFSET(void*, Child, Offsets::PropertyDataOffset);
        FString ret = L"TArray<";
        ret += GetCPPType(Inner);
        ret += L">";
        return ret;
    }
    else if (ClassPrivate == BytePropertyClass)
    {
        void* Enum = GET_OFFSET(void*, Child, Offsets::PropertyDataOffset);
        if (Enum)
            return FNameToString(GET_OFFSET(FName, Enum, Offsets::NamePrivateOffset));
        else
            return L"uint8";
    }
    else if (ClassPrivate == EnumPropertyClass)
    {
        void* Enum = GET_OFFSET(void*, Child, Offsets::PropertyDataOffset);
        return FNameToString(GET_OFFSET(FName, Enum, Offsets::NamePrivateOffset));
    }
    else if (ClassPrivate == StructPropertyClass)
    {
        void* Struct = GET_OFFSET(void*, Child, Offsets::PropertyDataOffset);
        auto StructName = FNameToString(GET_OFFSET(FName, Struct, Offsets::NamePrivateOffset));
        FString ret = L"F";
        ret += StructName;
        StructName.Free();
        return ret;
    }

    /*
    * NumericPropertyClass
    * DelegatePropertyClass
    * InterfacePropertyClass
    * LazyObjectPropertyClass
    * MapPropertyClass
    * MulticastDelegatePropertyClass
    * SetPropertyClass
    * SoftObjectPropertyClass
    * SoftClassPropertyClass
    * WeakObjectPropertyClass
    */
    
    PRINT("Unknown type %s", FNameToString(GET_OFFSET(FName, ClassPrivate, Offsets::NamePrivateOffset)).ToString().c_str());

    return L"Unknown";
}

FString GenerateFuncDesc(void* Func)
{
    FString ret;
    
    void* Children = GET_OFFSET(void*, Func, Offsets::ChildrenOffset);
    for (void* Child = Children; Child; Child = GET_OFFSET(void*, Child, Offsets::UFieldNextOffset))
    {
        if (GET_OFFSET(uint64, Child, Offsets::PropertyFlagsOffset) & 0x0000000000000080)
        {
            auto ChildName = FNameToString(GET_OFFSET(FName, Child, Offsets::NamePrivateOffset));
            ret += ChildName;
            ret += L"[";
            ret += GetCPPType(Child);
            ret += L"] ";
            ChildName.Free();
        }
    }

    return ret;
}

static void* ConsoleManager;

struct UConsole
{
    uint8 pad[104];
    TArray<FString> HistoryBuffer;

    /** The command the user is currently typing. */
    FString TypedStr;

    int32 TypedStrPos;    //Current position in TypedStr

    /** The command the user would get if they autocompleted their current input. Empty if no autocompletion entries are available. */
    FString PrecompletedInputLine;

    /** The most recent input that was autocompleted during this open console session. */
    FString LastAutoCompletedCommand;

    /**
     * Indicates that InputChar events should be captured to prevent them from being passed on to other interactions.  Reset
     * when the another keydown event is received.
     */
    uint32 bCaptureKeyInput : 1;

    /** True while a control key is pressed. */
    uint32 bCtrl : 1;

    /** Full list of auto-complete commands and info */
    TArray<FAutoCompleteCommand> AutoCompleteList;

    /** Is the current auto-complete selection locked */
    uint32 bAutoCompleteLocked : 1;

    /** Currently selected auto complete index */
    int32 AutoCompleteIndex;

    /** -1: auto complete cursor is not visible, >=0 */
    int32 AutoCompleteCursor;

    /** Do we need to rebuild auto complete? */
    uint32 bIsRuntimeAutoCompleteUpToDate : 1;

    // NAME_Typing, NAME_Open or NAME_None
    int64 ConsoleState;

    FAutoCompleteNode AutoCompleteTree;

    TArray<FAutoCompleteCommand> AutoComplete;

    const void* ConsoleSettings; // UConsoleSettings*

    // https://github.com/EpicGames/UnrealEngine/blob/4.20/Engine/Source/Runtime/Engine/Private/UserInterface/Console.cpp#L141
    void BuildRuntimeAutoCompleteList()
    {
        FColor AutoCompleteCommandColor = GET_OFFSET(FColor, ConsoleSettings, Offsets::AutoCompleteCommandColorOffset);
        FColor AutoCompleteCVarColor = GET_OFFSET(FColor, ConsoleSettings, Offsets::AutoCompleteCVarColorOffset);
        FColor AutoCompleteFadedColor = GET_OFFSET(FColor, ConsoleSettings, Offsets::AutoCompleteFadedColorOffset);

        {
            TArray<FAutoCompleteCommand> ManualAutoCompleteList = GET_OFFSET(TArray<FAutoCompleteCommand>, ConsoleSettings, Offsets::ManualAutoCompleteListOffset);
            for (int32 i = 0; i < ManualAutoCompleteList.Num(); i++)
            {
                FAutoCompleteCommand toadd;

                toadd.Command = ManualAutoCompleteList[i].Command;
                toadd.Desc = ManualAutoCompleteList[i].Desc;
                toadd.Color = AutoCompleteCommandColor;
                AutoCompleteList.Add(toadd);
            }
        }

        {
            void* FunctionClass = StaticFindObject(nullptr, nullptr, L"/Script/CoreUObject.Function", false);
            PRINT("Total objects: %i", GUObjectArray->Num());
            for (int32 i = 0; i < GUObjectArray->Num(); i++)
            {
                void* obj = GUObjectArray->GetObj(i);
                if (!obj)
                {
                    continue;
                }

                // TODO: LevelScriptActor?

                if (GET_OFFSET(void*, obj, Offsets::ClassPrivateOffset) == FunctionClass)
                {
                    if (GET_OFFSET(uint32, obj, Offsets::FunctionFlagsOffset) & 0x00000200)
                    {
                        FAutoCompleteCommand test_cmd;
                        test_cmd.Command = FNameToString(GET_OFFSET(FName, obj, Offsets::NamePrivateOffset));
                        test_cmd.Color = AutoCompleteCommandColor;
                        auto desc = GenerateFuncDesc(obj);
                        test_cmd.Desc = desc;

                        AutoCompleteList.Add(test_cmd);
                    }
                }
            }
        }
        
        {
            FAutoCompleteCommand test_cmd;
            test_cmd.Command = FString(L"open 127.0.0.1");
            test_cmd.Desc = FString(L"(opens connection to localhost)");
            test_cmd.Color = AutoCompleteCommandColor;
            AutoCompleteList.Add(test_cmd);
        }

#if 0 // Does not work
        {
            TArray<FString> AutoCompleteMapPaths = *(TArray<FString>*)((uint64)ConsoleSettings + AutoCompleteMapPathsOffset);
            TArray<FString> Packages;
            for (int32 i = 0; i < AutoCompleteMapPaths.Num(); i++)
            {
                FindPackagesInDirectory(Packages, AutoCompleteMapPaths[i]);
                PRINT("Map count: %i", Packages.Num());
            }
        }
#endif
        {
            typedef TMap<FString, void*> COType;
            auto ConsoleObjects = GET_OFFSET(COType, ConsoleManager, Offsets::ConsoleObjectsOffset);
            for (int32 i = 0; i < ConsoleObjects.Num(); i++)
            {
                FAutoCompleteCommand test_cmd;
                test_cmd.Command = ConsoleObjects[i].First;
                test_cmd.Color = AutoCompleteCVarColor;
                AutoCompleteList.Add(test_cmd);
            }
        }

        for (int32 ListIdx = 0; ListIdx < AutoCompleteList.Num(); ListIdx++)
        {
            FString Command = AutoCompleteList[ListIdx].Command;
            FAutoCompleteNode* Node = &AutoCompleteTree;
            for (int32 Depth = 0; Depth < Command.Len(); Depth++)
            {
                int32 Char = tolower(Command[Depth]);
                int32 FoundNodeIdx = INDEX_NONE;
                TArray<FAutoCompleteNode*>& NodeList = Node->ChildNodes;
                for (int32 NodeIdx = 0; NodeIdx < NodeList.Num(); NodeIdx++)
                {
                    if (NodeList[NodeIdx]->IndexChar == Char)
                    {
                        FoundNodeIdx = NodeIdx;
                        Node = NodeList[FoundNodeIdx];
                        NodeList[FoundNodeIdx]->AutoCompleteListIndices.Add(ListIdx);
                        break;
                    }
                }
                if (FoundNodeIdx == INDEX_NONE)
                {
                    FAutoCompleteNode* NewNode = new FAutoCompleteNode(Char);
                    NewNode->AutoCompleteListIndices.Add(ListIdx);
                    Node->ChildNodes.Add(NewNode);
                    Node = NewNode;
                }
            }
        }

        bIsRuntimeAutoCompleteUpToDate = true;
    }
};

DWORD WINAPI Main(LPVOID lpParam)
{
#ifdef ALLOC_CONSOLE
    AllocConsole();
    FILE* f = nullptr;
    freopen_s(&f, "CONOUT$", "w", stdout);
#endif // ALLOC_CONSOLE

    auto ConstructObjectAddr = Memcury::Scanner::FindPattern("4C 89 44 24 ? 53 55 56 57 41 54 41 56 41 57 48 81 EC B0 01 00 00").Get();
    CheckAddr(ConstructObjectAddr, "Failed to find StaticConstructObject_Internal");
    StaticConstructObject_Internal = decltype(StaticConstructObject_Internal)(ConstructObjectAddr);
    
    auto FindObjectAddr = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 55 57 41 54 41 56 41 57 48 8B EC 48 83 EC 60 80 3D ? ? ? ? ? 45 0F B6 F1").Get();
    CheckAddr(FindObjectAddr, "Failed to find StaticFindObject");
    StaticFindObject = decltype(StaticFindObject)(FindObjectAddr);
    
    auto FMemoryFreeAddr = Memcury::Scanner::FindPattern("48 85 C9 74 ? 4C 8B 05 ? ? ? ? 4D 85 C0 0F 84").Get();
    CheckAddr(FMemoryFreeAddr, "Failed to find FMemoryFree");
    FMemoryFree = decltype(FMemoryFree)(FMemoryFreeAddr);
    
    auto FMemoryReallocAddr = Memcury::Scanner::FindPattern("4C 8B D1 48 8B 0D ? ? ? ? 48 85 C9 75 ? 49 8B CA").Get();
    CheckAddr(FMemoryReallocAddr, "Failed to find FMemoryRealloc");
    FMemoryRealloc = decltype(FMemoryRealloc)(FMemoryReallocAddr);
    
    auto FMemoryMallocAddr = Memcury::Scanner::FindPattern("4C 8B C9 48 8B 0D ? ? ? ? 48 85 C9").Get();
    CheckAddr(FMemoryMallocAddr, "Failed to find FMemoryMalloc");
    FMemoryMalloc = decltype(FMemoryMalloc)(FMemoryMallocAddr);
    
    auto ObjectsArrAddr = Memcury::Scanner::FindPattern("48 8B 05 ? ? ? ? 48 8D 1C C8 81 4B ? ? ? ? ? 49 63 76").RelativeOffset(3).Get();
    CheckAddr(ObjectsArrAddr, "Failed to find ObjectsArray");
    GUObjectArray = decltype(GUObjectArray)(ObjectsArrAddr);
    
    auto ProcessEventAddr = Memcury::Scanner::FindPattern("40 55 56 57 41 54 41 55 41 56 41 57 48 81 EC F0 00 00 00").Get();
    CheckAddr(ProcessEventAddr, "Failed to find ProcessEvent");
    ProcessEvent = decltype(ProcessEvent)(ProcessEventAddr);

    auto ConsoleManagerAddr = Memcury::Scanner::FindPattern("48 89 05 ? ? ? ? 48 8B C8 48 85 C0").RelativeOffset(3).GetAs<void**>();
    CheckAddr(ConsoleManagerAddr, "Failed to find ConsoleManager");
    ConsoleManager = *ConsoleManagerAddr;

    /*auto FindPackagesAddr = Memcury::Scanner::FindPattern("48 8B C4 53 56 48 83 EC 68 48 89 68").Get();
    CheckAddr(FindPackagesAddr, "Failed to find FindPackagesInDirectory");
    FindPackagesInDirectory = decltype(FindPackagesInDirectory)(FindPackagesAddr);*/

    //FString test;
    //for (int32 i = 0; i < 100; i++)
    //{
    //    test += std::to_wstring(i).c_str();
    //}
    //PRINT("TEST: %s", test.ToString().c_str());

    PRINT("Press F9 to construct console");

    while (true)
    {
        if (KeyPressed(VK_F9))
        {
            auto Engine = StaticFindObject(nullptr, nullptr, L"/Engine/Transient.FortEngine_0", false);
            if (!Engine)
            {
                BasicMessageBox("Didn't find engine :(");
                break;
            }
            
            auto ConsoleClass = StaticFindObject(nullptr, nullptr, L"/Script/Engine.Console", false);
            if (!ConsoleClass)
            {
                BasicMessageBox("Didn't find ConsoleClass :(");
                break;
            }

            auto GameViewport = GET_OFFSET(void*, Engine, Offsets::GameViewportClientOffset);
            auto ViewportConsole = GET_OFFSETPTR(void*, GameViewport, Offsets::ViewportConsoleOffset);

            auto ConstructedConsole = (UConsole*)StaticConstructObject_Internal(ConsoleClass, GameViewport, 0, 0, 0, nullptr, false, nullptr, false);
            if (!ConstructedConsole)
            {
                BasicMessageBox("Failed to construct console :(");
                break;
            }
            ConstructedConsole->BuildRuntimeAutoCompleteList();
            *ViewportConsole = ConstructedConsole;

            break;
        }
        Sleep(16);
    }

    PRINT("Finished!");
    return 0;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        CreateThread(0, 0, Main, 0, 0, 0);
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

