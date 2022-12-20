#pragma once

#include "version.h"
#include "loader.h"
#include "manager.h"
#include "utils.h"

namespace synthium {
    std::string version() {
        return SYNTHIUM_VERSION;
    }
}