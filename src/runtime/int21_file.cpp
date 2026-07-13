#include "dosrecomp/runtime/int21_file.hpp"

namespace dosrecomp::runtime {
std::expected<int21_file_result, int21_file_error>
int21_file_dispatcher::dispatch(const int21_file_request& request, virtual_file_system& files) {
    if (request.ah == 0x3c) {
        const auto handle = files.open_write(request.path);
        if (!handle) return std::unexpected(int21_file_error{handle.error().message});
        return int21_file_result{.handle = *handle, .data = {}, .bytes_transferred = 0, .position = std::nullopt};
    }
    if (request.ah == 0x3d) {
        const auto handle = files.open_read(request.path);
        if (!handle) return std::unexpected(int21_file_error{handle.error().message});
        return int21_file_result{.handle = *handle, .data = {}, .bytes_transferred = 0, .position = std::nullopt};
    }
    if (request.ah == 0x3f) {
        const auto data = files.read(request.handle, request.count);
        if (!data) return std::unexpected(int21_file_error{data.error().message});
        return int21_file_result{.handle = std::nullopt, .data = *data, .bytes_transferred = data->size(), .position = std::nullopt};
    }
    if (request.ah == 0x40) {
        const auto written = files.write(request.handle, request.data);
        if (!written) return std::unexpected(int21_file_error{written.error().message});
        return int21_file_result{.handle = std::nullopt, .data = {}, .bytes_transferred = *written, .position = std::nullopt};
    }
    if (request.ah == 0x42) {
        if (request.origin > 2) return std::unexpected(int21_file_error{"INT 21h seek origin is invalid"});
        const auto position = files.seek(request.handle, request.offset, static_cast<seek_origin>(request.origin));
        if (!position) return std::unexpected(int21_file_error{position.error().message});
        return int21_file_result{.handle = std::nullopt, .data = {}, .bytes_transferred = 0, .position = *position};
    }
    if (request.ah == 0x3e) {
        const auto closed = files.close(request.handle);
        if (!closed) return std::unexpected(int21_file_error{closed.error().message});
        return int21_file_result{.handle = std::nullopt, .data = {}, .bytes_transferred = 0, .position = std::nullopt};
    }
    return std::unexpected(int21_file_error{"unsupported INT 21h file function"});
}

} // namespace dosrecomp::runtime
