#include "dosrecomp/runtime/pc_speaker.hpp"

#include <cstdlib>
#include <iostream>

int main() {
    dosrecomp::runtime::pc_speaker speaker;
    const auto tone = speaker.start(440);
    const auto invalid = speaker.start(0);
    speaker.stop();
    if (!tone || invalid || speaker.active() || speaker.frequency_hz() != 0) {
        std::cerr << "failed PC Speaker state\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

