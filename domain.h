#pragma once

#include <string>
#include <vector>
#include "geo.h"

namespace domain {

    struct Stop {
        std::string name;
        geo::Coordinates coordinates;
    };

    struct Bus {
        std::string name;
        std::vector<const Stop*> stops;
        bool is_roundtrip;
    };

} // namespace domain