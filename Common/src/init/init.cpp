#include "init.hpp"
#include "physics/physics_world.hpp"
#include "renderer/opengl_renderer.hpp"
#include "renderer/renderer.hpp"
#include "renderer/vulkan_renderer.hpp"
#include "world/voxel_world.hpp"
#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

struct Point3D {
    float x;
    float y;
    float z;
};

struct Point2D {
    float x;
    float y;
};

struct RenderFace {
    std::array<Point2D, 4> vertices;
    SDL_FColor color;
    float depth;
};

struct Camera {
    Point3D position{-0.9f, 2.85f, -6.3f};
    float yaw = 0.0f;
    float pitch = -0.1f;
};

struct CameraBasis {
    Point3D forward;
    Point3D right;
    Point3D up;
};

constexpr std::array<std::array<int, 4>, 6> faceVertices{{
    {{0, 1, 2, 3}}, {{4, 7, 6, 5}},
    {{0, 4, 5, 1}}, {{3, 2, 6, 7}},
    {{0, 3, 7, 4}}, {{1, 5, 6, 2}}
}};

constexpr float playerEyeHeight = 1.65f;
constexpr float fallThreshold = -30.0f;

struct SceneBuffers {
    std::vector<RenderFace> faces;
    std::vector<const World::Face*> sourceFaces;
    std::vector<VulkanVertex> vertices;
    std::vector<std::uint32_t> indices;

    SceneBuffers() {
        faces.reserve(4225 * faceVertices.size());
        sourceFaces.reserve(32768);
        vertices.reserve(4225 * faceVertices.size() * 4);
        indices.reserve(4225 * faceVertices.size() * 6);
    }
};
enum class RendererBackend {
    Vulkan,
    OpenGL
};

#if defined(MYGAME_ENABLE_OPENGL) && MYGAME_ENABLE_OPENGL
constexpr bool openGLAvailable = true;
#else
constexpr bool openGLAvailable = false;
#endif

bool prepareRendererWindow(RendererBackend backend, std::string& error) {
    if (backend == RendererBackend::Vulkan) {
        return true;
    }
    if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2) != 0 ||
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1) != 0 ||
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1) != 0) {
        error = std::string("SDL_GL_SetAttribute failed: ") + SDL_GetError();
        return false;
    }
    return true;
}

std::unique_ptr<Renderer> createRenderer(RendererBackend backend, bool verticalSync) {
    if (backend == RendererBackend::OpenGL) {
        return std::make_unique<OpenGLRenderer>();
    }
    return std::make_unique<VulkanRenderer>(verticalSync);
}

CameraBasis makeCameraBasis(const Camera& camera) {
    const float sinYaw = std::sin(camera.yaw);
    const float cosYaw = std::cos(camera.yaw);
    const float sinPitch = std::sin(camera.pitch);
    const float cosPitch = std::cos(camera.pitch);
    CameraBasis basis{};
    basis.forward = {sinYaw * cosPitch, sinPitch, cosYaw * cosPitch};
    basis.right = {cosYaw, 0.0f, -sinYaw};
    basis.up = {
        basis.forward.y * basis.right.z - basis.forward.z * basis.right.y,
        basis.forward.z * basis.right.x - basis.forward.x * basis.right.z,
        basis.forward.x * basis.right.y - basis.forward.y * basis.right.x
    };
    return basis;
}

Point3D subtract(Point3D left, Point3D right) {
    return {left.x - right.x, left.y - right.y, left.z - right.z};
}

float dot(Point3D left, Point3D right) {
    return left.x * right.x + left.y * right.y + left.z * right.z;
}

Point3D add(Point3D left, Point3D right) {
    return {left.x + right.x, left.y + right.y, left.z + right.z};
}

Point3D scale(Point3D point, float amount) {
    return {point.x * amount, point.y * amount, point.z * amount};
}

void movePlayer(Physics::PhysicsWorld& physics, Camera& camera, float forwardAmount,
                float rightAmount, bool sprint, bool jumpPressed, float deltaTime,
                World::VoxelWorld& world) {
    const float sinYaw = std::sin(camera.yaw);
    const float cosYaw = std::cos(camera.yaw);
    Point3D movement{
        sinYaw * forwardAmount + cosYaw * rightAmount,
        0.0f,
        cosYaw * forwardAmount - sinYaw * rightAmount
    };
    const float movementLength = std::sqrt(dot(movement, movement));
    const float speed = sprint ? 8.5f : 5.0f;
    if (movementLength > 0.0f) {
        movement = scale(movement, speed / movementLength);
    }
    physics.setPlayerMovement({movement.x, 0.0f, movement.z}, jumpPressed);
    physics.step(deltaTime);
    const Physics::Vector3 playerPosition = physics.playerEyePosition();
    camera.position = {playerPosition.x, playerPosition.y, playerPosition.z};

    if (camera.position.y - playerEyeHeight < fallThreshold) {
        const float spawnHeight = static_cast<float>(world.surfaceHeight(0, -7)) +
                                  playerEyeHeight;
        const Point3D safeSpawn{0.5f, spawnHeight, -6.5f};
        physics.resetPlayer({safeSpawn.x, safeSpawn.y, safeSpawn.z});
        camera.position = safeSpawn;
    }
}

void appendQuad(SceneBuffers& buffers, const std::array<Point2D, 4>& points, SDL_FColor color) {
    constexpr std::array<std::uint32_t, 6> indices{{0, 1, 2, 0, 2, 3}};
    const std::uint32_t firstVertex = static_cast<std::uint32_t>(buffers.vertices.size());
    for (const Point2D point : points) {
        buffers.vertices.push_back({point.x, point.y, color.r, color.g, color.b, color.a});
    }
    for (const std::uint32_t index : indices) {
        buffers.indices.push_back(firstVertex + index);
    }
}

std::array<std::uint8_t, 7> glyph(char character) {
    switch (character) {
    case 'A': return {14, 17, 17, 31, 17, 17, 17};
    case 'B': return {30, 17, 17, 30, 17, 17, 30};
    case 'C': return {14, 17, 16, 16, 16, 17, 14};
    case 'D': return {30, 17, 17, 17, 17, 17, 30};
    case 'E': return {31, 16, 16, 30, 16, 16, 31};
    case 'F': return {31, 16, 16, 30, 16, 16, 16};
    case 'G': return {14, 17, 16, 23, 17, 17, 15};
    case 'H': return {17, 17, 17, 31, 17, 17, 17};
    case 'I': return {14, 4, 4, 4, 4, 4, 14};
    case 'K': return {17, 18, 20, 24, 20, 18, 17};
    case 'L': return {16, 16, 16, 16, 16, 16, 31};
    case 'M': return {17, 27, 21, 21, 17, 17, 17};
    case 'N': return {17, 25, 21, 19, 17, 17, 17};
    case 'O': return {14, 17, 17, 17, 17, 17, 14};
    case 'P': return {30, 17, 17, 30, 16, 16, 16};
    case 'R': return {30, 17, 17, 30, 20, 18, 17};
    case 'S': return {15, 16, 16, 14, 1, 1, 30};
    case 'T': return {31, 4, 4, 4, 4, 4, 4};
    case 'U': return {17, 17, 17, 17, 17, 17, 14};
    case 'V': return {17, 17, 17, 17, 17, 10, 4};
    case 'W': return {17, 17, 17, 21, 21, 21, 10};
    case '0': return {14, 17, 19, 21, 25, 17, 14};
    case '1': return {4, 12, 4, 4, 4, 4, 14};
    case '2': return {14, 17, 1, 2, 4, 8, 31};
    case '3': return {30, 1, 1, 14, 1, 1, 30};
    case '4': return {2, 6, 10, 18, 31, 2, 2};
    case '5': return {31, 16, 16, 30, 1, 1, 30};
    case '6': return {14, 16, 16, 30, 17, 17, 14};
    case '7': return {31, 1, 2, 4, 8, 8, 8};
    case '8': return {14, 17, 17, 14, 17, 17, 14};
    case '9': return {14, 17, 17, 15, 1, 1, 14};
    case ':': return {0, 4, 4, 0, 4, 4, 0};
    case '/': return {1, 2, 2, 4, 8, 8, 16};
    case '|': return {4, 4, 4, 4, 4, 4, 4};
    case '-': return {0, 0, 0, 31, 0, 0, 0};
    default: return {};
    }
}

void appendText(SceneBuffers& buffers, const char* text, float x, float y, float scale,
                SDL_FColor color) {
    const float originX = x;
    for (const char* character = text; *character != '\0'; ++character) {
        if (*character == '\n') {
            x = originX;
            y += 9.0f * scale;
            continue;
        }
        const auto rows = glyph(*character);
        for (std::size_t row = 0; row < rows.size(); ++row) {
            for (int column = 0; column < 5; ++column) {
                if ((rows[row] & (1u << (4 - column))) == 0) {
                    continue;
                }
                const float left = x + static_cast<float>(column) * scale;
                const float top = y + static_cast<float>(row) * scale;
                appendQuad(buffers,
                           {{{left, top}, {left + scale, top},
                             {left + scale, top + scale}, {left, top + scale}}},
                           color);
            }
        }
        x += 6.0f * scale;
    }
}

std::array<Point2D, 4> makeLineQuad(Point2D from, Point2D to) {
    constexpr float halfWidth = 0.7f;
    const float dx = to.x - from.x;
    const float dy = to.y - from.y;
    const float length = std::sqrt(dx * dx + dy * dy);
    if (length == 0.0f) {
        return {from, from, from, from};
    }

    const float offsetX = -dy * halfWidth / length;
    const float offsetY = dx * halfWidth / length;
    return {{
        {from.x + offsetX, from.y + offsetY},
        {to.x + offsetX, to.y + offsetY},
        {to.x - offsetX, to.y - offsetY},
        {from.x - offsetX, from.y - offsetY}
    }};
}

void renderScene(int width, int height, const Camera& camera, const CameraBasis& basis,
                 const char* fpsText, bool settingsOpen, bool canSelectOpenGL,
                 const char* activeRenderer,
                 World::VoxelWorld& world, Point3D enemyPosition, int health,
                 const std::array<std::uint32_t, 10>& inventory, std::size_t selectedSlot,
                 const char* statusText, SceneBuffers& buffers) {
    const float focalLength = static_cast<float>(std::min(width, height)) * 0.8f;
    const float centerX = static_cast<float>(width) * 0.5f;
    const float centerY = static_cast<float>(height) * 0.5f;

    buffers.faces.clear();
    buffers.vertices.clear();
    buffers.indices.clear();

    constexpr int chunkRadius = 1;
    world.appendVisibleFaces(static_cast<int>(std::floor(camera.position.x)),
                             static_cast<int>(std::floor(camera.position.z)),
                             chunkRadius, buffers.sourceFaces);
    auto appendWorldFace = [&](const std::array<World::Point, 4>& points,
                               const std::array<float, 4>& color) {
        RenderFace face{};
        face.color = {color[0], color[1], color[2], color[3]};
        float depthTotal = 0.0f;
        float minimumX = static_cast<float>(width);
        float minimumY = static_cast<float>(height);
        float maximumX = 0.0f;
        float maximumY = 0.0f;
        for (std::size_t vertex = 0; vertex < points.size(); ++vertex) {
            const Point3D relative{points[vertex].x - camera.position.x,
                                   points[vertex].y - camera.position.y,
                                   points[vertex].z - camera.position.z};
            const float depth = dot(relative, basis.forward);
            if (depth <= 0.1f) {
                return;
            }
            face.vertices[vertex] = {
                centerX + dot(relative, basis.right) * focalLength / depth,
                centerY - dot(relative, basis.up) * focalLength / depth
            };
            minimumX = std::min(minimumX, face.vertices[vertex].x);
            minimumY = std::min(minimumY, face.vertices[vertex].y);
            maximumX = std::max(maximumX, face.vertices[vertex].x);
            maximumY = std::max(maximumY, face.vertices[vertex].y);
            depthTotal += depth;
        }
        if (maximumX < 0.0f || minimumX > static_cast<float>(width) ||
            maximumY < 0.0f || minimumY > static_cast<float>(height)) {
            return;
        }
        face.depth = depthTotal * 0.25f;
        buffers.faces.push_back(face);
    };
    for (const World::Face* face : buffers.sourceFaces) {
        const World::Point center{
            (face->vertices[0].x + face->vertices[2].x) * 0.5f,
            (face->vertices[0].y + face->vertices[2].y) * 0.5f,
            (face->vertices[0].z + face->vertices[2].z) * 0.5f
        };
        const Point3D toCamera{camera.position.x - center.x,
                               camera.position.y - center.y,
                               camera.position.z - center.z};
        if (face->normal[0] * toCamera.x + face->normal[1] * toCamera.y +
                face->normal[2] * toCamera.z <= 0.0f) {
            continue;
        }
        appendWorldFace(face->vertices, face->color);
    }

    constexpr std::array<std::array<int, 4>, 6> cubeFaces{{
        {{0, 1, 2, 3}}, {{4, 7, 6, 5}},
        {{0, 4, 5, 1}}, {{3, 2, 6, 7}},
        {{0, 3, 7, 4}}, {{1, 5, 6, 2}}
    }};
    constexpr std::array<Point3D, 8> enemyCube{{
        {-0.35f, 0.0f, -0.35f}, {0.35f, 0.0f, -0.35f},
        {0.35f, 1.1f, -0.35f}, {-0.35f, 1.1f, -0.35f},
        {-0.35f, 0.0f, 0.35f}, {0.35f, 0.0f, 0.35f},
        {0.35f, 1.1f, 0.35f}, {-0.35f, 1.1f, 0.35f}
    }};
    constexpr std::array<float, 4> enemyColor{{0.78f, 0.17f, 0.13f, 1.0f}};
    for (const auto& side : cubeFaces) {
        std::array<World::Point, 4> points{};
        for (std::size_t vertex = 0; vertex < points.size(); ++vertex) {
            const Point3D point = add(enemyPosition, enemyCube[side[vertex]]);
            points[vertex] = {point.x, point.y, point.z};
        }
        appendWorldFace(points, enemyColor);
    }
    std::sort(buffers.faces.begin(), buffers.faces.end(),
              [](const RenderFace& left, const RenderFace& right) {
        return left.depth > right.depth;
    });

    for (const RenderFace& face : buffers.faces) {
        appendQuad(buffers, face.vertices, face.color);
    }

    appendText(buffers, fpsText, 12.0f, 12.0f, 1.5f,
               {0.94f, 0.96f, 1.0f, 1.0f});
    appendText(buffers,
               "WASD MOVE SPACE JUMP E MINE F ATTACK RIGHT PLACE C CRAFT F5 SAVE F9 LOAD",
               12.0f, 28.0f, 1.0f, {0.94f, 0.96f, 1.0f, 1.0f});
    const std::string healthText = "HP " + std::to_string(health);
    appendText(buffers, healthText.c_str(), 12.0f, 42.0f, 1.0f,
               {1.0f, 0.35f, 0.25f, 1.0f});
    std::string hotbar = "HOTBAR 1DIRT " + std::to_string(inventory[0]) + " 2STONE " +
                         std::to_string(inventory[1]) + " 3SAND " +
                         std::to_string(inventory[2]) + " 4WOOD " +
                         std::to_string(inventory[3]) + " 5LEAVES " +
                         std::to_string(inventory[4]) + " 6PLANKS " +
                         std::to_string(inventory[5]) + " STICKS " +
                         std::to_string(inventory[9]) + " TOOLS " +
                         std::to_string(inventory[8]) + " 7SNOW " +
                         std::to_string(inventory[6]) + " 8GRASS " +
                         std::to_string(inventory[7]);
    appendText(buffers, hotbar.c_str(), 42.0f, 42.0f, 1.0f,
               {0.94f, 0.96f, 1.0f, 1.0f});
    const std::string selectedText = "SLOT " + std::to_string(selectedSlot + 1);
    appendText(buffers, selectedText.c_str(), 12.0f, 56.0f, 1.0f,
               {0.50f, 0.85f, 1.0f, 1.0f});
    appendText(buffers, statusText, 12.0f, static_cast<float>(height) - 28.0f, 1.0f,
               {0.94f, 0.96f, 1.0f, 1.0f});
    appendText(buffers, "TOUCH LEFT MOVE RIGHT AIM TOP MINE PLACE BOTTOM JUMP",
               12.0f, static_cast<float>(height) - 14.0f, 1.0f,
               {0.78f, 0.82f, 0.88f, 1.0f});
    if (settingsOpen) {
        appendText(buffers, canSelectOpenGL
                                ? "SETTINGS\n1 VULKAN\n2 OPENGL\nESC CLOSE"
                                : "SETTINGS\n1 VULKAN\nESC CLOSE",
                   12.0f, 48.0f, 1.5f, {0.94f, 0.96f, 1.0f, 1.0f});
        appendText(buffers, activeRenderer, 12.0f, 118.0f, 1.5f,
                   {0.37f, 0.80f, 0.49f, 1.0f});
    }
}

} // namespace

namespace Engine {

bool run(bool useOpenGL, bool verticalSync) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return false;
    }

    RendererBackend backend =
        useOpenGL ? RendererBackend::OpenGL : RendererBackend::Vulkan;
    std::string rendererError;
    if (!prepareRendererWindow(backend, rendererError)) {
        std::cerr << "Could not prepare selected renderer: " << rendererError << std::endl;
        SDL_Quit();
        return false;
    }
    const SDL_WindowFlags windowFlags =
        SDL_WINDOW_RESIZABLE |
        (backend == RendererBackend::Vulkan ? SDL_WINDOW_VULKAN : SDL_WINDOW_OPENGL);
    SDL_Window* window = SDL_CreateWindow("Engine Window", 1280, 720, windowFlags);

    if (!window) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return false;
    }

    std::unique_ptr<Renderer> renderer = createRenderer(backend, verticalSync);
    if (!renderer->initialize(window, rendererError, verticalSync)) {
        std::cerr << (backend == RendererBackend::Vulkan ? "Vulkan" : "OpenGL")
                  << " renderer initialization failed: " << rendererError << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }

    bool isRunning = true;
    bool succeeded = true;
    bool mouseCaptured = false;
    bool settingsOpen = false;
    SDL_Event event;
    const std::uint64_t performanceFrequency = SDL_GetPerformanceFrequency();
    const std::uint64_t startTime = SDL_GetPerformanceCounter();
    std::uint64_t previousFrame = startTime;
    std::uint64_t fpsUpdateTime = previousFrame;
    std::uint64_t fpsFrameCount = 0;
    std::string fpsText = "FPS: 0";
    World::VoxelWorld world;
    const Point3D spawnPosition{
        0.5f, static_cast<float>(world.surfaceHeight(0, -7)) + playerEyeHeight, -6.5f};
    Camera camera;
    camera.position = spawnPosition;
    Physics::PhysicsWorld physics;
    if (!physics.initialize()) {
        std::cerr << "Jolt Physics initialization failed" << std::endl;
        renderer->shutdown();
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }
    physics.createPlayer({spawnPosition.x, spawnPosition.y, spawnPosition.z});
    SceneBuffers sceneBuffers;
    constexpr float mouseSensitivity = 0.0025f;
    bool jumpPressed = false;
    std::array<std::uint32_t, 10> inventory{{0, 32, 16, 8, 0, 0, 0, 0, 0, 0}};
    constexpr std::array<World::Block, 8> hotbarBlocks{{
        World::Block::Dirt, World::Block::Stone, World::Block::Sand,
        World::Block::Wood, World::Block::Leaves, World::Block::Planks,
        World::Block::Snow, World::Block::Grass
    }};
    std::size_t selectedSlot = 0;
    std::string statusText = "EXPLORE | E MINE | RIGHT PLACE | C CRAFT | F5 SAVE | F9 LOAD";
    Point3D enemyPosition{5.5f,
        static_cast<float>(world.surfaceHeight(5, -7)), -6.5f};
    int health = 10;
    int enemyHealth = 3;
    float enemyAttackCooldown = 0.0f;
    std::unordered_map<std::int64_t, std::pair<Physics::BodyHandle, std::uint64_t>>
        terrainBodies;
    std::vector<World::Point> surfacePoints;
    std::vector<Physics::Vector3> colliderPositions;
    auto terrainChunkKey = [](int chunkX, int chunkZ) {
        return static_cast<std::int64_t>(
            (static_cast<std::uint64_t>(static_cast<std::uint32_t>(chunkX)) << 32) |
            static_cast<std::uint32_t>(chunkZ));
    };
    auto updateTerrainColliders = [&]() {
        const int centerChunkX = static_cast<int>(
            std::floor(camera.position.x / World::VoxelWorld::chunkSize));
        const int centerChunkZ = static_cast<int>(
            std::floor(camera.position.z / World::VoxelWorld::chunkSize));
        for (int z = centerChunkZ - 2; z <= centerChunkZ + 2; ++z) {
            for (int x = centerChunkX - 2; x <= centerChunkX + 2; ++x) {
                const std::int64_t key = terrainChunkKey(x, z);
                const std::uint64_t revision = world.chunkRevision(x, z);
                const auto found = terrainBodies.find(key);
                if (found != terrainBodies.end() && found->second.second == revision) {
                    continue;
                }
                if (found != terrainBodies.end()) {
                    physics.destroyBody(found->second.first);
                    terrainBodies.erase(found);
                }
                world.surfacePositions(x, z, surfacePoints);
                colliderPositions.clear();
                colliderPositions.reserve(surfacePoints.size());
                for (const World::Point point : surfacePoints) {
                    colliderPositions.push_back({point.x, point.y, point.z});
                }
                const Physics::BodyHandle body =
                    physics.createStaticCubeBatch(colliderPositions);
                terrainBodies.emplace(key, std::make_pair(body, revision));
            }
        }
        for (auto iterator = terrainBodies.begin(); iterator != terrainBodies.end();) {
            const int chunkX = static_cast<int>(static_cast<std::uint32_t>(
                static_cast<std::uint64_t>(iterator->first) >> 32));
            const int chunkZ = static_cast<int>(static_cast<std::uint32_t>(iterator->first));
            if (std::abs(chunkX - centerChunkX) > 2 || std::abs(chunkZ - centerChunkZ) > 2) {
                physics.destroyBody(iterator->second.first);
                iterator = terrainBodies.erase(iterator);
            } else {
                ++iterator;
            }
        }
    };
    updateTerrainColliders();

    char* preferencePath = SDL_GetPrefPath("MyGame", "VoxelGame");
    std::filesystem::path saveFile;
    if (preferencePath) {
        saveFile = std::filesystem::path(preferencePath) / "world.sav";
        SDL_free(preferencePath);
    } else {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not locate a world save directory: %s",
                     SDL_GetError());
        statusText = "SAVE DIRECTORY UNAVAILABLE";
    }

    auto inventorySlot = [](World::Block block) -> std::size_t {
        switch (block) {
        case World::Block::Dirt: return 0;
        case World::Block::Stone: return 1;
        case World::Block::Sand: return 2;
        case World::Block::Wood: return 3;
        case World::Block::Leaves: return 4;
        case World::Block::Planks: return 5;
        case World::Block::Snow: return 6;
        case World::Block::Grass: return 7;
        default: return 10;
        }
    };
    auto mineTarget = [&]() {
        const CameraBasis aim = makeCameraBasis(camera);
        World::Position hit{};
        World::Position previous{};
        if (!world.raycast({camera.position.x, camera.position.y, camera.position.z},
                           {aim.forward.x, aim.forward.y, aim.forward.z}, 6.0f, hit, previous)) {
            statusText = "NO BLOCK IN REACH";
            return;
        }
        const World::Block block = world.blockAt(hit.x, hit.y, hit.z);
        if (block == World::Block::Bedrock) {
            statusText = "BEDROCK CANNOT BE MINED";
            return;
        }
        const std::size_t slot = inventorySlot(block);
        if (world.setBlock(hit, World::Block::Air)) {
            if (slot < inventory.size()) {
                ++inventory[slot];
            }
            statusText = "BLOCK MINED";
        }
    };
    auto placeTarget = [&]() {
        const CameraBasis aim = makeCameraBasis(camera);
        World::Position hit{};
        World::Position previous{};
        if (!world.raycast({camera.position.x, camera.position.y, camera.position.z},
                           {aim.forward.x, aim.forward.y, aim.forward.z}, 6.0f, hit, previous)) {
            statusText = "NO PLACEMENT TARGET";
            return;
        }
        const World::Block block = hotbarBlocks[selectedSlot];
        const std::size_t slot = inventorySlot(block);
        const int playerX = static_cast<int>(std::floor(camera.position.x));
        const int playerY = static_cast<int>(std::floor(camera.position.y - 1.0f));
        const int playerZ = static_cast<int>(std::floor(camera.position.z));
        if (slot >= inventory.size() || inventory[slot] == 0) {
            statusText = "NO BLOCKS IN SLOT";
        } else if (previous.x == playerX && previous.z == playerZ &&
                   previous.y >= playerY && previous.y <= playerY + 2) {
            statusText = "CANNOT PLACE IN PLAYER";
        } else if (world.setBlock(previous, block)) {
            --inventory[slot];
            statusText = "BLOCK PLACED";
        }
    };
    auto attackEnemy = [&]() {
        const float dx = enemyPosition.x - camera.position.x;
        const float dy = enemyPosition.y + 0.55f - camera.position.y;
        const float dz = enemyPosition.z - camera.position.z;
        const float distance = std::sqrt(dx * dx + dy * dy + dz * dz);
        const CameraBasis aim = makeCameraBasis(camera);
        if (distance <= 0.0f || distance > 4.0f ||
            (dx * aim.forward.x + dy * aim.forward.y + dz * aim.forward.z) / distance < 0.75f) {
            statusText = "NO ENEMY IN REACH";
            return;
        }
        enemyHealth -= inventory[8] > 0 ? 2 : 1;
        if (enemyHealth <= 0) {
            enemyHealth = 3;
            enemyPosition = {static_cast<float>(std::floor(camera.position.x)) + 5.5f,
                static_cast<float>(world.surfaceHeight(
                    static_cast<int>(std::floor(camera.position.x)) + 5,
                    static_cast<int>(std::floor(camera.position.z)))),
                static_cast<float>(std::floor(camera.position.z))};
            statusText = "ENEMY DEFEATED";
        } else {
            statusText = "ENEMY HIT";
        }
    };
    SDL_FingerID moveFinger = 0;
    SDL_FingerID lookFinger = 0;
    bool movingByTouch = false;
    bool lookingByTouch = false;
    float touchForward = 0.0f;
    float touchRight = 0.0f;
    float touchStartX = 0.0f;
    float touchStartY = 0.0f;

    auto switchRenderer = [&](RendererBackend nextBackend) {
        if (nextBackend == backend) {
            settingsOpen = false;
            return;
        }
        if (!prepareRendererWindow(nextBackend, rendererError)) {
            std::cerr << "Could not prepare renderer window: " << rendererError << std::endl;
            return;
        }

        const SDL_WindowFlags nextWindowFlags =
            SDL_WINDOW_RESIZABLE |
            (nextBackend == RendererBackend::Vulkan ? SDL_WINDOW_VULKAN : SDL_WINDOW_OPENGL);
        SDL_Window* nextWindow =
            SDL_CreateWindow("Engine Window", 1280, 720, nextWindowFlags);
        if (!nextWindow) {
            std::cerr << "Could not create renderer window: " << SDL_GetError() << std::endl;
            return;
        }

        std::unique_ptr<Renderer> nextRenderer = createRenderer(nextBackend, verticalSync);
        if (!nextRenderer->initialize(nextWindow, rendererError, verticalSync)) {
            std::cerr << "Could not initialize selected renderer: " << rendererError << std::endl;
            nextRenderer->shutdown();
            SDL_DestroyWindow(nextWindow);
            return;
        }

        if (mouseCaptured) {
            if (!SDL_SetWindowRelativeMouseMode(window, false)) {
                std::cerr << "Could not release mouse: " << SDL_GetError() << std::endl;
            }
            mouseCaptured = false;
        }
        renderer->shutdown();
        renderer = std::move(nextRenderer);
        SDL_DestroyWindow(window);
        window = nextWindow;
        backend = nextBackend;
        settingsOpen = false;
        previousFrame = SDL_GetPerformanceCounter();
        fpsUpdateTime = previousFrame;
        fpsFrameCount = 0;
        fpsText = "FPS: 0";
    };

    while (isRunning) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
                isRunning = false;
            } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && !settingsOpen) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    if (!mouseCaptured) {
                        mouseCaptured = SDL_SetWindowRelativeMouseMode(window, true);
                        if (!mouseCaptured) {
                            std::cerr << "Could not capture mouse: " << SDL_GetError() << std::endl;
                        }
                    }
                    mineTarget();
                } else if (event.button.button == SDL_BUTTON_RIGHT) {
                    placeTarget();
                }
            } else if (event.type == SDL_EVENT_MOUSE_MOTION && mouseCaptured) {
                camera.yaw += event.motion.xrel * mouseSensitivity;
                camera.pitch = std::clamp(camera.pitch - event.motion.yrel * mouseSensitivity,
                                          -1.5f, 1.5f);
            } else if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat &&
                       !settingsOpen && event.key.scancode >= SDL_SCANCODE_1 &&
                       event.key.scancode <= SDL_SCANCODE_8) {
                selectedSlot = static_cast<std::size_t>(
                    event.key.scancode - SDL_SCANCODE_1);
            } else if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat &&
                       !settingsOpen && event.key.scancode == SDL_SCANCODE_E) {
                mineTarget();
            } else if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat &&
                       !settingsOpen && event.key.scancode == SDL_SCANCODE_F) {
                attackEnemy();
            } else if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat &&
                       !settingsOpen && event.key.scancode == SDL_SCANCODE_C) {
                if (inventory[3] > 0) {
                    --inventory[3];
                    inventory[5] += 4;
                    statusText = "CRAFTED FOUR PLANKS";
                } else if (inventory[5] >= 2) {
                    inventory[5] -= 2;
                    inventory[9] += 4;
                    statusText = "CRAFTED FOUR STICKS";
                } else if (inventory[1] >= 3 && inventory[9] >= 2) {
                    inventory[1] -= 3;
                    inventory[9] -= 2;
                    ++inventory[8];
                    statusText = "STONE PICK CRAFTED";
                } else {
                    statusText = "NEED WOOD, PLANKS OR STONE";
                }
            } else if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat &&
                       !settingsOpen && event.key.scancode == SDL_SCANCODE_F5) {
                std::string saveError;
                if (!saveFile.empty() && world.save(saveFile, inventory, saveError)) {
                    statusText = "WORLD SAVED";
                } else {
                    statusText = saveError.empty() ? "SAVE DIRECTORY UNAVAILABLE" : saveError;
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "World save failed: %s",
                                 statusText.c_str());
                }
            } else if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat &&
                       !settingsOpen && event.key.scancode == SDL_SCANCODE_F9) {
                std::string loadError;
                if (!saveFile.empty() && world.load(saveFile, inventory, loadError)) {
                    for (const auto& [key, body] : terrainBodies) {
                        static_cast<void>(key);
                        physics.destroyBody(body.first);
                    }
                    terrainBodies.clear();
                    updateTerrainColliders();
                    statusText = "WORLD LOADED";
                } else {
                    statusText = loadError.empty() ? "SAVE DIRECTORY UNAVAILABLE" : loadError;
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "World load failed: %s",
                                 statusText.c_str());
                }
            } else if (event.type == SDL_EVENT_FINGER_DOWN && !settingsOpen) {
                if (event.tfinger.y > 0.9f && event.tfinger.x >= 0.35f &&
                    event.tfinger.x < 0.85f) {
                    selectedSlot = std::min<std::size_t>(
                        7, static_cast<std::size_t>((event.tfinger.x - 0.35f) * 16.0f));
                } else if (event.tfinger.x < 0.45f && !movingByTouch) {
                    moveFinger = event.tfinger.fingerID;
                    movingByTouch = true;
                    touchStartX = event.tfinger.x;
                    touchStartY = event.tfinger.y;
                } else if (event.tfinger.x > 0.85f && event.tfinger.y > 0.78f) {
                    jumpPressed = true;
                } else if (event.tfinger.x > 0.85f && event.tfinger.y < 0.10f) {
                    mineTarget();
                } else if (event.tfinger.x > 0.85f && event.tfinger.y < 0.22f) {
                    attackEnemy();
                } else if (event.tfinger.x > 0.85f && event.tfinger.y < 0.44f) {
                    placeTarget();
                } else if (!lookingByTouch) {
                    lookFinger = event.tfinger.fingerID;
                    lookingByTouch = true;
                }
            } else if (event.type == SDL_EVENT_FINGER_MOTION && !settingsOpen) {
                if (movingByTouch && event.tfinger.fingerID == moveFinger) {
                    touchRight = std::clamp((event.tfinger.x - touchStartX) * 3.0f, -1.0f, 1.0f);
                    touchForward =
                        std::clamp((touchStartY - event.tfinger.y) * 3.0f, -1.0f, 1.0f);
                } else if (lookingByTouch && event.tfinger.fingerID == lookFinger) {
                    camera.yaw += event.tfinger.dx * 5.0f;
                    camera.pitch = std::clamp(camera.pitch - event.tfinger.dy * 5.0f,
                                              -1.5f, 1.5f);
                }
            } else if (event.type == SDL_EVENT_FINGER_UP) {
                if (movingByTouch && event.tfinger.fingerID == moveFinger) {
                    movingByTouch = false;
                    touchForward = 0.0f;
                    touchRight = 0.0f;
                }
                if (lookingByTouch && event.tfinger.fingerID == lookFinger) {
                    lookingByTouch = false;
                }
            } else if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat &&
                       event.key.scancode == SDL_SCANCODE_ESCAPE) {
                if (mouseCaptured) {
                    if (SDL_SetWindowRelativeMouseMode(window, false)) {
                        mouseCaptured = false;
                        settingsOpen = true;
                    } else {
                        std::cerr << "Could not release mouse: " << SDL_GetError() << std::endl;
                    }
                } else {
                    settingsOpen = !settingsOpen;
                }
            } else if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat && settingsOpen &&
                       event.key.scancode == SDL_SCANCODE_1) {
                switchRenderer(RendererBackend::Vulkan);
            } else if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat && settingsOpen &&
                       event.key.scancode == SDL_SCANCODE_2) {
                if (openGLAvailable) {
                    switchRenderer(RendererBackend::OpenGL);
                }
            } else if (event.type == SDL_EVENT_KEY_DOWN && !settingsOpen &&
                       event.key.scancode == SDL_SCANCODE_SPACE && !event.key.repeat) {
                jumpPressed = true;
            }
        }

        if (!isRunning) {
            break;
        }

        const std::uint64_t now = SDL_GetPerformanceCounter();
        const float deltaTime = std::min(
            static_cast<float>(static_cast<double>(now - previousFrame) /
                               static_cast<double>(performanceFrequency)),
            0.1f);
        previousFrame = now;
        const CameraBasis cameraBasis = makeCameraBasis(camera);
        if (!settingsOpen) {
            const bool* keys = SDL_GetKeyboardState(nullptr);
            float forward = static_cast<float>(keys[SDL_SCANCODE_W]) -
                            static_cast<float>(keys[SDL_SCANCODE_S]);
            float right = static_cast<float>(keys[SDL_SCANCODE_D]) -
                          static_cast<float>(keys[SDL_SCANCODE_A]);
            if (movingByTouch) {
                forward = touchForward;
                right = touchRight;
            }
            const bool sprint = keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT];
            movePlayer(physics, camera, forward, right, sprint, jumpPressed, deltaTime, world);
            updateTerrainColliders();
        }
        jumpPressed = false;
        enemyAttackCooldown = std::max(0.0f, enemyAttackCooldown - deltaTime);
        const float enemyDx = camera.position.x - enemyPosition.x;
        const float enemyDz = camera.position.z - enemyPosition.z;
        const float enemyDistance = std::sqrt(enemyDx * enemyDx + enemyDz * enemyDz);
        if (enemyDistance > 1.15f && enemyDistance < 24.0f) {
            const float travel = std::min(1.1f * deltaTime, enemyDistance - 1.0f);
            enemyPosition.x += enemyDx / enemyDistance * travel;
            enemyPosition.z += enemyDz / enemyDistance * travel;
            enemyPosition.y = static_cast<float>(world.surfaceHeight(
                static_cast<int>(std::floor(enemyPosition.x)),
                static_cast<int>(std::floor(enemyPosition.z))));
        } else if (enemyDistance <= 1.15f && enemyAttackCooldown == 0.0f) {
            health = std::max(0, health - 1);
            enemyAttackCooldown = 1.0f;
            statusText = health == 0 ? "YOU WERE DEFEATED" : "ENEMY HIT";
            if (health == 0) {
                health = 10;
                physics.resetPlayer({spawnPosition.x, spawnPosition.y, spawnPosition.z});
                camera.position = spawnPosition;
                enemyPosition = {5.5f,
                    static_cast<float>(world.surfaceHeight(5, -7)), -6.5f};
                statusText = "RESPAWNED";
            }
        }

        ++fpsFrameCount;
        const double fpsInterval = static_cast<double>(now - fpsUpdateTime) /
                                   static_cast<double>(performanceFrequency);
        if (fpsInterval >= 1.0) {
            const int displayedFps = static_cast<int>(std::lround(
                static_cast<double>(fpsFrameCount) / fpsInterval));
            fpsText = "FPS: " + std::to_string(displayedFps);
            fpsFrameCount = 0;
            fpsUpdateTime = now;
        }

        int width = 0;
        int height = 0;
        if (!SDL_GetWindowSizeInPixels(window, &width, &height)) {
            std::cerr << "SDL_GetWindowSizeInPixels Error: " << SDL_GetError() << std::endl;
            succeeded = false;
            break;
        }
        if (width > 0 && height > 0) {
            const char* activeRenderer =
                backend == RendererBackend::Vulkan ? "ACTIVE VULKAN" : "ACTIVE OPENGL";
            renderScene(width, height, camera, cameraBasis, fpsText.c_str(), settingsOpen,
                        openGLAvailable, activeRenderer, world, enemyPosition, health,
                        inventory, selectedSlot, statusText.c_str(), sceneBuffers);
            if (!renderer->render(sceneBuffers.vertices, sceneBuffers.indices, rendererError)) {
                std::cerr << "Frame rendering failed: " << rendererError << std::endl;
                succeeded = false;
                break;
            }
        }
    }

    if (mouseCaptured) {
        SDL_SetWindowRelativeMouseMode(window, false);
    }
    renderer->shutdown();
    SDL_DestroyWindow(window);
    SDL_Quit();
    return succeeded;
}

} // namespace Engine
