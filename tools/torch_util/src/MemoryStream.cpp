#include "trc_util/MemoryStream.h"

#include <cassert>



namespace trc::util::internal
{
    memory_streambuf::memory_streambuf(char* data, size_t size)
        : begin(data), end(data + size)
    {
        setg(begin, begin, end);
        setp(begin, end);
    }

    auto memory_streambuf::underflow() -> int_type
    {
        return this->gptr() == this->egptr()
            ? traits_type::eof()
            : traits_type::to_int_type(*this->gptr());
    }

    auto memory_streambuf::overflow(int_type c) -> int_type
    {
        return std::streambuf::overflow(c);
    }

    auto memory_streambuf::seekpos(pos_type pos, std::ios_base::openmode which) -> pos_type
    {
        assert(begin + pos < end);
        if (which == std::ios_base::in) {
            setg(begin + pos, begin + pos, end);
        }
        if (which == std::ios_base::out) {
            setp(begin + pos, end);
        }

        return pos;
    }
} // namespace trc::util::internal
