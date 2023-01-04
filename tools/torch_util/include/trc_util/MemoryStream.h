#pragma once

#include <istream>
#include <ostream>

namespace trc::util
{
    namespace internal
    {
        class memory_streambuf : public std::streambuf
        {
        public:
            memory_streambuf(char* data, size_t size);

            auto underflow() -> int_type override;
            auto overflow(int_type c) -> int_type override;

            auto seekpos(pos_type pos, std::ios_base::openmode which) -> pos_type override;

        private:
            char* begin;
            char* end;
        };

        struct memory_stream_base
        {
            memory_stream_base(char* data, size_t size) : sbuf(data, size) {}
            memory_streambuf sbuf;
        };
    } // namespace internal

    /**
     * @brief A std::istream and std::ostream interface to a chunk of memory
     *
     * Allows absolute positioning via `seekp` and `seekg`.
     */
    class MemoryStream : private virtual internal::memory_stream_base
                       , public std::istream
                       , public std::ostream
    {
    public:
        MemoryStream(char* data, size_t size)
            :
            memory_stream_base(data, size),
            std::ios(&this->sbuf),
            std::istream(&this->sbuf),
            std::ostream(&this->sbuf)
        {
        }
    };
} // namespace trc::util
