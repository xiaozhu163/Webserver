#ifndef FIXEDBUFFER_H
#define FIXEDBUFFER_H

#include <cstring>
#include <string>

template <int SIZE>
class FixedBuffer {
public:
    FixedBuffer() : cur_(data_) {}

    void append(const char* buf, size_t len) {
        if (avail() > len) {
            memcpy(cur_, buf, len);
            cur_ += len;
        }
    }

    const char* data() const { return data_; }
    int length() const { return static_cast<int>(cur_ - data_); }

    char* current() { return cur_; }
    int avail() const { return static_cast<int>(end() - cur_); }
    void add(size_t len) { cur_ += len; }

    void reset() { cur_ = data_; }
    void bzero() { memset(data_, 0, sizeof(data_)); }

private:
    const char* end() const { return data_ + sizeof(data_); }

    char data_[SIZE];
    char* cur_;
};

#endif // FIXEDBUFFER_H
