#include <Windows.h>
#include <stdlib.h>
#include <format>
#include <string>
#include <fstream>
#include <Minhook.h>
#include <cwctype>

#include "memcury.h"
#include "UE.h"
#include "Console.h"

UObject* GetEngine()
{
    static UObject* EngineClass = UObject::FindObject(L"/Script/FortniteGame.FortEngine");
    static UObject* Engine = nullptr;

    if (!Engine)
    {
        for (int i = 0; i < UObject::Objects->Num(); i++)
        {
            auto Object = UObject::Objects->Get(i);
            if (!Object) continue;

            if (Object->ObjectFlags & EObjectFlags::RF_ClassDefaultObject || Object->ClassPrivate != EngineClass) continue;

            Engine = Object;
        }
    }

    return Engine;
}

template <typename T = void*>
void HookFunction(uintptr_t Addr, void* Hook, T* Original = nullptr)
{
    MH_CreateHook((LPVOID)Addr, Hook, (LPVOID*)Original);
    MH_EnableHook((LPVOID)Addr);
}

UObject* SpawnObject(UObject* ObjectClass, UObject* Outer)
{
    static auto GameplayStatics = UObject::FindObject(L"/Script/Engine.Default__GameplayStatics");
    static auto Func = UObject::FindObject(L"/Script/Engine.GameplayStatics.SpawnObject");
    struct
    {
        UObject* ObjectClass;
        UObject* Outer;
        UObject* ReturnValue;
    } args {ObjectClass, Outer};
    GameplayStatics->ProcessEvent(Func, &args);
    return args.ReturnValue;
}

void RequestExitHook()
{
    return;
}

bool (*UWorldExecOriginal)(UObject*, UObject*, const wchar_t*, int64);
bool UWorldExecHook(UObject* World, UObject* WorldPart2, const wchar_t* Cmd, int64 a4)
{
    static int32 OwningGameInstanceOffset = World->GetOffset("OwningGameInstance");
    UObject* OwningGameInstance = World->GetChild(OwningGameInstanceOffset);
    static int32 LocalPlayersOffset = OwningGameInstance->GetOffset("LocalPlayers");
    auto LocalPlayers = OwningGameInstance->GetChild<TArray<UObject*>>(LocalPlayersOffset);
    static int32 PlayerControllerOffset = LocalPlayers[0]->GetOffset("PlayerController");
    auto PlayerController = LocalPlayers[0]->GetChild(PlayerControllerOffset);

#define CheckCmd(cmdname) wcscmp(Cmd, cmdname) == 0

    if (CheckCmd(L"givemecheats"))
    {
        static int32 CheatManagerOffset = PlayerController->GetOffset("CheatManager");
        static UObject* CheatManagerClass = UObject::FindObject(L"/Script/Engine.CheatManager");
        PlayerController->GetChild(CheatManagerOffset) = SpawnObject(CheatManagerClass, PlayerController);

        return true;
    }
    else if (CheckCmd(L"dumpobjects"))
    {
        std::ofstream outfile("objects.txt");
        for (int i = 0; i < UObject::Objects->Num(); i++)
        {
            auto Object = UObject::Objects->Get(i);
            if (!Object) continue;
            outfile << Object->GetFullName() << '\n';
        }
        outfile.close();

        return true;
    }

    return UWorldExecOriginal(World, WorldPart2, Cmd, a4);
}

DWORD WINAPI Main(LPVOID lpParam)
{
#ifdef ALLOC_CONSOLE
    AllocConsole();
    FILE* f = nullptr;
    freopen_s(&f, "CONOUT$", "w", stdout);
#endif // ALLOC_CONSOLE

    MH_Initialize();

    // FMemory::Realloc
    {
        auto Addr = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 20 48 8B F1 41 8B D8 48 8B 0D").Get();
        
        if (!Addr)
        {
            SimpleMessageBox("Couldn't find FMemory::Realloc");
            return 0;
        }

        FMemory::Init((void*)Addr);
    }

    // UObject::Objects
    {
        auto Addr = Memcury::Scanner::FindPattern("48 8B 05 ? ? ? ? 48 8B 0C C8 4C 8D 3C D1 EB ? 4D 8B FD 41 8B 5F ? 85 DB 75 ? BB 01 00 00 00 F0 0F C1 1D ? ? ? ? FF C3 81 FB E8 03 00 00 7F ? 44 8B C3 48 8D 15 ? ? ? ? 48 8D 0D ? ? ? ? E8 ? ? ? ? 33 C0 F0 41 0F B1 5F ? 0F 45 D8 89 5D ? 4C 8D 45").RelativeOffset(3).Get();

        if (!Addr)
        {
            SimpleMessageBox("Couldn't find UObject::Objects");
            return 0;
        }

        UObject::Objects = (FChunkedFixedUObjectArray*)(Addr);
    }

    // RequestExit
    {
        auto Addr = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 20 33 DB 0F B6 F9").Get();

        if (Addr)
        {
            HookFunction(Addr, RequestExitHook);
        }
        else
        {
            SimpleMessageBox("Couldn't find RequestExit - Will probably crash if you open BR map on later builds");
        }

    }

    // UWorld::Exec
    {
        auto Addr = Memcury::Scanner::FindStringRef(L"FLUSHPERSISTENTDEBUGLINES", true).ScanFor({0xCC}, false).Get() + 1;

        if (Addr)
        {
            HookFunction(Addr, UWorldExecHook, &UWorldExecOriginal);
        }
        else
        {
            SimpleMessageBox("Couldn't find UWorld::Exec - Custom commands will not work");
        }
    }

    InitOffsets();

    // Allows for console input on loadingscreen
    UObject::FindObject(L"/Script/FortniteGame.Default__FortRuntimeOptions")->GetChild<bool>("bLoadingScreenInputPreprocessorEnabled") = false;

    auto GameViewport = GetEngine()->GetChild("GameViewport");
    auto NewConsole = (UConsole*)SpawnObject(UObject::FindObject(L"/Script/Engine.Console"), GameViewport);
    NewConsole->BuildRuntimeAutoCompleteList();
    GameViewport->GetChild("ViewportConsole") = NewConsole;

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
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

