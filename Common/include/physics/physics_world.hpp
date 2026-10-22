#ifndef PHYSICS_WORLD_HPP
#define PHYSICS_WORLD_HPP

#include <cstdint>
#include <memory>
#include <span>

namespace Physics {

struct Vector3 {
    float x;
    float y;
    float z;
};

struct BodyHandle {
    std::uint32_t value = 0xffffffffu;
    bool isValid() const;
};

struct Cube {
    BodyHandle body;
    Vector3 position;
    Vector3 halfExtents;
};

struct Sphere {
    BodyHandle body;
    Vector3 position;
    float radius;
};

class PhysicsWorld {
public:
    PhysicsWorld();
    ~PhysicsWorld();

    PhysicsWorld(const PhysicsWorld&) = delete;
    PhysicsWorld& operator=(const PhysicsWorld&) = delete;
    PhysicsWorld(PhysicsWorld&&) = delete;
    PhysicsWorld& operator=(PhysicsWorld&&) = delete;

    bool initialize();
    Cube createCube(Vector3 position, Vector3 halfExtents = {0.5f, 0.5f, 0.5f});
    BodyHandle createStaticCubeBatch(std::span<const Vector3> positions,
                                    Vector3 halfExtents = {0.5f, 0.5f, 0.5f});
    Sphere createSphere(Vector3 position, float radius = 0.5f);
    void createPlayer(Vector3 eyePosition);
    void setPlayerMovement(Vector3 horizontalVelocity, bool jump);
    void step(float deltaTime);
    Vector3 playerEyePosition() const;
    void resetPlayer(Vector3 eyePosition);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    bool runtimeAcquired_ = false;
};

} // namespace Physics

#endif // PHYSICS_WORLD_HPP
