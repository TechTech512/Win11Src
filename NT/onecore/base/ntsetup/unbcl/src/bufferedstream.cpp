// bufferedstream.cpp

#include <windows.h>

class Stream
{
public:
    virtual ~Stream() {}
    virtual int Read(BYTE* buffer, int count) = 0;
    virtual int Write(const BYTE* buffer, int count) = 0;
    virtual BOOL CanRead() const = 0;
    virtual BOOL CanWrite() const = 0;
    virtual void Close() = 0;
};

class BufferedStream : public Stream
{
private:
    Stream* m_InnerStream;
    BYTE* m_Buffer;
    int m_BufferSize;
    int m_Position;
    BOOL m_OwnsBuffer;

public:
    BufferedStream(Stream* stream, int bufferSize)
        : m_InnerStream(stream),
          m_Buffer(nullptr),
          m_BufferSize(0),
          m_Position(0),
          m_OwnsBuffer(FALSE)
    {
        if (stream && bufferSize > 0)
        {
            m_Buffer = new BYTE[bufferSize];
            if (m_Buffer)
            {
                m_BufferSize = bufferSize;
                m_Position = 0;
                m_OwnsBuffer = TRUE;
            }
        }
    }

    ~BufferedStream()
    {
        if (m_OwnsBuffer && m_Buffer)
        {
            delete[] m_Buffer;
            m_Buffer = nullptr;
        }

        if (m_InnerStream)
        {
            m_InnerStream->Close();
            delete m_InnerStream;
            m_InnerStream = nullptr;
        }
    }

    int Read(BYTE* buffer, int count) override
    {
        if (!m_InnerStream || !buffer || count <= 0)
            return 0;

        return m_InnerStream->Read(buffer, count);
    }

    int Write(const BYTE* buffer, int count) override
    {
        if (!m_InnerStream || !buffer || count <= 0)
            return 0;

        return m_InnerStream->Write(buffer, count);
    }

    BOOL CanRead() const override
    {
        return (m_InnerStream != nullptr) ? m_InnerStream->CanRead() : FALSE;
    }

    BOOL CanWrite() const override
    {
        return (m_InnerStream != nullptr) ? m_InnerStream->CanWrite() : FALSE;
    }

    void Close() override
    {
        if (m_InnerStream)
        {
            m_InnerStream->Close();
        }
    }

    // Optionally expose buffer size for diagnostics
    int BufferSize() const { return m_BufferSize; }
};
