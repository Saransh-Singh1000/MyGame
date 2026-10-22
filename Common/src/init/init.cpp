#include "init.hpp"
#include "physics/physics_world.hpp"
#include "renderer/opengl_renderer.hpp"
#include "renderer/renderer.hpp"
#include "renderer/vulkan_renderer.hpp"
#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
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

constexpr std::array<Point3D, 8> cubeVertices{{
    {-0.49f, -0.5f, -0.49f},
    { 0.49f, -0.5f, -0.49f},
    { 0.49f,  0.5f, -0.49f},
    {-0.49f,  0.5f, -0.49f},
    {-0.49f, -0.5f,  0.49f},
    { 0.49f, -0.5f,  0.49f},
    { 0.49f,  0.5f,  0.49f},
    {-0.49f,  0.5f,  0.49f}
}};

constexpr std::array<std::array<int, 4>, 6> faceVertices{{
    {{0, 1, 2, 3}}, {{4, 7, 6, 5}},
    {{0, 4, 5, 1}}, {{3, 2, 6, 7}},
    {{0, 3, 7, 4}}, {{1, 5, 6, 2}}
}};

constexpr float fallThreshold = -50.0f;
constexpr float playerEyeHeight = 1.65f;
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
        faces.reserve(4225 * faceVertices.size());
        vertices.reserve(4225 * faceVertices.size() * 5 * 4);
        indices.reserve(4225 * faceVertices.size() * 5 * 6);
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

std::unique_ptr<Renderer> createRenderer(RendererBackend backend) {
    if (backend == RendererBackend::OpenGL) {
        return std::make_unique<OpenGLRenderer>();
    }
    return std::make_unique<VulkanRenderer>();
}

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

void movePlayer(Physics::PhysicsWorld& physics, Camera& camera, const bool* keys,
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
    const float speed = keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT]
                            ? 8.5f
                            : 5.0f;
    if (movementLength > 0.0f) {
        movement = scale(movement, speed / movementLength);
    }
    physics.setPlayerMovement({movement.x, 0.0f, movement.z}, jumpPressed);
    physics.step(deltaTime);
    camera.position = physics.playerEyePosition();

    if (camera.position.y - playerEyeHeight < fallThreshold) {
        physics.resetPlayer(spawnPosition);
        camera.position = spawnPosition;
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
                 SceneBuffers& buffers) {
    const float focalLength = static_cast<float>(std::min(width, height)) * 0.8f;
    const float centerX = static_cast<float>(width) * 0.5f;
    const float centerY = static_cast<float>(height) * 0.5f;

    buffers.faces.clear();
    buffers.vertices.clear();
    buffers.indices.clear();

    constexpr int platformTileCount = 1000;
    constexpr int renderRadius = 32;
    const int centerColumn = std::clamp(
        static_cast<int>(std::floor(camera.position.x + platformTileCount * 0.5f)),
        0, platformTileCount - 1);
    const int centerRow = std::clamp(
        static_cast<int>(std::floor(camera.position.z + platformTileCount * 0.5f)),
        0, platformTileCount - 1);
    const int firstColumn = std::max(0, centerColumn - renderRadius);
    const int lastColumn = std::min(platformTileCount - 1, centerColumn + renderRadius);
    const int firstRow = std::max(0, centerRow - renderRadius);
    const int lastRow = std::min(platformTileCount - 1, centerRow + renderRadius);

    for (int row = firstRow; row <= lastRow; ++row) {
        for (int column = firstColumn; column <= lastColumn; ++column) {
            const Point3D center{static_cast<float>(column) - 499.5f, 0.7f,
                                 static_cast<float>(row) - 499.5f};
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
               "WASD MOVE | SPACE JUMP | SHIFT RUN | CLICK LOOK | ESC SETTINGS",
               12.0f, 28.0f, 1.0f, {0.94f, 0.96f, 1.0f, 1.0f});
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

bool run(bool useOpenGL) {
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

    std::unique_ptr<Renderer> renderer = createRenderer(backend);
    if (!renderer->initialize(window, rendererError)) {
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
    Camera camera;
    Physics::PhysicsWorld physics;
    if (!physics.initialize()) {
        std::cerr << "Jolt Physics initialization failed" << std::endl;
        renderer->shutdown();
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }
    physics.createCube({0.0f, 0.7f, 0.0f}, {500.0f, 0.5f, 500.0f});
    physics.createPlayer(spawnPosition);
    SceneBuffers sceneBuffers;
    constexpr float mouseSensitivity = 0.0025f;
    bool jumpPressed = false;

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

        std::unique_ptr<Renderer> nextRenderer = createRenderer(nextBackend);
        if (!nextRenderer->initialize(nextWindow, rendererError)) {
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
            } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && !mouseCaptured &&
                       !settingsOpen) {
                mouseCaptured = SDL_SetWindowRelativeMouseMode(window, true);
                if (!mouseCaptured) {
                    std::cerr << "Could not capture mouse: " << SDL_GetError() << std::endl;
                }
            } else if (event.type == SDL_EVENT_MOUSE_MOTION && mouseCaptured) {
                camera.yaw += event.motion.xrel * mouseSensitivity;
                camera.pitch = std::clamp(camera.pitch - event.motion.yrel * mouseSensitivity,
                                          -1.5f, 1.5f);
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
            movePlayer(physics, camera, SDL_GetKeyboardState(nullptr), jumpPressed, deltaTime);
        }
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
            const char* activeRenderer =
                backend == RendererBackend::Vulkan ? "ACTIVE VULKAN" : "ACTIVE OPENGL";
            renderScene(width, height, camera, cameraBasis, fpsText.c_str(), settingsOpen,
                        openGLAvailable, activeRenderer, sceneBuffers);
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
