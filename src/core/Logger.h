#pragma once

#include <spdlog/spdlog.h>
#include <memory>

namespace OpenSplat {
namespace Core {

class Logger {
public:
    static void Init();
    static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
    static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }

private:
    static std::shared_ptr<spdlog::logger> s_CoreLogger;
    static std::shared_ptr<spdlog::logger> s_ClientLogger;
};

} // namespace Core
} // namespace OpenSplat

// Logging macros for easier access
#define OPENSPLAT_CORE_TRACE(...)    ::OpenSplat::Core::Logger::GetCoreLogger()->trace(__VA_ARGS__)
#define OPENSPLAT_CORE_INFO(...)     ::OpenSplat::Core::Logger::GetCoreLogger()->info(__VA_ARGS__)
#define OPENSPLAT_CORE_WARN(...)     ::OpenSplat::Core::Logger::GetCoreLogger()->warn(__VA_ARGS__)
#define OPENSPLAT_CORE_ERROR(...)    ::OpenSplat::Core::Logger::GetCoreLogger()->error(__VA_ARGS__)
#define OPENSPLAT_CORE_FATAL(...)    ::OpenSplat::Core::Logger::GetCoreLogger()->critical(__VA_ARGS__)

#define OPENSPLAT_TRACE(...)         ::OpenSplat::Core::Logger::GetClientLogger()->trace(__VA_ARGS__)
#define OPENSPLAT_INFO(...)          ::OpenSplat::Core::Logger::GetClientLogger()->info(__VA_ARGS__)
#define OPENSPLAT_WARN(...)          ::OpenSplat::Core::Logger::GetClientLogger()->warn(__VA_ARGS__)
#define OPENSPLAT_ERROR(...)         ::OpenSplat::Core::Logger::GetClientLogger()->error(__VA_ARGS__)
#define OPENSPLAT_FATAL(...)         ::OpenSplat::Core::Logger::GetClientLogger()->critical(__VA_ARGS__)
