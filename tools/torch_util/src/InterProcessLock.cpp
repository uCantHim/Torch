#include "trc_util/InterProcessLock.h"

#include <cstring>
#include <stdexcept>

// The following is done by https://github.com/DanielTillett/Simple-Windows-Posix-Semaphore
// on windows
#if defined (unix) || defined (__unix) || defined (__unix__) || defined (__APPLE__)
#include <fcntl.h>
#include <sys/stat.h>
#endif

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
