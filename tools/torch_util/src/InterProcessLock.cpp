#include "trc_util/InterProcessLock.h"

#include <cerrno>
#include <cstring>
#include <stdexcept>

#if defined (unix) || defined (__unix) || defined (__unix__) || defined (__APPLE__)
    #include <fcntl.h>
    #include <sys/stat.h>
#else // if windows
    #include <fcntl.h>
    #include <sys/stat.h>
    // Apparently, the following can actually happen (it happened on one of my machines):
    // Use values from https://en.wikibooks.org/wiki/C_Programming/POSIX_Reference/sys/stat.h
    #if !defined (S_IRUSR)
        #define S_IRUSR 0x00400
    #endif
    #if !defined (S_IWUSR)
        #define S_IWUSR 0x00200
    #endif
#endif // if windows

#define NAME_MAX 255



namespace trc::util
{

InterProcessLock::InterProcessLock(const char* name)
{
    if (strlen(name) > NAME_MAX - 4)
    {
        throw std::invalid_argument("A semaphore name must not be longer than "
                                    + std::to_string(NAME_MAX - 4) + " characters!");
    }
    if (strlen(name) == 0 || name[0] != '/') {
        throw std::invalid_argument("A semaphore name must begin with a slash ('/')!");
    }

    semaphore = sem_open(name, O_CREAT, S_IRUSR | S_IWUSR, 1);
    if (semaphore == SEM_FAILED)
    {
        throw std::runtime_error("Semaphore creation failed with error: "
                                 + std::string(std::strerror(errno)));
    }
}

InterProcessLock::~InterProcessLock() noexcept
{
    sem_close(semaphore);
}

void InterProcessLock::lock()
{
    sem_wait(semaphore);
}

void InterProcessLock::unlock()
{
    sem_post(semaphore);
}

} // namespace trc::util
