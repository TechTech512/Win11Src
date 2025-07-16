// streamreader.cpp

#pragma once

#include <windows.h>
#include <string>
#include "unbclbase.h"
#include "encoding.h"
#include "stream.h"

class StreamReader : public UnBCL::Object, public UnBCL::IDisposable, public UnBCL::TextReader {
private:
    UnBCL::Stream* m_Stream;
    UnBCL::Encoding* m_Encoding;
    UnBCL::Decoder* m_Decoder;

public:
    StreamReader(UnBCL::Stream* stream, UnBCL::Encoding* encoding)
        : m_Stream(stream), m_Encoding(encoding), m_Decoder(nullptr) {
        if (m_Encoding) {
            m_Decoder = m_Encoding->GetDecoder();
        }
    }

    ~StreamReader() {
        Close();
        delete m_Decoder;
        m_Decoder = nullptr;
    }

    void Close() {
        if (m_Stream) {
            m_Stream->Close();
            m_Stream = nullptr;
        }
    }

    void Dispose() override {
        Close();
        delete this;
    }
};
