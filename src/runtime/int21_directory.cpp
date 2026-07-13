#include "dosrecomp/runtime/int21_directory.hpp"

namespace dosrecomp::runtime {
std::expected<int21_directory_result, int21_directory_error>
int21_directory_dispatcher::dispatch(const int21_directory_request& request, current_directory& directory) {
    if (request.ah == 0x3b) {
        const auto changed = directory.change_to(request.path);
        if (!changed) return std::unexpected(int21_directory_error{changed.error().message});
        return int21_directory_result{directory.path()};
    }
    if (request.ah == 0x47) return int21_directory_result{directory.path()};
    return std::unexpected(int21_directory_error{"unsupported INT 21h directory function"});
}

} // namespace dosrecomp::runtime

