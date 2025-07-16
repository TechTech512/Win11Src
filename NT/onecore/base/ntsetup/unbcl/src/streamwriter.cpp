// streamwriter.cpp

#pragma once

#include <windows.h>
#include <string>
#include "unbclbase.h"
#include "encoding.h"
#include "stream.h"

class StreamWriter : public UnBCL::Object, public UnBCL::IDisposable, public UnBCL::TextWriter {
private:
    UnBCL::Stream* m_Stream;
    UnBCL::Encoding* m_Encoding;
    bool m_FreeStream;
    bool m_FreeEncoding;

public:
    StreamWriter(UnBCL::Stream* stream, UnBCL::Encoding* encoding, bool freeStream, bool freeEncoding)
        : m_Stream(stream), m_Encoding(encoding),
          m_FreeStream(freeStream), m_FreeEncoding(freeEncoding) {}

    ~StreamWriter() {
        Close();
        if (m_FreeEncoding) {
            delete m_Encoding;
            m_Encoding = nullptr;
        }
    }

    void Close() {
        if (m_Stream) {
            m_Stream->Close();
            if (m_FreeStream) {
                delete m_Stream;
            }
            m_Stream = nullptr;
        }
    }

    void Dispose() override {
        Close();
        delete this;
    }
};

