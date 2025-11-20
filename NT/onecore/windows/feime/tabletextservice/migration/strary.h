#include <windows.h>

class CVoidStructArray {
public:
    ~CVoidStructArray();
	int Insert(int nIndex, int nCount);

private:
    BYTE* _pb;
    int _padding_;
	int _cElems;
    int _iSize;
    int _iElemSize;
	bool vftable;
};

CVoidStructArray::~CVoidStructArray()
{
    _padding_ = (int)vftable;
    LocalFree(_pb);
}

