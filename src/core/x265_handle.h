#pragma once
#include <memory>
#include <cstdint>

#ifndef X265_H
struct x265_param;
struct x265_picture;
struct x265_encoder;
struct x265_nal;
struct x265_api;
#endif

namespace sk265::core {

struct ParamDeleter {
    const x265_api* api{nullptr};
    void operator()(x265_param* p) const;
};

struct PictureDeleter {
    const x265_api* api{nullptr};
    void operator()(x265_picture* p) const;
};

struct EncoderDeleter {
    const x265_api* api{nullptr};
    void operator()(x265_encoder* e) const;
};

class ParamHandle {
public:
    ParamHandle(const x265_api* api, x265_param* p);
    x265_param* get() const noexcept { return ptr_.get(); }
    x265_param* operator->() const noexcept { return ptr_.get(); }
    x265_param* raw() const noexcept { return ptr_.get(); }
    explicit operator bool() const noexcept { return ptr_ != nullptr; }
    bool operator==(std::nullptr_t) const noexcept { return ptr_ == nullptr; }
    bool operator!=(std::nullptr_t) const noexcept { return ptr_ != nullptr; }
    const x265_api* api{nullptr};

private:
    std::unique_ptr<x265_param, ParamDeleter> ptr_;
};

class PictureHandle {
public:
    PictureHandle(const x265_api* api, x265_picture* p);
    x265_picture* get() const noexcept { return ptr_.get(); }
    x265_picture* operator->() const noexcept { return ptr_.get(); }
    x265_picture* raw() const noexcept { return ptr_.get(); }
    explicit operator bool() const noexcept { return ptr_ != nullptr; }
    bool operator==(std::nullptr_t) const noexcept { return ptr_ == nullptr; }
    bool operator!=(std::nullptr_t) const noexcept { return ptr_ != nullptr; }
    const x265_api* api{nullptr};

private:
    std::unique_ptr<x265_picture, PictureDeleter> ptr_;
};

class EncoderHandle {
public:
    EncoderHandle(const x265_api* api, x265_encoder* e);
    x265_encoder* get() const noexcept { return ptr_.get(); }
    x265_encoder* operator->() const noexcept { return ptr_.get(); }
    x265_encoder* raw() const noexcept { return ptr_.get(); }
    explicit operator bool() const noexcept { return ptr_ != nullptr; }
    bool operator==(std::nullptr_t) const noexcept { return ptr_ == nullptr; }
    bool operator!=(std::nullptr_t) const noexcept { return ptr_ != nullptr; }
    const x265_api* api{nullptr};

private:
    std::unique_ptr<x265_encoder, EncoderDeleter> ptr_;
};

ParamHandle make_param_handle(const x265_api* api);
PictureHandle make_picture_handle(const x265_api* api);
EncoderHandle make_encoder_handle(const x265_api* api, x265_param* param);

} // namespace sk265::core
