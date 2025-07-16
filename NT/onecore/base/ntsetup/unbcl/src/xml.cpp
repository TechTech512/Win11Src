// xml.cpp

#include <windows.h>
#include <msxml6.h>
#include <stdexcept>

#pragma comment(lib, "msxml6.lib")

namespace UnBCL {

class Object {
public:
    virtual ~Object() {}
};

// === XmlNode ===

class XmlNode : public Object {
protected:
    IXMLDOMNode* m_XMLDOMNode;

public:
    XmlNode() : m_XMLDOMNode(nullptr) {}

    XmlNode(IXMLDOMNode* node) : m_XMLDOMNode(node) {
        if (m_XMLDOMNode)
            m_XMLDOMNode->AddRef();
    }

    virtual ~XmlNode() {
        if (m_XMLDOMNode) {
            m_XMLDOMNode->Release();
            m_XMLDOMNode = nullptr;
        }
    }
};

// === XmlAttribute ===

class XmlAttribute : public XmlNode {
public:
    XmlAttribute() = default;

    XmlAttribute(IXMLDOMNode* node)
        : XmlNode(node) {}
};

// === XmlAttributeCollection ===

class XmlAttributeCollection : public Object {
private:
    IXMLDOMNamedNodeMap* m_Map;

public:
    XmlAttributeCollection() : m_Map(nullptr) {}

    XmlAttributeCollection(IXMLDOMNamedNodeMap* map)
        : m_Map(map) {
        if (m_Map)
            m_Map->AddRef();
    }

    ~XmlAttributeCollection() {
        if (m_Map) {
            m_Map->Release();
            m_Map = nullptr;
        }
    }
};

// === XmlDocument ===

class XmlDocument : public XmlNode {
protected:
    IXMLDOMDocument3* m_Document;
    IXMLDOMSchemaCollection2* m_Schema;

public:
    XmlDocument()
        : XmlNode(), m_Document(nullptr), m_Schema(nullptr) {
        HRESULT hr = CoCreateInstance(CLSID_DOMDocument60, nullptr, CLSCTX_INPROC_SERVER,
                                      IID_PPV_ARGS(&m_Document));
        if (FAILED(hr) || !m_Document) {
            throw std::runtime_error("XmlDocument: Failed to create DOMDocument60");
        }

        // Cast to IXMLDOMNode for base XmlNode class
        IXMLDOMNode* base = nullptr;
        hr = m_Document->QueryInterface(IID_IXMLDOMNode, (void**)&base);
        if (FAILED(hr)) {
            m_Document->Release();
            throw std::runtime_error("XmlDocument: Cannot QI for IXMLDOMNode");
        }
        m_XMLDOMNode = base; // Already AddRef'd
    }

    XmlDocument(IXMLDOMDocument3* existing)
        : m_Document(existing), m_Schema(nullptr) {
        if (!existing)
            throw std::invalid_argument("XmlDocument: null IXMLDOMDocument3");

        m_Document->AddRef();
        IXMLDOMNode* base = nullptr;
        HRESULT hr = m_Document->QueryInterface(IID_IXMLDOMNode, (void**)&base);
        if (FAILED(hr)) {
            m_Document->Release();
            throw std::runtime_error("XmlDocument: Cannot QI for IXMLDOMNode");
        }
        m_XMLDOMNode = base;
    }

    ~XmlDocument() override {
        if (m_Schema) m_Schema->Release();
        if (m_Document) m_Document->Release();
    }
};

// === XmlNodeList ===

class XmlNodeList : public Object {
private:
    IXMLDOMNodeList* m_List;

public:
    XmlNodeList() : m_List(nullptr) {}

    XmlNodeList(IXMLDOMNodeList* list) : m_List(list) {
        if (m_List)
            m_List->AddRef();
    }

    ~XmlNodeList() {
        if (m_List) {
            m_List->Release();
            m_List = nullptr;
        }
    }
};

// === XmlSchemaSet ===

class XmlSchemaSet : public Object {
private:
    IXMLDOMSchemaCollection2* m_Schema;

public:
    XmlSchemaSet()
        : m_Schema(nullptr) {
        HRESULT hr = CoCreateInstance(__uuidof(XMLSchemaCache60), nullptr, CLSCTX_INPROC_SERVER,
                                      IID_PPV_ARGS(&m_Schema));
        if (FAILED(hr) || !m_Schema) {
            throw std::runtime_error("XmlSchemaSet: Failed to create XMLSchemaCache60");
        }
    }

    ~XmlSchemaSet() {
        if (m_Schema) {
            m_Schema->Release();
            m_Schema = nullptr;
        }
    }
};

// === XmlNamespaceManager ===

class XmlNamespaceManager : public Object {
public:
    // Placeholder: should use Hashtable<String,String>* m_NamespaceList
    XmlNamespaceManager() {
        // Simulate: m_NamespaceList = new Hashtable<String, String>();
    }

    virtual ~XmlNamespaceManager() {
        // cleanup
    }
};

} // namespace UnBCL

