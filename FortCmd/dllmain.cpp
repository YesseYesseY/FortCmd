#include <Windows.h>
#include <stdlib.h>

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

void(__fastcall * ProcessEvent)(
    void* Object,
    void* Function,
    void* Parms
    );

// 
template <typename T>
struct TArray
{
    T* Data = nullptr;
    int32 ArrayNum = 0;
    int32 ArrayMax = 0;

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

    FORCEINLINE void Add(T Val)
    {
        if (ArrayMax <= ArrayNum)
        {
            Data = (T*)FMemoryRealloc(Data, sizeof(T) * (ArrayNum + 1), 0);
        }
        Data[ArrayNum] = Val;
        ArrayMax++;
        ArrayNum++;
    }
};

struct FString
{
    TArray<wchar_t> Data;

    FString() = default;

    FORCEINLINE FString(const wchar_t* Str)
    {
        int32 size = wcslen(Str) + 1;
        Data.ArrayNum = size;
        Data.ArrayMax = size;
        Data.Data = (wchar_t*)Str;
    }

    FORCEINLINE int32 Len() const
    {
        return Data.Num() - 1;
    }

    FORCEINLINE TCHAR& operator[](int32 Index)
    {
        return Data.GetData()[Index];
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
        int32 ManualAutoCompleteListOffset = 48;
        int32 AutoCompleteMapPathsOffset = 64;
        int32 AutoCompleteCommandColorOffset = 96;

        FColor AutoCompleteCommandColor = GET_OFFSET(FColor, ConsoleSettings, AutoCompleteCommandColorOffset);

        {
            TArray<FAutoCompleteCommand> ManualAutoCompleteList = GET_OFFSET(TArray<FAutoCompleteCommand>, ConsoleSettings, ManualAutoCompleteListOffset);
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
            int32 ClassPrivateOffset = 16;
            int32 NamePrivateOffset = 24;
            int32 FunctionFlagsOffset = 136;
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

                if (GET_OFFSET(void*, obj, ClassPrivateOffset) == FunctionClass)
                {
                    if (GET_OFFSET(uint32, obj, FunctionFlagsOffset) & 0x00000200)
                    {
                        FAutoCompleteCommand test_cmd;
                        test_cmd.Command = FNameToString(GET_OFFSET(FName, obj, NamePrivateOffset));
                        test_cmd.Color = AutoCompleteCommandColor;

                        // TODO: Generate description

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
    
    auto ObjectsArrAddr = Memcury::Scanner::FindPattern("48 8B 05 ? ? ? ? 48 8D 1C C8 81 4B ? ? ? ? ? 49 63 76").RelativeOffset(3).Get();
    CheckAddr(ObjectsArrAddr, "Failed to find ObjectsArray");
    GUObjectArray = decltype(GUObjectArray)(ObjectsArrAddr);
    
    auto ProcessEventAddr = Memcury::Scanner::FindPattern("40 55 56 57 41 54 41 55 41 56 41 57 48 81 EC F0 00 00 00").Get();
    CheckAddr(ProcessEventAddr, "Failed to find ProcessEvent");
    ProcessEvent = decltype(ProcessEvent)(ProcessEventAddr);

    /*auto FindPackagesAddr = Memcury::Scanner::FindPattern("48 8B C4 53 56 48 83 EC 68 48 89 68").Get();
    CheckAddr(FindPackagesAddr, "Failed to find FindPackagesInDirectory");
    FindPackagesInDirectory = decltype(FindPackagesInDirectory)(FindPackagesAddr);*/

    PRINT("Press F9 to construct console");

    int32 GameViewportClientOffset = 1832;
    int32 ViewportConsoleOffset = 64;

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

            auto GameViewport = GET_OFFSET(void*, Engine, GameViewportClientOffset);
            auto ViewportConsole = GET_OFFSETPTR(void*, GameViewport, ViewportConsoleOffset);

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

