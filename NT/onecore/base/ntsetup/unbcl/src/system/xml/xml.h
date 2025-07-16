#ifndef UNBCL_XML_H
#define UNBCL_XML_H

#include <windows.h>

// Forward declarations for your XML stream abstractions
class XmlReader;
class XmlWriter;

class XmlSerializable
{
public:
    virtual ~XmlSerializable() {}

    // Serialize the object into XML
    virtual void WriteXml(XmlWriter* writer) = 0;

    // Deserialize the object from XML
    virtual void ReadXml(XmlReader* reader) = 0;
};

#endif // UNBCL_XML_H

