#include "dosrecomp/runtime/int16.hpp"

#include <cstdlib>
#include <iostream>

int main() {
    std::deque<dosrecomp::runtime::keyboard_key> queue{{.ascii = 'A', .scan_code = 0x1e}};
    const auto peek = dosrecomp::runtime::int16_dispatcher::dispatch({.ah = 0x01}, queue);
    const auto size_after_peek = queue.size();
    const auto read = dosrecomp::runtime::int16_dispatcher::dispatch({.ah = 0x00}, queue);
    const auto empty = dosrecomp::runtime::int16_dispatcher::dispatch({.ah = 0x00}, queue);
    if (!peek || !peek->available || size_after_peek != 1 || !read || !read->key || read->key->ascii != 'A' || empty) {
        std::cerr << "failed INT 16h keyboard dispatch\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

