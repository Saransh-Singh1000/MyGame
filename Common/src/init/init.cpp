#include "init.hpp"
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
    Point3D position{0.0f, 5.0f, -15.0f};
    float yaw = 0.0f;
    float pitch = -0.25f;
};

struct CameraBasis {
    Point3D forward;
    Point3D right;
    Point3D up;
};

struct Rotation {
    float xx, xy, xz;
    float yx, yy, yz;
    float zx, zy, zz;
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

struct SceneBuffers {
    std::vector<RenderFace> faces;
    std::vector<SDL_Vertex> vertices;
    std::vector<int> indices;

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

Rotation makeRotation(float angleX, float angleY, float cosZ, float sinZ) {
    const float cosX = std::cos(angleX);
    const float sinX = std::sin(angleX);
    const float cosY = std::cos(angleY);
    const float sinY = std::sin(angleY);

    return {
        cosY * cosZ,
        sinX * sinY * cosZ - cosX * sinZ,
        cosX * sinY * cosZ + sinX * sinZ,
        cosY * sinZ,
        sinX * sinY * sinZ + cosX * cosZ,
        cosX * sinY * sinZ - sinX * cosZ,
        -sinY,
        sinX * cosY,
        cosX * cosY
    };
}

Point3D rotatePoint(Point3D point, const Rotation& rotation) {
    return {
        point.x * rotation.xx + point.y * rotation.xy + point.z * rotation.xz,
        point.x * rotation.yx + point.y * rotation.yy + point.z * rotation.yz,
        point.x * rotation.zx + point.y * rotation.zy + point.z * rotation.zz
    };
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

void appendQuad(SceneBuffers& buffers, const std::array<Point2D, 4>& points, SDL_FColor color) {
    constexpr std::array<int, 6> indices{{0, 1, 2, 0, 2, 3}};
    const int firstVertex = static_cast<int>(buffers.vertices.size());
    for (const Point2D point : points) {
        SDL_Vertex vertex{};
        vertex.position = {point.x, point.y};
        vertex.color = color;
        buffers.vertices.push_back(vertex);
    }
    for (const int index : indices) {
        buffers.indices.push_back(firstVertex + index);
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

bool renderScene(SDL_Renderer* renderer, int width, int height, const Camera& camera,
                 const CameraBasis& basis, float elapsed, SceneBuffers& buffers) {
    const float focalLength = static_cast<float>(std::min(width, height)) * 0.8f;
    const float centerX = static_cast<float>(width) * 0.5f;
    const float centerY = static_cast<float>(height) * 0.5f;

    buffers.faces.clear();
    buffers.vertices.clear();
    buffers.indices.clear();

    const float angleZ = elapsed * 0.2f;
    const float cosZ = std::cos(angleZ);
    const float sinZ = std::sin(angleZ);

    for (int row = 0; row < 8; ++row) {
        for (int column = 0; column < 8; ++column) {
            const Point3D cubeCenter{
                (static_cast<float>(column) - 3.5f) * 1.8f,
                0.65f,
                (static_cast<float>(row) - 3.5f) * 1.8f
            };
            const float phase = static_cast<float>(row * 8 + column) * 0.11f;
            const float angleX = elapsed * 0.35f + phase;
            const float angleY = elapsed * 0.55f + phase;
            const Rotation rotation = makeRotation(angleX, angleY, cosZ, sinZ);

            std::array<Point2D, 8> projectedVertices{};
            std::array<float, 8> depths{};
            for (std::size_t i = 0; i < cubeVertices.size(); ++i) {
                const Point3D world = add(rotatePoint(cubeVertices[i], rotation), cubeCenter);
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
                RenderFace face{};
                face.color = faceColors[i];
                bool visible = true;
                for (std::size_t vertex = 0; vertex < face.vertices.size(); ++vertex) {
                    const int index = faceVertices[i][vertex];
                    if (depths[index] <= 0.1f) {
                        visible = false;
                        break;
                    }
                    face.vertices[vertex] = projectedVertices[index];
                    face.depth += depths[index] * 0.25f;
                }
                if (visible) {
                    buffers.faces.push_back(face);
                }
            }
        }
    }
    std::sort(buffers.faces.begin(), buffers.faces.end(),
              [](const RenderFace& left, const RenderFace& right) {
        return left.depth > right.depth;
    });

    if (!SDL_SetRenderDrawColor(renderer, 14, 18, 32, 255) || !SDL_RenderClear(renderer)) {
        return false;
    }

    constexpr SDL_FColor edgeColor{0.094f, 0.110f, 0.173f, 1.0f};
    for (const RenderFace& face : buffers.faces) {
        appendQuad(buffers, face.vertices, face.color);
        for (std::size_t i = 0; i < face.vertices.size(); ++i) {
            const Point2D from = face.vertices[i];
            const Point2D to = face.vertices[(i + 1) % face.vertices.size()];
            appendQuad(buffers, makeLineQuad(from, to), edgeColor);
        }
    }

    return buffers.indices.empty() ||
           SDL_RenderGeometry(renderer, nullptr, buffers.vertices.data(),
                              static_cast<int>(buffers.vertices.size()), buffers.indices.data(),
                              static_cast<int>(buffers.indices.size()));
}

void moveCamera(Camera& camera, const CameraBasis& basis, const bool* keys, float deltaTime) {
    const float forwardAmount = static_cast<float>(keys[SDL_SCANCODE_W]) -
                                static_cast<float>(keys[SDL_SCANCODE_S]);
    const float rightAmount = static_cast<float>(keys[SDL_SCANCODE_D]) -
                              static_cast<float>(keys[SDL_SCANCODE_A]);
    const float verticalAmount = static_cast<float>(keys[SDL_SCANCODE_SPACE]) -
                                 static_cast<float>(keys[SDL_SCANCODE_LCTRL] ||
                                                    keys[SDL_SCANCODE_RCTRL]);

    Point3D movement = add(add(scale(basis.forward, forwardAmount), scale(basis.right, rightAmount)),
                           {0.0f, verticalAmount, 0.0f});
    const float magnitude = std::sqrt(dot(movement, movement));
    if (magnitude > 0.0f) {
        const float speed = keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT] ? 18.0f : 6.0f;
        camera.position = add(camera.position, scale(movement, speed * deltaTime / magnitude));
    }
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
        SDL_WINDOW_RESIZABLE
    );

    if (!window) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return false;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }

    if (!SDL_SetRenderVSync(renderer, 0)) {
        std::cerr << "Could not disable renderer VSync: " << SDL_GetError() << std::endl;
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
    SceneBuffers sceneBuffers;
    constexpr float mouseSensitivity = 0.0025f;

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
        moveCamera(camera, cameraBasis, SDL_GetKeyboardState(nullptr), deltaTime);

        int width = 0;
        int height = 0;
        if (!SDL_GetCurrentRenderOutputSize(renderer, &width, &height)) {
            std::cerr << "SDL_GetCurrentRenderOutputSize Error: " << SDL_GetError() << std::endl;
            succeeded = false;
            break;
        }

        if (width > 0 && height > 0) {
            if (!renderScene(renderer, width, height, camera,
                             cameraBasis,
                             static_cast<float>(static_cast<double>(now - startTime) /
                                                static_cast<double>(performanceFrequency)),
                             sceneBuffers)) {
                std::cerr << "Scene rendering failed: " << SDL_GetError() << std::endl;
                succeeded = false;
                break;
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

            if (!SDL_SetRenderDrawColor(renderer, 240, 245, 255, 255) ||
                !SDL_RenderDebugText(renderer, 12.0f, 12.0f, fpsText.c_str()) ||
                !SDL_RenderDebugText(renderer, 12.0f, 28.0f,
                                     mouseCaptured
                                         ? "WASD move | Space/Ctrl up/down | Shift boost | Esc unlock"
                                         : "WASD move | Space/Ctrl up/down | Shift boost | Click to look")) {
                std::cerr << "FPS overlay rendering failed: " << SDL_GetError() << std::endl;
                succeeded = false;
                break;
            }

            if (!SDL_RenderPresent(renderer)) {
                std::cerr << "SDL_RenderPresent Error: " << SDL_GetError() << std::endl;
                succeeded = false;
                break;
            }
        }
    }

    if (mouseCaptured) {
        SDL_SetWindowRelativeMouseMode(window, false);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return succeeded;
}

} // namespace Engine
