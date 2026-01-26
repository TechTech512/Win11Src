#pragma warning (disable:4530)
#pragma warning (disable:4995)

#include <windows.h>
#include <ole2.h>
#include <oleauto.h>
#include <xmllite.h>
#include <msxml6.h>
#include <strsafe.h>
#include <string>
#include <memory>
#include "resource.h"

IXMLDOMDocument* g_pIDoc = 0;
HINSTANCE g_hinst = NULL;
unsigned short g_szFallBackMessage = NULL;
unsigned short g_szXmlHeader = NULL;
IMXWriter* g_pIWriter = 0;
ISAXXMLReader* g_pISaxReader = 0;

IXMLDOMNode* GetChildNode(IXMLDOMNode* pParentNode, const WCHAR* nodeName);

/* WCHAR* __cdecl GetLastInsertString(struct _EVENTLOGRECORD*) */

WCHAR* __cdecl GetLastInsertString(_EVENTLOGRECORD* pRecord)
{
    WCHAR* pCurrentString = NULL;
    
    // Check if there are any strings in the event record
    if (pRecord->NumStrings != 0) {
        // Calculate pointer to the first string
        pCurrentString = (WCHAR*)((BYTE*)pRecord + pRecord->StringOffset);
    }
    
    // Default error message
    WCHAR szErrorMsg[] = L"Corrupted process";
    
    // Check if the string pointer is valid
    if (IsBadStringPtrW(pCurrentString, (UINT_PTR)-1) && pRecord->NumStrings != 0) {
        // Use error message instead
        pCurrentString = szErrorMsg;
        pRecord->NumStrings = 1;
    }
    
    // Skip to the last insert string (NumStrings - 1)
    if (pRecord->NumStrings > 1) {
        DWORD stringsToSkip = pRecord->NumStrings - 1;
        
        while (stringsToSkip > 0) {
            // Find the end of the current string
            WCHAR* pEnd = pCurrentString;
            while (*pEnd != L'\0') {
                pEnd++;
            }
            
            // Move to the next string (skip the null terminator)
            pCurrentString = pEnd + 1;
            stringsToSkip--;
        }
    }
    
    return pCurrentString;
}

/* long __cdecl InitXmlParser(WCHAR*, unsigned long long) */

long __cdecl InitXmlParser(WCHAR* pXmlString, unsigned long long xmlLength)
{
    HRESULT hr;
    long result;
    VARIANT_BOOL vbSuccess;
    BSTR bstrXml = NULL;
    
    // Create XML document object if not already created
    if (g_pIDoc == NULL) {
        const CLSID CLSID_DOMDocument = {0x2933BF90, 0x7B36, 0x11D2, {0xB2, 0x0E, 0x00, 0xC0, 0x4F, 0x98, 0x3E, 0x60}};
        const IID IID_IXMLDOMDocument = {0x2933BF81, 0x7B36, 0x11D2, {0xB2, 0x0E, 0x00, 0xC0, 0x4F, 0x98, 0x3E, 0x60}};
        
        hr = CoCreateInstance(CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, IID_IXMLDOMDocument, (void**)&g_pIDoc);
        if (FAILED(hr)) {
            return hr; // Return error code
        }
    }
    
    // Initialize success flag
    vbSuccess = VARIANT_FALSE;
    
    // Check if the XML length is valid (less than maximum BSTR length)
    if (xmlLength < 0xFFFFFFFF) {
        // Create BSTR from XML string
        bstrXml = SysAllocString(pXmlString);
        if (bstrXml != NULL) {
            // Load XML into document (method offset 0x208 = loadXML)
            // This is IXMLDOMDocument::loadXML method
            hr = g_pIDoc->loadXML(bstrXml, &vbSuccess);
            SysFreeString(bstrXml);
            
            if (SUCCEEDED(hr) && vbSuccess == VARIANT_TRUE) {
                result = 0; // Success
            } else {
                result = hr; // Return error code
                if (SUCCEEDED(hr) && vbSuccess == VARIANT_FALSE) {
                    result = E_FAIL; // XML parsing failed
                }
            }
        } else {
            result = E_OUTOFMEMORY; // Memory allocation failed
        }
    } else {
        // XML too large
        result = XML_E_INVALID_DECIMAL; // -0x7fffbffb = XML_E_INVALID_DECIMAL
    }
    
    return result;
}

/* int __cdecl GetRenderedValue(WCHAR*, WCHAR*, unsigned long long) */

int __cdecl GetRenderedValue(WCHAR* pNodeName, WCHAR* pOutBuffer, unsigned long long bufferSize)
{
    HRESULT hr;
    int result = 0;
    IXMLDOMNode* pRootNode = NULL;
    IXMLDOMNode* pRenderingInfoNode = NULL;
    IXMLDOMNode* pTargetNode = NULL;
    IXMLDOMNode* pChildNodes = NULL;
    VARIANT varValue;
    
    // Initialize output buffer
    if (pOutBuffer != NULL && bufferSize > 0) {
        *pOutBuffer = L'\0';
    }
    
    // Get document root node (method offset 0x168 = get_documentElement)
    hr = g_pIDoc->get_documentElement((IXMLDOMElement**)&pRootNode);
    
    if (SUCCEEDED(hr) && pRootNode != NULL) {
        // Find RenderingInfo node
        pRenderingInfoNode = GetChildNode(pRootNode, L"RenderingInfo");
        
        if (pRenderingInfoNode != NULL) {
            // Find the target node within RenderingInfo
            pTargetNode = GetChildNode(pRenderingInfoNode, pNodeName);
            pRenderingInfoNode->Release();
            
            if (pTargetNode != NULL) {
                // Initialize variant
                VariantInit(&varValue);
                
                // Get child nodes of the target node (method offset 0x68 = get_childNodes)
                hr = pTargetNode->get_childNodes((IXMLDOMNodeList**)&pChildNodes);
                
                if (SUCCEEDED(hr) && pChildNodes != NULL) {
                    // Get the text value (method offset 0x40 = get_text)
                    hr = ((IXMLDOMNode*)pChildNodes)->get_text((BSTR*)&varValue);
                    
                    if (SUCCEEDED(hr) && varValue.vt == VT_BSTR) {
                        // Get the string length
                        size_t textLength = SysStringLen(varValue.bstrVal);
                        
                        if (bufferSize > 0) {
                            // Check for valid buffer size
                            if (bufferSize < 0x80000000 && textLength < 0x7FFFFFFF) {
                                // Copy the string to output buffer
                                size_t dummyLen = 0;
                                StringCopyWorkerW(pOutBuffer, (unsigned int)bufferSize, &dummyLen, 
                                                  varValue.bstrVal, textLength);
                                result = 1;
                            } else {
                                // Buffer too small or string too large
                                if (pOutBuffer != NULL) {
                                    *pOutBuffer = L'\0';
                                }
                            }
                        } else {
                            // No buffer provided, just indicate success
                            result = 1;
                        }
                    }
                    
                    pChildNodes->Release();
                }
                
                VariantClear(&varValue);
                pTargetNode->Release();
            }
        }
        
        pRootNode->Release();
    }
    
    return result;
}

void* __cdecl operator new(size_t size)
{
    return HeapAlloc(GetProcessHeap(), 0, size);
}

void __cdecl operator delete(void* ptr)
{
    if (ptr)
        HeapFree(GetProcessHeap(), 0, ptr);
}

void __cdecl operator delete(void* ptr, unsigned int)
{
    if (ptr)
        HeapFree(GetProcessHeap(), 0, ptr);
}

void __cdecl operator delete[](void* ptr)
{
    if (ptr)
        HeapFree(GetProcessHeap(), 0, ptr);
}

void __cdecl operator delete[](void* ptr, unsigned int)
{
    if (ptr)
        HeapFree(GetProcessHeap(), 0, ptr);
}

int GetFormattedXML(WCHAR** ppOutXml);

/* int __cdecl GetMessageFromXML(WCHAR**, unsigned long long*) */

int __cdecl GetMessageFromXML(WCHAR** ppOutMessage, unsigned long long* pOutSize)
{
    int result;
    size_t fallbackLen = 0;
    WCHAR* pTempBuffer = NULL;
    size_t requiredSize = 0;
    
    // Try to get the Message value from XML
    result = GetRenderedValue(L"Message", *ppOutMessage, *pOutSize);
    
    if (result == 0) {
        // If getting Message failed, use fallback message
        
        // Get length of fallback message
        size_t dummyLen = 0;
        unsigned int strRes = StringLengthWorkerW((STRSAFE_PCNZWCH)g_szFallBackMessage, 0xFF, &dummyLen);
        
        if (strRes == 0) {
            // Calculate required buffer size (length + null terminator)
            requiredSize = dummyLen + 1;
            
            // Check if current buffer is too small
            if (*pOutSize < requiredSize) {
                // Free existing buffer if it exists
                if (*ppOutMessage != NULL) {
                    operator delete(*ppOutMessage);
                    *ppOutMessage = NULL;
                }
                
                // Allocate new buffer
                pTempBuffer = (WCHAR*)operator new(requiredSize * sizeof(WCHAR));
                *ppOutMessage = pTempBuffer;
                *pOutSize = requiredSize;
            }
            
            pTempBuffer = *ppOutMessage;
            if (pTempBuffer != NULL) {
                if (requiredSize > 0) {
                    // Check for valid size
                    if (requiredSize < 0x80000000) {
                        // Copy fallback message to buffer
                        size_t dummyOutLen = 0;
                        unsigned int copyRes = StringCopyWorkerW(pTempBuffer, 
                                                                 (unsigned int)requiredSize, 
                                                                 &dummyOutLen,
                                                                 (STRSAFE_PCNZWCH)g_szFallBackMessage, 
                                                                 0x7FFFFFFE);
                        
                        if ((int)copyRes >= 0) {
                            return 0; // Success
                        }
                    } else {
                        // Size too large, just set empty string
                        *pTempBuffer = L'\0';
                    }
                }
                
                // Ensure null termination
                if (*ppOutMessage != NULL) {
                    **ppOutMessage = L'\0';
                }
            }
        }
    }
    
    return result;
}

// DllMain
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_DETACH)
    {
        if (g_pIDoc != NULL)
        {
            g_pIDoc->Release();
            g_pIDoc = NULL;
        }
    }
    else if (fdwReason == DLL_PROCESS_ATTACH)
    {
        g_hinst = hinstDLL;
        LoadStringW(hinstDLL, IDS_MESSAGENOTFOUND, (LPWSTR)g_szFallBackMessage, 255);
        LoadStringW(g_hinst, IDS_EVENTXML, (LPWSTR)g_szXmlHeader, 255);
        DisableThreadLibraryCalls(hinstDLL);
    }
    return TRUE;
}

// GetCategoryStr
int GetCategoryStr(_EVENTLOGRECORD* pRecord, WCHAR* outBuf, unsigned int outBufSize)
{
    int result = 0;
    
    if (pRecord == NULL || (pRecord->Reserved & 0x8000) == 0 || outBuf == NULL || outBufSize == 0)
        return 0;
    
    WCHAR* lastInsert = GetLastInsertString(pRecord);
    if (lastInsert == NULL)
        return 0;
    
    unsigned long long xmlLen = 0;
    unsigned int strLenRes = StringLengthWorkerW(lastInsert, 5112, (size_t*)&xmlLen);
    if (strLenRes != 0)
        return 0;
    
    lastInsert[xmlLen] = L'\0';
    long parseRes = InitXmlParser(lastInsert, xmlLen);
    if (parseRes >= 0)
    {
        result = GetRenderedValue(L"Task", outBuf, outBufSize);
    }
    
    return result;
}

// GetChildNode
IXMLDOMNode* GetChildNode(IXMLDOMNode* pParentNode, const WCHAR* nodeName)
{
    IXMLDOMNode* pFoundNode = NULL;
    IXMLDOMNodeList* pChildList = NULL;
    IXMLDOMNode* pCurrentChild = NULL;
    BSTR childName = NULL;
    HRESULT hr;
    
    hr = pParentNode->get_childNodes((IXMLDOMNodeList**)&pChildList);
    if (FAILED(hr) || pChildList == NULL)
        return NULL;
    
    long listLen = 0;
    hr = pChildList->get_length(&listLen);
    
    for (long i = 0; i < listLen && pFoundNode == NULL; i++)
    {
        if (pCurrentChild)
        {
            pCurrentChild->Release();
            pCurrentChild = NULL;
        }
        
        hr = pChildList->get_item(i, &pCurrentChild);
        if (FAILED(hr) || pCurrentChild == NULL)
            continue;
        
        if (nodeName == NULL)
        {
            pFoundNode = pCurrentChild;
            pCurrentChild = NULL;
            break;
        }
        
        childName = NULL;
        hr = pCurrentChild->get_nodeName(&childName);
        if (SUCCEEDED(hr) && childName != NULL)
        {
            if (wcscmp(nodeName, childName) == 0)
            {
                pFoundNode = pCurrentChild;
                pCurrentChild = NULL;
            }
            SysFreeString(childName);
        }
        
        if (pFoundNode == NULL)
        {
            IXMLDOMNode* pNextChild = NULL;
            hr = pChildList->nextNode(&pNextChild);
            if (pCurrentChild)
            {
                pCurrentChild->Release();
                pCurrentChild = pNextChild;
            }
        }
    }
    
    if (pChildList)
        pChildList->Release();
    if (pCurrentChild && pCurrentChild != pFoundNode)
        pCurrentChild->Release();
    
    return pFoundNode;
}

// GetDescriptionStr
int GetDescriptionStr(_EVENTLOGRECORD* pRecord, unsigned long long param2, long long* pOutParam)
{
    if (pRecord == NULL || (pRecord->Reserved & 0x8000) == 0 || pOutParam == NULL || *pOutParam != 0)
        return 0;
    
    WCHAR* lastInsert = GetLastInsertString(pRecord);
    if (lastInsert == NULL)
        return 0;
    
    unsigned long long xmlLen = 0;
    unsigned int strLenRes = StringLengthWorkerW(lastInsert, 5112, (size_t*)&xmlLen);
    if (strLenRes != 0)
        return 0;
    
    lastInsert[xmlLen] = L'\0';
    long parseRes = InitXmlParser(lastInsert, xmlLen);
    
    WCHAR* xmlMessage = NULL;
    unsigned long long messageLen = 0;
    WCHAR* formattedXml = NULL;
    int hasFormattedXml = 0;
    unsigned long long formattedLen = 0;
    int finalResult = 0;
    
    if (parseRes < 0)
    {
        // Fallback: use default message
        xmlLen = 500;
        xmlMessage = (WCHAR*)operator new(xmlLen * sizeof(WCHAR));
        if (xmlMessage != NULL)
        {
            unsigned long long dummyLen = 0;
            StringCopyWorkerW(xmlMessage, 500, (size_t*)&dummyLen, (STRSAFE_PCNZWCH)g_szFallBackMessage, 0x7FFFFFFE);
            messageLen = wcslen((const wchar_t*)g_szFallBackMessage);
        }
    }
    else
    {
        xmlMessage = (WCHAR*)operator new(xmlLen * sizeof(WCHAR));
        if (xmlMessage == NULL)
            return 0;
        
        messageLen = xmlLen;
        finalResult = GetMessageFromXML(&xmlMessage, &messageLen);
        
        if (finalResult != 0)
        {
            int formatRes = GetFormattedXML(&formattedXml);
            if (formatRes > 0)
            {
                hasFormattedXml = 1;
                formattedLen = formatRes;
                lastInsert = formattedXml;
            }
        }
    }
    
    if (finalResult == 0)
    {
        if (xmlMessage)
            operator delete[](xmlMessage);
        if (formattedXml)
            operator delete[](formattedXml);
        return 0;
    }
    
    // Build final string: xmlMessage + header + lastInsert
    unsigned long long headerLen = wcslen((const wchar_t*)g_szXmlHeader);
    if (0x7FFFFFFE - headerLen - messageLen < formattedLen + 1)
    {
        finalResult = 0;
        goto cleanup;
    }
    
    unsigned long long totalLen = formattedLen + messageLen + 1 + headerLen;
    WCHAR* finalBuffer = (WCHAR*)LocalAlloc(LMEM_FIXED, totalLen * sizeof(WCHAR));
    if (finalBuffer == NULL)
    {
        finalResult = 0;
        goto cleanup;
    }
    
    *pOutParam = (long long)finalBuffer;
    finalBuffer[0] = L'\0';
    
    if (xmlMessage != NULL && totalLen > 0)
    {
        unsigned long long dummyLen = 0;
        StringCopyWorkerW(finalBuffer, totalLen, (size_t*)&dummyLen, xmlMessage, 0x7FFFFFFE);
    }
    
    // Append header
    unsigned long long currentLen = wcslen(finalBuffer);
    if (currentLen < totalLen)
    {
        unsigned long long dummyLen = 0;
        StringCopyWorkerW(finalBuffer + currentLen, totalLen - currentLen, (size_t*)&dummyLen, (STRSAFE_PCNZWCH)g_szXmlHeader, headerLen);
    }
    
    // Append lastInsert if present
    currentLen = wcslen(finalBuffer);
    if (lastInsert != NULL && formattedLen > 0 && currentLen < totalLen)
    {
        unsigned long long dummyLen = 0;
        StringCopyWorkerW(finalBuffer + currentLen, totalLen - currentLen, (size_t*)&dummyLen, lastInsert, formattedLen);
    }
    
cleanup:
    if (xmlMessage)
        operator delete[](xmlMessage);
    if (formattedXml && hasFormattedXml)
        operator delete[](formattedXml);
    
    return finalResult;
}

// GetFormattedXML
int GetFormattedXML(WCHAR** ppOutXml)
{
    int result = 0;
    
    if (g_pIWriter == NULL)
    {
        if (g_pISaxReader == NULL)
        {
            HRESULT hr = CoCreateInstance(CLSID_SAXXMLReader60, NULL, CLSCTX_INPROC_SERVER, IID_ISAXXMLReader, (void**)&g_pISaxReader);
            if (FAILED(hr))
                return 0;
        }
        
        HRESULT hr = CoCreateInstance(CLSID_MXXMLWriter60, NULL, CLSCTX_INPROC_SERVER, IID_IMXWriter, (void**)&g_pIWriter);
        if (SUCCEEDED(hr))
        {
            g_pIWriter->put_omitXMLDeclaration(VARIANT_TRUE);
            g_pIWriter->put_indent(VARIANT_TRUE);
        }
        
        if (FAILED(hr))
            return 0;
    }
    
    if (g_pIDoc != NULL)
    {
        IUnknown* pContentHandler = NULL;
        VARIANT varInput;
        VariantInit(&varInput);
        BSTR bstrOutput = NULL;
        
        HRESULT hr = g_pIWriter->QueryInterface(IID_ISAXContentHandler, (void**)&pContentHandler);
        if (SUCCEEDED(hr))
        {
            hr = g_pISaxReader->putContentHandler((ISAXContentHandler*)pContentHandler);
            
            varInput.vt = VT_UNKNOWN;
            hr = g_pIDoc->QueryInterface(IID_IUnknown, (void**)&varInput.punkVal);
            if (SUCCEEDED(hr) && varInput.punkVal != NULL)
            {
                hr = g_pISaxReader->parse(varInput);
            }
            
            VariantClear(&varInput);
            
            if (SUCCEEDED(hr))
            {
                g_pIWriter->flush();
                hr = g_pIWriter->get_output(&varInput);
                if (SUCCEEDED(hr) && varInput.vt == VT_BSTR)
                {
                    bstrOutput = varInput.bstrVal;
                    int outLen = SysStringLen(bstrOutput);
                    WCHAR* pCopy = (WCHAR*)operator new((outLen + 1) * sizeof(WCHAR));
                    if (pCopy != NULL)
                    {
                        memcpy(pCopy, bstrOutput, outLen * sizeof(WCHAR));
                        pCopy[outLen] = L'\0';
                        *ppOutXml = pCopy;
                        result = outLen + 1;
                    }
                }
                VariantClear(&varInput);
            }
        }
        
        if (g_pISaxReader != NULL)
            g_pISaxReader->putContentHandler(NULL);
        
        if (pContentHandler != NULL)
            pContentHandler->Release();
        
        if (g_pIWriter != NULL)
        {
            g_pIWriter->Release();
            g_pIWriter = NULL;
        }
    }
    
    return result;
}

