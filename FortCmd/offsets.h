#pragma once

namespace Offsets
{
    int32 UFieldNextOffset;
    int32 ChildrenOffset;
    int32 ClassPrivateOffset;
    int32 NamePrivateOffset;
    int32 FunctionFlagsOffset;
    int32 ManualAutoCompleteListOffset;
    int32 AutoCompleteMapPathsOffset;
    int32 AutoCompleteCommandColorOffset;
    int32 AutoCompleteCVarColorOffset;
    int32 AutoCompleteFadedColorOffset;
    int32 GameViewportClientOffset;
    int32 ViewportConsoleOffset;
    int32 PropertyDataOffset;
    int32 PropertyFlagsOffset;
    int32 ConsoleObjectsOffset;
    int32 OuterPrivateOffset;

    void Init()
    {
        UFieldNextOffset = 40;
        ChildrenOffset = 56;
        ClassPrivateOffset = 16;
        NamePrivateOffset = 24;
        FunctionFlagsOffset = 136;
        ManualAutoCompleteListOffset = 48;
        AutoCompleteMapPathsOffset = 64;
        AutoCompleteCommandColorOffset = 96;
        AutoCompleteCVarColorOffset = 100;
        AutoCompleteFadedColorOffset = 104;
        GameViewportClientOffset = 1832;
        ViewportConsoleOffset = 64;
        PropertyDataOffset = 112;
        PropertyFlagsOffset = 56;
        ConsoleObjectsOffset = 8;
        OuterPrivateOffset = 32;
    }
}