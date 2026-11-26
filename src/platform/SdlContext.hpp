#pragma once

namespace thj::platform {

class SdlContext final {
  public:
    SdlContext();
    ~SdlContext() noexcept;

    SdlContext(const SdlContext &) = delete;
    SdlContext &operator=(const SdlContext &) = delete;

    SdlContext(SdlContext &&) = delete;
    SdlContext &operator=(SdlContext &&) = delete;
};

} // namespace thj::platform
