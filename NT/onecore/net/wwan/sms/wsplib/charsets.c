#include <stdlib.h>
#include <string.h>

/* Enum definition */
typedef enum t_LngIanaCharset {
    eLngCharsetUnknown = 0,
    eLngCharsetUsAscii = 3,
    eLngCharsetIso8859_1 = 4,
    eLngCharsetIso8859_2 = 5,
    eLngCharsetIso8859_3 = 6,
    eLngCharsetIso8859_4 = 7,
    eLngCharsetIso8859_5 = 8,
    eLngCharsetIso8859_6 = 9,
    eLngCharsetIso8859_7 = 10,
    eLngCharsetIso8859_8 = 11,
    eLngCharsetIso8859_9 = 12,
    eLngCharsetShiftJis = 17,
    eLngCharsetUtf8 = 106,
    eLngCharsetUnicode = 1000,
    eLngCharsetGB2312 = 2025,
    eLngCharsetBig5 = 2026,
    eLngCharsetWindows1251 = 2251,
    eLngCharsetUnicodeBigEndian = 32766,
    eLngCharsetUnicodePlatform = 32767,
    eLngCharsetEnd = 32768
} t_LngIanaCharset;

/* Global/external table reference */
#define CHARSET_TABLE_ADDRESS 0x10000f2c
#define charsetTable (*(int ***)CHARSET_TABLE_ADDRESS)

long LngFindIanaFromMime(t_LngIanaCharset *result, char *mimeCharset, unsigned int param_3)
{
    char firstChar;
    int comparisonResult;
    unsigned int lowercaseChar;
    int **tableEntry;
    
    if (result != NULL) {
        firstChar = *mimeCharset;
        
        /* Convert to lowercase if character is uppercase A-Z */
        if ((firstChar < '[') && ('@' < firstChar)) {
            lowercaseChar = (int)firstChar | 0x20;  /* Convert to lowercase */
        }
        else {
            lowercaseChar = (unsigned int)firstChar;
        }
        
        /* Check if character is a-z and table entry exists */
        if ((lowercaseChar - 0x61 < 0x1A) && 
            (tableEntry = (int **)((lowercaseChar - 0x61) * 4 + (int)charsetTable), 
             tableEntry != NULL)) {
            
            comparisonResult = *tableEntry[0];
            
            while (comparisonResult != 0) {
                comparisonResult = _strnicmp((const char *)comparisonResult, mimeCharset, (size_t)result);
                
                if ((comparisonResult == 0) && (*(char *)((int)result + *tableEntry[0]) == '\0')) {
                    *result = (t_LngIanaCharset)tableEntry[1];
                    return 0;
                }
                
                tableEntry = tableEntry + 2;
                comparisonResult = *tableEntry[0];
            }
        }
    }
    
    *result = eLngCharsetUnknown;
    return -0x7FE4FFFD;  /* Error code: 0x801B0003 in two's complement */
}

