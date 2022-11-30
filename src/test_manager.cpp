#include "synthium.h"

#include <glob/glob.h>
#include <fstream>

int main() {
    synthium::Manager manager(
        glob::glob(
            "C:/Users/Public/Daybreak Game Company/Installed Games/Planetside 2 Test/Resources/Assets/assets_x64_*.pack2"
        )
    );

    std::string filename = "Common_HeavyFighter_Weapon_MachineGun_001.adr";

    std::vector<uint8_t> data = manager.get(filename).get_data();
    std::ofstream output(filename, std::ios::binary);
    output.write((char*)data.data(), data.size());
    output.close();
    return 0;
}