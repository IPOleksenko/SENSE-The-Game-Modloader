#pragma once

#include "types.hpp"
#include <string>
#include <vector>
#include <functional>
#include <cmath>

namespace sense {

// ============================================================================
// 1. 3D Vector Math
// ============================================================================
struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    Vec3() = default;
    Vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

    Vec3 operator+(const Vec3& o) const { return Vec3(x + o.x, y + o.y, z + o.z); }
    Vec3 operator-(const Vec3& o) const { return Vec3(x - o.x, y - o.y, z - o.z); }
    Vec3 operator*(float s) const { return Vec3(x * s, y * s, z * s); }
    Vec3 operator/(float s) const { return (s != 0.0f) ? Vec3(x / s, y / s, z / s) : *this; }

    float length() const { return std::sqrt(x * x + y * y + z * z); }
    Vec3 normalized() const {
        float l = length();
        return (l > 0.0001f) ? (*this / l) : *this;
    }

    static float dot(const Vec3& a, const Vec3& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    static Vec3 cross(const Vec3& a, const Vec3& b) {
        return Vec3(
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        );
    }
};

// ============================================================================
// 2. 3D Mesh
// ============================================================================
struct Mesh3D {
    std::string name;
    std::vector<Vec3> vertices;
    std::vector<int> indices;      // Triplet indices (0, 1, 2)
    std::vector<Color> faceColors; // Optional color per triangle
};

// ============================================================================
// 3. 3D World Object
// ============================================================================
struct Object3D {
    int id = 0;
    std::string name;
    Mesh3D mesh;

    Vec3 position;      // X (-300..300 road width), Y (0..25000 track distance), Z (elevation)
    Vec3 rotation;      // Degrees: Pitch, Yaw, Roll
    Vec3 rotationSpeed; // Degrees per second (for auto-spin)
    Vec3 scale = { 1.0f, 1.0f, 1.0f };

    Color tint = Color::White();
    bool visible = true;
    bool wireframe = false;

    bool hasCollision = true;
    float collisionRadius = 35.0f;

    std::function<void(Object3D&, float dt)> onUpdate;
    std::function<void(Object3D&)> onCollideWithPlayer;
};

// ============================================================================
// 4. 2D World Entity
// ============================================================================
struct Entity2D {
    int id = 0;
    std::string name;

    float worldX = 0.0f; // Track horizontal offset (-300..300)
    float worldY = 0.0f; // Track distance (0..25000)
    float width = 64.0f;
    float height = 64.0f;
    float rotation = 0.0f;

    Color color = Color::White();
    SDL_Texture* texture = nullptr;
    std::string label;

    bool visible = true;
    bool hasCollision = true;
    float collisionRadius = 35.0f;

    std::function<void(Entity2D&, float dt)> onUpdate;
    std::function<void(Entity2D&)> onCollideWithPlayer;
};

// ============================================================================
// 5. Master Object & Entity Controller
// ============================================================================
class ObjectController {
public:
    static ObjectController& instance();

    // 2D Entity Management
    int spawnEntity2D(
        float worldX,
        float worldY,
        float width = 64.0f,
        float height = 64.0f,
        const Color& color = Color::White(),
        SDL_Texture* texture = nullptr,
        const std::string& label = ""
    );
    Entity2D* getEntity2D(int id);
    void removeEntity2D(int id);
    void clearEntities2D();
    const std::vector<Entity2D>& getEntities2D() const { return m_entities2D; }

    // 3D Object Management
    int spawnObject3D(
        const Mesh3D& mesh,
        const Vec3& position,
        const Vec3& rotation = { 0.0f, 0.0f, 0.0f },
        const Vec3& scale = { 1.0f, 1.0f, 1.0f },
        const Color& tint = Color::White()
    );
    int spawnCube(const Vec3& pos, float size = 40.0f, const Color& color = Color::Cyan());
    int spawnPyramid(const Vec3& pos, float width = 45.0f, float height = 55.0f, const Color& color = Color::Yellow());
    int spawnCoin(const Vec3& pos, float radius = 25.0f, const Color& color = Color(255, 215, 0));
    int spawnCrystal(const Vec3& pos, float radius = 30.0f, float height = 60.0f, const Color& color = Color::Magenta());
    int spawnCheckpointArch(float worldY, float width = 380.0f, float height = 220.0f, const Color& color = Color::Green());
    int spawnModelObj(const std::string& objFilePath, const Vec3& pos, const Vec3& scale = { 1.0f, 1.0f, 1.0f }, const Color& tint = Color::White());

    Object3D* getObject3D(int id);
    void removeObject3D(int id);
    void clearObjects3D();
    const std::vector<Object3D>& getObjects3D() const { return m_objects3D; }

    void clearAll();

    // Procedural Mesh Factories
    static Mesh3D makeCube(float size, const Color& color = Color::White());
    static Mesh3D makePyramid(float width, float height, const Color& color = Color::White());
    static Mesh3D makeCoin(float radius, float thickness = 8.0f, int segments = 12, const Color& color = Color(255, 215, 0));
    static Mesh3D makeCrystal(float radius, float height = 60.0f, const Color& color = Color::Magenta());
    static Mesh3D makeArch(float width, float height, float thickness = 25.0f, const Color& color = Color::Green());
    static Mesh3D loadObj(const std::string& filepath, const Color& defaultColor = Color::White());

    // Engine Lifecycle Hooks
    void update(float deltaTime);
    void render(SDL_Renderer* renderer, int screenW, int screenH);

private:
    ObjectController();

    int m_nextId = 1;
    std::vector<Entity2D> m_entities2D;
    std::vector<Object3D> m_objects3D;
};

// Convenience namespace functions
namespace objects {
    inline int spawnEntity2D(float x, float y, float w = 64.0f, float h = 64.0f, const Color& col = Color::White(), SDL_Texture* tex = nullptr, const std::string& lbl = "") {
        return ObjectController::instance().spawnEntity2D(x, y, w, h, col, tex, lbl);
    }
    inline int spawnObject3D(const Mesh3D& m, const Vec3& pos, const Vec3& rot = {0,0,0}, const Vec3& scale = {1,1,1}, const Color& tint = Color::White()) {
        return ObjectController::instance().spawnObject3D(m, pos, rot, scale, tint);
    }
    inline int spawnCube(const Vec3& pos, float sz = 40.0f, const Color& col = Color::Cyan()) { return ObjectController::instance().spawnCube(pos, sz, col); }
    inline int spawnPyramid(const Vec3& pos, float w = 45.0f, float h = 55.0f, const Color& col = Color::Yellow()) { return ObjectController::instance().spawnPyramid(pos, w, h, col); }
    inline int spawnCoin(const Vec3& pos, float r = 25.0f, const Color& col = Color(255, 215, 0)) { return ObjectController::instance().spawnCoin(pos, r, col); }
    inline int spawnCrystal(const Vec3& pos, float r = 30.0f, float h = 60.0f, const Color& col = Color::Magenta()) { return ObjectController::instance().spawnCrystal(pos, r, h, col); }
    inline int spawnArch(float y, float w = 380.0f, float h = 220.0f, const Color& col = Color::Green()) { return ObjectController::instance().spawnCheckpointArch(y, w, h, col); }
    inline void clearAll() { ObjectController::instance().clearAll(); }
}

} // namespace sense

