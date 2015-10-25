#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

//#define DEBUG_NO_ASSERT
#define DEBUG_NO_WINDOWS_ERRORS
#include "debug.h"

// Local debug switches
//#define DEBUG_BOX_FIND_CHILDREN
//#define DEBUG_BOX_MARK
//#define DEBUG_MISC

// Use the Row/Column convention to iterate over maps. The first index is the
// row, the second is the column. There are Height rows and Width columns.
typedef struct {
    unsigned int Height, Width;
    char** Tiles;
} MapT;

// Creates and allocates a MapT. Puts one byte of padding on each row, for the
// null terminator. Note that the map will be filled with '\0's - PrintMap will
// fail to print it properly.
MapT CreateMap (unsigned int Height, unsigned int Width) {
    MapT Map = { Height, Width, NULL };
    Map.Tiles = (char**) calloc(Height, sizeof(char*));
    for (unsigned int Row = 0; Row < Height; Row++) {
        Map.Tiles[Row] = (char*) calloc(Width + 1, sizeof(char));
    }
    return Map;
}

// Returns a copy of a given MapT.
MapT CopyMap (MapT SourceMap) {
    MapT Map = CreateMap(SourceMap.Height, SourceMap.Width);
    for (unsigned int Row = 0; Row < Map.Height; Row++) {
        for (unsigned int Col = 0; Col < Map.Width; Col++) {
            Map.Tiles[Row][Col] = SourceMap.Tiles[Row][Col];
        }
    }
    return Map;
}

// Frees a MapT from memory.
void FreeMap (MapT Map) {
    free(Map.Tiles);
}

// Reads a map from a given filename.
MapT ReadMapFromFile (const char* SourceFilename) {
    FILE* File = fopen(SourceFilename, "r");
    unsigned int Height, Width;
    fscanf(File, "%u %u\n", &Height, &Width);
    MapT Map = CreateMap(Height, Width);

    for (unsigned int Row = 0; Row < Map.Height; Row++) {
        for (unsigned int Col = 0; Col < Map.Width; Col++) {
            fscanf(File, "%c", &Map.Tiles[Row][Col]);
        }
        Map.Tiles[Row][Map.Width] = '\0';
        fscanf(File, "\n");
    }

    fclose(File);
    return Map;
}

// Prints a map. Note that this relies on each row of the map being null
// terminated. A freshly-created map will not print properly until every null
// inside it has been replaced.
void PrintMap (MapT Map) {
    for (unsigned int Row = 0; Row < Map.Height; Row++) {
        LogS(Map.Tiles[Row]);
        LogS("\n");
    }
}

// Boxes have two points (top-left, bottom-right) and an array of children.
typedef struct BoxStruct {
    unsigned int Top, Left;
    unsigned int Bottom, Right;
    unsigned int ChildrenCount;
    unsigned int ChildrenAllocated;
    struct BoxStruct* Children;
} BoxT;

// Create a box given the top, left, bottom and right coordinates.
BoxT BoxCreate (unsigned int Top, unsigned int Left, unsigned int Bottom,
    unsigned int Right) {
    BoxT Box = { Top, Left, Bottom, Right, 0, 0, NULL };
    return Box;
}

// Attach a child to a BoxT.
void BoxAddChild (BoxT *Parent, BoxT Child) {
    Parent->ChildrenCount++;
    if (Parent->ChildrenAllocated == 0 || Parent->Children == NULL) {
        Parent->ChildrenAllocated = 1;
        Parent->Children = (struct BoxStruct*) calloc(Parent->ChildrenAllocated,
            sizeof(struct BoxStruct));
    } else if (Parent->ChildrenCount > Parent->ChildrenAllocated) {
        Parent->ChildrenAllocated *= 2;
        Parent->Children = (struct BoxStruct*) realloc(Parent->Children,
            Parent->ChildrenAllocated * sizeof(BoxT));
        for (unsigned int I = Parent->ChildrenCount;
        I < Parent->ChildrenAllocated; I++) {
            Parent->Children[I].Top = 0;
            Parent->Children[I].Left = 0;
            Parent->Children[I].Bottom = 0;
            Parent->Children[I].Right = 0;
            Parent->Children[I].ChildrenCount = 0;
            Parent->Children[I].ChildrenAllocated = 0;
            Parent->Children[I].Children = NULL;
        }
    }
    Parent->Children[Parent->ChildrenCount - 1] = Child;
}

// Check if a box is zero-sized.
bool BoxIsZeroSized (BoxT Box) {
    return ((Box.Top == Box.Bottom) || (Box.Left == Box.Right));
}

// Check if a box is valid (i.e. is correctly represented on the map).
bool BoxIsValid (MapT Map, BoxT Box) {
    if (BoxIsZeroSized(Box)) return false;
    if (!(Map.Tiles[Box.Top][Box.Left] == '+')) return false;
    if (!(Map.Tiles[Box.Bottom][Box.Right] == '+')) return false;
    for (unsigned int Row = Box.Top + 1; Row < Box.Bottom; Row++) {
        if (!(Map.Tiles[Row][Box.Left] == '|')) return false;
        if (!(Map.Tiles[Row][Box.Right] == '|')) return false;
    }
    for (unsigned int Col = Box.Left + 1; Col < Box.Right; Col++) {
        if (!(Map.Tiles[Box.Top][Col] == '-')) return false;
        if (!(Map.Tiles[Box.Bottom][Col] == '-')) return false;
    }
    return true;
}

// Return the box starting at the given top-left coords if it exists, and a
// zeroed out box if not. This implementation will not work properly with
// touching boxes; it will find boxes that touch other boxes on their left or
// bottom sides, but not on the top or right sides, AFAICT.
BoxT FindBoxAt (MapT Map, unsigned int Top, unsigned int Left) {
    BoxT EmptyBox = { 0 };
    unsigned int Row = Top;
    unsigned int Col = Left;

    if (Map.Tiles[Row][Col] != '+') return EmptyBox;
    Col++;
    while (Col < Map.Width && Map.Tiles[Row][Col] == '-') Col++;
    if (Col >= Map.Width) return EmptyBox;
    
    if (Map.Tiles[Row][Col] != '+') return EmptyBox;
    Row++;
    while (Row < Map.Height && Map.Tiles[Row][Col] == '|') Row++;
    if (Row >= Map.Height) return EmptyBox;

    BoxT Box = BoxCreate(Top, Left, Row, Col);
    if (!BoxIsValid(Map, Box)) return EmptyBox;
    return Box;
}

// Replace a rectangular session on a map with a given character.
void MapReplaceRectangle(MapT Map, char Char, unsigned int Top,
    unsigned int Left, unsigned int Bottom, unsigned int Right) {
    for (unsigned int Row = Top; Row <= Bottom; Row++) {
        for (unsigned int Col = Left; Col <= Right; Col++) {
            Map.Tiles[Row][Col] = Char;
        }
    }
}

// Find a box's children.
void BoxFindChildren (MapT Map, BoxT *Box, bool Recursive) {
    MapT MarkMap = CopyMap(Map);
    for (unsigned int Row = Box->Top + 1; Row < Box->Bottom; Row++) {
        for (unsigned int Col = Box->Left + 1; Col < Box->Right; Col++) {
            BoxT BoxAtPosition = FindBoxAt(MarkMap, Row, Col);
            if (!BoxIsZeroSized(BoxAtPosition)) {
                Assert(BoxAtPosition.Top == Row && BoxAtPosition.Left == Col);
                
                MapReplaceRectangle(MarkMap, '#',
                    BoxAtPosition.Top, BoxAtPosition.Left,
                    BoxAtPosition.Bottom, BoxAtPosition.Right);
                
                #ifdef DEBUG_BOX_FIND_CHILDREN
                LogF("Found box from top %u left %u to bottom %u right %u\n",
                    BoxAtPosition.Top, BoxAtPosition.Left,
                    BoxAtPosition.Bottom, BoxAtPosition.Right);
                PrintMap(MarkMap);
                #endif

                if (Recursive) {
                    BoxFindChildren(Map, &BoxAtPosition, true);
                }
                BoxAddChild(Box, BoxAtPosition);
            }
        }
    }
}

// Recursively mark a box and its children on a map depending on their depth.
void BoxMarkChildren(MapT Map, BoxT Box, unsigned int Depth) {
    #ifdef DEBUG_BOX_MARK
    char DepthChar = (char) (48 + Depth);
    #else
    char DepthChar = ' ';
    switch (Depth) {
    case 0:
        DepthChar = '#';
        break;
    case 1:
        DepthChar = '=';
        break;
    case 2:
        DepthChar = '-';
        break;
    case 3:
        DepthChar = '.';
        break;
    }
    #endif

    Map.Tiles[Box.Top][Box.Left] = '+';
    Map.Tiles[Box.Bottom][Box.Left] = '+';
    Map.Tiles[Box.Top][Box.Right] = '+';
    Map.Tiles[Box.Bottom][Box.Right] = '+';
    for (unsigned int Row = Box.Top + 1; Row < Box.Bottom; Row++) {
        Map.Tiles[Row][Box.Left] = '|';
        Map.Tiles[Row][Box.Right] = '|';
    }
    for (unsigned int Col = Box.Left + 1; Col < Box.Right; Col++) {
        Map.Tiles[Box.Top][Col] = '-';
        Map.Tiles[Box.Bottom][Col] = '-';
    }
    for (unsigned int Row = Box.Top + 1; Row < Box.Bottom; Row++) {
        for (unsigned int Col = Box.Left + 1; Col < Box.Right; Col++) {
            Map.Tiles[Row][Col] = DepthChar;
        }
    }
    for (unsigned int ChildIdx = 0; ChildIdx < Box.ChildrenCount; ChildIdx++) {
        BoxMarkChildren(Map, Box.Children[ChildIdx], Depth + 1);
    }
}

// Runs the program for a specific filename.
void Run (const char* SourceFilename) {
    MapT Map = ReadMapFromFile(SourceFilename);

    #ifdef DEBUG_MISC
    PrintMap(Map);
    #endif

    BoxT RootBox = BoxCreate(0, 0, Map.Height - 1, Map.Width - 1);
    CheckM(BoxIsValid(Map, RootBox), "Root box not found");

    BoxFindChildren(Map, &RootBox, true);

    MapT MarkMap = CreateMap(Map.Height, Map.Width);
    BoxMarkChildren(MarkMap, RootBox, 0);
    
    #ifdef DEBUG_MISC
    LogF("Marked final map:\n");
    #endif
    
    PrintMap(MarkMap);
    LogS("\n");
}

int main (int ArgumentCount, const char** Arguments) {
    Run("examples/1.txt");
    Run("examples/2.txt");
    Run("examples/3.txt");
    return 0;
}