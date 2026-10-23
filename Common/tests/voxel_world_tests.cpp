#include "world/voxel_world.hpp"

#include <array>
#include <cassert>
#include <filesystem>
#include <string>
#include <vector>

int main() {
    World::VoxelWorld world;
    const int height = world.surfaceHeight(0, 0);
    assert(height > 0 && height < World::VoxelWorld::worldHeight);

    World::Position hit{};
    World::Position previous{};
    assert(world.raycast({0.5f, static_cast<float>(height + 4), 0.5f},
                         {0.0f, -1.0f, 0.0f}, 8.0f, hit, previous));
    assert(hit.x == 0 && hit.z == 0 && hit.y == height - 1);
    assert(previous.y == height);

    std::vector<const World::Face*> faces;
    world.appendVisibleFaces(0, 0, 1, faces);
    assert(!faces.empty());

    const World::Position edit{0, height + 1, 0};
    assert(world.setBlock(edit, World::Block::Stone));
    assert(world.blockAt(edit.x, edit.y, edit.z) == World::Block::Stone);

    const auto savePath = std::filesystem::temp_directory_path() / "mygame-voxel-test.sav";
    const std::array<std::uint32_t, 10> inventory{{1, 2, 3, 4, 5, 6, 7, 8, 9, 10}};
    std::string error;
    assert(world.save(savePath, inventory, error));

    World::VoxelWorld restored;
    std::array<std::uint32_t, 10> restoredInventory{};
    assert(restored.load(savePath, restoredInventory, error));
    assert(restored.blockAt(edit.x, edit.y, edit.z) == World::Block::Stone);
    assert(restoredInventory == inventory);

    std::error_code removeError;
    std::filesystem::remove(savePath, removeError);
    assert(!removeError);
}
