#include <string>
#include <vector>

namespace Arkitekt {
    void Init();
}

extern _NODISCARD_LOCK std::pair<std::unique_lock<std::shared_mutex>&&, std::ofstream&> infoLog(const std::string_view& name="info.log", int32_t mode = std::ios_base::out);
extern _NODISCARD_LOCK std::pair<std::unique_lock<std::shared_mutex>&&, std::ofstream&>  errorLog(const std::string_view& name="error.log", int32_t mode = std::ios_base::out);

extern std::shared_mutex& ArkitektHookMutex();
