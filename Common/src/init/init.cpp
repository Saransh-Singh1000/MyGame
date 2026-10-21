#include "init.hpp"
#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>

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

struct CubeFace {
    std::array<int, 4> vertices;
    SDL_FColor color;
    float depth;
};

Point3D rotatePoint(Point3D point, float angleX, float angleY, float angleZ) {
    const float cosX = std::cos(angleX);
    const float sinX = std::sin(angleX);
    const float cosY = std::cos(angleY);
    const float sinY = std::sin(angleY);
    const float cosZ = std::cos(angleZ);
    const float sinZ = std::sin(angleZ);

    const float yAfterX = point.y * cosX - point.z * sinX;
    const float zAfterX = point.y * sinX + point.z * cosX;
    const float xAfterY = point.x * cosY + zAfterX * sinY;
    const float zAfterY = -point.x * sinY + zAfterX * cosY;

    return {
        xAfterY * cosZ - yAfterX * sinZ,
        xAfterY * sinZ + yAfterX * cosZ,
        zAfterY
    };
}

bool renderCube(SDL_Renderer* renderer, int width, int height, float elapsed) {
    constexpr std::array<Point3D, 8> cubeVertices{{
        {-1.0f, -1.0f, -1.0f},
        { 1.0f, -1.0f, -1.0f},
        { 1.0f,  1.0f, -1.0f},
        {-1.0f,  1.0f, -1.0f},
        {-1.0f, -1.0f,  1.0f},
        { 1.0f, -1.0f,  1.0f},
        { 1.0f,  1.0f,  1.0f},
        {-1.0f,  1.0f,  1.0f}
    }};
    constexpr std::array<std::array<int, 4>, 6> faceVertices{{
        {{0, 1, 2, 3}}, {{4, 7, 6, 5}},
        {{0, 4, 5, 1}}, {{3, 2, 6, 7}},
        {{0, 3, 7, 4}}, {{1, 5, 6, 2}}
    }};
    constexpr std::array<SDL_FColor, 6> faceColors{{
        {0.92f, 0.33f, 0.33f, 1.0f}, {0.29f, 0.75f, 0.92f, 1.0f},
        {0.37f, 0.80f, 0.49f, 1.0f}, {0.96f, 0.75f, 0.27f, 1.0f},
        {0.65f, 0.45f, 0.92f, 1.0f}, {0.94f, 0.47f, 0.73f, 1.0f}
    }};
    constexpr std::array<int, 6> indices{{0, 1, 2, 0, 2, 3}};
    constexpr float cameraDistance = 4.5f;

    const float angleX = elapsed * 0.75f;
    const float angleY = elapsed * 1.0f;
    const float angleZ = elapsed * 0.35f;
    const float focalLength = static_cast<float>(std::min(width, height)) * 0.8f;
    const float centerX = static_cast<float>(width) * 0.5f;
    const float centerY = static_cast<float>(height) * 0.5f;

    std::array<Point3D, 8> transformedVertices{};
    std::array<Point2D, 8> projectedVertices{};
    for (std::size_t i = 0; i < cubeVertices.size(); ++i) {
        transformedVertices[i] = rotatePoint(cubeVertices[i], angleX, angleY, angleZ);
        const float depth = cameraDistance - transformedVertices[i].z;
        projectedVertices[i] = {
            centerX + transformedVertices[i].x * focalLength / depth,
            centerY - transformedVertices[i].y * focalLength / depth
        };
    }

    std::array<CubeFace, 6> faces{};
    for (std::size_t i = 0; i < faces.size(); ++i) {
        float depth = 0.0f;
        for (const int vertex : faceVertices[i]) {
            depth += cameraDistance - transformedVertices[vertex].z;
        }
        faces[i] = {faceVertices[i], faceColors[i], depth * 0.25f};
    }
    std::sort(faces.begin(), faces.end(), [](const CubeFace& left, const CubeFace& right) {
        return left.depth > right.depth;
    });

    if (!SDL_SetRenderDrawColor(renderer, 14, 18, 32, 255) || !SDL_RenderClear(renderer)) {
        return false;
    }

    for (const CubeFace& face : faces) {
        std::array<SDL_Vertex, 4> vertices{};
        for (std::size_t i = 0; i < vertices.size(); ++i) {
            const Point2D point = projectedVertices[face.vertices[i]];
            vertices[i].position = {point.x, point.y};
            vertices[i].color = face.color;
        }

        if (!SDL_RenderGeometry(renderer, nullptr, vertices.data(),
                                static_cast<int>(vertices.size()), indices.data(),
                                static_cast<int>(indices.size()))) {
            return false;
        }

        if (!SDL_SetRenderDrawColor(renderer, 24, 28, 44, 255)) {
            return false;
        }
        for (std::size_t i = 0; i < vertices.size(); ++i) {
            const Point2D from = projectedVertices[face.vertices[i]];
            const Point2D to = projectedVertices[face.vertices[(i + 1) % vertices.size()]];
            if (!SDL_RenderLine(renderer, from.x, from.y, to.x, to.y)) {
                return false;
            }
        }
    }

    return true;
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

    bool isRunning = true;
    bool succeeded = true;
    SDL_Event event;
    const std::uint64_t startTime = SDL_GetPerformanceCounter();
    const std::uint64_t performanceFrequency = SDL_GetPerformanceFrequency();

    while (isRunning) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
                isRunning = false;
            }
        }

        if (!isRunning) {
            break;
        }

        int width = 0;
        int height = 0;
        if (!SDL_GetCurrentRenderOutputSize(renderer, &width, &height)) {
            std::cerr << "SDL_GetCurrentRenderOutputSize Error: " << SDL_GetError() << std::endl;
            succeeded = false;
            break;
        }

        if (width > 0 && height > 0) {
            const float elapsed = static_cast<float>(
                static_cast<double>(SDL_GetPerformanceCounter() - startTime) /
                static_cast<double>(performanceFrequency));
            if (!renderCube(renderer, width, height, elapsed)) {
                std::cerr << "Cube rendering failed: " << SDL_GetError() << std::endl;
                succeeded = false;
                break;
            }
            if (!SDL_RenderPresent(renderer)) {
                std::cerr << "SDL_RenderPresent Error: " << SDL_GetError() << std::endl;
                succeeded = false;
                break;
            }
        }

        SDL_Delay(1);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return succeeded;
}

} // namespace Engine
