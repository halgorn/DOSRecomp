#include "dosrecomp/loader/binary_loader.hpp"

#include <fstream>
#include <iterator>
#include <limits>

namespace dosrecomp::loader {
namespace {

[[nodiscard]] std::uint16_t read_u16(const std::vector<std::byte>& bytes, std::size_t offset) {
    return static_cast<std::uint16_t>(std::to_integer<unsigned char>(bytes[offset])) |
           static_cast<std::uint16_t>(std::to_integer<unsigned char>(bytes[offset + 1])) << 8U;
}

void write_u16(std::vector<std::byte>& bytes, std::size_t offset, std::uint16_t value) {
    bytes[offset] = static_cast<std::byte>(value & 0xffU);
    bytes[offset + 1] = static_cast<std::byte>(value >> 8U);
}

[[nodiscard]] std::expected<program_image, load_error>
parse_mz(const std::vector<std::byte>& file) {
    constexpr std::size_t minimum_header_size = 28;
    if (file.size() < minimum_header_size) {
        return std::unexpected(load_error{"MZ header is truncated"});
    }

    const auto bytes_in_last_page = read_u16(file, 2);
    const auto page_count = read_u16(file, 4);
    const auto relocation_count = read_u16(file, 6);
    const auto header_paragraphs = read_u16(file, 8);
    const auto initial_ss = read_u16(file, 14);
    const auto initial_sp = read_u16(file, 16);
    const auto initial_ip = read_u16(file, 20);
    const auto initial_cs = read_u16(file, 22);
    const auto relocation_offset = read_u16(file, 24);

    if (bytes_in_last_page > 512 || page_count == 0 ||
        (page_count == 1 && bytes_in_last_page == 0)) {
        return std::unexpected(load_error{"MZ file size fields are invalid"});
    }
    const std::size_t declared_size =
        static_cast<std::size_t>(page_count - 1) * 512U + (bytes_in_last_page == 0 ? 512U : bytes_in_last_page);
    const std::size_t header_size = static_cast<std::size_t>(header_paragraphs) * 16U;
    if (declared_size > file.size() || header_size < minimum_header_size || header_size > declared_size) {
        return std::unexpected(load_error{"MZ header or declared file size is invalid"});
    }
    if (relocation_offset > header_size || relocation_count > (header_size - relocation_offset) / 4U) {
        return std::unexpected(load_error{"MZ relocation table exceeds header bounds"});
    }

    program_image image{
        .format = executable_format::mz,
        .bytes = {file.begin() + static_cast<std::ptrdiff_t>(header_size),
                  file.begin() + static_cast<std::ptrdiff_t>(declared_size)},
        .entry_point = {initial_cs, initial_ip},
        .initial_stack = {initial_ss, initial_sp},
        .relocations = {},
    };
    for (std::size_t index = 0; index < relocation_count; ++index) {
        const std::size_t entry = static_cast<std::size_t>(relocation_offset) + index * 4U;
        const segmented_address address{read_u16(file, entry + 2), read_u16(file, entry)};
        const std::size_t linear_offset = static_cast<std::size_t>(address.segment) * 16U + address.offset;
        if (linear_offset > image.bytes.size() || image.bytes.size() - linear_offset < 2U) {
            return std::unexpected(load_error{"MZ relocation points outside the load module"});
        }
        image.relocations.push_back(relocation{address});
    }
    if (initial_ip >= image.bytes.size()) {
        return std::unexpected(load_error{"MZ entry point points outside the load module"});
    }
    return image;
}

} // namespace

std::expected<program_image, load_error> binary_loader::load_file(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return std::unexpected(load_error{"cannot open '" + path.string() + "'"});
    }
    std::vector<char> raw((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    if (input.bad()) {
        return std::unexpected(load_error{"cannot read '" + path.string() + "'"});
    }
    std::vector<std::byte> bytes;
    bytes.reserve(raw.size());
    for (const char value : raw) {
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(value)));
    }
    return load_bytes(bytes);
}

std::expected<program_image, load_error>
binary_loader::load_bytes(const std::vector<std::byte>& file) {
    if (file.empty()) {
        return std::unexpected(load_error{"input is empty"});
    }
    if (file.size() >= 2 && std::to_integer<unsigned char>(file[0]) == 'M' &&
        std::to_integer<unsigned char>(file[1]) == 'Z') {
        return parse_mz(file);
    }
    if (file.size() > std::numeric_limits<std::uint16_t>::max()) {
        return std::unexpected(load_error{"COM image exceeds the 64 KiB real-mode segment limit"});
    }
    return program_image{
        .format = executable_format::com,
        .bytes = file,
        .entry_point = {0, 0x100},
        .initial_stack = {0, 0xfffe},
        .relocations = {},
    };
}

std::expected<program_image, load_error>
binary_loader::apply_relocations(const program_image& image, std::uint16_t load_segment) {
    if (image.format != executable_format::mz) {
        return std::unexpected(load_error{"COM images do not contain MZ relocations"});
    }
    auto relocated = image;
    for (const auto& entry : relocated.relocations) {
        const auto offset = static_cast<std::size_t>(entry.address.segment) * 16U + entry.address.offset;
        if (offset > relocated.bytes.size() || relocated.bytes.size() - offset < 2U) {
            return std::unexpected(load_error{"MZ relocation points outside the load module"});
        }
        const auto value = static_cast<std::uint16_t>(read_u16(relocated.bytes, offset) + load_segment);
        write_u16(relocated.bytes, offset, value);
    }
    return relocated;
}

} // namespace dosrecomp::loader
