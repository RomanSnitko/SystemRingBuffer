#pragma once

#include <cstdint>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <stdexcept>
#include <system_error>
#include <span>
#include <algorithm>

template<typename T>
concept RingBufferElement = std::is_trivially_copyable_v<T>;

template<RingBufferElement T>
class RingBuffer
{
private:
    size_t m_size_bytes{};
    size_t m_capacity{};
    T* m_buffer{};
    
    size_t m_head{};
    size_t m_tail{};

    static size_t get_page_size() 
    {
        static const size_t ps = [](){
            long res = sysconf(_SC_PAGESIZE);

            if (res <= 0) 
            {
                throw std::system_error(errno, std::generic_category(), "failed to get page size");
            }

            return static_cast<size_t>(res);
        }();

        return ps;
    }

    //rounding to page size
    static size_t round_to_page(size_t size)
    {
        const size_t ps = get_page_size();

        if (size == 0) 
        {
            return ps;
        }

        return ((size + ps - 1) / ps) * ps;
    }

public:
    explicit RingBuffer(size_t requested_capacity) 
    {
        if (requested_capacity == 0) 
        {
            throw std::invalid_argument("capacity must be greater than zero");
        }

        m_size_bytes = round_to_page(requested_capacity * sizeof(T));
        m_capacity = m_size_bytes / sizeof(T);

        //create file in RAM
        int fd = memfd_create("ring_buffer", MFD_CLOEXEC);
        if (fd == -1) 
        {
            throw std::system_error(errno, std::system_category(), "memfd_create failed");
        }

        //expension to our size
        if (ftruncate(fd, static_cast<off_t>(m_size_bytes)) == -1)
        {
            close(fd);
            throw std::system_error(errno, std::system_category(), "ftruncate failed");
        }

        //reserve null memory in address space VM
        void* reserve_ptr = mmap(nullptr, 2 * m_size_bytes, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (reserve_ptr == MAP_FAILED) 
        {
            close(fd);
            throw std::system_error(errno, std::system_category(), "mmap reservation failed");
        }

        //pointer of the start VM
        m_buffer = static_cast<T*>(reserve_ptr);

        //mapping in the first half in VM
        if (mmap(m_buffer, m_size_bytes, 
            PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, 0) == MAP_FAILED) 
        {
            munmap(m_buffer, 2 * m_size_bytes);
            close(fd);
            throw std::system_error(errno, std::system_category(), "mmap first half failed");
        }

        //mapping in the second half in Vm
        if (mmap(static_cast<uint8_t*>(reserve_ptr) + m_size_bytes, m_size_bytes, 
                 PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, 0) == MAP_FAILED) 
        {
            munmap(m_buffer, 2 * m_size_bytes);
            close(fd);
            throw std::system_error(errno, std::system_category(), "mmap second half failed");
        }

        close(fd);
    }

    RingBuffer(const RingBuffer&) = delete;
    RingBuffer& operator=(const RingBuffer&) = delete;

    RingBuffer(RingBuffer&& other) noexcept 
        : m_size_bytes(other.m_size_bytes), m_capacity(other.m_capacity), 
          m_buffer(other.m_buffer), m_head(other.m_head), m_tail(other.m_tail) 
    {
        other.m_buffer = nullptr;
    }

    ~RingBuffer() 
    {
        if (m_buffer)
        {
            munmap(m_buffer, 2 * m_size_bytes);
        }
    }

    void write(std::span<const T> data)
    {
        T* target = m_buffer + (m_head % m_capacity);
        
        std::memcpy(target, data.data(), data.size_bytes());

        m_head += data.size();

        //rewriting
        if (m_head - m_tail > m_capacity) 
        {
            m_tail = m_head - m_capacity;
        }
    }

    size_t read(std::span<T> output)
    {
        size_t available = m_head - m_tail;
        size_t to_copy = std::min(available, output.size());

        T* source = m_buffer + (m_tail % m_capacity);
        
        std::memcpy(output.data(), source, to_copy * sizeof(T));

        m_tail += to_copy;
        return to_copy;
    }

    [[nodiscard]] size_t size() const noexcept { return m_head - m_tail; }
    [[nodiscard]] size_t capacity() const noexcept { return m_capacity; }
    [[nodiscard]] bool empty() const noexcept { return m_head == m_tail; }
    
    void clear() noexcept
    {
        m_head = 0;
        m_tail = 0;
    }
};