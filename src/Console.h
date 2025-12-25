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

    ~FAutoCompleteNode()
    {
        for (int32 Idx = 0; Idx < ChildNodes.Num(); Idx++)
        {
            FAutoCompleteNode* Node = ChildNodes[Idx];
            delete Node;
        }
        
        ChildNodes.Clear();
    }
};

struct FColor
{
    union { struct{ uint8 B,G,R,A; }; uint32 Bits; };
};

struct FAutoCompleteCommand
{
    FString Command;
    FString Decs;
    FColor Color;
};

struct UConsole : UObject
{
};
