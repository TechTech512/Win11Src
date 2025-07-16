#ifndef UNBCL_STREAM_H
#define UNBCL_STREAM_H

namespace UnBCL {

class Stream {
public:
    virtual ~Stream() {}

    virtual void Close() {}
    virtual int Read(BYTE* buffer, int count) { return 0; }
    virtual int Write(const BYTE* buffer, int count) { return 0; }
};

} // namespace UnBCL

#endif // UNBCL_STREAM_H

