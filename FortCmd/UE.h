#pragma once

template <typename ElementType>
struct TArray
{
    ElementType* Data;
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
        Data = (ElementType*)FMemory::Malloc(sizeof(ElementType) * size);
        ArrayNum = 0;
        ArrayMax = sizeof(ElementType);
    }

    FORCEINLINE int32 Num() const
    {
        return ArrayNum;
    }

    FORCEINLINE int32 Max()
    {
        return ArrayMax;
    }

    FORCEINLINE ElementType* GetData()
    {
        return Data;
    }

    FORCEINLINE ElementType& operator[](int32 Index)
    {
        return GetData()[Index];
    }

    FORCEINLINE void IncreaseSize(int32 amount)
    {
        Data = (ElementType*)FMemory::Realloc(Data, sizeof(ElementType) * (ArrayMax + amount));
        ArrayMax += amount;
    }

    FORCEINLINE void Add(ElementType Val)
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
            FMemory::Free(Data);
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

    bool EndsWith(const wchar_t* Str)
    {
        int32 StrSize = wcslen(Str);
        int32 offset = Len() - StrSize;
        for (int32 i = offset; i < Len(); i++)
        {
            if (Data[i] != Str[i - offset])
            {
                return false;
            }
        }
        return true;
    }

    int32 FindLastOf(wchar_t Char)
    {
        int32 ret = -1;
        for (int32 i = 0; i < Len(); i++)
            if (Data[i] == Char)
                ret = i;
        return ret;
    }

    FString Substring(int32 start, int32 length)
    {
        FString ret;
        ret.Data.Data = (wchar_t*)FMemory::Malloc((length + 1) * sizeof(wchar_t));
        ret.Data.ArrayNum = length + 1;
        ret.Data.ArrayMax = length + 1;
        memcpy(
            (void*)ret.Data.Data,
            (void*)((uint64)Data.Data + (start * sizeof(wchar_t))),
            length * sizeof(wchar_t)
        );
        ret.Data.Data[length] = L'\0';
        return ret;
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

    FORCEINLINE FString Copy()
    {
        FString ret;
        ret.Data.ArrayMax = Data.ArrayMax;
        ret.Data.ArrayNum = Data.ArrayNum;
        ret.Data.Data = (wchar_t*)FMemory::Malloc(sizeof(wchar_t) * ret.Data.ArrayNum);
        memcpy(
            (void*)ret.Data.Data,
            (void*)Data.Data,
            ret.Data.ArrayNum * sizeof(wchar_t)
        );
        return ret;
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
    FORCEINLINE bool operator==(const wchar_t* Str)
    {
        return memcmp(Data.Data, Str, Len()) == 0;
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

template <typename ElementType>
struct TSparseArray
{
    typedef TSparseArrayElementOrFreeListLink<
        TAlignedBytes<sizeof(ElementType), alignof(ElementType)>
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

    ElementType& operator[](int32 Index)
    {
        return *(ElementType*)&GetData(Index).ElementData;
    }
};

template <typename InElementType>
struct TSetElement
{
    InElementType Value;
    mutable int32 HashNextId; // FSetElementId
    mutable int32 HashIndex;
};

template <typename ElementType>
struct TSet
{
    TSparseArray<TSetElement<ElementType>> Elements;
    mutable TInlineAllocator<1>::ForElementType<int32> Hash;
    mutable int32 HashSize;

    int32 Num() const
    {
        return Elements.Num();
    }

    FORCEINLINE ElementType& operator[](int32 Index)
    {
        return Elements[Index].Value;
    }
};

template <typename FirstType, typename SecondType>
struct TPair
{
    FirstType First;
    SecondType Second;
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
    static void* KismetStringLibrary = FindObject(L"/Script/Engine.KismetStringLibrary");
    if (!KismetStringLibrary)
    {
        BasicMessageBox("Didn't find KismetStringLibrary :(");
        return FString();
    }
    static void* Conv_NameToString = FindObject(L"/Script/Engine.KismetStringLibrary.Conv_NameToString");
    if (!Conv_NameToString)
    {
        BasicMessageBox("Didn't find Conv_NameToString :(");
        return FString();
    }

    struct {
        FName InName;
        FString ReturnValue;
    } Parms{ Name };

    ProcessEvent(KismetStringLibrary, Conv_NameToString, &Parms);
    return Parms.ReturnValue;
}

FString GetCPPType(void* Child)
{
#pragma region Hide the copy paste
#define CheckPropClass(Class) if (!Class) { BasicMessageBox("Didn't Find " #Class " :("); return L""; }
    static void* BoolPropertyClass = FindObject(L"/Script/CoreUObject.BoolProperty");
    static void* ArrayPropertyClass = FindObject(L"/Script/CoreUObject.ArrayProperty");
    static void* EnumPropertyClass = FindObject(L"/Script/CoreUObject.EnumProperty");
    static void* NumericPropertyClass = FindObject(L"/Script/CoreUObject.NumericProperty");
    static void* BytePropertyClass = FindObject(L"/Script/CoreUObject.ByteProperty");
    static void* ObjectPropertyClass = FindObject(L"/Script/CoreUObject.ObjectProperty");
    static void* ClassPropertyClass = FindObject(L"/Script/CoreUObject.ClassProperty");
    static void* DelegatePropertyClass = FindObject(L"/Script/CoreUObject.DelegateProperty");
    static void* DoublePropertyClass = FindObject(L"/Script/CoreUObject.DoubleProperty");
    static void* FloatPropertyClass = FindObject(L"/Script/CoreUObject.FloatProperty");
    static void* IntPropertyClass = FindObject(L"/Script/CoreUObject.IntProperty");
    static void* Int16PropertyClass = FindObject(L"/Script/CoreUObject.Int16Property");
    static void* Int64PropertyClass = FindObject(L"/Script/CoreUObject.Int64Property");
    static void* Int8PropertyClass = FindObject(L"/Script/CoreUObject.Int8Property");
    static void* InterfacePropertyClass = FindObject(L"/Script/CoreUObject.InterfaceProperty");
    static void* LazyObjectPropertyClass = FindObject(L"/Script/CoreUObject.LazyObjectProperty");
    static void* MapPropertyClass = FindObject(L"/Script/CoreUObject.MapProperty");
    static void* MulticastDelegatePropertyClass = FindObject(L"/Script/CoreUObject.MulticastDelegateProperty");
    static void* NamePropertyClass = FindObject(L"/Script/CoreUObject.NameProperty");
    static void* SetPropertyClass = FindObject(L"/Script/CoreUObject.SetProperty");
    static void* SoftObjectPropertyClass = FindObject(L"/Script/CoreUObject.SoftObjectProperty");
    static void* SoftClassPropertyClass = FindObject(L"/Script/CoreUObject.SoftClassProperty");
    static void* StrPropertyClass = FindObject(L"/Script/CoreUObject.StrProperty");
    static void* StructPropertyClass = FindObject(L"/Script/CoreUObject.StructProperty");
    static void* UInt16PropertyClass = FindObject(L"/Script/CoreUObject.UInt16Property");
    static void* UInt32PropertyClass = FindObject(L"/Script/CoreUObject.UInt32Property");
    static void* UInt64PropertyClass = FindObject(L"/Script/CoreUObject.UInt64Property");
    static void* WeakObjectPropertyClass = FindObject(L"/Script/CoreUObject.WeakObjectProperty");
    static void* TextPropertyClass = FindObject(L"/Script/CoreUObject.TextProperty");
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
    if (ClassPrivate == BoolPropertyClass)   return L"bool";
    else if (ClassPrivate == FloatPropertyClass)  return L"float";
    else if (ClassPrivate == DoublePropertyClass) return L"double";
    else if (ClassPrivate == IntPropertyClass)    return L"int32";
    else if (ClassPrivate == Int16PropertyClass)  return L"int16";
    else if (ClassPrivate == Int64PropertyClass)  return L"int64";
    else if (ClassPrivate == Int8PropertyClass)   return L"int8";
    else if (ClassPrivate == UInt16PropertyClass) return L"uint16";
    else if (ClassPrivate == UInt32PropertyClass) return L"uint32";
    else if (ClassPrivate == UInt64PropertyClass) return L"uint64";
    else if (ClassPrivate == StrPropertyClass)    return L"FString";
    else if (ClassPrivate == NamePropertyClass)   return L"FName";
    else if (ClassPrivate == TextPropertyClass)   return L"FText";
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
        else return L"UClass*";
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

    PRINT("Unknown type %s", FNameToString(GET_OFFSET(FName, ClassPrivate, Offsets::NamePrivateOffset)).ToString().c_str());

    return L"Unknown";
}

struct UObject
{
    FName& NamePrivate()
    {
        return GET_OFFSET(FName, this, Offsets::NamePrivateOffset);
    }

    struct UStruct*& ClassPrivate()
    {
        return GET_OFFSET(struct UStruct*, this, Offsets::ClassPrivateOffset);
    }


};

struct UField : public UObject
{
    UField*& Next()
    {
        return GET_OFFSET(UField*, this, Offsets::UFieldNextOffset);
    }
};

struct UProperty : public UField
{
    uint64& PropertyFlags()
    {
        return GET_OFFSET(uint64, this, Offsets::PropertyFlagsOffset);
    }

    int32& Offset_Internal()
    {
        return GET_OFFSET(int32, this, Offsets::Offset_Internal);
    }
};

struct UStruct : public UField
{
    UStruct*& SuperStruct()
    {
        return GET_OFFSET(UStruct*, this, Offsets::SuperStruct);
    }

    UField*& Children()
    {
        return GET_OFFSET(UField*, this, Offsets::ChildrenOffset);
    }

    int32 GetChildOffset(const wchar_t* Name)
    {
        for (auto Struct = this; Struct; Struct = Struct->SuperStruct())
        {
            for (auto Child = Struct->Children(); Child; Child = Child->Next())
            {
                auto ChildName = FNameToString(Child->NamePrivate());
                if (ChildName == Name)
                {
                    ChildName.Free();

                    return ((UProperty*)Child)->Offset_Internal();
                }
                ChildName.Free();
            }
        }

        return -1;
    }
};

struct UFunction : public UStruct
{
    uint32& FunctionFlags()
    {
        return GET_OFFSET(uint32, this, Offsets::FunctionFlagsOffset);
    }

    static void* StaicClass()
    {
        static void* FunctionClass = FindObject(L"/Script/CoreUObject.Function");
        return FunctionClass;
    }
};

struct UConsoleSettings : public UObject
{
    FColor& AutoCompleteCommandColor()
    {
        return GET_OFFSET(FColor, this, Offsets::AutoCompleteCommandColorOffset);
    }
    FColor& AutoCompleteCVarColor()
    {
        return GET_OFFSET(FColor, this, Offsets::AutoCompleteCVarColorOffset);
    }
    FColor& AutoCompleteFadedColor()
    {
        return GET_OFFSET(FColor, this, Offsets::AutoCompleteFadedColorOffset);
    }
    TArray<FAutoCompleteCommand>& ManualAutoCompleteList()
    {
        return GET_OFFSET(TArray<FAutoCompleteCommand>, this, Offsets::ManualAutoCompleteListOffset);
    }
    TArray<FString>& AutoCompleteMapPaths()
    {
        return GET_OFFSET(TArray<FString>, this, Offsets::AutoCompleteMapPathsOffset);
    }
};

struct FConsoleManager
{
    TMap<FString, void*>& ConsoleObjects()
    {
        return *((TMap<FString, void*>*)((uint64)this + Offsets::ConsoleObjectsOffset));
    }
};