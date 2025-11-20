#include "strary.h"

int CVoidStructArray::Insert(int nIndex, int nCount)
{
    int nCurrentElems;
    DWORD dwNewSize;
    DWORD dwAllocSize;
    BYTE* pbNew;
    
    if ((nIndex >= 0) && (nCurrentElems = _cElems, nIndex <= nCurrentElems) && 
        (dwNewSize = nCurrentElems + 1, nCurrentElems <= (int)dwNewSize)) {
        
        if ((int)dwNewSize <= _iSize) {
            goto LAB_INSERT;
        }
        
        dwNewSize = nCurrentElems / 2 + nCurrentElems;
        if ((int)(nCurrentElems + 1) <= (int)dwNewSize) {
            dwNewSize = nCurrentElems + 1;
        }
        
        dwAllocSize = _iElemSize * dwNewSize;
        if (_pb == NULL) {
            pbNew = (BYTE*)LocalAlloc(LMEM_ZEROINIT, dwAllocSize);
        }
        else {
            pbNew = (BYTE*)LocalReAlloc(_pb, dwAllocSize, LMEM_ZEROINIT | LMEM_MOVEABLE);
        }
        
        if (pbNew != NULL) {
            _pb = pbNew;
            _iSize = dwNewSize;
            goto LAB_INSERT;
        }
    }
    return 0;

LAB_INSERT:
    if (nIndex < _cElems) {
        memmove(_pb + (nIndex + 1) * _iElemSize, 
                _pb + nIndex * _iElemSize, 
                (_cElems - nIndex) * _iElemSize);
    }
    _cElems = _cElems + 1;
    return 1;
}

