#ifndef VOXEL_WORLD_HPP
#define VOXEL_WORLD_HPP

#include <array>
#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace World {

enum class Block : std::uint8_t {
    Air,
    Bedrock,
    Grass,
    Dirt,
    Stone,
    Sand,
    Snow,
    Water,
    Wood,
    Leaves,
    Planks,
    Cloud
};

struct Position {
    int x;
    int y;
    int z;

    friend bool operator<(Position left, Position right) {
        if (left.x != right.x) return left.x < right.x;
        if (left.y != right.y) return left.y < right.y;
        return left.z < right.z;
    }
};

struct Point {
    float x;
    float y;
    float z;
};

struct Face {
    std::array<Point, 4> vertices;
    std::array<float, 4> color;
    std::array<float, 3> normal;
};

class VoxelWorld {
public:
    static constexpr int chunkSize = 16;
    static constexpr int worldHeight = 40;

    void prepareAround(int worldX, int worldZ, int chunkRadius);
    void appendVisibleFaces(int worldX, int worldZ, int chunkRadius,
                            std::vector<const Face*>& faces);
    std::uint64_t chunkRevision(int chunkX, int chunkZ);
    Block blockAt(int x, int y, int z);
    bool setBlock(Position position, Block block);
    bool raycast(Point origin, Point direction, float reach, Position& hit,
                 Position& previous);
    int surfaceHeight(int x, int z);
    void surfacePositions(int chunkX, int chunkZ, std::vector<Point>& positions);
    bool save(const std::filesystem::path& file, const std::array<std::uint32_t, 10>& inventory,
              std::string& error) const;
    bool load(const std::filesystem::path& file, std::array<std::uint32_t, 10>& inventory,
              std::string& error);

private:
    struct Chunk {
        std::vector<Block> blocks;
        std::vector<Face> faces;
        bool meshDirty = true;
        std::uint64_t revision = 1;
    };

    static std::int64_t chunkKey(int chunkX, int chunkZ);
    static int floorDiv(int value, int divisor);
    static int localCoordinate(int value, int chunkCoordinate);
    Chunk& ensureChunk(int chunkX, int chunkZ);
    void generateChunk(int chunkX, int chunkZ, Chunk& chunk);
    void rebuildMesh(int chunkX, int chunkZ, Chunk& chunk);
    Block blockAtGenerated(int x, int y, int z);
    static std::size_t blockIndex(int x, int y, int z);

    std::unordered_map<std::int64_t, Chunk> chunks_;
    std::map<Position, Block> edits_;
};

} // namespace World

#endif
