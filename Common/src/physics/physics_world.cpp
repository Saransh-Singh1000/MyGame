#include "physics/physics_world.hpp"

#include <Jolt/Jolt.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Collision/ContactListener.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>
#include <algorithm>
#include <atomic>
#include <cstddef>
#include <mutex>
#include <stdexcept>
#include <thread>

namespace Physics {
namespace {

constexpr JPH::ObjectLayer objectLayer = 0;
constexpr float playerRadius = 0.25f;
constexpr float playerHeight = 1.8f;
constexpr float playerEyeHeight = 1.65f;
constexpr float jumpSpeed = 8.5f;
constexpr float fixedTimeStep = 1.0f / 60.0f;
std::mutex runtimeMutex;
std::size_t runtimeUsers = 0;
bool allocatorRegistered = false;
bool ownsFactory = false;

void acquireJoltRuntime() {
    std::lock_guard<std::mutex> lock(runtimeMutex);
    if (!allocatorRegistered) {
        JPH::RegisterDefaultAllocator();
        allocatorRegistered = true;
    }
    if (JPH::Factory::sInstance == nullptr) {
        JPH::Factory::sInstance = new JPH::Factory();
        JPH::RegisterTypes();
        ownsFactory = true;
    }
    ++runtimeUsers;
}

void releaseJoltRuntime() {
    std::lock_guard<std::mutex> lock(runtimeMutex);
    if (--runtimeUsers == 0 && ownsFactory) {
        JPH::UnregisterTypes();
        delete JPH::Factory::sInstance;
        JPH::Factory::sInstance = nullptr;
        ownsFactory = false;
    }
}

int physicsWorkerCount() {
    const unsigned int hardwareThreads = std::thread::hardware_concurrency();
    return static_cast<int>(hardwareThreads > 1 ? hardwareThreads - 1 : 1);
}

class BroadPhaseLayers final : public JPH::BroadPhaseLayerInterface {
public:
    JPH::uint GetNumBroadPhaseLayers() const override { return 1; }
    JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer) const override {
        return JPH::BroadPhaseLayer(0);
    }
#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer) const override {
        return "Objects";
    }
#endif
};

class ObjectVsBroadPhaseFilter final : public JPH::ObjectVsBroadPhaseLayerFilter {
public:
    bool ShouldCollide(JPH::ObjectLayer, JPH::BroadPhaseLayer) const override { return true; }
};

class ObjectLayerPairFilter final : public JPH::ObjectLayerPairFilter {
public:
    bool ShouldCollide(JPH::ObjectLayer, JPH::ObjectLayer) const override { return true; }
};

class PlayerContactListener final : public JPH::ContactListener {
public:
    void setPlayer(JPH::BodyID bodyId) { playerBodyId_ = bodyId; }
    void resetGrounded() { grounded_.store(false, std::memory_order_relaxed); }
    bool isGrounded() const { return grounded_.load(std::memory_order_relaxed); }

    void OnContactAdded(const JPH::Body& body1, const JPH::Body& body2,
                        const JPH::ContactManifold& manifold,
                        JPH::ContactSettings&) override {
        updateGrounded(body1, body2, manifold);
    }

    void OnContactPersisted(const JPH::Body& body1, const JPH::Body& body2,
                            const JPH::ContactManifold& manifold,
                            JPH::ContactSettings&) override {
        updateGrounded(body1, body2, manifold);
    }

private:
    void updateGrounded(const JPH::Body& body1, const JPH::Body& body2,
                        const JPH::ContactManifold& manifold) {
        if (body1.GetID() == playerBodyId_ && manifold.mWorldSpaceNormal.GetY() < -0.5f) {
            grounded_.store(true, std::memory_order_relaxed);
        } else if (body2.GetID() == playerBodyId_ &&
                   manifold.mWorldSpaceNormal.GetY() > 0.5f) {
            grounded_.store(true, std::memory_order_relaxed);
        }
    }

    JPH::BodyID playerBodyId_;
    std::atomic<bool> grounded_{false};
};

JPH::RVec3 toJolt(Vector3 value) {
    return {value.x, value.y, value.z};
}

} // namespace

bool BodyHandle::isValid() const {
    return value != JPH::BodyID().GetIndexAndSequenceNumber();
}

struct PhysicsWorld::Impl {
    BroadPhaseLayers broadPhaseLayers;
    ObjectVsBroadPhaseFilter objectVsBroadPhaseFilter;
    ObjectLayerPairFilter objectLayerPairFilter;
    JPH::PhysicsSystem physicsSystem;
    JPH::TempAllocatorImpl tempAllocator{16 * 1024 * 1024};
    JPH::JobSystemThreadPool jobSystem{
        JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, physicsWorkerCount()};
    PlayerContactListener contactListener;
    JPH::BodyID playerBodyId;
    float accumulatedTime = 0.0f;
    bool initialized = false;

    bool initialize() {
        if (initialized) {
            return true;
        }

        physicsSystem.Init(65536, 0, 65536, 10240, broadPhaseLayers,
                           objectVsBroadPhaseFilter, objectLayerPairFilter);
        physicsSystem.SetGravity(JPH::Vec3(0.0f, -20.0f, 0.0f));
        physicsSystem.SetContactListener(&contactListener);
        initialized = true;
        return true;
    }

    JPH::BodyID addBody(const JPH::Shape* shape, Vector3 position,
                        JPH::EMotionType motionType, bool activate) {
        if (!initialized || shape == nullptr) {
            throw std::logic_error("Physics world must be initialized and shape must be valid");
        }
        JPH::BodyCreationSettings settings(shape, toJolt(position), JPH::Quat::sIdentity(),
                                          motionType, objectLayer);
        JPH::BodyInterface& bodyInterface = physicsSystem.GetBodyInterface();
        const JPH::BodyID bodyId = bodyInterface.CreateAndAddBody(
            settings, activate ? JPH::EActivation::Activate : JPH::EActivation::DontActivate);
        if (bodyId.IsInvalid()) {
            throw std::runtime_error("Jolt could not create a physics body");
        }
        return bodyId;
    }
};

PhysicsWorld::PhysicsWorld() {
    acquireJoltRuntime();
    runtimeAcquired_ = true;
    impl_ = std::make_unique<Impl>();
}

PhysicsWorld::~PhysicsWorld() {
    impl_.reset();
    if (runtimeAcquired_) {
        releaseJoltRuntime();
    }
}

bool PhysicsWorld::initialize() {
    return impl_->initialize();
}

Cube PhysicsWorld::createCube(Vector3 position, Vector3 halfExtents) {
    if (halfExtents.x <= 0.0f || halfExtents.y <= 0.0f || halfExtents.z <= 0.0f) {
        throw std::invalid_argument("Cube half-extents must be positive");
    }
    const JPH::RefConst<JPH::Shape> shape =
        new JPH::BoxShape(JPH::Vec3(halfExtents.x, halfExtents.y, halfExtents.z));
    const JPH::BodyID bodyId = impl_->addBody(shape.GetPtr(), position,
                                              JPH::EMotionType::Static, false);
    return {{bodyId.GetIndexAndSequenceNumber()}, position, halfExtents};
}

Sphere PhysicsWorld::createSphere(Vector3 position, float radius) {
    if (radius <= 0.0f) {
        throw std::invalid_argument("Sphere radius must be positive");
    }
    const JPH::RefConst<JPH::Shape> shape = new JPH::SphereShape(radius);
    const JPH::BodyID bodyId = impl_->addBody(shape.GetPtr(), position,
                                              JPH::EMotionType::Dynamic, true);
    return {{bodyId.GetIndexAndSequenceNumber()}, position, radius};
}

void PhysicsWorld::createPlayer(Vector3 eyePosition) {
    const float capsuleHalfHeight = (playerHeight - 2.0f * playerRadius) * 0.5f;
    const JPH::RefConst<JPH::Shape> shape =
        new JPH::CapsuleShape(capsuleHalfHeight, playerRadius);
    const Vector3 center{eyePosition.x,
                         eyePosition.y - playerEyeHeight + playerHeight * 0.5f,
                         eyePosition.z};
    JPH::BodyCreationSettings settings(shape.GetPtr(), toJolt(center), JPH::Quat::sIdentity(),
                                      JPH::EMotionType::Dynamic, objectLayer);
    settings.mAllowedDOFs = JPH::EAllowedDOFs::TranslationX |
                            JPH::EAllowedDOFs::TranslationY |
                            JPH::EAllowedDOFs::TranslationZ;
    settings.mFriction = 0.0f;
    settings.mRestitution = 0.0f;
    settings.mLinearDamping = 0.0f;
    settings.mAllowSleeping = false;
    settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
    settings.mMassPropertiesOverride.mMass = 1.0f;

    JPH::BodyInterface& bodyInterface = impl_->physicsSystem.GetBodyInterface();
    impl_->playerBodyId = bodyInterface.CreateAndAddBody(settings, JPH::EActivation::Activate);
    if (impl_->playerBodyId.IsInvalid()) {
        throw std::runtime_error("Jolt could not create the player body");
    }
    impl_->contactListener.setPlayer(impl_->playerBodyId);
}

void PhysicsWorld::setPlayerMovement(Vector3 horizontalVelocity, bool jump) {
    if (impl_->playerBodyId.IsInvalid()) {
        throw std::logic_error("Player body has not been created");
    }
    JPH::BodyInterface& bodyInterface = impl_->physicsSystem.GetBodyInterface();
    const JPH::Vec3 currentVelocity = bodyInterface.GetLinearVelocity(impl_->playerBodyId);
    const float verticalVelocity =
        jump && impl_->contactListener.isGrounded() ? jumpSpeed : currentVelocity.GetY();
    bodyInterface.SetLinearVelocity(impl_->playerBodyId,
        JPH::Vec3(horizontalVelocity.x, verticalVelocity, horizontalVelocity.z));
}

void PhysicsWorld::step(float deltaTime) {
    impl_->accumulatedTime += std::clamp(deltaTime, 0.0f, 0.1f);
    int steps = 0;
    while (impl_->accumulatedTime >= fixedTimeStep && steps < 6) {
        if (steps == 0) {
            impl_->contactListener.resetGrounded();
        }
        impl_->physicsSystem.Update(fixedTimeStep, 1, &impl_->tempAllocator, &impl_->jobSystem);
        impl_->accumulatedTime -= fixedTimeStep;
        ++steps;
    }
}

Vector3 PhysicsWorld::playerEyePosition() const {
    if (impl_->playerBodyId.IsInvalid()) {
        throw std::logic_error("Player body has not been created");
    }
    const JPH::RVec3 center =
        impl_->physicsSystem.GetBodyInterface().GetPosition(impl_->playerBodyId);
    return {static_cast<float>(center.GetX()),
            static_cast<float>(center.GetY()) + playerEyeHeight - playerHeight * 0.5f,
            static_cast<float>(center.GetZ())};
}

void PhysicsWorld::resetPlayer(Vector3 eyePosition) {
    if (impl_->playerBodyId.IsInvalid()) {
        throw std::logic_error("Player body has not been created");
    }
    const Vector3 center{eyePosition.x,
                         eyePosition.y - playerEyeHeight + playerHeight * 0.5f,
                         eyePosition.z};
    JPH::BodyInterface& bodyInterface = impl_->physicsSystem.GetBodyInterface();
    bodyInterface.SetPosition(impl_->playerBodyId, toJolt(center), JPH::EActivation::Activate);
    bodyInterface.SetLinearVelocity(impl_->playerBodyId, JPH::Vec3::sZero());
    impl_->contactListener.resetGrounded();
}

} // namespace Physics
