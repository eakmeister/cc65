/*****************************************************************************/
/*                                                                           */
/*                                 dbginfo.h                                 */
/*                                                                           */
/*                         cc65 debug info handling                          */
/*                                                                           */
/*                                                                           */
/*                                                                           */
/* (C) 2010-2011, Ullrich von Bassewitz                                      */
/*                Roemerstrasse 52                                           */
/*                D-70794 Filderstadt                                        */
/* EMail:         uz@cc65.org                                                */
/*                                                                           */
/*                                                                           */
/* This software is provided 'as-is', without any expressed or implied       */
/* warranty.  In no event will the authors be held liable for any damages    */
/* arising from the use of this software.                                    */
/*                                                                           */
/* Permission is granted to anyone to use this software for any purpose,     */
/* including commercial applications, and to alter it and redistribute it    */
/* freely, subject to the following restrictions:                            */
/*                                                                           */
/* 1. The origin of this software must not be misrepresented; you must not   */
/*    claim that you wrote the original software. If you use this software   */
/*    in a product, an acknowledgment in the product documentation would be  */
/*    appreciated but is not required.                                       */
/* 2. Altered source versions must be plainly marked as such, and must not   */
/*    be misrepresented as being the original software.                      */
/* 3. This notice may not be removed or altered from any source              */
/*    distribution.                                                          */
/*                                                                           */
/*****************************************************************************/



#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <assert.h>
#include <errno.h>

#include "dbginfo.h"



/*****************************************************************************/
/*     	       	       	 	     Data				     */
/*****************************************************************************/



/* Use this for debugging - beware, lots of output */
#define DEBUG           0

/* Version numbers of the debug format we understand */
#define VER_MAJOR       2U
#define VER_MINOR       0U

/* Dynamic strings */
typedef struct StrBuf StrBuf;
struct StrBuf {
    char*       Buf;                    /* Pointer to buffer */
    unsigned    Len;                    /* Length of the string */
    unsigned    Allocated;              /* Size of allocated memory */
};

/* Initializer for a string buffer */
#define STRBUF_INITIALIZER      { 0, 0, 0 }

/* An array of pointers that grows if needed */
typedef struct Collection Collection;
struct Collection {
    unsigned   	       	Count;		/* Number of items in the list */
    unsigned   	       	Size;		/* Size of allocated array */
    void**	    	Items;		/* Array with dynamic size */
};

/* Initializer for static collections */
#define COLLECTION_INITIALIZER  { 0, 0, 0 }

/* Line info management */
typedef struct LineInfoListEntry LineInfoListEntry;
struct LineInfoListEntry {
    cc65_addr           Addr;           /* Unique address */
    unsigned            Count;          /* Number of LineInfos for this address */
    void*               Data;           /* Either LineInfo* or LineInfo** */
};

typedef struct LineInfoList LineInfoList;
struct LineInfoList {
    unsigned            Count;          /* Number of entries */
    LineInfoListEntry*  List;           /* Dynamic array with entries */
};

/* Input tokens */
typedef enum {

    TOK_INVALID,                        /* Invalid token */
    TOK_EOF,                            /* End of file reached */

    TOK_INTCON,                         /* Integer constant */
    TOK_STRCON,                         /* String constant */

    TOK_EQUAL,                          /* = */
    TOK_COMMA,                          /* , */
    TOK_MINUS,                          /* - */
    TOK_PLUS,                           /* + */
    TOK_EOL,                            /* \n */

    TOK_FIRST_KEYWORD,
    TOK_ABSOLUTE = TOK_FIRST_KEYWORD,   /* ABSOLUTE keyword */
    TOK_ADDRSIZE,                       /* ADDRSIZE keyword */
    TOK_COUNT,                          /* COUNT keyword */
    TOK_ENUM,                           /* ENUM keyword */
    TOK_EQUATE,                         /* EQUATE keyword */
    TOK_FILE,                           /* FILE keyword */
    TOK_GLOBAL,                         /* GLOBAL keyword */
    TOK_ID,                             /* ID keyword */
    TOK_INFO,                           /* INFO keyword */
    TOK_LABEL,                          /* LABEL keyword */
    TOK_LIBRARY,                        /* LIBRARY keyword */
    TOK_LINE,                           /* LINE keyword */
    TOK_LONG,                           /* LONG_keyword */
    TOK_MAJOR,                          /* MAJOR keyword */
    TOK_MINOR,                          /* MINOR keyword */
    TOK_MODULE,                         /* MODULE keyword */
    TOK_MTIME,                          /* MTIME keyword */
    TOK_NAME,                           /* NAME keyword */
    TOK_OUTPUTNAME,                     /* OUTPUTNAME keyword */
    TOK_OUTPUTOFFS,                     /* OUTPUTOFFS keyword */
    TOK_PARENT,                         /* PARENT keyword */
    TOK_RANGE,                          /* RANGE keyword */
    TOK_RO,                             /* RO keyword */
    TOK_RW,                             /* RW keyword */
    TOK_SCOPE,                          /* SCOPE keyword */
    TOK_SEGMENT,                        /* SEGMENT keyword */
    TOK_SIZE,                           /* SIZE keyword */
    TOK_START,                          /* START keyword */
    TOK_STRUCT,                         /* STRUCT keyword */
    TOK_SYM,                            /* SYM keyword */
    TOK_TYPE,                           /* TYPE keyword */
    TOK_VALUE,                          /* VALUE keyword */
    TOK_VERSION,                        /* VERSION keyword */
    TOK_ZEROPAGE,                       /* ZEROPAGE keyword */
    TOK_LAST_KEYWORD = TOK_ZEROPAGE,

    TOK_IDENT,                          /* To catch unknown keywords */
} Token;



/* Data structure containing information from the debug info file. A pointer
 * to this structure is passed as handle to callers from the outside.
 */
typedef struct DbgInfo DbgInfo;
struct DbgInfo {

    /* First we have all items in collections sorted by id. The ids are
     * continous, so an access by id is almost as cheap as an array access.
     * The collections are also used when the objects are deleted, so they're
     * actually the ones that "own" the items.
     */
    Collection          FileInfoById;   /* File infos sorted by id */
    Collection          LibInfoById;    /* Library infos sorted by id */
    Collection          LineInfoById;   /* Line infos sorted by id */
    Collection          ModInfoById;    /* Module infos sorted by id */
    Collection          ScopeInfoById;  /* Scope infos sorted by id */
    Collection          SegInfoById;    /* Segment infos sorted by id */
    Collection          SymInfoById;    /* Symbol infos sorted by id */

    /* Collections with other sort criteria */
    Collection          FileInfoByName; /* File infos sorted by name */
    Collection          SegInfoByName;  /* Segment infos sorted by name */
    Collection          SymInfoByName;  /* Symbol infos sorted by name */
    Collection          SymInfoByVal;   /* Symbol infos sorted by value */

    /* Other stuff */
    LineInfoList        LineInfoByAddr; /* Line infos sorted by unique address */
};

/* Data used when parsing the debug info file */
typedef struct InputData InputData;
struct InputData {
    const char*         FileName;       /* Name of input file */
    cc65_line           Line;           /* Current line number */
    unsigned            Col;            /* Current column number */
    cc65_line           SLine;          /* Line number at start of token */
    unsigned            SCol;           /* Column number at start of token */
    unsigned            Errors;         /* Number of errors */
    FILE*               F;              /* Input file */
    int                 C;              /* Input character */
    Token               Tok;            /* Token from input stream */
    unsigned long       IVal;           /* Integer constant */
    StrBuf              SVal;           /* String constant */
    cc65_errorfunc      Error;          /* Function called in case of errors */
    unsigned            MajorVersion;   /* Major version number */
    unsigned            MinorVersion;   /* Minor version number */
    DbgInfo*            Info;           /* Pointer to debug info */
};

/* Typedefs for the item structures. Do also serve as forwards */
typedef struct FileInfo FileInfo;
typedef struct LibInfo LibInfo;
typedef struct LineInfo LineInfo;
typedef struct ModInfo ModInfo;
typedef struct ScopeInfo ScopeInfo;
typedef struct SegInfo SegInfo;
typedef struct SymInfo SymInfo;

/* Internally used file info struct */
struct FileInfo {
    unsigned            Id;             /* Id of file */
    unsigned long       Size;           /* Size of file */
    unsigned long       MTime;          /* Modification time */
    union {
        unsigned        Id;             /* Id of module */
        ModInfo*        Info;           /* Pointer to module info */
    } Mod;
    Collection          LineInfoByLine; /* Line infos sorted by line */
    char                Name[1];        /* Name of file with full path */
};

/* Internally used library info struct */
struct LibInfo {
    unsigned            Id;             /* Id of library */
    char                Name[1];        /* Name of library with path */
};

/* Internally used line info struct */
struct LineInfo {
    unsigned            Id;             /* Id of line info */
    cc65_addr           Start;          /* Start of data range */
    cc65_addr           End;            /* End of data range */
    cc65_line           Line;           /* Line number */
    union {
        unsigned        Id;             /* Id of file */
        FileInfo*       Info;           /* Pointer to file info */
    } File;
    union {
        unsigned        Id;             /* Id of segment */
        SegInfo*        Info;           /* Pointer to segment info */
    } Seg;
    cc65_line_type      Type;           /* Type of line */
    unsigned            Count;          /* Nesting counter for macros */
};

/* Internally used module info struct */
struct ModInfo {
    unsigned            Id;             /* Id of library */
    union {
        unsigned        Id;             /* Id of main source file */
        FileInfo*       Info;           /* Pointer to file info */
    } File;
    union {
        unsigned        Id;             /* Id of library if any */
        LibInfo*        Info;           /* Pointer to library info */
    } Lib;
    char                Name[1];        /* Name of module with path */
};

/* Internally used scope info struct */
struct ScopeInfo {
    unsigned            Id;             /* Id of scope */
    cc65_scope_type     Type;           /* Type of scope */
    cc65_size           Size;           /* Size of scope */
    union {
        unsigned        Id;             /* Id of module */
        ModInfo*        Info;           /* Pointer to module info */
    } Mod;
    union {
        unsigned        Id;             /* Id of parent scope */
        ScopeInfo*      Info;           /* Pointer to parent scope */
    } Parent;
    union {
        unsigned        Id;             /* Id of label symbol */
        SymInfo*        Info;           /* Pointer to label symbol */
    } Label;
    char                Name[1];        /* Name of scope */
};

/* Internally used segment info struct */
struct SegInfo {
    unsigned            Id;             /* Id of segment */
    cc65_addr           Start;          /* Start address of segment */
    cc65_addr           Size;           /* Size of segment */
    char*               OutputName;     /* Name of output file */
    unsigned long       OutputOffs;     /* Offset in output file */
    char                Name[1];        /* Name of segment */
};

/* Internally used symbol info struct */
struct SymInfo {
    unsigned            Id;             /* Id of symbol */
    cc65_symbol_type    Type;           /* Type of symbol */
    long                Value;          /* Value of symbol */
    cc65_size           Size;           /* Size of symbol */
    union {
        unsigned        Id;             /* Id of segment if any */
        SegInfo*        Info;           /* Pointer to segment if any */
    } Seg;
    union {
        unsigned        Id;             /* Id of symbol scope */
        ScopeInfo*      Info;           /* Pointer to symbol scope */
    } Scope;
    union {
        unsigned        Id;             /* Parent symbol if any */
        SymInfo*        Info;           /* Pointer to parent symbol if any */
    } Parent;
    char                Name[1];        /* Name of symbol */
};



/*****************************************************************************/
/*                                 Forwards                                  */
/*****************************************************************************/



static void NextToken (InputData* D);
/* Read the next token from the input stream */



/*****************************************************************************/
/*                             Memory allocation                             */
/*****************************************************************************/



static void* xmalloc (size_t Size)
/* Allocate memory, check for out of memory condition. Do some debugging */
{
    void* P = 0;

    /* Allow zero sized requests and return NULL in this case */
    if (Size) {

        /* Allocate memory */
        P = malloc (Size);

        /* Check for errors */
        assert (P != 0);
    }

    /* Return a pointer to the block */
    return P;
}



static void* xrealloc (void* P, size_t Size)
/* Reallocate a memory block, check for out of memory */
{
    /* Reallocate the block */
    void* N = realloc (P, Size);

    /* Check for errors */
    assert (N != 0 || Size == 0);

    /* Return the pointer to the new block */
    return N;
}



static void xfree (void* Block)
/* Free the block, do some debugging */
{
    free (Block);
}



/*****************************************************************************/
/*                              Dynamic strings                              */
/*****************************************************************************/



static void SB_Done (StrBuf* B)
/* Free the data of a string buffer (but not the struct itself) */
{
    if (B->Allocated) {
        xfree (B->Buf);
    }
}



static void SB_Realloc (StrBuf* B, unsigned NewSize)
/* Reallocate the string buffer space, make sure at least NewSize bytes are
 * available.
 */
{
    /* Get the current size, use a minimum of 8 bytes */
    unsigned NewAllocated = B->Allocated;
    if (NewAllocated == 0) {
       	NewAllocated = 8;
    }

    /* Round up to the next power of two */
    while (NewAllocated < NewSize) {
       	NewAllocated *= 2;
    }

    /* Reallocate the buffer. Beware: The allocated size may be zero while the
     * length is not. This means that we have a buffer that wasn't allocated
     * on the heap.
     */
    if (B->Allocated) {
        /* Just reallocate the block */
        B->Buf = xrealloc (B->Buf, NewAllocated);
    } else {
        /* Allocate a new block and copy */
        B->Buf = memcpy (xmalloc (NewAllocated), B->Buf, B->Len);
    }

    /* Remember the new block size */
    B->Allocated = NewAllocated;
}



static void SB_CheapRealloc (StrBuf* B, unsigned NewSize)
/* Reallocate the string buffer space, make sure at least NewSize bytes are
 * available. This function won't copy the old buffer contents over to the new
 * buffer and may be used if the old contents are overwritten later.
 */
{
    /* Get the current size, use a minimum of 8 bytes */
    unsigned NewAllocated = B->Allocated;
    if (NewAllocated == 0) {
     	NewAllocated = 8;
    }

    /* Round up to the next power of two */
    while (NewAllocated < NewSize) {
     	NewAllocated *= 2;
    }

    /* Free the old buffer if there is one */
    if (B->Allocated) {
        xfree (B->Buf);
    }

    /* Allocate a fresh block */
    B->Buf = xmalloc (NewAllocated);

    /* Remember the new block size */
    B->Allocated = NewAllocated;
}



static unsigned SB_GetLen (const StrBuf* B)
/* Return the length of the buffer contents */
{
    return B->Len;
}



static const char* SB_GetConstBuf (const StrBuf* B)
/* Return a buffer pointer */
{
    return B->Buf;
}



static void SB_Terminate (StrBuf* B)
/* Zero terminate the given string buffer. NOTE: The terminating zero is not
 * accounted for in B->Len, if you want that, you have to use AppendChar!
 */
{
    unsigned NewLen = B->Len + 1;
    if (NewLen > B->Allocated) {
       	SB_Realloc (B, NewLen);
    }
    B->Buf[B->Len] = '\0';
}



static void SB_Clear (StrBuf* B)
/* Clear the string buffer (make it empty) */
{
    B->Len = 0;
}



static void SB_CopyBuf (StrBuf* Target, const char* Buf, unsigned Size)
/* Copy Buf to Target, discarding the old contents of Target */
{
    if (Size) {
        if (Target->Allocated < Size) {
            SB_CheapRealloc (Target, Size);
        }
        memcpy (Target->Buf, Buf, Size);
    }
    Target->Len = Size;
}



static void SB_Copy (StrBuf* Target, const StrBuf* Source)
/* Copy Source to Target, discarding the old contents of Target */
{
    SB_CopyBuf (Target, Source->Buf, Source->Len);
}



static void SB_AppendChar (StrBuf* B, int C)
/* Append a character to a string buffer */
{
    unsigned NewLen = B->Len + 1;
    if (NewLen > B->Allocated) {
       	SB_Realloc (B, NewLen);
    }
    B->Buf[B->Len] = (char) C;
    B->Len = NewLen;
}



static char* SB_StrDup (const StrBuf* B)
/* Return the contents of B as a dynamically allocated string. The string
 * will always be NUL terminated.
 */
{
    /* Allocate memory */
    char* S = xmalloc (B->Len + 1);

    /* Copy the string */
    memcpy (S, B->Buf, B->Len);

    /* Terminate it */
    S[B->Len] = '\0';

    /* And return the result */
    return S;
}



/*****************************************************************************/
/*                                Collections                                */
/*****************************************************************************/



static Collection* InitCollection (Collection* C)
/* Initialize a collection and return it. */
{
    /* Intialize the fields. */
    C->Count = 0;
    C->Size  = 0;
    C->Items = 0;

    /* Return the new struct */
    return C;
}



static void DoneCollection (Collection* C)
/* Free the data for a collection. This will not free the data contained in
 * the collection.
 */
{
    /* Free the pointer array */
    xfree (C->Items);

    /* Clear the fields, so the collection may be reused (or DoneCollection
     * called) again
     */
    C->Count = 0;
    C->Size  = 0;
    C->Items = 0;
}



static unsigned CollCount (const Collection* C)
/* Return the number of items in the collection */
{
    return C->Count;
}



static void CollGrow (Collection* C, unsigned Size)
/* Grow the collection C so it is able to hold Size items without a resize
 * being necessary. This can be called for performance reasons if the number
 * of items to be placed in the collection is known in advance.
 */
{
    void** NewItems;

    /* Ignore the call if the collection is already large enough */
    if (Size <= C->Size) {
        return;
    }

    /* Grow the collection */
    C->Size = Size;
    NewItems = xmalloc (C->Size * sizeof (void*));
    memcpy (NewItems, C->Items, C->Count * sizeof (void*));
    xfree (C->Items);
    C->Items = NewItems;
}



static void CollInsert (Collection* C, void* Item, unsigned Index)
/* Insert the data at the given position in the collection */
{
    /* Check for invalid indices */
    assert (Index <= C->Count);

    /* Grow the array if necessary */
    if (C->Count >= C->Size) {
       	/* Must grow */
        CollGrow (C, (C->Size == 0)? 8 : C->Size * 2);
    }

    /* Move the existing elements if needed */
    if (C->Count != Index) {
       	memmove (C->Items+Index+1, C->Items+Index, (C->Count-Index) * sizeof (void*));
    }
    ++C->Count;

    /* Store the new item */
    C->Items[Index] = Item;
}



static void CollReplaceExpand (Collection* C, void* Item, unsigned Index)
/* If Index is a valid index for the collection, replace the item at this
 * position by the one passed. If the collection is too small, expand it,
 * filling unused pointers with NULL, then add the new item at the given
 * position.
 */
{
    if (Index < C->Count) {
        /* Collection is already large enough */
        C->Items[Index] = Item;
    } else {
        /* Must expand the collection */
        unsigned Size = C->Size;
        if (Size == 0) {
            Size = 8;
        }
        while (Index >= Size) {
            Size *= 2;
        }
        CollGrow (C, Size);

        /* Fill up unused slots with NULL */
        while (C->Count < Index) {
            C->Items[C->Count++] = 0;
        }

        /* Fill in the item */
        C->Items[C->Count++] = Item;
    }
}



static void CollAppend (Collection* C, void* Item)
/* Append an item to the end of the collection */
{
    /* Insert the item at the end of the current list */
    CollInsert (C, Item, C->Count);
}



static void* CollAt (Collection* C, unsigned Index)
/* Return the item at the given index */
{
    /* Check the index */
    assert (Index < C->Count);

    /* Return the element */
    return C->Items[Index];
}



static void* CollFirst (Collection* C)
/* Return the first item in a collection */
{
    /* We must have at least one entry */
    assert (C->Count > 0);

    /* Return the element */
    return C->Items[0];
}



static void CollQuickSort (Collection* C, int Lo, int Hi,
   	                   int (*Compare) (const void*, const void*))
/* Internal recursive sort function. */
{
    /* Get a pointer to the items */
    void** Items = C->Items;

    /* Quicksort */
    while (Hi > Lo) {
   	int I = Lo + 1;
   	int J = Hi;
   	while (I <= J) {
   	    while (I <= J && Compare (Items[Lo], Items[I]) >= 0) {
   	     	++I;
   	    }
   	    while (I <= J && Compare (Items[Lo], Items[J]) < 0) {
   	     	--J;
   	    }
   	    if (I <= J) {
		/* Swap I and J */
	    	void* Tmp = Items[I];
		Items[I]  = Items[J];
		Items[J]  = Tmp;
   	     	++I;
   	     	--J;
   	    }
      	}
   	if (J != Lo) {
	    /* Swap J and Lo */
	    void* Tmp = Items[J];
	    Items[J]  = Items[Lo];
	    Items[Lo] = Tmp;
   	}
	if (J > (Hi + Lo) / 2) {
	    CollQuickSort (C, J + 1, Hi, Compare);
	    Hi = J - 1;
	} else {
	    CollQuickSort (C, Lo, J - 1, Compare);
	    Lo = J + 1;
	}
    }
}



void CollSort (Collection* C, int (*Compare) (const void*, const void*))
/* Sort the collection using the given compare function. */
{
    if (C->Count > 1) {
        CollQuickSort (C, 0, C->Count-1, Compare);
    }
}



/*****************************************************************************/
/*                              Debugging stuff                              */
/*****************************************************************************/



#if DEBUG

/* Output */
#define DBGPRINT(format, ...)   printf ((format), __VA_ARGS__)



static void DumpFileInfo (Collection* FileInfos)
/* Dump a list of file infos */
{
    unsigned I;

    /* File info */
    for (I = 0; I < CollCount (FileInfos); ++I) {
        const FileInfo* FI = CollAt (FileInfos, I);
        printf ("File info %u:\n"
                "  Name:   %s\n"
                "  Size:   %lu\n"
                "  MTime:  %lu\n",
                FI->Id,
                FI->Name,
                (unsigned long) FI->Size,
                (unsigned long) FI->MTime);
    }
}



static void DumpOneLineInfo (unsigned Num, LineInfo* LI)
/* Dump one line info entry */
{
    printf ("  Index:  %u\n"
            "    File:   %s\n"
            "    Line:   %lu\n"
            "    Range:  0x%06lX-0x%06lX\n"
            "    Type:   %u\n"
            "    Count:  %u\n",
            Num,
            LI->File.Info->Name,
            (unsigned long) LI->Line,
            (unsigned long) LI->Start,
            (unsigned long) LI->End,
            LI->Type,
            LI->Count);
}



static void DumpLineInfo (LineInfoList* L)
/* Dump a list of line infos */
{
    unsigned I, J;

    /* Line info */
    for (I = 0; I < L->Count; ++I) {
        const LineInfoListEntry* E = &L->List[I];
        printf ("Addr:   %lu\n", (unsigned long) E->Addr);
        if (E->Count == 1) {
            DumpOneLineInfo (0, E->Data);
        } else {
            for (J = 0; J < E->Count; ++J) {
                DumpOneLineInfo (J, ((LineInfo**) E->Data)[J]);
            }
        }
    }
}



static void DumpData (InputData* D)
/* Dump internal data to stdout for debugging */
{
    /* Dump data */
    DumpFileInfo (&D->Info->FileInfoById);
    DumpLineInfo (&D->Info->LineInfoByAddr);
}
#else

/* No output */
#define DBGPRINT(format, ...)



#endif



/*****************************************************************************/
/*                                 File info                                 */
/*****************************************************************************/



static FileInfo* NewFileInfo (const StrBuf* Name)
/* Create a new FileInfo struct and return it */
{
    /* Allocate memory */
    FileInfo* F = xmalloc (sizeof (FileInfo) + SB_GetLen (Name));

    /* Initialize it */
    InitCollection (&F->LineInfoByLine);
    memcpy (F->Name, SB_GetConstBuf (Name), SB_GetLen (Name) + 1);

    /* Return it */
    return F;
}



static void FreeFileInfo (FileInfo* F)
/* Free a FileInfo struct */
{
    /* Delete the collection with the line infos */
    DoneCollection (&F->LineInfoByLine);

    /* Free the file info structure itself */
    xfree (F);
}



static cc65_sourceinfo* new_cc65_sourceinfo (unsigned Count)
/* Allocate and return a cc65_sourceinfo struct that is able to hold Count
 * entries. Initialize the count field of the returned struct.
 */
{
    cc65_sourceinfo* S = xmalloc (sizeof (*S) - sizeof (S->data[0]) +
                                  Count * sizeof (S->data[0]));
    S->count = Count;
    return S;
}



static void CopyFileInfo (cc65_sourcedata* D, const FileInfo* F)
/* Copy data from a FileInfo struct to a cc65_sourcedata struct */
{
    D->source_id    = F->Id;
    D->source_name  = F->Name;
    D->source_size  = F->Size;
    D->source_mtime = F->MTime;
}



static int CompareFileInfoByName (const void* L, const void* R)
/* Helper function to sort file infos in a collection by name */
{
    /* Sort by file name. If names are equal, sort by timestamp,
     * then sort by size. Which means, identical files will go
     * together.
     */
    int Res = strcmp (((const FileInfo*) L)->Name,
                      ((const FileInfo*) R)->Name);
    if (Res != 0) {
        return Res;
    }
    if (((const FileInfo*) L)->MTime > ((const FileInfo*) R)->MTime) {
        return 1;
    } else if (((const FileInfo*) L)->MTime < ((const FileInfo*) R)->MTime) {
        return -1;
    }
    if (((const FileInfo*) L)->Size > ((const FileInfo*) R)->Size) {
        return 1;
    } else if (((const FileInfo*) L)->Size < ((const FileInfo*) R)->Size) {
        return -1;
    } else {
        return 0;
    }
}



/*****************************************************************************/
/*                               Library info                                */
/*****************************************************************************/



static LibInfo* NewLibInfo (const StrBuf* Name)
/* Create a new LibInfo struct, intialize and return it */
{
    /* Allocate memory */
    LibInfo* L = xmalloc (sizeof (LibInfo) + SB_GetLen (Name));

    /* Initialize the name */
    memcpy (L->Name, SB_GetConstBuf (Name), SB_GetLen (Name) + 1);

    /* Return it */
    return L;
}



static void FreeLibInfo (LibInfo* L)
/* Free a LibInfo struct */
{
    xfree (L);
}



static cc65_libraryinfo* new_cc65_libraryinfo (unsigned Count)
/* Allocate and return a cc65_libraryinfo struct that is able to hold Count
 * entries. Initialize the count field of the returned struct.
 */
{
    cc65_libraryinfo* L = xmalloc (sizeof (*L) - sizeof (L->data[0]) +
                                   Count * sizeof (L->data[0]));
    L->count = Count;
    return L;
}



static void CopyLibInfo (cc65_librarydata* D, const LibInfo* L)
/* Copy data from a LibInfo struct to a cc65_librarydata struct */
{
    D->library_id   = L->Id;
    D->library_name = L->Name;
}



/*****************************************************************************/
/*                                 Line info                                 */
/*****************************************************************************/



static LineInfo* NewLineInfo (void)
/* Create a new LineInfo struct and return it */
{
    /* Allocate memory and return it */
    return xmalloc (sizeof (LineInfo));
}



static void FreeLineInfo (LineInfo* L)
/* Free a LineInfo struct */
{
    xfree (L);
}



static cc65_lineinfo* new_cc65_lineinfo (unsigned Count)
/* Allocate and return a cc65_lineinfo struct that is able to hold Count
 * entries. Initialize the count field of the returned struct.
 */
{
    cc65_lineinfo* L = xmalloc (sizeof (*L) - sizeof (L->data[0]) +
                                Count * sizeof (L->data[0]));
    L->count = Count;
    return L;
}



static void CopyLineInfo (cc65_linedata* D, const LineInfo* L)
/* Copy data from a LineInfo struct to a cc65_linedata struct */
{
    D->line_id      = L->Id;
    D->line_start   = L->Start;
    D->line_end     = L->End;
    D->source_name  = L->File.Info->Name;
    D->source_size  = L->File.Info->Size;
    D->source_mtime = L->File.Info->MTime;
    D->source_line  = L->Line;
    if (L->Seg.Info->OutputName) {
        D->output_name  = L->Seg.Info->OutputName;
        D->output_offs  = L->Seg.Info->OutputOffs + L->Start - L->Seg.Info->Start;
    } else {
        D->output_name  = 0;
        D->output_offs  = 0;
    }
    D->line_type    = L->Type;
    D->count        = L->Count;
}



static int CompareLineInfoByAddr (const void* L, const void* R)
/* Helper function to sort line infos in a collection by address. Line infos
 * with smaller start address are considered smaller. If start addresses are
 * equal, line infos with smaller end address are considered smaller. This
 * means, that when CompareLineInfoByAddr is used for sorting, a range with
 * identical start addresses will have smaller ranges first, followed by
 * larger ranges.
 */
{
    /* Sort by start of range */
    if (((const LineInfo*) L)->Start > ((const LineInfo*) R)->Start) {
        return 1;
    } else if (((const LineInfo*) L)->Start < ((const LineInfo*) R)->Start) {
        return -1;
    } else if (((const LineInfo*) L)->End > ((const LineInfo*) R)->End) {
        return 1;
    } else if (((const LineInfo*) L)->End < ((const LineInfo*) R)->End) {
        return -1;
    } else {
        return 0;
    }
}



static int CompareLineInfoByLine (const void* L, const void* R)
/* Helper function to sort line infos in a collection by line. If the line
 * is identical, sort by the address of the range.
 */
{
    if (((const LineInfo*) L)->Line > ((const LineInfo*) R)->Line) {
        return 1;
    } else if (((const LineInfo*) L)->Line < ((const LineInfo*) R)->Line) {
        return -1;
    } else {
        return CompareLineInfoByAddr (L, R);
    }
}



/*****************************************************************************/
/*                               LineInfoList                                */
/*****************************************************************************/



static void InitLineInfoList (LineInfoList* L)
/* Initialize a line info list */
{
    L->Count = 0;
    L->List  = 0;
}



static void CreateLineInfoList (LineInfoList* L, Collection* LineInfos)
/* Create a LineInfoList from a Collection with line infos. The collection
 * must be sorted by ascending start addresses.
 */
{
    unsigned I, J;
    LineInfo* LI;
    LineInfoListEntry* List;
    unsigned StartIndex;
    cc65_addr Start;
    cc65_addr Addr;
    cc65_addr End;

    /* Initialize and check if there's something to do */
    L->Count = 0;
    L->List  = 0;
    if (CollCount (LineInfos) == 0) {
        /* No entries */
        return;
    }

    /* Step 1: Determine the number of unique address entries needed */
    LI = CollFirst (LineInfos);
    L->Count += (LI->End - LI->Start) + 1;
    End = LI->End;
    for (I = 1; I < CollCount (LineInfos); ++I) {

        /* Get next entry */
        LI = CollAt (LineInfos, I);

        /* Check for additional unique addresses in this line info */
        if (LI->Start > End) {
            L->Count += (LI->End - LI->Start) + 1;
            End = LI->End;
        } else if (LI->End > End) {
            L->Count += (LI->End - End);
            End = LI->End;
        }

    }

    /* Step 2: Allocate memory and initialize it */
    L->List = List = xmalloc (L->Count * sizeof (*List));
    for (I = 0; I < L->Count; ++I) {
        List[I].Count = 0;
        List[I].Data  = 0;
    }

    /* Step 3: Determine the number of entries per unique address */
    List = L->List;
    LI = CollFirst (LineInfos);
    StartIndex = 0;
    Start = LI->Start;
    End = LI->End;
    for (J = StartIndex, Addr = LI->Start; Addr <= LI->End; ++J, ++Addr) {
        List[J].Addr = Addr;
        ++List[J].Count;
    }
    for (I = 1; I < CollCount (LineInfos); ++I) {

        /* Get next entry */
        LI = CollAt (LineInfos, I);

        /* Determine the start index of the next range. Line infos are sorted
         * by ascending start address, so the start address of the next entry
         * is always larger than the previous one - we don't need to check
         * that.
         */
        if (LI->Start <= End) {
            /* Range starts within out already known linear range */
            StartIndex += (unsigned) (LI->Start - Start);
            Start = LI->Start;
            if (LI->End > End) {
                End = LI->End;
            }
        } else {
            /* Range starts after the already known */
            StartIndex += (unsigned) (End - Start) + 1;
            Start = LI->Start;
            End = LI->End;
        }
        for (J = StartIndex, Addr = LI->Start; Addr <= LI->End; ++J, ++Addr) {
            List[J].Addr = Addr;
            ++List[J].Count;
        }
    }

    /* Step 4: Allocate memory for the indirect tables */
    for (I = 0, List = L->List; I < L->Count; ++I, ++List) {

        /* For a count of 1, we store the pointer to the lineinfo for this
         * address in the Data pointer directly. For counts > 1, we allocate
         * an array of pointers and reset the counter, so we can use it as
         * an index later. This is dangerous programming since it disables
         * all possible checks!
         */
        if (List->Count > 1) {
            List->Data = xmalloc (List->Count * sizeof (LineInfo*));
            List->Count = 0;
        }
    }

    /* Step 5: Enter the data into the table */
    List = L->List;
    LI = CollFirst (LineInfos);
    StartIndex = 0;
    Start = LI->Start;
    End = LI->End;
    for (J = StartIndex, Addr = LI->Start; Addr <= LI->End; ++J, ++Addr) {
        assert (List[J].Addr == Addr);
        if (List[J].Count == 1 && List[J].Data == 0) {
            List[J].Data = LI;
        } else {
            ((LineInfo**) List[J].Data)[List[J].Count++] = LI;
        }
    }
    for (I = 1; I < CollCount (LineInfos); ++I) {

        /* Get next entry */
        LI = CollAt (LineInfos, I);

        /* Determine the start index of the next range. Line infos are sorted
         * by ascending start address, so the start address of the next entry
         * is always larger than the previous one - we don't need to check
         * that.
         */
        if (LI->Start <= End) {
            /* Range starts within out already known linear range */
            StartIndex += (unsigned) (LI->Start - Start);
            Start = LI->Start;
            if (LI->End > End) {
                End = LI->End;
            }
        } else {
            /* Range starts after the already known */
            StartIndex += (unsigned) (End - Start) + 1;
            Start = LI->Start;
            End = LI->End;
        }
        for (J = StartIndex, Addr = LI->Start; Addr <= LI->End; ++J, ++Addr) {
            assert (List[J].Addr == Addr);
            if (List[J].Count == 1 && List[J].Data == 0) {
                List[J].Data = LI;
            } else {
                ((LineInfo**) List[J].Data)[List[J].Count++] = LI;
            }
        }
    }
}



static void DoneLineInfoList (LineInfoList* L)
/* Delete the contents of a line info list */
{
    unsigned I;

    /* Delete the line info and the indirect data */
    for (I = 0; I < L->Count; ++I) {

        /* Get a pointer to the entry */
        LineInfoListEntry* E = &L->List[I];

        /* Check for indirect memory */
        if (E->Count > 1) {
            /* LineInfo addressed indirectly */
            xfree (E->Data);
        }
    }

    /* Delete the list */
    xfree (L->List);
}



/*****************************************************************************/
/*                                Module info                                */
/*****************************************************************************/



static ModInfo* NewModInfo (const StrBuf* Name)
/* Create a new ModInfo struct, intialize and return it */
{
    /* Allocate memory */
    ModInfo* M = xmalloc (sizeof (ModInfo) + SB_GetLen (Name));

    /* Initialize the name */
    memcpy (M->Name, SB_GetConstBuf (Name), SB_GetLen (Name) + 1);

    /* Return it */
    return M;
}



static void FreeModInfo (ModInfo* M)
/* Free a ModInfo struct */
{
    xfree (M);
}



static cc65_moduleinfo* new_cc65_moduleinfo (unsigned Count)
/* Allocate and return a cc65_moduleinfo struct that is able to hold Count
 * entries. Initialize the count field of the returned struct.
 */
{
    cc65_moduleinfo* M = xmalloc (sizeof (*M) - sizeof (M->data[0]) +
                                  Count * sizeof (M->data[0]));
    M->count = Count;
    return M;
}



static void CopyModInfo (cc65_moduledata* D, const ModInfo* M)
/* Copy data from a ModInfo struct to a cc65_moduledata struct */
{
    D->module_id      = M->Id;
    D->module_name    = M->Name;
    D->source_id      = M->File.Info->Id;
    if (M->Lib.Info) {
        D->library_id = M->Lib.Info->Id;
    } else {
        D->library_id = CC65_INV_ID;
    }
}



/*****************************************************************************/
/*                                Scope info                                 */
/*****************************************************************************/



static ScopeInfo* NewScopeInfo (const StrBuf* Name)
/* Create a new ScopeInfo struct, intialize and return it */
{
    /* Allocate memory */
    ScopeInfo* S = xmalloc (sizeof (ScopeInfo) + SB_GetLen (Name));

    /* Initialize the name */
    memcpy (S->Name, SB_GetConstBuf (Name), SB_GetLen (Name) + 1);

    /* Return it */
    return S;
}



static void FreeScopeInfo (ScopeInfo* S)
/* Free a ScopeInfo struct */
{
    xfree (S);
}



static cc65_scopeinfo* new_cc65_scopeinfo (unsigned Count)
/* Allocate and return a cc65_scopeinfo struct that is able to hold Count
 * entries. Initialize the count field of the returned struct.
 */
{
    cc65_scopeinfo* S = xmalloc (sizeof (*S) - sizeof (S->data[0]) +
                                 Count * sizeof (S->data[0]));
    S->count = Count;
    return S;
}



static void CopyScopeInfo (cc65_scopedata* D, const ScopeInfo* S)
/* Copy data from a ScopeInfo struct to a cc65_scopedata struct */
{
    D->scope_id         = S->Id;
    D->scope_name       = S->Name;
    D->scope_type       = S->Type;
    D->scope_size       = S->Size;
    if (S->Parent.Info) {
        D->scope_parent = S->Parent.Info->Id;
    } else {
        D->scope_parent = CC65_INV_ID;
    }
    if (S->Label.Info) {
        D->symbol_id    = S->Label.Info->Id;
    } else {
        D->symbol_id    = CC65_INV_ID;
    }
    D->module_id        = S->Mod.Info->Id;
}



static int CompareScopeInfoByName (const void* L, const void* R)
/* Helper function to sort scope infos in a collection by name */
{
    /* Compare symbol name */
    return strcmp (((const ScopeInfo*) L)->Name,
                   ((const ScopeInfo*) R)->Name);
}



/*****************************************************************************/
/*                               Segment info                                */
/*****************************************************************************/



static SegInfo* NewSegInfo (const StrBuf* Name, unsigned Id,
                            cc65_addr Start, cc65_addr Size,
                            const StrBuf* OutputName, unsigned long OutputOffs)
/* Create a new SegInfo struct and return it */
{
    /* Allocate memory */
    SegInfo* S = xmalloc (sizeof (SegInfo) + SB_GetLen (Name));

    /* Initialize it */
    S->Id         = Id;
    S->Start      = Start;
    S->Size       = Size;
    if (SB_GetLen (OutputName) > 0) {
        /* Output file given */
        S->OutputName = SB_StrDup (OutputName);
        S->OutputOffs = OutputOffs;
    } else {
        /* No output file given */
        S->OutputName = 0;
        S->OutputOffs = 0;
    }
    memcpy (S->Name, SB_GetConstBuf (Name), SB_GetLen (Name) + 1);

    /* Return it */
    return S;
}



static void FreeSegInfo (SegInfo* S)
/* Free a SegInfo struct */
{
    xfree (S->OutputName);
    xfree (S);
}



static cc65_segmentinfo* new_cc65_segmentinfo (unsigned Count)
/* Allocate and return a cc65_segmentinfo struct that is able to hold Count
 * entries. Initialize the count field of the returned struct.
 */
{
    cc65_segmentinfo* S = xmalloc (sizeof (*S) - sizeof (S->data[0]) +
                                   Count * sizeof (S->data[0]));
    S->count = Count;
    return S;
}



static void CopySegInfo (cc65_segmentdata* D, const SegInfo* S)
/* Copy data from a SegInfo struct to a cc65_segmentdata struct */
{
    D->segment_id    = S->Id;
    D->segment_name  = S->Name;
    D->segment_start = S->Start;
    D->segment_size  = S->Size;
    D->output_name   = S->OutputName;
    D->output_offs   = S->OutputOffs;
}



static int CompareSegInfoByName (const void* L, const void* R)
/* Helper function to sort segment infos in a collection by name */
{
    /* Sort by file name */
    return strcmp (((const SegInfo*) L)->Name,
                   ((const SegInfo*) R)->Name);
}



/*****************************************************************************/
/*                                Symbol info                                */
/*****************************************************************************/



static SymInfo* NewSymInfo (const StrBuf* Name)
/* Create a new SymInfo struct, intialize and return it */
{
    /* Allocate memory */
    SymInfo* S = xmalloc (sizeof (SymInfo) + SB_GetLen (Name));

    /* Initialize the name */
    memcpy (S->Name, SB_GetConstBuf (Name), SB_GetLen (Name) + 1);

    /* Return it */
    return S;
}



static void FreeSymInfo (SymInfo* S)
/* Free a SymInfo struct */
{
    xfree (S);
}



static cc65_symbolinfo* new_cc65_symbolinfo (unsigned Count)
/* Allocate and return a cc65_symbolinfo struct that is able to hold Count
 * entries. Initialize the count field of the returned struct.
 */
{
    cc65_symbolinfo* S = xmalloc (sizeof (*S) - sizeof (S->data[0]) +
                                  Count * sizeof (S->data[0]));
    S->count = Count;
    return S;
}



static void CopySymInfo (cc65_symboldata* D, const SymInfo* S)
/* Copy data from a SymInfo struct to a cc65_symboldata struct */
{
    D->symbol_id        = S->Id;
    D->symbol_name      = S->Name;
    D->symbol_type      = S->Type;
    D->symbol_size      = S->Size;
    D->symbol_value     = S->Value;
    if (S->Seg.Info) {
        D->segment_id   = S->Seg.Info->Id;
    } else {
        D->segment_id   = CC65_INV_ID;
    }
    D->scope_id         = S->Scope.Info->Id;
    if (S->Parent.Info) {
        D->parent_id    = S->Parent.Info->Id;
    } else {
        D->parent_id    = CC65_INV_ID;
    }
}



static int CompareSymInfoByName (const void* L, const void* R)
/* Helper function to sort symbol infos in a collection by name */
{
    /* Sort by symbol name */
    return strcmp (((const SymInfo*) L)->Name,
                   ((const SymInfo*) R)->Name);
}



static int CompareSymInfoByVal (const void* L, const void* R)
/* Helper function to sort symbol infos in a collection by value */
{
    /* Sort by symbol value. If both are equal, sort by symbol name so it
     * looks nice when such a list is returned.
     */
    if (((const SymInfo*) L)->Value > ((const SymInfo*) R)->Value) {
        return 1;
    } else if (((const SymInfo*) L)->Value < ((const SymInfo*) R)->Value) {
        return -1;
    } else {
        return CompareSymInfoByName (L, R);
    }
}



/*****************************************************************************/
/*                                Debug info                                 */
/*****************************************************************************/



static DbgInfo* NewDbgInfo (void)
/* Create a new DbgInfo struct and return it */
{
    /* Allocate memory */
    DbgInfo* Info = xmalloc (sizeof (DbgInfo));

    /* Initialize it */
    InitCollection (&Info->FileInfoById);
    InitCollection (&Info->LibInfoById);
    InitCollection (&Info->LineInfoById);
    InitCollection (&Info->ModInfoById);
    InitCollection (&Info->ScopeInfoById);
    InitCollection (&Info->SegInfoById);
    InitCollection (&Info->SymInfoById);

    InitCollection (&Info->FileInfoByName);
    InitCollection (&Info->SegInfoByName);
    InitCollection (&Info->SymInfoByName);
    InitCollection (&Info->SymInfoByVal);

    InitLineInfoList (&Info->LineInfoByAddr);

    /* Return it */
    return Info;
}



static void FreeDbgInfo (DbgInfo* Info)
/* Free a DbgInfo struct */
{
    unsigned I;

    /* First, free the items in the collections */
    for (I = 0; I < CollCount (&Info->FileInfoById); ++I) {
        FreeFileInfo (CollAt (&Info->FileInfoById, I));
    }
    for (I = 0; I < CollCount (&Info->LibInfoById); ++I) {
        FreeLibInfo (CollAt (&Info->LibInfoById, I));
    }
    for (I = 0; I < CollCount (&Info->LineInfoById); ++I) {
        FreeLineInfo (CollAt (&Info->LineInfoById, I));
    }
    for (I = 0; I < CollCount (&Info->ModInfoById); ++I) {
        FreeModInfo (CollAt (&Info->ModInfoById, I));
    }
    for (I = 0; I < CollCount (&Info->ScopeInfoById); ++I) {
        FreeScopeInfo (CollAt (&Info->ScopeInfoById, I));
    }
    for (I = 0; I < CollCount (&Info->SegInfoById); ++I) {
        FreeSegInfo (CollAt (&Info->SegInfoById, I));
    }
    for (I = 0; I < CollCount (&Info->SymInfoById); ++I) {
        FreeSymInfo (CollAt (&Info->SymInfoById, I));
    }

    /* Free the memory used by the collections themselves */
    DoneCollection (&Info->FileInfoById);
    DoneCollection (&Info->LibInfoById);
    DoneCollection (&Info->LineInfoById);
    DoneCollection (&Info->ModInfoById);
    DoneCollection (&Info->ScopeInfoById);
    DoneCollection (&Info->SegInfoById);
    DoneCollection (&Info->SymInfoById);

    DoneCollection (&Info->FileInfoByName);
    DoneCollection (&Info->SegInfoByName);
    DoneCollection (&Info->SymInfoByName);
    DoneCollection (&Info->SymInfoByVal);

    /* Free line info */
    DoneLineInfoList (&Info->LineInfoByAddr);

    /* Free the structure itself */
    xfree (Info);
}



/*****************************************************************************/
/*                             Helper functions                              */
/*****************************************************************************/



static void ParseError (InputData* D, cc65_error_severity Type, const char* Msg, ...)
/* Call the user supplied parse error function */
{
    va_list             ap;
    int                 MsgSize;
    cc65_parseerror*    E;

    /* Test-format the error message so we know how much space to allocate */
    va_start (ap, Msg);
    MsgSize = vsnprintf (0, 0, Msg, ap);
    va_end (ap);

    /* Allocate memory */
    E = xmalloc (sizeof (*E) + MsgSize);

    /* Write data to E */
    E->type   = Type;
    E->name   = D->FileName;
    E->line   = D->SLine;
    E->column = D->SCol;
    va_start (ap, Msg);
    vsnprintf (E->errormsg, MsgSize+1, Msg, ap);
    va_end (ap);

    /* Call the caller:-) */
    D->Error (E);

    /* Free the data structure */
    xfree (E);

    /* Count errors */
    if (Type == CC65_ERROR) {
        ++D->Errors;
    }
}



static void SkipLine (InputData* D)
/* Error recovery routine. Skip tokens until EOL or EOF is reached */
{
    while (D->Tok != TOK_EOL && D->Tok != TOK_EOF) {
        NextToken (D);
    }
}



static void UnexpectedToken (InputData* D)
/* Call ParseError with a message about an unexpected input token */
{
    ParseError (D, CC65_ERROR, "Unexpected input token %d", D->Tok);
    SkipLine (D);
}



static void UnknownKeyword (InputData* D)
/* Print a warning about an unknown keyword in the file. Try to do smart
 * recovery, so if later versions of the debug information add additional
 * keywords, this code may be able to at least ignore them.
 */
{
    /* Output a warning */
    ParseError (D, CC65_WARNING, "Unknown keyword \"%s\" - skipping",
                SB_GetConstBuf (&D->SVal));

    /* Skip the identifier */
    NextToken (D);

    /* If an equal sign follows, ignore anything up to the next line end
     * or comma. If a comma or line end follows, we're already done. If
     * we have none of both, we ignore the remainder of the line.
     */
    if (D->Tok == TOK_EQUAL) {
        NextToken (D);
        while (D->Tok != TOK_COMMA && D->Tok != TOK_EOL && D->Tok != TOK_EOF) {
            NextToken (D);
        }
    } else if (D->Tok != TOK_COMMA && D->Tok != TOK_EOL && D->Tok != TOK_EOF) {
        SkipLine (D);
    }
}



/*****************************************************************************/
/*                            Scanner and parser                             */
/*****************************************************************************/



static int DigitVal (int C)
/* Return the value for a numeric digit. Return -1 if C is invalid */
{
    if (isdigit (C)) {
	return C - '0';
    } else if (isxdigit (C)) {
	return toupper (C) - 'A' + 10;
    } else {
        return -1;
    }
}



static void NextChar (InputData* D)
/* Read the next character from the input. Count lines and columns */
{
    /* Check if we've encountered EOF before */
    if (D->C >= 0) {
        if (D->C == '\n') {
            ++D->Line;
            D->Col = 0;
        }
        D->C = fgetc (D->F);
        ++D->Col;
    }
}



static void NextToken (InputData* D)
/* Read the next token from the input stream */
{
    static const struct KeywordEntry  {
        const char      Keyword[12];
        Token           Tok;
    } KeywordTable[] = {
        { "abs",        TOK_ABSOLUTE    },
        { "addrsize",   TOK_ADDRSIZE    },
        { "count",      TOK_COUNT       },
        { "enum",       TOK_ENUM        },
        { "equ",        TOK_EQUATE      },
        { "file",       TOK_FILE        },
        { "global",     TOK_GLOBAL      },
        { "id",         TOK_ID          },
        { "info",       TOK_INFO        },
        { "lab",        TOK_LABEL       },
        { "lib",        TOK_LIBRARY     },
        { "line",       TOK_LINE        },
        { "long",       TOK_LONG        },
        { "major",      TOK_MAJOR       },
        { "minor",      TOK_MINOR       },
        { "mod",        TOK_MODULE      },
        { "mtime",      TOK_MTIME       },
        { "name",       TOK_NAME        },
        { "oname",      TOK_OUTPUTNAME  },
        { "ooffs",      TOK_OUTPUTOFFS  },
        { "parent",     TOK_PARENT      },
        { "range",      TOK_RANGE       },
        { "ro",         TOK_RO          },
        { "rw",         TOK_RW          },
        { "scope",      TOK_SCOPE       },
        { "seg",        TOK_SEGMENT     },
        { "size",       TOK_SIZE        },
        { "start",      TOK_START       },
        { "struct",     TOK_STRUCT      },
        { "sym",        TOK_SYM         },
        { "type",       TOK_TYPE        },
        { "val",        TOK_VALUE       },
        { "version",    TOK_VERSION     },
        { "zp",         TOK_ZEROPAGE    },
    };


    /* Skip whitespace */
    while (D->C == ' ' || D->C == '\t' || D->C == '\r') {
     	NextChar (D);
    }

    /* Remember the current position as start of the next token */
    D->SLine = D->Line;
    D->SCol  = D->Col;

    /* Identifier? */
    if (D->C == '_' || isalpha (D->C)) {

        const struct KeywordEntry* Entry;

	/* Read the identifier */
        SB_Clear (&D->SVal);
	while (D->C == '_' || isalnum (D->C)) {
            SB_AppendChar (&D->SVal, D->C);
	    NextChar (D);
     	}
       	SB_Terminate (&D->SVal);

        /* Search the identifier in the keyword table */
        Entry = bsearch (SB_GetConstBuf (&D->SVal),
                         KeywordTable,
                         sizeof (KeywordTable) / sizeof (KeywordTable[0]),
                         sizeof (KeywordTable[0]),
                         (int (*)(const void*, const void*)) strcmp);
        if (Entry == 0) {
            D->Tok = TOK_IDENT;
        } else {
            D->Tok = Entry->Tok;
        }
	return;
    }

    /* Number? */
    if (isdigit (D->C)) {
        int Base = 10;
        int Val;
        if (D->C == '0') {
            NextChar (D);
            if (toupper (D->C) == 'X') {
                NextChar (D);
                Base = 16;
            } else {
                Base = 8;
            }
        } else {
            Base = 10;
        }
       	D->IVal = 0;
        while ((Val = DigitVal (D->C)) >= 0 && Val < Base) {
       	    D->IVal = D->IVal * Base + Val;
	    NextChar (D);
       	}
	D->Tok = TOK_INTCON;
	return;
    }

    /* Other characters */
    switch (D->C) {

        case '-':
            NextChar (D);
            D->Tok = TOK_MINUS;
            break;

        case '+':
            NextChar (D);
            D->Tok = TOK_PLUS;
            break;

	case ',':
	    NextChar (D);
	    D->Tok = TOK_COMMA;
	    break;

	case '=':
	    NextChar (D);
	    D->Tok = TOK_EQUAL;
	    break;

        case '\"':
            SB_Clear (&D->SVal);
            NextChar (D);
            while (1) {
                if (D->C == '\n' || D->C == EOF) {
                    ParseError (D, CC65_ERROR, "Unterminated string constant");
                    break;
                }
                if (D->C == '\"') {
                    NextChar (D);
                    break;
                }
                SB_AppendChar (&D->SVal, D->C);
                NextChar (D);
            }
            SB_Terminate (&D->SVal);
            D->Tok = TOK_STRCON;
       	    break;

        case '\n':
            NextChar (D);
            D->Tok = TOK_EOL;
            break;

        case EOF:
       	    D->Tok = TOK_EOF;
	    break;

	default:
       	    ParseError (D, CC65_ERROR, "Invalid input character `%c'", D->C);

    }
}



static int TokenIsKeyword (Token Tok)
/* Return true if the given token is a keyword */
{
    return (Tok >= TOK_FIRST_KEYWORD && Tok <= TOK_LAST_KEYWORD);
}



static int TokenFollows (InputData* D, Token Tok, const char* Name)
/* Check for a specific token that follows. */
{
    if (D->Tok != Tok) {
        ParseError (D, CC65_ERROR, "%s expected", Name);
        SkipLine (D);
        return 0;
    } else {
        return 1;
    }
}



static int IntConstFollows (InputData* D)
/* Check for an integer constant */
{
    return TokenFollows (D, TOK_INTCON, "Integer constant");
}



static int StrConstFollows (InputData* D)
/* Check for a string literal */
{
    return TokenFollows (D, TOK_STRCON, "String literal");
}



static int Consume (InputData* D, Token Tok, const char* Name)
/* Check for a token and consume it. Return true if the token was comsumed,
 * return false otherwise.
 */
{
    if (TokenFollows (D, Tok, Name)) {
        NextToken (D);
        return 1;
    } else {
        return 0;
    }
}



static int ConsumeEqual (InputData* D)
/* Consume an equal sign */
{
    return Consume (D, TOK_EQUAL, "'='");
}



static void ConsumeEOL (InputData* D)
/* Consume an end-of-line token, if we aren't at end-of-file */
{
    if (D->Tok != TOK_EOF) {
        if (D->Tok != TOK_EOL) {
            ParseError (D, CC65_ERROR, "Extra tokens in line");
            SkipLine (D);
        }
        NextToken (D);
    }
}



static void ParseFile (InputData* D)
/* Parse a FILE line */
{
    unsigned      Id = 0;
    unsigned long Size = 0;
    unsigned long MTime = 0;
    unsigned      ModId = CC65_INV_ID;
    StrBuf        Name = STRBUF_INITIALIZER;
    FileInfo*     F;
    enum {
        ibNone      = 0x00,
        ibId        = 0x01,
        ibName      = 0x02,
        ibSize      = 0x04,
        ibMTime     = 0x08,
        ibModId     = 0x10,
        ibRequired  = ibId | ibName | ibSize | ibMTime | ibModId,
    } InfoBits = ibNone;

    /* Skip the FILE token */
    NextToken (D);

    /* More stuff follows */
    while (1) {

        Token Tok;

        /* Something we know? */
        if (D->Tok != TOK_ID            && D->Tok != TOK_MODULE         &&
            D->Tok != TOK_MTIME         && D->Tok != TOK_NAME           &&
            D->Tok != TOK_SIZE) {

            /* Try smart error recovery */
            if (D->Tok == TOK_IDENT || TokenIsKeyword (D->Tok)) {
                UnknownKeyword (D);
                continue;
            }

            /* Done */
            break;
        }

        /* Remember the token, skip it, check for equal */
        Tok = D->Tok;
        NextToken (D);
        if (!ConsumeEqual (D)) {
            goto ErrorExit;
        }

        /* Check what the token was */
        switch (Tok) {

            case TOK_ID:
                if (!IntConstFollows (D)) {
                    goto ErrorExit;
                }
                Id = D->IVal;
                InfoBits |= ibId;
                NextToken (D);
                break;

            case TOK_MTIME:
                if (!IntConstFollows (D)) {
                    goto ErrorExit;
                }
                MTime = D->IVal;
                NextToken (D);
                InfoBits |= ibMTime;
                break;

            case TOK_MODULE:
                while (1) {
                    if (!IntConstFollows (D)) {
                        goto ErrorExit;
                    }
                    ModId = D->IVal;
                    InfoBits |= ibModId;
                    NextToken (D);
                    if (D->Tok != TOK_PLUS) {
                        break;
                    }
                    NextToken (D);
                }
                break;

            case TOK_NAME:
                if (!StrConstFollows (D)) {
                    goto ErrorExit;
                }
                SB_Copy (&Name, &D->SVal);
                SB_Terminate (&Name);
                InfoBits |= ibName;
                NextToken (D);
                break;

            case TOK_SIZE:
                if (!IntConstFollows (D)) {
                    goto ErrorExit;
                }
                Size = D->IVal;
                NextToken (D);
                InfoBits |= ibSize;
                break;

            default:
                /* NOTREACHED */
                UnexpectedToken (D);
                goto ErrorExit;

        }

        /* Comma or done */
        if (D->Tok != TOK_COMMA) {
            break;
        }
        NextToken (D);
    }

    /* Check for end of line */
    if (D->Tok != TOK_EOL && D->Tok != TOK_EOF) {
        UnexpectedToken (D);
        SkipLine (D);
        goto ErrorExit;
    }

    /* Check for required information */
    if ((InfoBits & ibRequired) != ibRequired) {
        ParseError (D, CC65_ERROR, "Required attributes missing");
        goto ErrorExit;
    }

    /* Create the file info and remember it */
    F = NewFileInfo (&Name);
    F->Id       = Id;
    F->Size     = Size;
    F->MTime    = MTime;
    F->Mod.Id   = ModId;
    CollReplaceExpand (&D->Info->FileInfoById, F, Id);
    CollAppend (&D->Info->FileInfoByName, F);

ErrorExit:
    /* Entry point in case of errors */
    SB_Done (&Name);
    return;
}



static void ParseInfo (InputData* D)
/* Parse an INFO line */
{
    /* Skip the INFO token */
    NextToken (D);

    /* More stuff follows */
    while (1) {

        Token Tok;

        /* Something we know? */
        if (D->Tok != TOK_FILE  && D->Tok != TOK_LIBRARY        &&
            D->Tok != TOK_LINE  && D->Tok != TOK_MODULE         &&
            D->Tok != TOK_SCOPE && D->Tok != TOK_SEGMENT        &&
            D->Tok != TOK_SYM) {

            /* Try smart error recovery */
            if (D->Tok == TOK_IDENT || TokenIsKeyword (D->Tok)) {
                UnknownKeyword (D);
                continue;
            }

            /* Done */
            break;
        }

        /* Remember the token, skip it, check for equal, check for an integer
         * constant.
         */
        Tok = D->Tok;
        NextToken (D);
        if (!ConsumeEqual (D)) {
            goto ErrorExit;
        }
        if (!IntConstFollows (D)) {
            goto ErrorExit;
        }

        /* Check what the token was */
        switch (Tok) {

            case TOK_FILE:
                CollGrow (&D->Info->FileInfoById, D->IVal);
                break;

            case TOK_LIBRARY:
                CollGrow (&D->Info->LibInfoById, D->IVal);
                break;

            case TOK_LINE:
                /*CollGrow (&D->Info->LineInfoById, D->IVal);*/
                break;

            case TOK_MODULE:
                CollGrow (&D->Info->ModInfoById, D->IVal);
                break;

            case TOK_SCOPE:
                CollGrow (&D->Info->ScopeInfoById, D->IVal);
                break;

            case TOK_SEGMENT:
                CollGrow (&D->Info->SegInfoById, D->IVal);
                break;

            case TOK_SYM:
                CollGrow (&D->Info->SymInfoById, D->IVal);
                break;

            default:
                /* NOTREACHED */
                UnexpectedToken (D);
                goto ErrorExit;

        }

        /* Skip the number */
        NextToken (D);

        /* Comma or done */
        if (D->Tok != TOK_COMMA) {
            break;
        }
        NextToken (D);
    }

    /* Check for end of line */
    if (D->Tok != TOK_EOL && D->Tok != TOK_EOF) {
        UnexpectedToken (D);
        SkipLine (D);
        goto ErrorExit;
    }

ErrorExit:
    /* Entry point in case of errors */
    return;
}



static void ParseLibrary (InputData* D)
/* Parse a LIBRARY line */
{
    unsigned      Id = 0;
    StrBuf        Name = STRBUF_INITIALIZER;
    LibInfo*      L;
    enum {
        ibNone      = 0x00,
        ibId        = 0x01,
        ibName      = 0x02,
        ibRequired  = ibId | ibName,
    } InfoBits = ibNone;

    /* Skip the LIBRARY token */
    NextToken (D);

    /* More stuff follows */
    while (1) {

        Token Tok;

        /* Something we know? */
        if (D->Tok != TOK_ID            && D->Tok != TOK_NAME) {

            /* Try smart error recovery */
            if (D->Tok == TOK_IDENT || TokenIsKeyword (D->Tok)) {
                UnknownKeyword (D);
                continue;
            }

            /* Done */
            break;
        }

        /* Remember the token, skip it, check for equal */
        Tok = D->Tok;
        NextToken (D);
        if (!ConsumeEqual (D)) {
            goto ErrorExit;
        }

        /* Check what the token was */
        switch (Tok) {

            case TOK_ID:
                if (!IntConstFollows (D)) {
                    goto ErrorExit;
                }
                Id = D->IVal;
                InfoBits |= ibId;
                NextToken (D);
                break;

            case TOK_NAME:
                if (!StrConstFollows (D)) {
                    goto ErrorExit;
                }
                SB_Copy (&Name, &D->SVal);
                SB_Terminate (&Name);
                InfoBits |= ibName;
                NextToken (D);
                break;

            default:
                /* NOTREACHED */
                UnexpectedToken (D);
                goto ErrorExit;

        }

        /* Comma or done */
        if (D->Tok != TOK_COMMA) {
            break;
        }
        NextToken (D);
    }

    /* Check for end of line */
    if (D->Tok != TOK_EOL && D->Tok != TOK_EOF) {
        UnexpectedToken (D);
        SkipLine (D);
        goto ErrorExit;
    }

    /* Check for required information */
    if ((InfoBits & ibRequired) != ibRequired) {
        ParseError (D, CC65_ERROR, "Required attributes missing");
        goto ErrorExit;
    }

    /* Create the library info and remember it */
    L = NewLibInfo (&Name);
    L->Id = Id;
    CollReplaceExpand (&D->Info->LibInfoById, L, Id);

ErrorExit:
    /* Entry point in case of errors */
    SB_Done (&Name);
    return;
}



static void ParseLine (InputData* D)
/* Parse a LINE line */
{
    unsigned        Id = CC65_INV_ID;
    unsigned        FileId = CC65_INV_ID;
    unsigned        SegId = CC65_INV_ID;
    cc65_line       Line = 0;
    cc65_addr       Start = 0;
    cc65_addr       End = 0;
    cc65_line_type  Type = CC65_LINE_ASM;
    unsigned        Count = 0;
    LineInfo*   L;
    enum {
        ibNone      = 0x00,

        ibCount     = 0x01,
        ibFileId    = 0x02,
        ibId        = 0x04,
        ibLine      = 0x08,
        ibRange     = 0x10,
        ibSegId     = 0x20,
        ibType      = 0x40,

        ibRequired  = ibFileId | ibId | ibLine | ibSegId | ibRange,
    } InfoBits = ibNone;

    /* Skip the LINE token */
    NextToken (D);

    /* More stuff follows */
    while (1) {

        Token Tok;

        /* Something we know? */
        if (D->Tok != TOK_COUNT   && D->Tok != TOK_FILE         &&
            D->Tok != TOK_ID      && D->Tok != TOK_LINE         &&
            D->Tok != TOK_RANGE   && D->Tok != TOK_SEGMENT      &&
            D->Tok != TOK_TYPE) {

            /* Try smart error recovery */
            if (D->Tok == TOK_IDENT || TokenIsKeyword (D->Tok)) {
                UnknownKeyword (D);
                continue;
            }

            /* Done */
            break;
        }

        /* Remember the token, skip it, check for equal */
        Tok = D->Tok;
        NextToken (D);
        if (!ConsumeEqual (D)) {
            goto ErrorExit;
        }

        /* Check what the token was */
        switch (Tok) {

            case TOK_FILE:
                if (!IntConstFollows (D)) {
                    goto ErrorExit;
                }
                FileId = D->IVal;
                InfoBits |= ibFileId;
                NextToken (D);
                break;

            case TOK_ID:
                if (!IntConstFollows (D)) {
                    goto ErrorExit;
                }
                Id = D->IVal;
                InfoBits |= ibId;
                NextToken (D);
                break;

            case TOK_LINE:
                if (!IntConstFollows (D)) {
                    goto ErrorExit;
                }
                Line = (cc65_line) D->IVal;
                NextToken (D);
                InfoBits |= ibLine;
                break;

            case TOK_RANGE:
                if (!IntConstFollows (D)) {
                    goto ErrorExit;
                }
                Start = (cc65_addr) D->IVal;
                NextToken (D);
                if (D->Tok == TOK_MINUS) {
                    /* End of range follows */
                    NextToken (D);
                    if (!IntConstFollows (D)) {
                        goto ErrorExit;
                    }
                    End = (cc65_addr) D->IVal;
                    NextToken (D);
                } else {
                    /* Start and end are identical */
                    End = Start;
                }
                InfoBits |= ibRange;
                break;

            case TOK_SEGMENT:
                if (!IntConstFollows (D)) {
                    goto ErrorExit;
                }
                SegId = D->IVal;
                InfoBits |= ibSegId;
                NextToken (D);
                break;

            case TOK_TYPE:
                if (!IntConstFollows (D)) {
                    goto ErrorExit;
                }
                Type = (cc65_line_type) D->IVal;
                InfoBits |= ibType;
                NextToken (D);
                break;

            case TOK_COUNT:
                if (!IntConstFollows (D)) {
                    goto ErrorExit;
                }
                Count = D->IVal;
                InfoBits |= ibCount;
                NextToken (D);
                break;

            default:
                /* NOTREACHED */
                UnexpectedToken (D);
                goto ErrorExit;

        }

        /* Comma or done */
        if (D->Tok != TOK_COMMA) {
            break;
        }
        NextToken (D);
    }

    /* Check for end of line */
    if (D->Tok != TOK_EOL && D->Tok != TOK_EOF) {
        UnexpectedToken (D);
        SkipLine (D);
        goto ErrorExit;
    }

    /* Check for required information */
    if ((InfoBits & ibRequired) != ibRequired) {
        ParseError (D, CC65_ERROR, "Required attributes missing");
        goto ErrorExit;
    }

    /* Create the line info and remember it */
    L = NewLineInfo ();
    L->Id       = Id;
    L->Start    = Start;
    L->End      = End;
    L->Line     = Line;
    L->File.Id  = FileId;
    L->Seg.Id   = SegId;
    L->Type     = Type;
    L->Count    = Count;
    CollReplaceExpand (&D->Info->LineInfoById, L, Id);

ErrorExit:
    /* Entry point in case of errors */
    return;
}



static void ParseModule (InputData* D)
/* Parse a MODULE line */
{
    /* Most of the following variables are initialized with a value that is
     * overwritten later. This is just to avoid compiler warnings.
     */
    unsigned            Id = CC65_INV_ID;
    StrBuf              Name = STRBUF_INITIALIZER;
    unsigned            FileId = CC65_INV_ID;
    unsigned            LibId = CC65_INV_ID;
    ModInfo*            M;
    enum {
        ibNone          = 0x000,

        ibFileId        = 0x001,
        ibId            = 0x002,
        ibName          = 0x004,
        ibLibId         = 0x008,

        ibRequired      = ibId | ibName | ibFileId,
    } InfoBits = ibNone;

    /* Skip the MODULE token */
    NextToken (D);

    /* More stuff follows */
    while (1) {

        Token Tok;

        /* Something we know? */
        if (D->Tok != TOK_FILE          && D->Tok != TOK_ID             &&
            D->Tok != TOK_NAME          && D->Tok != TOK_LIBRARY) {

            /* Try smart error recovery */
            if (D->Tok == TOK_IDENT || TokenIsKeyword (D->Tok)) {
                UnknownKeyword (D);
                continue;
            }

            /* Done */
            break;
        }

        /* Remember the token, skip it, check for equal */
        Tok = D->Tok;
        NextToken (D);
        if (!ConsumeEqual (D)) {
            goto ErrorExit;
        }

        /* Check what the token was */
        switch (Tok) {

            case TOK_FILE:
                if (!IntConstFollows (D)) {
                    goto ErrorExit;
                }
                FileId = D->IVal;
                InfoBits |= ibFileId;
                NextToken (D);
                break;

            case TOK_ID:
                if (!IntConstFollows (D)) {
                    goto ErrorExit;
                }
                Id = D->IVal;
                InfoBits |= ibId;
                NextToken (D);
                break;

            case TOK_NAME:
                if (!StrConstFollows (D)) {
                    goto ErrorExit;
                }
                SB_Copy (&Name, &D->SVal);
                SB_Terminate (&Name);
                InfoBits |= ibName;
                NextToken (D);
                break;

            case TOK_LIBRARY:
                if (!IntConstFollows (D)) {
                    goto ErrorExit;
                }
                LibId = D->IVal;
                InfoBits |= ibLibId;
                NextToken (D);
                break;

            default:
                /* NOTREACHED */
                UnexpectedToken (D);
                goto ErrorExit;

        }

        /* Comma or done */
        if (D->Tok != TOK_COMMA) {
            break;
        }
        NextToken (D);
    }

    /* Check for end of line */
    if (D->Tok != TOK_EOL && D->Tok != TOK_EOF) {
        UnexpectedToken (D);
        SkipLine (D);
        goto ErrorExit;
    }

    /* Check for required and/or matched information */
    if ((InfoBits & ibRequired) != ibRequired) {
        ParseError (D, CC65_ERROR, "Required attributes missing");
        goto ErrorExit;
    }

    /* Create the scope info */
    M = NewModInfo (&Name);
    M->File.Id = FileId;
    M->Id      = Id;
    M->Lib.Id  = LibId;

    /* ... and remember it */
    CollReplaceExpand (&D->Info->ModInfoById, M, Id);

ErrorExit:
    /* Entry point in case of errors */
    SB_Done (&Name);
    return;
}






static void ParseScope (InputData* D)
/* Parse a SCOPE line */
{
    /* Most of the following variables are initialized with a value that is
     * overwritten later. This is just to avoid compiler warnings.
     */
    unsigned            Id = CC65_INV_ID;
    cc65_scope_type     Type = CC65_SCOPE_MODULE;
    cc65_size           Size = 0;
    StrBuf              Name = STRBUF_INITIALIZER;
    unsigned            ModId = CC65_INV_ID;
    unsigned            ParentId = CC65_INV_ID;
    unsigned            SymId = CC65_INV_ID;
    ScopeInfo*          S;
    enum {
        ibNone          = 0x000,

        ibId            = 0x001,
        ibModId         = 0x002,
        ibName          = 0x004,
        ibParentId      = 0x008,
        ibSize          = 0x010,
        ibSymId         = 0x020,
        ibType          = 0x040,

        ibRequired      = ibId | ibModId | ibName,
    } InfoBits = ibNone;

    /* Skip the SCOPE token */
    NextToken (D);

    /* More stuff follows */
    while (1) {

        Token Tok;

        /* Something we know? */
        if (D->Tok != TOK_ID            && D->Tok != TOK_MODULE         &&
            D->Tok != TOK_NAME          && D->Tok != TOK_PARENT         &&
            D->Tok != TOK_SIZE          && D->Tok != TOK_SYM            &&
            D->Tok != TOK_TYPE) {

            /* Try smart error recovery */
            if (D->Tok == TOK_IDENT || TokenIsKeyword (D->Tok)) {
                UnknownKeyword (D);
                continue;
            }

            /* Done */
            break;
        }

        /* Remember the token, skip it, check for equal */
        Tok = D->Tok;
        NextToken (D);
        if (!ConsumeEqual (D)) {
            goto ErrorExit;
        }

        /* Check what the token was */
        switch (Tok) {

            case TOK_ID:
                if (!IntConstFollows (D)) {
                    goto ErrorExit;
                }
                Id = D->IVal;
                InfoBits |= ibId;
                NextToken (D);
                break;

            case TOK_MODULE:
                if (!IntConstFollows (D)) {
                    goto ErrorExit;
                }
                ModId = D->IVal;
                InfoBits |= ibModId;
                NextToken (D);
                break;

            case TOK_NAME:
                if (!StrConstFollows (D)) {
                    goto ErrorExit;
                }
                SB_Copy (&Name, &D->SVal);
                SB_Terminate (&Name);
                InfoBits |= ibName;
                NextToken (D);
                break;

            case TOK_PARENT:
                if (!IntConstFollows (D)) {
                    goto ErrorExit;
                }
                ParentId = D->IVal;
                NextToken (D);
                InfoBits |= ibParentId;
                break;

            case TOK_SIZE:
                if (!IntConstFollows (D)) {
                    goto ErrorExit;
                }
                Size = (cc65_size) D->IVal;
                InfoBits |= ibSize;
                NextToken (D);
                break;

            case TOK_SYM:
                if (!IntConstFollows (D)) {
                    goto ErrorExit;
                }
                SymId = D->IVal;
                NextToken (D);
                InfoBits |= ibSymId;
                break;

            case TOK_TYPE:
                switch (D->Tok) {
                    case TOK_GLOBAL:    Type = CC65_SCOPE_GLOBAL;       break;
                    case TOK_FILE:      Type = CC65_SCOPE_MODULE;       break;
                    case TOK_SCOPE:     Type = CC65_SCOPE_SCOPE;        break;
                    case TOK_STRUCT:    Type = CC65_SCOPE_STRUCT;       break;
                    case TOK_ENUM:      Type = CC65_SCOPE_ENUM;         break;
                    default:
                        ParseError (D, CC65_ERROR,
                                    "Unknown value for attribute \"type\"");
                        SkipLine (D);
                        goto ErrorExit;
                }
                NextToken (D);
                InfoBits |= ibType;
                break;

            default:
                /* NOTREACHED */
                UnexpectedToken (D);
                goto ErrorExit;

        }

        /* Comma or done */
        if (D->Tok != TOK_COMMA) {
            break;
        }
        NextToken (D);
    }

    /* Check for end of line */
    if (D->Tok != TOK_EOL && D->Tok != TOK_EOF) {
        UnexpectedToken (D);
        SkipLine (D);
        goto ErrorExit;
    }

    /* Check for required and/or matched information */
    if ((InfoBits & ibRequired) != ibRequired) {
        ParseError (D, CC65_ERROR, "Required attributes missing");
        goto ErrorExit;
    }

    /* Create the scope info */
    S = NewScopeInfo (&Name);
    S->Id        = Id;
    S->Type      = Type;
    S->Size      = Size;
    S->Mod.Id    = ModId;
    S->Parent.Id = ParentId;
    S->Label.Id  = SymId;

    /* ... and remember it */
    CollReplaceExpand (&D->Info->ScopeInfoById, S, Id);

ErrorExit:
    /* Entry point in case of errors */
    SB_Done (&Name);
    return;
}



static void ParseSegment (InputData* D)
/* Parse a SEGMENT line */
{
    unsigned        Id = 0;
    cc65_addr       Start = 0;
    cc65_addr       Size = 0;
    StrBuf          Name = STRBUF_INITIALIZER;
    StrBuf          OutputName = STRBUF_INITIALIZER;
    unsigned long   OutputOffs = 0;
    SegInfo*        S;
    enum {
        ibNone      = 0x000,

        ibAddrSize  = 0x001,
        ibId        = 0x002,
        ibOutputName= 0x004,
        ibOutputOffs= 0x008,
        ibName      = 0x010,
        ibSize      = 0x020,
        ibStart     = 0x040,
        ibType      = 0x080,

        ibRequired  = ibId | ibName | ibStart | ibSize | ibAddrSize | ibType,
    } InfoBits = ibNone;

    /* Skip the SEGMENT token */
    NextToken (D);

    /* More stuff follows */
    while (1) {

        Token Tok;

        /* Something we know? */
        if (D->Tok != TOK_ADDRSIZE      && D->Tok != TOK_ID         &&
            D->Tok != TOK_NAME          && D->Tok != TOK_OUTPUTNAME &&
            D->Tok != TOK_OUTPUTOFFS    && D->Tok != TOK_SIZE       &&
            D->Tok != TOK_START         && D->Tok != TOK_TYPE) {

            /* Try smart error recovery */
            if (D->Tok == TOK_IDENT || TokenIsKeyword (D->Tok)) {
                UnknownKeyword (D);
                continue;
            }
            /* Done */
            break;
        }

        /* Remember the token, skip it, check for equal */
        Tok = D->Tok;
        NextToken (D);
        if (!ConsumeEqual (D)) {
            goto ErrorExit;
        }

        /* Check what the token was */
        switch (Tok) {

            case TOK_ADDRSIZE:
                NextToken (D);
                InfoBits |= ibAddrSize;
                break;

            case TOK_ID:
                if (!IntConstFollows (D)) {
                    goto ErrorExit;
                }
                Id = D->IVal;
                InfoBits |= ibId;
                NextToken (D);
                break;

            case TOK_NAME:
                if (!StrConstFollows (D)) {
                    goto ErrorExit;
                }
                SB_Copy (&Name, &D->SVal);
                SB_Terminate (&Name);
                InfoBits |= ibName;
                NextToken (D);
                break;

            case TOK_OUTPUTNAME:
                if (!StrConstFollows (D)) {
                    goto ErrorExit;
                }
                SB_Copy (&OutputName, &D->SVal);
                SB_Terminate (&OutputName);
                InfoBits |= ibOutputName;
                NextToken (D);
                break;

            case TOK_OUTPUTOFFS:
                if (!IntConstFollows (D)) {
                    goto ErrorExit;
                }
                OutputOffs = D->IVal;
                NextToken (D);
                InfoBits |= ibOutputOffs;
                break;

            case TOK_SIZE:
                if (!IntConstFollows (D)) {
                    goto ErrorExit;
                }
                Size = D->IVal;
                NextToken (D);
                InfoBits |= ibSize;
                break;

            case TOK_START:
                if (!IntConstFollows (D)) {
                    goto ErrorExit;
                }
                Start = (cc65_addr) D->IVal;
                NextToken (D);
                InfoBits |= ibStart;
                break;

            case TOK_TYPE:
                NextToken (D);
                InfoBits |= ibType;
                break;

            default:
                /* NOTREACHED */
                UnexpectedToken (D);
                goto ErrorExit;

        }

        /* Comma or done */
        if (D->Tok != TOK_COMMA) {
            break;
        }
        NextToken (D);
    }

    /* Check for end of line */
    if (D->Tok != TOK_EOL && D->Tok != TOK_EOF) {
        UnexpectedToken (D);
        SkipLine (D);
        goto ErrorExit;
    }

    /* Check for required and/or matched information */
    if ((InfoBits & ibRequired) != ibRequired) {
        ParseError (D, CC65_ERROR, "Required attributes missing");
        goto ErrorExit;
    }
    InfoBits &= (ibOutputName | ibOutputOffs);
    if (InfoBits != ibNone && InfoBits != (ibOutputName | ibOutputOffs)) {
        ParseError (D, CC65_ERROR,
                    "Attributes \"outputname\" and \"outputoffs\" must be paired");
        goto ErrorExit;
    }

    /* Fix OutputOffs if not given */
    if (InfoBits == ibNone) {
        OutputOffs = 0;
    }

    /* Create the segment info and remember it */
    S = NewSegInfo (&Name, Id, Start, Size, &OutputName, OutputOffs);
    CollReplaceExpand (&D->Info->SegInfoById, S, Id);
    CollAppend (&D->Info->SegInfoByName, S);

ErrorExit:
    /* Entry point in case of errors */
    SB_Done (&Name);
    SB_Done (&OutputName);
    return;
}



static void ParseSym (InputData* D)
/* Parse a SYM line */
{
    /* Most of the following variables are initialized with a value that is
     * overwritten later. This is just to avoid compiler warnings.
     */
    unsigned            FileId = CC65_INV_ID;
    unsigned            Id = CC65_INV_ID;
    StrBuf              Name = STRBUF_INITIALIZER;
    unsigned            ParentId = CC65_INV_ID;
    unsigned            ScopeId = CC65_INV_ID;
    unsigned            SegId = CC65_INV_ID;
    cc65_size           Size = 0;
    cc65_symbol_type    Type = CC65_SYM_EQUATE;
    long                Value = 0;

    SymInfo*            S;
    enum {
        ibNone          = 0x000,

        ibAddrSize      = 0x001,
        ibFileId        = 0x002,
        ibId            = 0x004,
        ibParentId      = 0x008,
        ibScopeId       = 0x010,
        ibSegId         = 0x020,
        ibSize          = 0x040,
        ibName          = 0x080,
        ibType          = 0x100,
        ibValue         = 0x200,

        ibRequired      = ibAddrSize | ibId | ibName | ibType | ibValue,
    } InfoBits = ibNone;

    /* Skip the SYM token */
    NextToken (D);

    /* More stuff follows */
    while (1) {

        Token Tok;

        /* Something we know? */
        if (D->Tok != TOK_ADDRSIZE      && D->Tok != TOK_FILE   &&
            D->Tok != TOK_ID            && D->Tok != TOK_NAME   &&
            D->Tok != TOK_PARENT        && D->Tok != TOK_SCOPE  &&
            D->Tok != TOK_SEGMENT       && D->Tok != TOK_SIZE   &&
            D->Tok != TOK_TYPE          && D->Tok != TOK_VALUE) {

            /* Try smart error recovery */
            if (D->Tok == TOK_IDENT || TokenIsKeyword (D->Tok)) {
                UnknownKeyword (D);
                continue;
            }

            /* Done */
            break;
        }

        /* Remember the token, skip it, check for equal */
        Tok = D->Tok;
        NextToken (D);
        if (!ConsumeEqual (D)) {
            goto ErrorExit;
        }

        /* Check what the token was */
        switch (Tok) {

            case TOK_ADDRSIZE:
                NextToken (D);
                InfoBits |= ibAddrSize;
                break;

            case TOK_FILE:
                if (!IntConstFollows (D)) {
                    goto ErrorExit;
                }
                FileId = D->IVal;
                InfoBits |= ibFileId;
                NextToken (D);
                break;

            case TOK_ID:
                if (!IntConstFollows (D)) {
                    goto ErrorExit;
                }
                Id = D->IVal;
                NextToken (D);
                InfoBits |= ibId;
                break;

            case TOK_NAME:
                if (!StrConstFollows (D)) {
                    goto ErrorExit;
                }
                SB_Copy (&Name, &D->SVal);
                SB_Terminate (&Name);
                InfoBits |= ibName;
                NextToken (D);
                break;

            case TOK_PARENT:
                if (!IntConstFollows (D)) {
                    goto ErrorExit;
                }
                ParentId = D->IVal;
                NextToken (D);
                InfoBits |= ibParentId;
                break;

            case TOK_SCOPE:
                if (!IntConstFollows (D)) {
                    goto ErrorExit;
                }
                ScopeId = D->IVal;
                NextToken (D);
                InfoBits |= ibScopeId;
                break;

            case TOK_SEGMENT:
                if (!IntConstFollows (D)) {
                    goto ErrorExit;
                }
                SegId = (unsigned) D->IVal;
                InfoBits |= ibSegId;
                NextToken (D);
                break;

            case TOK_SIZE:
                if (!IntConstFollows (D)) {
                    goto ErrorExit;
                }
                Size = (cc65_size) D->IVal;
                InfoBits |= ibSize;
                NextToken (D);
                break;

            case TOK_TYPE:
                switch (D->Tok) {
                    case TOK_EQUATE:    Type = CC65_SYM_EQUATE;         break;
                    case TOK_LABEL:     Type = CC65_SYM_LABEL;          break;

                    default:
                        ParseError (D, CC65_ERROR,
                                    "Unknown value for attribute \"type\"");
                        SkipLine (D);
                        goto ErrorExit;
                }
                NextToken (D);
                InfoBits |= ibType;
                break;

            case TOK_VALUE:
                if (!IntConstFollows (D)) {
                    goto ErrorExit;
                }
                Value = D->IVal;
                InfoBits |= ibValue;
                NextToken (D);
                break;

            default:
                /* NOTREACHED */
                UnexpectedToken (D);
                goto ErrorExit;

        }

        /* Comma or done */
        if (D->Tok != TOK_COMMA) {
            break;
        }
        NextToken (D);
    }

    /* Check for end of line */
    if (D->Tok != TOK_EOL && D->Tok != TOK_EOF) {
        UnexpectedToken (D);
        SkipLine (D);
        goto ErrorExit;
    }

    /* Check for required and/or matched information */
    if ((InfoBits & ibRequired) != ibRequired) {
        ParseError (D, CC65_ERROR, "Required attributes missing");
        goto ErrorExit;
    }
    if ((InfoBits & (ibScopeId | ibParentId)) == 0 ||
        (InfoBits & (ibScopeId | ibParentId)) == (ibScopeId | ibParentId)) {
        ParseError (D, CC65_ERROR, "Only one of \"parent\", \"scope\" must be specified");
        goto ErrorExit;
    }

    /* Create the symbol info */
    S = NewSymInfo (&Name);
    S->Id         = Id;
    S->Type       = Type;
    S->Value      = Value;
    S->Size       = Size;
    S->Seg.Id     = SegId;
    S->Scope.Id   = ScopeId;
    S->Parent.Id  = ParentId;

    /* Remember it */
    CollReplaceExpand (&D->Info->SymInfoById, S, Id);
    CollAppend (&D->Info->SymInfoByName, S);
    CollAppend (&D->Info->SymInfoByVal, S);

ErrorExit:
    /* Entry point in case of errors */
    SB_Done (&Name);
    return;
}



static void ParseVersion (InputData* D)
/* Parse a VERSION line */
{
    enum {
        ibNone      = 0x00,
        ibMajor     = 0x01,
        ibMinor     = 0x02,
        ibRequired  = ibMajor | ibMinor,
    } InfoBits = ibNone;

    /* Skip the VERSION token */
    NextToken (D);

    /* More stuff follows */
    while (D->Tok != TOK_EOL && D->Tok != TOK_EOF) {

        switch (D->Tok) {

            case TOK_MAJOR:
                NextToken (D);
                if (!ConsumeEqual (D)) {
                    goto ErrorExit;
                }
                if (!IntConstFollows (D)) {
                    goto ErrorExit;
                }
                D->MajorVersion = D->IVal;
                NextToken (D);
                InfoBits |= ibMajor;
                break;

            case TOK_MINOR:
                NextToken (D);
                if (!ConsumeEqual (D)) {
                    goto ErrorExit;
                }
                if (!IntConstFollows (D)) {
                    goto ErrorExit;
                }
                D->MinorVersion = D->IVal;
                NextToken (D);
                InfoBits |= ibMinor;
                break;

            case TOK_IDENT:
                /* Try to skip unknown keywords that may have been added by
                 * a later version.
                 */
                UnknownKeyword (D);
                break;

            default:
                UnexpectedToken (D);
                SkipLine (D);
                goto ErrorExit;
        }

        /* Comma follows before next attribute */
        if (D->Tok == TOK_COMMA) {
            NextToken (D);
        } else if (D->Tok == TOK_EOL || D->Tok == TOK_EOF) {
            break;
        } else {
            UnexpectedToken (D);
            goto ErrorExit;
        }
    }

    /* Check for required information */
    if ((InfoBits & ibRequired) != ibRequired) {
        ParseError (D, CC65_ERROR, "Required attributes missing");
        goto ErrorExit;
    }

ErrorExit:
    /* Entry point in case of errors */
    return;
}



/*****************************************************************************/
/*                              Data processing                              */
/*****************************************************************************/



static int FindFileInfoByName (Collection* FileInfos, const char* Name,
                               unsigned* Index)
/* Find the FileInfo for a given file name. The function returns true if the
 * name was found. In this case, Index contains the index of the first item
 * that matches. If the item wasn't found, the function returns false and
 * Index contains the insert position for Name.
 */
{
    /* Do a binary search */
    int Lo = 0;
    int Hi = (int) CollCount (FileInfos) - 1;
    int Found = 0;
    while (Lo <= Hi) {

        /* Mid of range */
        int Cur = (Lo + Hi) / 2;

        /* Get item */
        FileInfo* CurItem = CollAt (FileInfos, Cur);

        /* Compare */
        int Res = strcmp (CurItem->Name, Name);

        /* Found? */
        if (Res < 0) {
            Lo = Cur + 1;
        } else {
            Hi = Cur - 1;
            /* Since we may have duplicates, repeat the search until we've
             * the first item that has a match.
             */
            if (Res == 0) {
                Found = 1;
            }
        }
    }

    /* Pass back the index. This is also the insert position */
    *Index = Lo;
    return Found;
}



static void ProcessSegInfo (InputData* D)
/* Postprocess segment infos */
{
    /* Sort the segment infos by name */
    CollSort (&D->Info->SegInfoByName, CompareSegInfoByName);
}



static void ProcessFileInfo (InputData* D)
/* Postprocess file infos */
{
    /* Sort the file infos by name, so we can do a binary search */
    CollSort (&D->Info->FileInfoByName, CompareFileInfoByName);
}



static void ProcessLineInfo (InputData* D)
/* Postprocess line infos */
{
    unsigned I;

    /* Temporary collection with line infos sorted by address */
    Collection LineInfoByAddr = COLLECTION_INITIALIZER;

    /* Get pointers to the collections */
    Collection* LineInfos = &D->Info->LineInfoById;
    Collection* FileInfos = &D->Info->FileInfoById;
    Collection* SegInfos  = &D->Info->SegInfoById;

    /* Resize the temporary collection */
    CollGrow (&LineInfoByAddr, CollCount (LineInfos));

    /* Walk over the line infos and replace the id numbers of file and segment
     * with pointers to the actual structs. Add the line info to each file
     * where it is defined.
     */
    for (I = 0; I < CollCount (LineInfos); ++I) {

        /* Get LineInfo struct */
        LineInfo* L = CollAt (LineInfos, I);

        /* Replace the file id by a pointer to the FileInfo. Add a back
         * pointer
         */
        if (L->File.Id >= CollCount (FileInfos)) {
            ParseError (D,
                        CC65_ERROR,
                        "Invalid file id %u for line with id %u",
                        L->File.Id, L->Id);
            L->File.Info = 0;
        } else {
            L->File.Info = CollAt (FileInfos, L->File.Id);
            CollAppend (&L->File.Info->LineInfoByLine, L);
        }

        /* Replace the segment id by a pointer to the SegInfo */
        if (L->Seg.Id >= CollCount (SegInfos)) {
            ParseError (D,
                        CC65_ERROR,
                        "Invalid segment id %u for line with id %u",
                        L->Seg.Id, L->Id);
            L->Seg.Info = 0;
        } else {
            L->Seg.Info = CollAt (SegInfos, L->Seg.Id);
        }

        /* Append this line info to the temporary collection that is later
         * sorted by address.
         */
        CollAppend (&LineInfoByAddr, L);

        /* Next one */
        ++I;
    }

    /* Walk over all files and sort the line infos for each file so we can
     * do a binary search later.
     */
    for (I = 0; I < CollCount (FileInfos); ++I) {

        /* Get a pointer to this file info */
        FileInfo* F = CollAt (FileInfos, I);

        /* Sort the line infos for this file */
        CollSort (&F->LineInfoByLine, CompareLineInfoByLine);
    }

    /* Sort the collection with all line infos by address */
    CollSort (&LineInfoByAddr, CompareLineInfoByAddr);

    /* Create the line info list from the line info collection */
    CreateLineInfoList (&D->Info->LineInfoByAddr, &LineInfoByAddr);

    /* Remove the temporary collection */
    DoneCollection (&LineInfoByAddr);
}



static LineInfoListEntry* FindLineInfoByAddr (const LineInfoList* L, cc65_addr Addr)
/* Find the index of a LineInfo for a given address. Returns 0 if no such
 * lineinfo was found.
 */
{
    /* Do a binary search */
    int Lo = 0;
    int Hi = (int) L->Count - 1;
    while (Lo <= Hi) {

        /* Mid of range */
        int Cur = (Lo + Hi) / 2;

        /* Get item */
        LineInfoListEntry* CurItem = &L->List[Cur];

        /* Found? */
        if (CurItem->Addr > Addr) {
            Hi = Cur - 1;
        } else if (CurItem->Addr < Addr) {
            Lo = Cur + 1;
        } else {
            /* Found */
            return CurItem;
        }
    }

    /* Not found */
    return 0;
}



static int FindLineInfoByLine (Collection* LineInfos, cc65_line Line,
                               unsigned* Index)
/* Find the LineInfo for a given line number. The function returns true if the
 * line was found. In this case, Index contains the index of the first item
 * that matches. If the item wasn't found, the function returns false and
 * Index contains the insert position for the line.
 */
{
    /* Do a binary search */
    int Lo = 0;
    int Hi = (int) CollCount (LineInfos) - 1;
    int Found = 0;
    while (Lo <= Hi) {

        /* Mid of range */
        int Cur = (Lo + Hi) / 2;

        /* Get item */
        const LineInfo* CurItem = CollAt (LineInfos, Cur);

        /* Found? */
        if (Line > CurItem->Line) {
            Lo = Cur + 1;
        } else {
            Hi = Cur - 1;
            /* Since we may have duplicates, repeat the search until we've
             * the first item that has a match.
             */
            if (Line == CurItem->Line) {
                Found = 1;
            }
        }
    }

    /* Pass back the index. This is also the insert position */
    *Index = Lo;
    return Found;
}



static void ProcessSymInfo (InputData* D)
/* Postprocess symbol infos */
{
    /* Walk over the symbols and resolve the references */
    unsigned I;
    for (I = 0; I < CollCount (&D->Info->SymInfoById); ++I) {

        /* Get the symbol info */
        SymInfo* S = CollAt (&D->Info->SymInfoById, I);

        /* Resolve segment */
        if (S->Seg.Id == CC65_INV_ID) {
            S->Seg.Info = 0;
        } else if (S->Seg.Id >= CollCount (&D->Info->SegInfoById)) {
            ParseError (D,
                        CC65_ERROR,
                        "Invalid segment id %u for symbol with id %u",
                        S->Seg.Id, S->Id);
            S->Seg.Info = 0;
        } else {
            S->Seg.Info = CollAt (&D->Info->SegInfoById, S->Seg.Id);
        }

        /* Resolve the scope */
        if (S->Scope.Id == CC65_INV_ID) {
            S->Scope.Info = 0;
        } else if (S->Scope.Id >= CollCount (&D->Info->ScopeInfoById)) {
            ParseError (D,
                        CC65_ERROR,
                        "Invalid scope id %u for symbol with id %u",
                        S->Scope.Id, S->Id);
            S->Scope.Info = 0;
        } else {
            S->Scope.Info = CollAt (&D->Info->ScopeInfoById, S->Scope.Id);
        }

        /* Resolve the parent */
        if (S->Parent.Id == CC65_INV_ID) {
            S->Parent.Info = 0;
        } else if (S->Parent.Id >= CollCount (&D->Info->SymInfoById)) {
            ParseError (D,
                        CC65_ERROR,
                        "Invalid parent id %u for symbol with id %u",
                        S->Parent.Id, S->Id);
            S->Parent.Info = 0;
        } else {
            S->Parent.Info = CollAt (&D->Info->SymInfoById, S->Parent.Id);
        }
    }

    /* Second run. Resolve scopes for cheap locals */
    for (I = 0; I < CollCount (&D->Info->SymInfoById); ++I) {

        /* Get the symbol info */
        SymInfo* S = CollAt (&D->Info->SymInfoById, I);

        /* Resolve the scope */
        if (S->Scope.Info == 0) {
            /* No scope - must have a parent */
            if (S->Parent.Info == 0) {
                ParseError (D,
                            CC65_ERROR,
                            "Symbol with id %u has no parent and no scope",
                            S->Id);
            } else if (S->Parent.Info->Scope.Info == 0) {
                ParseError (D,
                            CC65_ERROR,
                            "Symbol with id %u has parent %u without a scope",
                            S->Id, S->Parent.Info->Id);
            } else {
                S->Scope.Info = S->Parent.Info->Scope.Info;
            }
        }
    }

    /* Sort the symbol infos */
    CollSort (&D->Info->SymInfoByName, CompareSymInfoByName);
    CollSort (&D->Info->SymInfoByVal,  CompareSymInfoByVal);
}



static int FindSymInfoByName (Collection* SymInfos, const char* Name, unsigned* Index)
/* Find the SymInfo for a given file name. The function returns true if the
 * name was found. In this case, Index contains the index of the first item
 * that matches. If the item wasn't found, the function returns false and
 * Index contains the insert position for Name.
 */
{
    /* Do a binary search */
    int Lo = 0;
    int Hi = (int) CollCount (SymInfos) - 1;
    int Found = 0;
    while (Lo <= Hi) {

        /* Mid of range */
        int Cur = (Lo + Hi) / 2;

        /* Get item */
        SymInfo* CurItem = CollAt (SymInfos, Cur);

        /* Compare */
        int Res = strcmp (CurItem->Name, Name);

        /* Found? */
        if (Res < 0) {
            Lo = Cur + 1;
        } else {
            Hi = Cur - 1;
            /* Since we may have duplicates, repeat the search until we've
             * the first item that has a match.
             */
            if (Res == 0) {
                Found = 1;
            }
        }
    }

    /* Pass back the index. This is also the insert position */
    *Index = Lo;
    return Found;
}



static int FindSymInfoByValue (Collection* SymInfos, long Value, unsigned* Index)
/* Find the SymInfo for a given value. The function returns true if the
 * value was found. In this case, Index contains the index of the first item
 * that matches. If the item wasn't found, the function returns false and
 * Index contains the insert position for the given value.
 */
{
    /* Do a binary search */
    int Lo = 0;
    int Hi = (int) CollCount (SymInfos) - 1;
    int Found = 0;
    while (Lo <= Hi) {

        /* Mid of range */
        int Cur = (Lo + Hi) / 2;

        /* Get item */
        SymInfo* CurItem = CollAt (SymInfos, Cur);

        /* Found? */
        if (Value > CurItem->Value) {
            Lo = Cur + 1;
        } else {
            Hi = Cur - 1;
            /* Since we may have duplicates, repeat the search until we've
             * the first item that has a match.
             */
            if (Value == CurItem->Value) {
                Found = 1;
            }
        }
    }

    /* Pass back the index. This is also the insert position */
    *Index = Lo;
    return Found;
}



static void ProcessScopeInfo (InputData* D)
/* Postprocess scope infos */
{
    /* Get pointers to the scope info collections */
    Collection* ScopeInfoById = &D->Info->ScopeInfoById;
}



/*****************************************************************************/
/*                             Debug info files                              */
/*****************************************************************************/



cc65_dbginfo cc65_read_dbginfo (const char* FileName, cc65_errorfunc ErrFunc)
/* Parse the debug info file with the given name. On success, the function
 * will return a pointer to an opaque cc65_dbginfo structure, that must be
 * passed to the other functions in this module to retrieve information.
 * errorfunc is called in case of warnings and errors. If the file cannot be
 * read successfully, NULL is returned.
 */
{
    /* Data structure used to control scanning and parsing */
    InputData D = {
        0,                      /* Name of input file */
        1,                      /* Line number */
        0,                      /* Input file */
        0,                      /* Line at start of current token */
        0,                      /* Column at start of current token */
        0,                      /* Number of errors */
        0,                      /* Input file */
        ' ',                    /* Input character */
        TOK_INVALID,            /* Input token */
        0,                      /* Integer constant */
        STRBUF_INITIALIZER,     /* String constant */
        0,                      /* Function called in case of errors */
        0,                      /* Major version number */
        0,                      /* Minor version number */
        0,                      /* Pointer to debug info */
    };
    D.FileName = FileName;
    D.Error    = ErrFunc;

    /* Open the input file */
    D.F = fopen (D.FileName, "rt");
    if (D.F == 0) {
        /* Cannot open */
        ParseError (&D, CC65_ERROR,
                    "Cannot open input file \"%s\": %s",
                     D.FileName, strerror (errno));
        return 0;
    }

    /* Create a new debug info struct */
    D.Info = NewDbgInfo ();

    /* Prime the pump */
    NextToken (&D);

    /* The first line in the file must specify version information */
    if (D.Tok != TOK_VERSION) {
        ParseError (&D, CC65_ERROR,
                    "\"version\" keyword missing in first line - this is not "
                    "a valid debug info file");
        goto CloseAndExit;
    }

    /* Parse the version directive */
    ParseVersion (&D);

    /* Do several checks on the version number */
    if (D.MajorVersion < VER_MAJOR) {
        ParseError (
            &D, CC65_ERROR,
            "This is an old version of the debug info format that is no "
            "longer supported. Version found = %u.%u, version supported "
            "= %u.%u",
             D.MajorVersion, D.MinorVersion, VER_MAJOR, VER_MINOR
        );
        goto CloseAndExit;
    } else if (D.MajorVersion == VER_MAJOR && D.MinorVersion > VER_MINOR) {
        ParseError (
            &D, CC65_ERROR,
            "This is a slightly newer version of the debug info format. "
            "It might work, but you may get errors about unknown keywords "
            "and similar. Version found = %u.%u, version supported = %u.%u",
             D.MajorVersion, D.MinorVersion, VER_MAJOR, VER_MINOR
        );
    } else if (D.MajorVersion > VER_MAJOR) {
        ParseError (
            &D, CC65_WARNING,
            "The format of this debug info file is newer than what we "
            "know. Will proceed but probably fail. Version found = %u.%u, "
            "version supported = %u.%u",
             D.MajorVersion, D.MinorVersion, VER_MAJOR, VER_MINOR
        );
    }
    ConsumeEOL (&D);

    /* Parse lines */
    while (D.Tok != TOK_EOF) {

        switch (D.Tok) {

            case TOK_FILE:
                ParseFile (&D);
                break;

            case TOK_INFO:
                ParseInfo (&D);
                break;

            case TOK_LIBRARY:
                ParseLibrary (&D);
                break;

            case TOK_LINE:
                ParseLine (&D);
                break;

            case TOK_MODULE:
                ParseModule (&D);
                break;

            case TOK_SCOPE:
                ParseScope (&D);
                break;

            case TOK_SEGMENT:
                ParseSegment (&D);
                break;

            case TOK_SYM:
                ParseSym (&D);
                break;

            case TOK_IDENT:
                /* Output a warning, then skip the line with the unknown
                 * keyword that may have been added by a later version.
                 */
                ParseError (&D, CC65_WARNING,
                            "Unknown keyword \"%s\" - skipping",
                            SB_GetConstBuf (&D.SVal));

                SkipLine (&D);
                break;

            default:
                UnexpectedToken (&D);

        }

        /* EOL or EOF must follow */
        ConsumeEOL (&D);
    }

CloseAndExit:
    /* Close the file */
    fclose (D.F);

    /* Free memory allocated for SVal */
    SB_Done (&D.SVal);

    /* In case of errors, delete the debug info already allocated and
     * return NULL
     */
    if (D.Errors > 0) {
        /* Free allocated stuff */
        FreeDbgInfo (D.Info);
        return 0;
    }

    /* We do now have all the information from the input file. Do
     * postprocessing.
     */
    ProcessSegInfo (&D);
    ProcessFileInfo (&D);
    ProcessLineInfo (&D);
    ProcessSymInfo (&D);
    ProcessScopeInfo (&D);

#if DEBUG
    /* Debug output */
    DumpData (&D);
#endif

    /* Return the debug info struct that was created */
    return D.Info;
}



void cc65_free_dbginfo (cc65_dbginfo Handle)
/* Free debug information read from a file */
{
    if (Handle) {
        FreeDbgInfo (Handle);
    }
}



/*****************************************************************************/
/*                                 Libraries                                 */
/*****************************************************************************/



cc65_libraryinfo* cc65_get_librarylist (cc65_dbginfo Handle)
/* Return a list of all libraries */
{
    DbgInfo*            Info;
    Collection*         LibInfoById;
    cc65_libraryinfo*   D;
    unsigned            I;

    /* Check the parameter */
    assert (Handle != 0);

    /* The handle is actually a pointer to a debug info struct */
    Info = (DbgInfo*) Handle;

    /* Get a pointer to the library list */
    LibInfoById = &Info->LibInfoById;

    /* Allocate memory for the data structure returned to the caller */
    D = new_cc65_libraryinfo (CollCount (LibInfoById));

    /* Fill in the data */
    for (I = 0; I < CollCount (LibInfoById); ++I) {
        /* Copy the data */
        CopyLibInfo (D->data + I, CollAt (LibInfoById, I));
    }

    /* Return the result */
    return D;
}



cc65_libraryinfo* cc65_libraryinfo_byid (cc65_dbginfo Handle, unsigned Id)
/* Return information about a library with a specific id. The function
 * returns NULL if the id is invalid (no such library) and otherwise a
 * cc65_libraryinfo structure with one entry that contains the requested
 * library information.
 */
{
    DbgInfo*            Info;
    cc65_libraryinfo*   D;

    /* Check the parameter */
    assert (Handle != 0);

    /* The handle is actually a pointer to a debug info struct */
    Info = (DbgInfo*) Handle;

    /* Check if the id is valid */
    if (Id >= CollCount (&Info->LibInfoById)) {
        return 0;
    }

    /* Allocate memory for the data structure returned to the caller */
    D = new_cc65_libraryinfo (1);

    /* Fill in the data */
    CopyLibInfo (D->data, CollAt (&Info->LibInfoById, Id));

    /* Return the result */
    return D;
}



void cc65_free_libraryinfo (cc65_dbginfo Handle, cc65_libraryinfo* Info)
/* Free a library info record */
{
    /* Just for completeness, check the handle */
    assert (Handle != 0);

    /* Just free the memory */
    xfree (Info);
}



/*****************************************************************************/
/*                                 Line info                                 */
/*****************************************************************************/



cc65_lineinfo* cc65_lineinfo_byaddr (cc65_dbginfo Handle, unsigned long Addr)
/* Return line information for the given address. The function returns 0
 * if no line information was found.
 */
{
    LineInfoListEntry* E;
    cc65_lineinfo*  D = 0;

    /* Check the parameter */
    assert (Handle != 0);

    /* Search in the line infos for address */
    E = FindLineInfoByAddr (&((DbgInfo*) Handle)->LineInfoByAddr, Addr);

    /* Do we have line infos? */
    if (E != 0) {

        unsigned I;

        /* Prepare the struct we will return to the caller */
        D = new_cc65_lineinfo (E->Count);
        if (E->Count == 1) {
            CopyLineInfo (D->data, E->Data);
        } else {
            for (I = 0; I < D->count; ++I) {
                /* Copy data */
                CopyLineInfo (D->data + I, ((LineInfo**) E->Data)[I]);
            }
        }
    }

    /* Return the struct we've created */
    return D;
}



cc65_lineinfo* cc65_lineinfo_byname (cc65_dbginfo Handle, const char* FileName,
                                     cc65_line Line)
/* Return line information for a file/line number combination. The function
 * returns NULL if no line information was found.
 */
{
    DbgInfo*        Info;
    FileInfo*       F;
    cc65_lineinfo*  D;
    int             Found;
    unsigned        FileIndex;
    unsigned        I;
    Collection      LineInfoList = COLLECTION_INITIALIZER;

    /* Check the parameter */
    assert (Handle != 0);

    /* The handle is actually a pointer to a debug info struct */
    Info = (DbgInfo*) Handle;

    /* Search for the first file with this name */
    Found = FindFileInfoByName (&Info->FileInfoByName, FileName, &FileIndex);
    if (!Found) {
        return 0;
    }

    /* Loop over all files with this name */
    F = CollAt (&Info->FileInfoByName, FileIndex);
    while (Found) {

        unsigned LineIndex;
        LineInfo* L = 0;

        /* Search in the file for the given line */
        Found = FindLineInfoByLine (&F->LineInfoByLine, Line, &LineIndex);
        if (Found) {
            L = CollAt (&F->LineInfoByLine, LineIndex);
        }

        /* Add all line infos for this line */
        while (Found) {
            /* Add next */
            CollAppend (&LineInfoList, L);

            /* Check if the next one is also a match */
            if (++LineIndex >= CollCount (&F->LineInfoByLine)) {
                break;
            }
            L = CollAt (&F->LineInfoByLine, LineIndex);
            if (L->Line != Line) {
                Found = 0;
            }
        }

        /* Next entry */
        ++FileIndex;

        /* If the index is valid, check if the next entry is a file with the
         * same name.
         */
        if (FileIndex < CollCount (&Info->FileInfoByName)) {
            F = CollAt (&Info->FileInfoByName, FileIndex);
            Found = (strcmp (F->Name, FileName) == 0);
        } else {
            Found = 0;
        }
    }

    /* Check if we have entries */
    if (CollCount (&LineInfoList) == 0) {
        /* Nope */
        return 0;
    }

    /* Prepare the struct we will return to the caller */
    D = new_cc65_lineinfo (CollCount (&LineInfoList));

    /* Copy the data */
    for (I = 0; I < CollCount (&LineInfoList); ++I) {
        CopyLineInfo (D->data + I, CollAt (&LineInfoList, I));
    }

    /* Delete the temporary data collection */
    DoneCollection (&LineInfoList);

    /* Return the allocated struct */
    return D;
}



void cc65_free_lineinfo (cc65_dbginfo Handle, cc65_lineinfo* Info)
/* Free line info returned by one of the other functions */
{
    /* Just for completeness, check the handle */
    assert (Handle != 0);

    /* Just free the memory */
    xfree (Info);
}



/*****************************************************************************/
/*                                  Modules                                  */
/*****************************************************************************/



cc65_moduleinfo* cc65_get_modulelist (cc65_dbginfo Handle)
/* Return a list of all modules */
{
    DbgInfo*            Info;
    Collection*         ModInfoById;
    cc65_moduleinfo*    D;
    unsigned            I;

    /* Check the parameter */
    assert (Handle != 0);

    /* The handle is actually a pointer to a debug info struct */
    Info = (DbgInfo*) Handle;

    /* Get a pointer to the module list */
    ModInfoById = &Info->ModInfoById;

    /* Allocate memory for the data structure returned to the caller */
    D = new_cc65_moduleinfo (CollCount (ModInfoById));

    /* Fill in the data */
    for (I = 0; I < CollCount (ModInfoById); ++I) {
        /* Copy the data */
        CopyModInfo (D->data + I, CollAt (ModInfoById, I));
    }

    /* Return the result */
    return D;
}



cc65_moduleinfo* cc65_moduleinfo_byid (cc65_dbginfo Handle, unsigned Id)
/* Return information about a module with a specific id. The function
 * returns NULL if the id is invalid (no such module) and otherwise a
 * cc65_moduleinfo structure with one entry that contains the requested
 * module information.
 */
{
    DbgInfo*            Info;
    cc65_moduleinfo*    D;

    /* Check the parameter */
    assert (Handle != 0);

    /* The handle is actually a pointer to a debug info struct */
    Info = (DbgInfo*) Handle;

    /* Check if the id is valid */
    if (Id >= CollCount (&Info->ModInfoById)) {
        return 0;
    }

    /* Allocate memory for the data structure returned to the caller */
    D = new_cc65_moduleinfo (1);

    /* Fill in the data */
    CopyModInfo (D->data, CollAt (&Info->ModInfoById, Id));

    /* Return the result */
    return D;
}



void cc65_free_moduleinfo (cc65_dbginfo Handle, cc65_moduleinfo* Info)
/* Free a module info record */
{
    /* Just for completeness, check the handle */
    assert (Handle != 0);

    /* Just free the memory */
    xfree (Info);
}



/*****************************************************************************/
/*                               Source files                                */
/*****************************************************************************/



cc65_sourceinfo* cc65_get_sourcelist (cc65_dbginfo Handle)
/* Return a list of all source files */
{
    DbgInfo*            Info;
    Collection*         FileInfoById;
    cc65_sourceinfo*    D;
    unsigned            I;

    /* Check the parameter */
    assert (Handle != 0);

    /* The handle is actually a pointer to a debug info struct */
    Info = (DbgInfo*) Handle;

    /* Get a pointer to the file list */
    FileInfoById = &Info->FileInfoById;

    /* Allocate memory for the data structure returned to the caller. */
    D = new_cc65_sourceinfo (CollCount (FileInfoById));

    /* Fill in the data */
    for (I = 0; I < CollCount (FileInfoById); ++I) {
        /* Copy the data */
        CopyFileInfo (D->data + I, CollAt (FileInfoById, I));
    }

    /* Return the result */
    return D;
}



cc65_sourceinfo* cc65_sourceinfo_byid (cc65_dbginfo Handle, unsigned Id)
/* Return information about a source file with a specific id. The function
 * returns NULL if the id is invalid (no such source file) and otherwise a
 * cc65_sourceinfo structure with one entry that contains the requested
 * source file information.
 */
{
    DbgInfo*            Info;
    cc65_sourceinfo*    D;

    /* Check the parameter */
    assert (Handle != 0);

    /* The handle is actually a pointer to a debug info struct */
    Info = (DbgInfo*) Handle;

    /* Check if the id is valid */
    if (Id >= CollCount (&Info->FileInfoById)) {
        return 0;
    }

    /* Allocate memory for the data structure returned to the caller */
    D = new_cc65_sourceinfo (1);

    /* Fill in the data */
    CopyFileInfo (D->data, CollAt (&Info->FileInfoById, Id));

    /* Return the result */
    return D;
}




void cc65_free_sourceinfo (cc65_dbginfo Handle, cc65_sourceinfo* Info)
/* Free a source info record */
{
    /* Just for completeness, check the handle */
    assert (Handle != 0);

    /* Free the memory */
    xfree (Info);
}



/*****************************************************************************/
/*                                 Segments                                  */
/*****************************************************************************/



cc65_segmentinfo* cc65_get_segmentlist (cc65_dbginfo Handle)
/* Return a list of all segments referenced in the debug information */
{
    DbgInfo*            Info;
    Collection*         SegInfoByName;
    cc65_segmentinfo*   D;
    unsigned            I;

    /* Check the parameter */
    assert (Handle != 0);

    /* The handle is actually a pointer to a debug info struct */
    Info = (DbgInfo*) Handle;

    /* Get a pointer to the segment list */
    SegInfoByName = &Info->SegInfoByName;

    /* Allocate memory for the data structure returned to the caller */
    D = new_cc65_segmentinfo (CollCount (SegInfoByName));

    /* Fill in the data */
    for (I = 0; I < CollCount (SegInfoByName); ++I) {
        /* Copy the data */
        CopySegInfo (D->data + I, CollAt (SegInfoByName, I));
    }

    /* Return the result */
    return D;
}



cc65_segmentinfo* cc65_segmentinfo_byid (cc65_dbginfo Handle, unsigned Id)
/* Return information about a segment with a specific id. The function returns
 * NULL if the id is invalid (no such segment) and otherwise a segmentinfo
 * structure with one entry that contains the requested segment information.
 */
{
    DbgInfo*            Info;
    cc65_segmentinfo*   D;

    /* Check the parameter */
    assert (Handle != 0);

    /* The handle is actually a pointer to a debug info struct */
    Info = (DbgInfo*) Handle;

    /* Check if the id is valid */
    if (Id >= CollCount (&Info->SegInfoById)) {
        return 0;
    }

    /* Allocate memory for the data structure returned to the caller */
    D = new_cc65_segmentinfo (1);

    /* Fill in the data */
    CopySegInfo (D->data, CollAt (&Info->SegInfoById, Id));

    /* Return the result */
    return D;
}



void cc65_free_segmentinfo (cc65_dbginfo Handle, cc65_segmentinfo* Info)
/* Free a segment info record */
{
    /* Just for completeness, check the handle */
    assert (Handle != 0);

    /* Free the memory */
    xfree (Info);
}



/*****************************************************************************/
/*                                  Symbols                                  */
/*****************************************************************************/



cc65_symbolinfo* cc65_symbol_byid (cc65_dbginfo Handle, unsigned Id)
/* Return the symbol with a given id. The function returns NULL if no symbol
 * with this id was found.
 */
{
    DbgInfo*            Info;
    cc65_symbolinfo*    D;

    /* Check the parameter */
    assert (Handle != 0);

    /* The handle is actually a pointer to a debug info struct */
    Info = (DbgInfo*) Handle;

    /* Check if the id is valid */
    if (Id >= CollCount (&Info->SymInfoById)) {
        return 0;
    }

    /* Allocate memory for the data structure returned to the caller */
    D = new_cc65_symbolinfo (1);

    /* Fill in the data */
    CopySymInfo (D->data, CollAt (&Info->SymInfoById, Id));

    /* Return the result */
    return D;
}



cc65_symbolinfo* cc65_symbol_byname (cc65_dbginfo Handle, const char* Name)
/* Return a list of symbols with a given name. The function returns NULL if
 * no symbol with this name was found.
 */
{
    DbgInfo*            Info;
    Collection*         SymInfoByName;
    cc65_symbolinfo*    D;
    unsigned            I;
    unsigned            Index;
    unsigned            Count;

    /* Check the parameter */
    assert (Handle != 0);

    /* The handle is actually a pointer to a debug info struct */
    Info = (DbgInfo*) Handle;

    /* Get a pointer to the symbol list */
    SymInfoByName = &Info->SymInfoByName;

    /* Search for the symbol */
    if (!FindSymInfoByName (SymInfoByName, Name, &Index)) {
        /* Not found */
        return 0;
    }

    /* Index contains the position. Count how many symbols with this name
     * we have. Skip the first one, since we have at least one.
     */
    Count = 1;
    while ((unsigned) Index + Count < CollCount (SymInfoByName)) {
        const SymInfo* S = CollAt (SymInfoByName, (unsigned) Index + Count);
        if (strcmp (S->Name, Name) != 0) {
            break;
        }
        ++Count;
    }

    /* Allocate memory for the data structure returned to the caller */
    D = new_cc65_symbolinfo (Count);

    /* Fill in the data */
    for (I = 0; I < Count; ++I) {
        /* Copy the data */
        CopySymInfo (D->data + I, CollAt (SymInfoByName, Index++));
    }

    /* Return the result */
    return D;
}



cc65_symbolinfo* cc65_symbol_inrange (cc65_dbginfo Handle, cc65_addr Start, cc65_addr End)
/* Return a list of labels in the given range. End is inclusive. The function
 * return NULL if no symbols within the given range are found. Non label
 * symbols are ignored and not returned.
 */
{
    DbgInfo*            Info;
    Collection*         SymInfoByVal;
    Collection          SymInfoList = COLLECTION_INITIALIZER;
    cc65_symbolinfo*    D;
    unsigned            I;
    unsigned            Index;

    /* Check the parameter */
    assert (Handle != 0);

    /* The handle is actually a pointer to a debug info struct */
    Info = (DbgInfo*) Handle;

    /* Get a pointer to the symbol list */
    SymInfoByVal = &Info->SymInfoByVal;

    /* Search for the symbol. Because we're searching for a range, we cannot
     * make use of the function result.
     */
    FindSymInfoByValue (SymInfoByVal, Start, &Index);

    /* Start from the given index, check all symbols until the end address is
     * reached. Place all symbols into SymInfoList for later.
     */
    for (I = Index; I < CollCount (SymInfoByVal); ++I) {

        /* Get the item */
        SymInfo* Item = CollAt (SymInfoByVal, I);

        /* The collection is sorted by address, so if we get a value larger
         * than the end address, we're done.
         */
        if (Item->Value > (long) End) {
            break;
        }

        /* Ignore non-labels */
        if (Item->Type != CC65_SYM_LABEL) {
            continue;
        }

        /* Ok, remember this one */
        CollAppend (&SymInfoList, Item);
    }

    /* If we don't have any labels within the range, bail out. No memory has
     * been allocated for SymInfoList.
     */
    if (CollCount (&SymInfoList) == 0) {
        return 0;
    }

    /* Allocate memory for the data structure returned to the caller */
    D = new_cc65_symbolinfo (CollCount (&SymInfoList));

    /* Fill in the data */
    for (I = 0; I < CollCount (&SymInfoList); ++I) {
        /* Copy the data */
        CopySymInfo (D->data + I, CollAt (&SymInfoList, I));
    }

    /* Free the collection */
    DoneCollection (&SymInfoList);

    /* Return the result */
    return D;
}



void cc65_free_symbolinfo (cc65_dbginfo Handle, cc65_symbolinfo* Info)
/* Free a symbol info record */
{
    /* Just for completeness, check the handle */
    assert (Handle != 0);

    /* Free the memory */
    xfree (Info);
}



/*****************************************************************************/
/*                                  Scopes                                   */
/*****************************************************************************/



cc65_scopeinfo* cc65_scope_byid (cc65_dbginfo Handle, unsigned Id)
/* Return the scope with a given id. The function returns NULL if no scope
 * with this id was found.
 */
{
    DbgInfo*            Info;
    cc65_scopeinfo*     D;

    /* Check the parameter */
    assert (Handle != 0);

    /* The handle is actually a pointer to a debug info struct */
    Info = (DbgInfo*) Handle;

    /* Check if the id is valid */
    if (Id >= CollCount (&Info->ScopeInfoById)) {
        return 0;
    }

    /* Allocate memory for the data structure returned to the caller */
    D = new_cc65_scopeinfo (1);

    /* Fill in the data */
    CopyScopeInfo (D->data, CollAt (&Info->ScopeInfoById, Id));

    /* Return the result */
    return D;
}



void cc65_free_scopeinfo (cc65_dbginfo Handle, cc65_scopeinfo* Info)
/* Free a scope info record */
{
    /* Just for completeness, check the handle */
    assert (Handle != 0);

    /* Free the memory */
    xfree (Info);
}



