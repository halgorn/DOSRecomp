#include "dosrecomp/runtime/int21_file.hpp"

namespace dosrecomp::runtime {
std::expected<int21_file_result, int21_file_error>
int21_file_dispatcher::dispatch(const int21_file_request& request, virtual_file_system& files) {
    if (request.ah == 0x3c) {
        const auto handle = files.open_write(request.path);
        if (!handle) return std::unexpected(int21_file_error{handle.error().message});
        return int21_file_result{.handle = *handle, .data = {}, .bytes_transferred = 0};
    }
    if (request.ah == 0x3d) {
        const auto handle = files.open_read(request.path);
        if (!handle) return std::unexpected(int21_file_error{handle.error().message});
        return int21_file_result{.handle = *handle, .data = {}, .bytes_transferred = 0};
    }
    if (request.ah == 0x3f) {
        const auto data = files.read(request.handle, request.count);
        if (!data) return std::unexpected(int21_file_error{data.error().message});
        return int21_file_result{.handle = std::nullopt, .data = *data, .bytes_transferred = data->size()};
    }
    if (request.ah == 0x40) {
        const auto written = files.write(request.handle, request.data);
        if (!written) return std::unexpected(int21_file_error{written.error().message});
        return int21_file_result{.handle = std::nullopt, .data = {}, .bytes_transferred = *written};
    }
    if (request.ah == 0x3e) {
        const auto closed = files.close(request.handle);
        if (!closed) return std::unexpected(int21_file_error{closed.error().message});
        return int21_file_result{.handle = std::nullopt, .data = {}, .bytes_transferred = 0};
    }
    return std::unexpected(int21_file_error{"unsupported INT 21h file function"});
}

} // namespace dosrecomp::runtime
