#include "world/voxel_world.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <limits>
#include <utility>

namespace World {
namespace {

constexpr int seaLevel = 7;
constexpr std::array<Position, 6> neighborOffsets{{
    {0, 0, -1}, {0, 0, 1}, {0, -1, 0},
    {0, 1, 0}, {-1, 0, 0}, {1, 0, 0}
}};
constexpr std::array<std::array<std::array<float, 3>, 4>, 6> faceVertices{{
    {{{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}}},
    {{{0, 0, 1}, {0, 1, 1}, {1, 1, 1}, {1, 0, 1}}},
    {{{0, 0, 0}, {0, 0, 1}, {1, 0, 1}, {1, 0, 0}}},
    {{{0, 1, 0}, {1, 1, 0}, {1, 1, 1}, {0, 1, 1}}},
    {{{0, 0, 0}, {0, 1, 0}, {0, 1, 1}, {0, 0, 1}}},
    {{{1, 0, 0}, {1, 0, 1}, {1, 1, 1}, {1, 1, 0}}}
}};
constexpr std::array<float, 6> faceBrightness{{0.78f, 0.9f, 0.62f, 1.0f, 0.72f, 0.84f}};

std::uint32_t coordinateHash(int x, int z, std::uint32_t seed = 0x9e3779b9u) {
    std::uint32_t value = static_cast<std::uint32_t>(x) * 0x8da6b343u ^
                          static_cast<std::uint32_t>(z) * 0xd8163841u ^ seed;
    value ^= value >> 13;
    value *= 0x85ebca6bu;
    value ^= value >> 16;
    return value;
}

float lattice(int x, int z) {
    return static_cast<float>(coordinateHash(x, z) & 0xffffu) / 32767.5f - 1.0f;
}

float smoothNoise(float x, float z, float scale) {
    const float gridX = x / scale;
    const float gridZ = z / scale;
    const int x0 = static_cast<int>(std::floor(gridX));
    const int z0 = static_cast<int>(std::floor(gridZ));
    const float tx0 = gridX - static_cast<float>(x0);
    const float tz0 = gridZ - static_cast<float>(z0);
    const float tx = tx0 * tx0 * (3.0f - 2.0f * tx0);
    const float tz = tz0 * tz0 * (3.0f - 2.0f * tz0);
    const float north = lattice(x0, z0) * (1.0f - tx) + lattice(x0 + 1, z0) * tx;
    const float south = lattice(x0, z0 + 1) * (1.0f - tx) + lattice(x0 + 1, z0 + 1) * tx;
    return north * (1.0f - tz) + south * tz;
}

int terrainHeight(int x, int z) {
    const float hills = smoothNoise(static_cast<float>(x), static_cast<float>(z), 44.0f);
    const float detail = smoothNoise(static_cast<float>(x), static_cast<float>(z), 13.0f);
    const float mountains = smoothNoise(static_cast<float>(x), static_cast<float>(z), 110.0f);
    return std::clamp(static_cast<int>(std::lround(7.0f + hills * 5.0f +
                                                   detail * 1.4f + mountains * 5.0f)),
                      2, VoxelWorld::worldHeight - 9);
}

bool desertBiome(int x, int z) {
    return smoothNoise(static_cast<float>(x), static_cast<float>(z), 190.0f) > 0.38f;
}

bool snowyBiome(int x, int z) {
    return smoothNoise(static_cast<float>(x), static_cast<float>(z), 190.0f) < -0.38f;
}

std::array<float, 3> blockColor(Block block) {
    switch (block) {
    case Block::Bedrock: return {0.20f, 0.21f, 0.23f};
    case Block::Grass: return {0.32f, 0.68f, 0.24f};
    case Block::Dirt: return {0.48f, 0.31f, 0.17f};
    case Block::Stone: return {0.48f, 0.50f, 0.53f};
    case Block::Sand: return {0.80f, 0.72f, 0.47f};
    case Block::Snow: return {0.88f, 0.93f, 0.98f};
    case Block::Water: return {0.18f, 0.45f, 0.82f};
    case Block::Wood: return {0.43f, 0.26f, 0.12f};
    case Block::Leaves: return {0.18f, 0.48f, 0.20f};
    case Block::Planks: return {0.68f, 0.46f, 0.23f};
    case Block::Cloud: return {0.84f, 0.88f, 0.91f};
    default: return {1.0f, 1.0f, 1.0f};
    }
}

bool isOpaque(Block block) {
    return block != Block::Air && block != Block::Water && block != Block::Cloud;
}

} // namespace

std::int64_t VoxelWorld::chunkKey(int chunkX, int chunkZ) {
    const std::uint64_t packed = (static_cast<std::uint64_t>(
                                      static_cast<std::uint32_t>(chunkX)) << 32) |
                                 static_cast<std::uint32_t>(chunkZ);
    return static_cast<std::int64_t>(packed);
}

int VoxelWorld::floorDiv(int value, int divisor) {
    const int quotient = value / divisor;
    const int remainder = value % divisor;
    return remainder < 0 ? quotient - 1 : quotient;
}

int VoxelWorld::localCoordinate(int value, int chunkCoordinate) {
    return value - chunkCoordinate * chunkSize;
}

std::size_t VoxelWorld::blockIndex(int x, int y, int z) {
    return static_cast<std::size_t>(x + chunkSize * (z + chunkSize * y));
}

VoxelWorld::Chunk& VoxelWorld::ensureChunk(int chunkX, int chunkZ) {
    const std::int64_t key = chunkKey(chunkX, chunkZ);
    auto [iterator, inserted] = chunks_.try_emplace(key);
    if (inserted) {
        generateChunk(chunkX, chunkZ, iterator->second);
    }
    return iterator->second;
}

void VoxelWorld::generateChunk(int chunkX, int chunkZ, Chunk& chunk) {
    chunk.blocks.assign(static_cast<std::size_t>(chunkSize * chunkSize * worldHeight), Block::Air);
    const int originX = chunkX * chunkSize;
    const int originZ = chunkZ * chunkSize;

    for (int z = 0; z < chunkSize; ++z) {
        for (int x = 0; x < chunkSize; ++x) {
            const int worldX = originX + x;
            const int worldZ = originZ + z;
            const int height = terrainHeight(worldX, worldZ);
            const bool desert = desertBiome(worldX, worldZ);
            const bool snow = snowyBiome(worldX, worldZ);
            const bool beach = height <= seaLevel + 1;
            const Block surface = desert || beach ? Block::Sand :
                                  snow ? Block::Snow : Block::Grass;
            for (int y = 0; y <= height; ++y) {
                const bool cave = y > 2 && y < height - 2 &&
                    (coordinateHash(worldX * 3 + y * 7, worldZ * 5 - y * 11, 0x17a2b5c3u) %
                     97u) < 8u;
                if (cave) {
                    continue;
                }
                const Block block = y == 0 ? Block::Bedrock :
                                    y == height ? surface :
                                    y >= height - 3 ? (desert || beach ? Block::Sand : Block::Dirt) :
                                                      Block::Stone;
                chunk.blocks[blockIndex(x, y, z)] = block;
            }
            for (int y = height + 1; y <= seaLevel && y < worldHeight; ++y) {
                chunk.blocks[blockIndex(x, y, z)] = Block::Water;
            }

            if (!desert && !snow && !beach &&
                (coordinateHash(worldX, worldZ, 0x6c8e9cf5u) % 83u) == 0u) {
                for (int y = height + 1; y <= height + 4; ++y) {
                    chunk.blocks[blockIndex(x, y, z)] = Block::Wood;
                }
                for (int dz = -2; dz <= 2; ++dz) {
                    for (int dx = -2; dx <= 2; ++dx) {
                        for (int dy = 2; dy <= 5; ++dy) {
                            const int lx = x + dx;
                            const int lz = z + dz;
                            const int ly = height + dy;
                            if (lx >= 0 && lx < chunkSize && lz >= 0 && lz < chunkSize &&
                                ly < worldHeight && std::abs(dx) + std::abs(dz) +
                                    (dy == 5 ? 2 : 0) <= 4 &&
                                chunk.blocks[blockIndex(lx, ly, lz)] == Block::Air) {
                                chunk.blocks[blockIndex(lx, ly, lz)] = Block::Leaves;
                            }
                        }
                    }
                }
            }

        }
    }

    for (int z = 0; z < chunkSize; ++z) {
        for (int x = 0; x < chunkSize; ++x) {
            const int worldX = originX + x;
            const int worldZ = originZ + z;
            if ((coordinateHash(worldX, worldZ, 0xa511e9b3u) % 997u) >= 3u) {
                continue;
            }
            for (int dz = -1; dz <= 1; ++dz) {
                for (int dx = -2; dx <= 2; ++dx) {
                    const int cloudX = x + dx;
                    const int cloudZ = z + dz;
                    if (cloudX >= 0 && cloudX < chunkSize &&
                        cloudZ >= 0 && cloudZ < chunkSize) {
                        const std::size_t index = blockIndex(cloudX, 25, cloudZ);
                        if (chunk.blocks[index] == Block::Air) {
                            chunk.blocks[index] = Block::Cloud;
                        }
                    }
                }
            }
        }
    }

    for (const auto& [position, block] : edits_) {
        const int editChunkX = floorDiv(position.x, chunkSize);
        const int editChunkZ = floorDiv(position.z, chunkSize);
        if (editChunkX == chunkX && editChunkZ == chunkZ &&
            position.y >= 0 && position.y < worldHeight) {
            chunk.blocks[blockIndex(localCoordinate(position.x, chunkX), position.y,
                                    localCoordinate(position.z, chunkZ))] = block;
        }
    }
    chunk.meshDirty = true;
}

Block VoxelWorld::blockAtGenerated(int x, int y, int z) {
    if (y < 0) {
        return Block::Stone;
    }
    if (y >= worldHeight) {
        return Block::Air;
    }
    const int chunkX = floorDiv(x, chunkSize);
    const int chunkZ = floorDiv(z, chunkSize);
    Chunk& chunk = ensureChunk(chunkX, chunkZ);
    return chunk.blocks[blockIndex(localCoordinate(x, chunkX), y,
                                   localCoordinate(z, chunkZ))];
}

Block VoxelWorld::blockAt(int x, int y, int z) {
    return blockAtGenerated(x, y, z);
}

void VoxelWorld::prepareAround(int worldX, int worldZ, int chunkRadius) {
    const int centerX = floorDiv(worldX, chunkSize);
    const int centerZ = floorDiv(worldZ, chunkSize);
    for (int z = centerZ - chunkRadius - 1; z <= centerZ + chunkRadius + 1; ++z) {
        for (int x = centerX - chunkRadius - 1; x <= centerX + chunkRadius + 1; ++x) {
            ensureChunk(x, z);
        }
    }
    const int keepRadius = chunkRadius + 2;
    for (auto iterator = chunks_.begin(); iterator != chunks_.end();) {
        const std::uint64_t packed = static_cast<std::uint64_t>(iterator->first);
        const int chunkX = static_cast<std::int32_t>(packed >> 32);
        const int chunkZ = static_cast<std::int32_t>(packed & 0xffffffffu);
        if (std::abs(chunkX - centerX) > keepRadius ||
            std::abs(chunkZ - centerZ) > keepRadius) {
            iterator = chunks_.erase(iterator);
        } else {
            ++iterator;
        }
    }
}

void VoxelWorld::rebuildMesh(int chunkX, int chunkZ, Chunk& chunk) {
    chunk.faces.clear();
    const int originX = chunkX * chunkSize;
    const int originZ = chunkZ * chunkSize;
    for (int y = 0; y < worldHeight; ++y) {
        for (int z = 0; z < chunkSize; ++z) {
            for (int x = 0; x < chunkSize; ++x) {
                const Block block = chunk.blocks[blockIndex(x, y, z)];
                if (block == Block::Air) {
                    continue;
                }
                const int worldX = originX + x;
                const int worldZ = originZ + z;
                const auto baseColor = blockColor(block);
                for (std::size_t side = 0; side < neighborOffsets.size(); ++side) {
                    const Position offset = neighborOffsets[side];
                    const Block adjacent = blockAtGenerated(worldX + offset.x,
                                                            y + offset.y,
                                                            worldZ + offset.z);
                    const bool showFace = adjacent == Block::Air ||
                        ((block == Block::Water || block == Block::Cloud) &&
                         !isOpaque(adjacent) && adjacent != block);
                    if (!showFace) {
                        continue;
                    }
                    Face face{};
                    face.normal = {static_cast<float>(offset.x),
                                   static_cast<float>(offset.y),
                                   static_cast<float>(offset.z)};
                    for (std::size_t vertex = 0; vertex < face.vertices.size(); ++vertex) {
                        face.vertices[vertex] = {
                            static_cast<float>(worldX) + faceVertices[side][vertex][0],
                            static_cast<float>(y) + faceVertices[side][vertex][1],
                            static_cast<float>(worldZ) + faceVertices[side][vertex][2]
                        };
                    }
                    face.color = {baseColor[0] * faceBrightness[side],
                                  baseColor[1] * faceBrightness[side],
                                  baseColor[2] * faceBrightness[side],
                                  block == Block::Water ? 0.82f : 1.0f};
                    chunk.faces.push_back(face);
                }
            }
        }
    }
    chunk.meshDirty = false;
}

void VoxelWorld::appendVisibleFaces(int worldX, int worldZ, int chunkRadius,
                                   std::vector<const Face*>& faces) {
    faces.clear();
    prepareAround(worldX, worldZ, chunkRadius);
    const int centerX = floorDiv(worldX, chunkSize);
    const int centerZ = floorDiv(worldZ, chunkSize);
    for (int z = centerZ - chunkRadius; z <= centerZ + chunkRadius; ++z) {
        for (int x = centerX - chunkRadius; x <= centerX + chunkRadius; ++x) {
            Chunk& chunk = ensureChunk(x, z);
            if (chunk.meshDirty) {
                rebuildMesh(x, z, chunk);
            }
            for (const Face& face : chunk.faces) {
                faces.push_back(&face);
            }
        }
    }
}

std::uint64_t VoxelWorld::chunkRevision(int chunkX, int chunkZ) {
    return ensureChunk(chunkX, chunkZ).revision;
}

bool VoxelWorld::setBlock(Position position, Block block) {
    if (position.y < 0 || position.y >= worldHeight || block == Block::Water ||
        block == Block::Cloud) {
        return false;
    }
    const int chunkX = floorDiv(position.x, chunkSize);
    const int chunkZ = floorDiv(position.z, chunkSize);
    Chunk& chunk = ensureChunk(chunkX, chunkZ);
    const std::size_t index = blockIndex(localCoordinate(position.x, chunkX), position.y,
                                         localCoordinate(position.z, chunkZ));
    if (chunk.blocks[index] == Block::Bedrock) {
        return false;
    }
    if (chunk.blocks[index] == block) {
        return false;
    }
    chunk.blocks[index] = block;
    edits_[position] = block;
    chunk.meshDirty = true;
    ++chunk.revision;
    const int localX = localCoordinate(position.x, chunkX);
    const int localZ = localCoordinate(position.z, chunkZ);
    if (localX == 0) {
        Chunk& adjacent = ensureChunk(chunkX - 1, chunkZ);
        adjacent.meshDirty = true;
        ++adjacent.revision;
    }
    if (localX == chunkSize - 1) {
        Chunk& adjacent = ensureChunk(chunkX + 1, chunkZ);
        adjacent.meshDirty = true;
        ++adjacent.revision;
    }
    if (localZ == 0) {
        Chunk& adjacent = ensureChunk(chunkX, chunkZ - 1);
        adjacent.meshDirty = true;
        ++adjacent.revision;
    }
    if (localZ == chunkSize - 1) {
        Chunk& adjacent = ensureChunk(chunkX, chunkZ + 1);
        adjacent.meshDirty = true;
        ++adjacent.revision;
    }
    return true;
}

bool VoxelWorld::raycast(Point origin, Point direction, float reach, Position& hit,
                         Position& previous) {
    const float directionLength = std::sqrt(direction.x * direction.x +
                                            direction.y * direction.y +
                                            direction.z * direction.z);
    if (directionLength <= std::numeric_limits<float>::epsilon() || reach <= 0.0f) {
        return false;
    }
    direction.x /= directionLength;
    direction.y /= directionLength;
    direction.z /= directionLength;
    constexpr float stride = 0.05f;
    Position last{static_cast<int>(std::floor(origin.x)), static_cast<int>(std::floor(origin.y)),
                  static_cast<int>(std::floor(origin.z))};
    for (float distance = 0.0f; distance <= reach; distance += stride) {
        const Position current{
            static_cast<int>(std::floor(origin.x + direction.x * distance)),
            static_cast<int>(std::floor(origin.y + direction.y * distance)),
            static_cast<int>(std::floor(origin.z + direction.z * distance))
        };
        const Block block = blockAt(current.x, current.y, current.z);
        if (block != Block::Air && block != Block::Water && block != Block::Cloud) {
            hit = current;
            previous = last;
            return true;
        }
        last = current;
    }
    return false;
}

int VoxelWorld::surfaceHeight(int x, int z) {
    for (int y = worldHeight - 1; y >= 0; --y) {
        const Block block = blockAt(x, y, z);
        if (block == Block::Bedrock || block == Block::Grass ||
            block == Block::Dirt || block == Block::Stone ||
            block == Block::Sand || block == Block::Snow || block == Block::Planks) {
            return y + 1;
        }
    }
    return 1;
}

void VoxelWorld::surfacePositions(int chunkX, int chunkZ, std::vector<Point>& positions) {
    positions.clear();
    positions.reserve(chunkSize * chunkSize);
    const int originX = chunkX * chunkSize;
    const int originZ = chunkZ * chunkSize;
    for (int z = 0; z < chunkSize; ++z) {
        for (int x = 0; x < chunkSize; ++x) {
            const int height = surfaceHeight(originX + x, originZ + z);
            positions.push_back({static_cast<float>(originX + x) + 0.5f,
                                 static_cast<float>(height) - 0.5f,
                                 static_cast<float>(originZ + z) + 0.5f});
        }
    }
}

bool VoxelWorld::save(const std::filesystem::path& file,
                      const std::array<std::uint32_t, 10>& inventory,
                      std::string& error) const {
    std::ofstream output(file, std::ios::binary | std::ios::trunc);
    if (!output) {
        error = "Could not open world save file for writing";
        return false;
    }
    constexpr std::array<char, 8> magic{{'M', 'Y', 'V', 'O', 'X', '0', '1', '\0'}};
    const std::uint32_t count = static_cast<std::uint32_t>(edits_.size());
    output.write(magic.data(), static_cast<std::streamsize>(magic.size()));
    output.write(reinterpret_cast<const char*>(&count), sizeof(count));
    output.write(reinterpret_cast<const char*>(inventory.data()),
                 static_cast<std::streamsize>(sizeof(inventory)));
    for (const auto& [position, block] : edits_) {
        const std::array<std::int32_t, 3> coordinates{{position.x, position.y, position.z}};
        const std::uint8_t value = static_cast<std::uint8_t>(block);
        output.write(reinterpret_cast<const char*>(coordinates.data()),
                     static_cast<std::streamsize>(sizeof(coordinates)));
        output.write(reinterpret_cast<const char*>(&value), sizeof(value));
    }
    if (!output) {
        error = "Could not finish writing the world save file";
        return false;
    }
    return true;
}

bool VoxelWorld::load(const std::filesystem::path& file,
                      std::array<std::uint32_t, 10>& inventory,
                      std::string& error) {
    std::ifstream input(file, std::ios::binary);
    if (!input) {
        error = "Could not open world save file";
        return false;
    }
    constexpr std::array<char, 8> expectedMagic{{'M', 'Y', 'V', 'O', 'X', '0', '1', '\0'}};
    std::array<char, 8> magic{};
    std::uint32_t count = 0;
    std::array<std::uint32_t, 10> loadedInventory{};
    input.read(magic.data(), static_cast<std::streamsize>(magic.size()));
    input.read(reinterpret_cast<char*>(&count), sizeof(count));
    input.read(reinterpret_cast<char*>(loadedInventory.data()),
               static_cast<std::streamsize>(sizeof(loadedInventory)));
    if (!input || magic != expectedMagic || count > 1000000u) {
        error = "World save file is invalid or unsupported";
        return false;
    }
    std::map<Position, Block> loadedEdits;
    for (std::uint32_t i = 0; i < count; ++i) {
        std::array<std::int32_t, 3> coordinates{};
        std::uint8_t blockValue = 0;
        input.read(reinterpret_cast<char*>(coordinates.data()),
                   static_cast<std::streamsize>(sizeof(coordinates)));
        input.read(reinterpret_cast<char*>(&blockValue), sizeof(blockValue));
        if (!input || coordinates[1] < 0 || coordinates[1] >= worldHeight ||
            blockValue > static_cast<std::uint8_t>(Block::Planks)) {
            error = "World save contains invalid block data";
            return false;
        }
        loadedEdits[{coordinates[0], coordinates[1], coordinates[2]}] =
            static_cast<Block>(blockValue);
    }
    if (input.peek() != std::char_traits<char>::eof()) {
        error = "World save contains unexpected trailing data";
        return false;
    }
    edits_ = std::move(loadedEdits);
    inventory = loadedInventory;
    chunks_.clear();
    return true;
}

} // namespace World
