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
    FString Desc;
    FColor Color;
};

struct UConsole : UObject
{
    // TODO Move offsets to Offsets::UConsole
    UObject* GetConsoleSettings()
    {
        return GetChild(0x118);
    }

    FAutoCompleteNode& GetAutoCompleteTree()
    {
        return BaseGetChild<FAutoCompleteNode>(0xE0);
    }

    TArray<FAutoCompleteCommand>& GetAutoCompleteList()
    {
        return BaseGetChild<TArray<FAutoCompleteCommand>>(0xB8);
    }

    void BuildRuntimeAutoCompleteList()
    {
        auto& AutoCompleteTree = GetAutoCompleteTree();
        auto& AutoCompleteList = GetAutoCompleteList();

        auto ConsoleSettings = GetConsoleSettings();
        auto AutoCompleteCommandColor = ConsoleSettings->GetChild<FColor>("AutoCompleteCommandColor");

        for (int32 Idx = 0; Idx < AutoCompleteTree.ChildNodes.Num(); Idx++)
        {
            FAutoCompleteNode* Node = AutoCompleteTree.ChildNodes[Idx];
            delete Node;
        }

        AutoCompleteTree.ChildNodes.Clear();

        AutoCompleteList.Clear();
        {
            static FAutoCompleteCommand ManualCommands[] = {
                { L"givemecheats", L"Spawns CheatManager for local player", AutoCompleteCommandColor },
                { L"dumpobjects", L"Object dump into objects.txt", AutoCompleteCommandColor },
            };
            static int32 ManualCommandsSize = sizeof(ManualCommands) / sizeof(ManualCommands[0]);

            for (int i = 0; i < ManualCommandsSize; i++)
            {
                AutoCompleteList.Add(ManualCommands[i]);
            }
        }

        for (int32 ListIdx = 0; ListIdx < AutoCompleteList.Num(); ListIdx++)
        {
            FString Command = AutoCompleteList[ListIdx].Command;
            FAutoCompleteNode* Node = &AutoCompleteTree;
            for (int32 Depth = 0; Depth < Command.Len(); Depth++)
            {
                int32 Char = std::towlower(Command[Depth]);
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
    }
};
