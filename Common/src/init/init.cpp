#include "init.hpp"
#include "renderer/vulkan_renderer.hpp"
#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <string>
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

struct PlayerPhysics {
    float verticalVelocity = 0.0f;
    bool grounded = true;
};

struct CameraBasis {
    Point3D forward;
    Point3D right;
    Point3D up;
};

constexpr std::array<Point3D, 8> cubeVertices{{
    {-0.55f, -0.55f, -0.55f},
    { 0.55f, -0.55f, -0.55f},
    { 0.55f,  0.55f, -0.55f},
    {-0.55f,  0.55f, -0.55f},
    {-0.55f, -0.55f,  0.55f},
    { 0.55f, -0.55f,  0.55f},
    { 0.55f,  0.55f,  0.55f},
    {-0.55f,  0.55f,  0.55f}
}};

constexpr std::array<std::array<int, 4>, 6> faceVertices{{
    {{0, 1, 2, 3}}, {{4, 7, 6, 5}},
    {{0, 4, 5, 1}}, {{3, 2, 6, 7}},
    {{0, 3, 7, 4}}, {{1, 5, 6, 2}}
}};

constexpr auto makeCubeCenters() {
    std::array<Point3D, 64> centers{};
    for (int row = 0; row < 8; ++row) {
        for (int column = 0; column < 8; ++column) {
            centers[static_cast<std::size_t>(row * 8 + column)] = {
                (static_cast<float>(column) - 3.5f) * 1.8f,
                0.65f,
                (static_cast<float>(row) - 3.5f) * 1.8f
            };
        }
    }
    return centers;
}

constexpr std::array<Point3D, 64> cubeCenters = makeCubeCenters();

constexpr Point3D cubeHalfExtents{0.55f, 0.55f, 0.55f};
constexpr float playerRadius = 0.25f;
constexpr float playerHeight = 1.8f;
constexpr float playerEyeHeight = 1.65f;
constexpr float gravity = 20.0f;
constexpr float jumpSpeed = 8.5f;
constexpr float fallThreshold = -50.0f;
constexpr Point3D spawnPosition{-0.9f, 2.85f, -6.3f};

constexpr std::array<Point3D, 6> faceNormals{{
    {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, 1.0f},
    {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 0.0f},
    {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}
}};

struct SceneBuffers {
    std::vector<RenderFace> faces;
    std::vector<VulkanVertex> vertices;
    std::vector<std::uint32_t> indices;

    SceneBuffers() {
        faces.reserve(8 * 8 * faceVertices.size());
        vertices.reserve(8 * 8 * faceVertices.size() * 5 * 4);
        indices.reserve(8 * 8 * faceVertices.size() * 5 * 6);
    }
};

constexpr std::array<SDL_FColor, 6> faceColors{{
    {0.92f, 0.33f, 0.33f, 1.0f}, {0.29f, 0.75f, 0.92f, 1.0f},
    {0.37f, 0.80f, 0.49f, 1.0f}, {0.96f, 0.75f, 0.27f, 1.0f},
    {0.65f, 0.45f, 0.92f, 1.0f}, {0.94f, 0.47f, 0.73f, 1.0f}
}};

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

Point3D cubeCenter(int row, int column) {
    return cubeCenters[static_cast<std::size_t>(row * 8 + column)];
}

bool overlapsCubeHorizontally(Point3D position, Point3D center) {
    return std::abs(position.x - center.x) < cubeHalfExtents.x + playerRadius &&
           std::abs(position.z - center.z) < cubeHalfExtents.z + playerRadius;
}

bool overlapsCubeVertically(Point3D position, Point3D center) {
    const float playerBottom = position.y - playerEyeHeight;
    const float playerTop = playerBottom + playerHeight;
    const float cubeBottom = center.y - cubeHalfExtents.y;
    const float cubeTop = center.y + cubeHalfExtents.y;
    return playerBottom < cubeTop - 0.001f && playerTop > cubeBottom + 0.001f;
}

bool collidesWithCubeSides(Point3D position) {
    for (int row = 0; row < 8; ++row) {
        for (int column = 0; column < 8; ++column) {
            const Point3D center = cubeCenter(row, column);
            if (overlapsCubeHorizontally(position, center) &&
                overlapsCubeVertically(position, center)) {
                return true;
            }
        }
    }
    return false;
}

bool hasCubeSupport(Point3D position) {
    const float playerFeet = position.y - playerEyeHeight;
    for (int row = 0; row < 8; ++row) {
        for (int column = 0; column < 8; ++column) {
            const Point3D center = cubeCenter(row, column);
            const float cubeTop = center.y + cubeHalfExtents.y;
            if (std::abs(playerFeet - cubeTop) <= 0.01f &&
                overlapsCubeHorizontally(position, center)) {
                return true;
            }
        }
    }
    return false;
}

void movePlayer(Camera& camera, PlayerPhysics& physics, const bool* keys,
                bool jumpPressed, float deltaTime) {
    const float forwardAmount = static_cast<float>(keys[SDL_SCANCODE_W]) -
                                static_cast<float>(keys[SDL_SCANCODE_S]);
    const float rightAmount = static_cast<float>(keys[SDL_SCANCODE_D]) -
                              static_cast<float>(keys[SDL_SCANCODE_A]);
    const float sinYaw = std::sin(camera.yaw);
    const float cosYaw = std::cos(camera.yaw);
    Point3D movement{
        sinYaw * forwardAmount + cosYaw * rightAmount,
        0.0f,
        cosYaw * forwardAmount - sinYaw * rightAmount
    };
    const float movementLength = std::sqrt(dot(movement, movement));
    const int steps = std::max(1, static_cast<int>(std::ceil(deltaTime * 60.0f)));
    const float stepDuration = deltaTime / static_cast<float>(steps);

    for (int step = 0; step < steps; ++step) {
        if (movementLength > 0.0f) {
            const float speed = keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT]
                                    ? 8.5f
                                    : 5.0f;
            const Point3D stepMovement =
                scale(movement, speed * stepDuration / movementLength);

            Point3D candidate = camera.position;
            candidate.x += stepMovement.x;
            if (!collidesWithCubeSides(candidate)) {
                camera.position.x = candidate.x;
            }

            candidate = camera.position;
            candidate.z += stepMovement.z;
            if (!collidesWithCubeSides(candidate)) {
                camera.position.z = candidate.z;
            }
        }

        if (jumpPressed && physics.grounded) {
            physics.verticalVelocity = jumpSpeed;
            physics.grounded = false;
        }
        jumpPressed = false;

        if (physics.grounded && !hasCubeSupport(camera.position)) {
            physics.grounded = false;
        }

        if (!physics.grounded) {
            const float previousFeet = camera.position.y - playerEyeHeight;
            physics.verticalVelocity -= gravity * stepDuration;
            const float nextFeet = previousFeet + physics.verticalVelocity * stepDuration;
            bool collided = false;

            for (int row = 0; row < 8; ++row) {
                for (int column = 0; column < 8; ++column) {
                    const Point3D center = cubeCenter(row, column);
                    const float cubeTop = center.y + cubeHalfExtents.y;
                    if (physics.verticalVelocity <= 0.0f && previousFeet >= cubeTop &&
                        nextFeet <= cubeTop &&
                        overlapsCubeHorizontally(camera.position, center)) {
                        camera.position.y = cubeTop + playerEyeHeight;
                        physics.verticalVelocity = 0.0f;
                        physics.grounded = true;
                        collided = true;
                        break;
                    }

                    const float cubeBottom = center.y - cubeHalfExtents.y;
                    const float previousHead = previousFeet + playerHeight;
                    const float nextHead = nextFeet + playerHeight;
                    if (physics.verticalVelocity > 0.0f && previousHead <= cubeBottom &&
                        nextHead >= cubeBottom &&
                        overlapsCubeHorizontally(camera.position, center)) {
                        camera.position.y = cubeBottom - (playerHeight - playerEyeHeight);
                        physics.verticalVelocity = 0.0f;
                        collided = true;
                        break;
                    }
                }
                if (collided) {
                    break;
                }
            }

            if (!collided) {
                camera.position.y = nextFeet + playerEyeHeight;
            }
        }

        if (camera.position.y - playerEyeHeight < fallThreshold) {
            camera.position = spawnPosition;
            physics.verticalVelocity = 0.0f;
            physics.grounded = true;
        }
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
                 const char* fpsText, SceneBuffers& buffers) {
    const float focalLength = static_cast<float>(std::min(width, height)) * 0.8f;
    const float centerX = static_cast<float>(width) * 0.5f;
    const float centerY = static_cast<float>(height) * 0.5f;

    buffers.faces.clear();
    buffers.vertices.clear();
    buffers.indices.clear();

    for (int row = 0; row < 8; ++row) {
        for (int column = 0; column < 8; ++column) {
            const Point3D center = cubeCenters[static_cast<std::size_t>(row * 8 + column)];
            const Point3D cameraRelative = subtract(camera.position, center);
            const bool cullBackFaces = dot(cameraRelative, cameraRelative) > 1.0f;

            std::array<Point2D, 8> projectedVertices{};
            std::array<float, 8> depths{};
            for (std::size_t i = 0; i < cubeVertices.size(); ++i) {
                const Point3D world = add(cubeVertices[i], center);
                const Point3D relative = subtract(world, camera.position);
                depths[i] = dot(relative, basis.forward);
                if (depths[i] > 0.1f) {
                    projectedVertices[i] = {
                        centerX + dot(relative, basis.right) * focalLength / depths[i],
                        centerY - dot(relative, basis.up) * focalLength / depths[i]
                    };
                }
            }

            for (std::size_t i = 0; i < faceVertices.size(); ++i) {
                if (cullBackFaces && dot(faceNormals[i], cameraRelative) <= 0.0f) {
                    continue;
                }
                RenderFace face{};
                face.color = faceColors[i];
                bool visible = true;
                float depthAccumulator = 0.0f;
                for (std::size_t vertex = 0; vertex < face.vertices.size(); ++vertex) {
                    const int index = faceVertices[i][vertex];
                    if (depths[index] <= 0.1f) {
                        visible = false;
                        break;
                    }
                    face.vertices[vertex] = projectedVertices[index];
                    depthAccumulator += depths[index];
                }
                if (visible) {
                    face.depth = depthAccumulator * 0.25f;
                    buffers.faces.push_back(face);
                }
            }
        }
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
               "WASD MOVE | SPACE JUMP | SHIFT RUN | CLICK LOOK | ESC UNLOCK",
               12.0f, 28.0f, 1.0f, {0.94f, 0.96f, 1.0f, 1.0f});
}

} // namespace

namespace Engine {

bool run() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Engine Window",
        1280, 720,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN
    );

    if (!window) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return false;
    }

    VulkanRenderer renderer;
    std::string rendererError;
    if (!renderer.initialize(window, rendererError)) {
        std::cerr << "Vulkan renderer initialization failed: " << rendererError << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }

    bool isRunning = true;
    bool succeeded = true;
    bool mouseCaptured = false;
    SDL_Event event;
    const std::uint64_t performanceFrequency = SDL_GetPerformanceFrequency();
    const std::uint64_t startTime = SDL_GetPerformanceCounter();
    std::uint64_t previousFrame = startTime;
    std::uint64_t fpsUpdateTime = previousFrame;
    std::uint64_t fpsFrameCount = 0;
    std::string fpsText = "FPS: 0";
    Camera camera;
    PlayerPhysics playerPhysics;
    SceneBuffers sceneBuffers;
    constexpr float mouseSensitivity = 0.0025f;
    bool jumpPressed = false;

    while (isRunning) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
                isRunning = false;
            } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && !mouseCaptured) {
                mouseCaptured = SDL_SetWindowRelativeMouseMode(window, true);
                if (!mouseCaptured) {
                    std::cerr << "Could not capture mouse: " << SDL_GetError() << std::endl;
                }
            } else if (event.type == SDL_EVENT_MOUSE_MOTION && mouseCaptured) {
                camera.yaw += event.motion.xrel * mouseSensitivity;
                camera.pitch = std::clamp(camera.pitch - event.motion.yrel * mouseSensitivity,
                                          -1.5f, 1.5f);
            } else if (event.type == SDL_EVENT_KEY_DOWN &&
                       event.key.scancode == SDL_SCANCODE_ESCAPE && mouseCaptured) {
                if (SDL_SetWindowRelativeMouseMode(window, false)) {
                    mouseCaptured = false;
                } else {
                    std::cerr << "Could not release mouse: " << SDL_GetError() << std::endl;
                }
            } else if (event.type == SDL_EVENT_KEY_DOWN &&
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
        movePlayer(camera, playerPhysics, SDL_GetKeyboardState(nullptr),
                   jumpPressed, deltaTime);
        jumpPressed = false;

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
            renderScene(width, height, camera, cameraBasis, fpsText.c_str(), sceneBuffers);
            if (!renderer.render(sceneBuffers.vertices, sceneBuffers.indices, rendererError)) {
                std::cerr << "Vulkan frame rendering failed: " << rendererError << std::endl;
                succeeded = false;
                break;
            }
        }
    }

    if (mouseCaptured) {
        SDL_SetWindowRelativeMouseMode(window, false);
    }
    renderer.shutdown();
    SDL_DestroyWindow(window);
    SDL_Quit();
    return succeeded;
}

} // namespace Engine
