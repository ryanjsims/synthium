#pragma once

#include "version.h"
#include "loader.h"
#include "manager.h"

namespace synthium {
    std::string version() {
        return SYNTHIUM_VERSION;
    }
}