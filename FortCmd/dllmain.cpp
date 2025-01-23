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

#define INDEX_NONE -1

typedef signed char int8;
typedef signed short int16;
typedef signed int int32;
typedef signed long long int64;
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;
typedef int64 FName;

#include "offsets.h"
#include "natives.h"
#include "UE.h"

static ObjectArray* GUObjectArray;

bool(__fastcall* FindPackagesInDirectory)(
    TArray<FString>& a1,
    const FString& a2
    );

__int64 (__fastcall* ProjectDir)(
    FString& a1
    );

FString GenerateFuncDesc(UFunction* Func)
{
    FString ret;
    
    UField* Children = Func->Children();
    for (UField* Child = Children; Child; Child = Child->Next())
    {
        if (((UProperty*)Child)->PropertyFlags() & 0x0000000000000080)
        {
            auto ChildName = FNameToString(Child->NamePrivate());
            ret += ChildName;
            ret += L"[";
            ret += GetCPPType(Child);
            ret += L"] ";
            ChildName.Free();
        }
    }

    return ret;
}

static FConsoleManager* ConsoleManager;

struct UConsole
{
    uint8 pad[104];
    TArray<FString> HistoryBuffer;
    FString TypedStr;
    int32 TypedStrPos;
    FString PrecompletedInputLine;
    FString LastAutoCompletedCommand;
    uint32 bCaptureKeyInput : 1;
    uint32 bCtrl : 1;
    TArray<FAutoCompleteCommand> AutoCompleteList;
    uint32 bAutoCompleteLocked : 1;
    int32 AutoCompleteIndex;
    int32 AutoCompleteCursor;
    uint32 bIsRuntimeAutoCompleteUpToDate : 1;
    int64 ConsoleState;
    FAutoCompleteNode AutoCompleteTree;
    TArray<FAutoCompleteCommand> AutoComplete;
    UConsoleSettings* ConsoleSettings;

    // https://github.com/EpicGames/UnrealEngine/blob/4.20/Engine/Source/Runtime/Engine/Private/UserInterface/Console.cpp#L141
    void BuildRuntimeAutoCompleteList()
    {
        FColor& AutoCompleteCommandColor = ConsoleSettings->AutoCompleteCommandColor();
        FColor& AutoCompleteCVarColor = ConsoleSettings->AutoCompleteCVarColor();
        FColor& AutoCompleteFadedColor = ConsoleSettings->AutoCompleteFadedColor();

        {
            TArray<FAutoCompleteCommand>& ManualAutoCompleteList = ConsoleSettings->ManualAutoCompleteList();
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
            void* FunctionClass = FindObject(L"/Script/CoreUObject.Function");
            
            for (int32 i = 0; i < GUObjectArray->Num(); i++)
            {
                UObject* obj = (UObject*)GUObjectArray->GetObj(i);
                if (!obj)
                {
                    continue;
                }

                // TODO: LevelScriptActor?

                if (obj->ClassPrivate() == FunctionClass)
                {
                    auto func = (UFunction*)obj;
                    if (func->FunctionFlags() & 0x00000200)
                    {
                        FAutoCompleteCommand test_cmd;
                        test_cmd.Command = FNameToString(obj->NamePrivate());
                        test_cmd.Color = AutoCompleteCommandColor;
                        auto desc = GenerateFuncDesc(func);
                        test_cmd.Desc = desc;

                        AutoCompleteList.Add(test_cmd);
                    }
                }
            }
        }
        
        {
            FString ret;
            ProjectDir(ret);
            TArray<FString> Packages;
            auto AutoCompleteMapPaths = ConsoleSettings->AutoCompleteMapPaths();
            AutoCompleteMapPaths.Add(L"Content/Athena/Maps");
            for (int i = 0; i < AutoCompleteMapPaths.Num(); i++)
            {
                FString tosearch = ret.Copy();
                tosearch += AutoCompleteMapPaths[i];
                FindPackagesInDirectory(Packages, tosearch);
                tosearch.Free();
            }
            ret.Free();

            for (int32 i = 0; i < Packages.Num(); i++)
            {
                if (Packages[i].EndsWith(L".umap"))
                {
                    int32 posofslash = Packages[i].FindLastOf(L'/') + 1;
                    auto substr = Packages[i].Substring(posofslash, (Packages[i].Len() - 5) - posofslash);
                    FString Final = L"open ";
                    Final += substr;
                    substr.Free();

                    FAutoCompleteCommand test_cmd;
                    test_cmd.Command = Final;
                    test_cmd.Color = AutoCompleteCommandColor;
                    AutoCompleteList.Add(test_cmd);
                }
                Packages[i].Free();
            }
        }

        {
            FAutoCompleteCommand test_cmd;
            test_cmd.Command = FString(L"open 127.0.0.1");
            test_cmd.Desc = FString(L"(opens connection to localhost)");
            test_cmd.Color = AutoCompleteCommandColor;
            AutoCompleteList.Add(test_cmd);
        }

        {
            auto ConsoleObjects = ConsoleManager->ConsoleObjects();
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
    Natives::StaticConstructObject_Internal = decltype(Natives::StaticConstructObject_Internal)(ConstructObjectAddr);
    
    auto FindObjectAddr = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 55 57 41 54 41 56 41 57 48 8B EC 48 83 EC 60 80 3D ? ? ? ? ? 45 0F B6 F1").Get();
    CheckAddr(FindObjectAddr, "Failed to find StaticFindObject");
    Natives::StaticFindObject = decltype(Natives::StaticFindObject)(FindObjectAddr);
    
    auto FMemoryFreeAddr = Memcury::Scanner::FindPattern("48 85 C9 74 ? 4C 8B 05 ? ? ? ? 4D 85 C0 0F 84").Get();
    CheckAddr(FMemoryFreeAddr, "Failed to find FMemoryFree");
    Natives::FMemoryFree = decltype(Natives::FMemoryFree)(FMemoryFreeAddr);
    
    auto FMemoryReallocAddr = Memcury::Scanner::FindPattern("4C 8B D1 48 8B 0D ? ? ? ? 48 85 C9 75 ? 49 8B CA").Get();
    CheckAddr(FMemoryReallocAddr, "Failed to find FMemoryRealloc");
    Natives::FMemoryRealloc = decltype(Natives::FMemoryRealloc)(FMemoryReallocAddr);
    
    auto FMemoryMallocAddr = Memcury::Scanner::FindPattern("4C 8B C9 48 8B 0D ? ? ? ? 48 85 C9").Get();
    CheckAddr(FMemoryMallocAddr, "Failed to find FMemoryMalloc");
    Natives::FMemoryMalloc = decltype(Natives::FMemoryMalloc)(FMemoryMallocAddr);
    
    auto ObjectsArrAddr = Memcury::Scanner::FindPattern("48 8B 05 ? ? ? ? 48 8D 1C C8 81 4B ? ? ? ? ? 49 63 76").RelativeOffset(3).Get();
    CheckAddr(ObjectsArrAddr, "Failed to find ObjectsArray");
    GUObjectArray = decltype(GUObjectArray)(ObjectsArrAddr);
    
    auto ProcessEventAddr = Memcury::Scanner::FindPattern("40 55 56 57 41 54 41 55 41 56 41 57 48 81 EC F0 00 00 00").Get();
    CheckAddr(ProcessEventAddr, "Failed to find ProcessEvent");
    Natives::ProcessEvent = decltype(Natives::ProcessEvent)(ProcessEventAddr);

    auto ConsoleManagerAddr = Memcury::Scanner::FindPattern("48 89 05 ? ? ? ? 48 8B C8 48 85 C0").RelativeOffset(3).GetAs<FConsoleManager**>();
    CheckAddr(ConsoleManagerAddr, "Failed to find ConsoleManager");
    ConsoleManager = *ConsoleManagerAddr;

    auto FindPackagesAddr = Memcury::Scanner::FindPattern("48 8B C4 53 56 48 83 EC 68 48 89 68").Get();
    CheckAddr(FindPackagesAddr, "Failed to find FindPackagesInDirectory");
    FindPackagesInDirectory = decltype(FindPackagesInDirectory)(FindPackagesAddr);
    
    auto ProjectDirAddr = Memcury::Scanner::FindPattern("48 89 74 24 ? 57 48 83 EC 20 48 8B F9 E8 ? ? ? ? 48 8B F0 33 C0 48 89 07 48 89 47 ? 48 85 F6 74 ? 66 39 06 74 ? 48 89 5C 24 ? 48 83 CB FF 48 FF C3 66 39 04 5E 75 ? FF C3 89 5F ? 85 DB 7E ? 33 D2 48 8B CF E8 ? ? ? ? 48 8B 0F 48 8B D6 4C 63 C3 4D 03 C0 E8 ? ? ? ? 48 8B 5C 24 ? 48 8B C7 48 8B 74 24 ? 48 83 C4 20 5F C3 CC 34 75").Get();
    CheckAddr(ProjectDirAddr, "Failed to find ProjectDir");
    ProjectDir = decltype(ProjectDir)(ProjectDirAddr);

    Offsets::Init();

    /*
    * Notes:
    * Can get version from KismetSystemLibrary.GetEngineVersion prob useful for porting to other versions
    * At some point AFTER 4.1 they added KismetSystemLibrary.GetProjectDirectory which could maybe be a replacement for ProjectDir()
    */


    PRINT("Press F9 to construct console");

    while (true)
    {
        if (KeyPressed(VK_F9))
        {
            auto Engine = FindObject(L"/Engine/Transient.FortEngine_0");
            if (!Engine)
            {
                BasicMessageBox("Didn't find engine :(");
                break;
            }
            
            auto ConsoleClass = FindObject(L"/Script/Engine.Console");
            if (!ConsoleClass)
            {
                BasicMessageBox("Didn't find ConsoleClass :(");
                break;
            }

            auto GameViewport = GET_OFFSET(void*, Engine, Offsets::GameViewportClientOffset);
            auto ViewportConsole = GET_OFFSETPTR(void*, GameViewport, Offsets::ViewportConsoleOffset);

            auto ConstructedConsole = (UConsole*)ConstructObject(ConsoleClass, GameViewport);
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

