#include <Windows.h>
#include <stdlib.h>
#include <format>
#include <string>

#include "memcury.h"
#include "UE.h"

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

DWORD WINAPI Main(LPVOID lpParam)
{
#ifdef ALLOC_CONSOLE
    AllocConsole();
    FILE* f = nullptr;
    freopen_s(&f, "CONOUT$", "w", stdout);
#endif // ALLOC_CONSOLE

    // 26.30
    // UGameViewportClient::SetupInitialLocalPlayer
    // 48 89 5C 24 ? 57 48 83 EC 30 83 64 24 ? ? 48 8D 05 ? ? ? ? 48 8B D9 C6 41

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

    auto GameViewport = GetEngine()->GetChild(0x08F0);
    GameViewport->GetChild(0x0040) = SpawnObject(UObject::FindObject(L"/Script/Engine.Console"), GameViewport);

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

