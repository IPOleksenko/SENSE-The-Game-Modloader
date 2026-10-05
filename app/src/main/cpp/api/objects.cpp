#include "objects.hpp"
#include "player.hpp"
#include "render.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>

namespace sense {

ObjectController& ObjectController::instance() {
    static ObjectController inst;
    return inst;
}

ObjectController::ObjectController() = default;

// 2D Entity Management
int ObjectController::spawnEntity2D(
    float worldX,
    float worldY,
    float width,
    float height,
    const Color& color,
    SDL_Texture* texture,
    const std::string& label
) {
    Entity2D ent;
    ent.id = m_nextId++;
    ent.name = "Entity2D_" + std::to_string(ent.id);
    ent.worldX = worldX;
    ent.worldY = worldY;
    ent.width = width;
    ent.height = height;
    ent.color = color;
    ent.texture = texture;
    ent.label = label;
    m_entities2D.push_back(ent);
    return ent.id;
}

Entity2D* ObjectController::getEntity2D(int id) {
    for (auto& e : m_entities2D) {
        if (e.id == id) return &e;
    }
    return nullptr;
}

void ObjectController::removeEntity2D(int id) {
    m_entities2D.erase(
        std::remove_if(m_entities2D.begin(), m_entities2D.end(), [id](const Entity2D& e) { return e.id == id; }),
        m_entities2D.end()
    );
}

void ObjectController::clearEntities2D() {
    m_entities2D.clear();
}

// 3D Object Management
int ObjectController::spawnObject3D(
    const Mesh3D& mesh,
    const Vec3& position,
    const Vec3& rotation,
    const Vec3& scale,
    const Color& tint
) {
    Object3D obj;
    obj.id = m_nextId++;
    obj.name = "Object3D_" + std::to_string(obj.id);
    obj.mesh = mesh;
    obj.position = position;
    obj.rotation = rotation;
    obj.scale = scale;
    obj.tint = tint;
    obj.rotationSpeed = Vec3(0.0f, 45.0f, 0.0f);
    m_objects3D.push_back(obj);
    return obj.id;
}

int ObjectController::spawnCube(const Vec3& pos, float size, const Color& color) {
    return spawnObject3D(makeCube(size, color), pos, { 0, 0, 0 }, { 1, 1, 1 }, color);
}

int ObjectController::spawnPyramid(const Vec3& pos, float width, float height, const Color& color) {
    return spawnObject3D(makePyramid(width, height, color), pos, { 0, 0, 0 }, { 1, 1, 1 }, color);
}

int ObjectController::spawnCoin(const Vec3& pos, float radius, const Color& color) {
    int id = spawnObject3D(makeCoin(radius, 8.0f, 12, color), pos, { 0, 0, 0 }, { 1, 1, 1 }, color);
    auto* obj = getObject3D(id);
    if (obj) {
        obj->rotationSpeed = Vec3(0.0f, 90.0f, 0.0f);
    }
    return id;
}

int ObjectController::spawnCrystal(const Vec3& pos, float radius, float height, const Color& color) {
    int id = spawnObject3D(makeCrystal(radius, height, color), pos, { 0, 0, 0 }, { 1, 1, 1 }, color);
    auto* obj = getObject3D(id);
    if (obj) {
        obj->rotationSpeed = Vec3(25.0f, 60.0f, 0.0f);
    }
    return id;
}

int ObjectController::spawnCheckpointArch(float worldY, float width, float height, const Color& color) {
    int id = spawnObject3D(makeArch(width, height, 25.0f, color), { 0.0f, worldY, 0.0f }, { 0, 0, 0 }, { 1, 1, 1 }, color);
    auto* obj = getObject3D(id);
    if (obj) {
        obj->rotationSpeed = Vec3(0.0f, 0.0f, 0.0f);
    }
    return id;
}

int ObjectController::spawnModelObj(const std::string& objFilePath, const Vec3& pos, const Vec3& scale, const Color& tint) {
    Mesh3D mesh = loadObj(objFilePath, tint);
    return spawnObject3D(mesh, pos, { 0, 0, 0 }, scale, tint);
}

Object3D* ObjectController::getObject3D(int id) {
    for (auto& o : m_objects3D) {
        if (o.id == id) return &o;
    }
    return nullptr;
}

void ObjectController::removeObject3D(int id) {
    m_objects3D.erase(
        std::remove_if(m_objects3D.begin(), m_objects3D.end(), [id](const Object3D& o) { return o.id == id; }),
        m_objects3D.end()
    );
}

void ObjectController::clearObjects3D() {
    m_objects3D.clear();
}

void ObjectController::clearAll() {
    clearEntities2D();
    clearObjects3D();
}

// Procedural Mesh Generators
Mesh3D ObjectController::makeCube(float size, const Color& color) {
    Mesh3D m;
    m.name = "Cube";
    float h = size * 0.5f;
    m.vertices = {
        { -h, -h, -h }, {  h, -h, -h }, {  h,  h, -h }, { -h,  h, -h },
        { -h, -h,  h }, {  h, -h,  h }, {  h,  h,  h }, { -h,  h,  h }
    };
    m.indices = {
        4, 5, 6,  4, 6, 7,
        1, 0, 3,  1, 3, 2,
        0, 4, 7,  0, 7, 3,
        5, 1, 2,  5, 2, 6,
        7, 6, 2,  7, 2, 3,
        0, 1, 5,  0, 5, 4
    };
    m.faceColors.assign(12, color);
    return m;
}

Mesh3D ObjectController::makePyramid(float width, float height, const Color& color) {
    Mesh3D m;
    m.name = "Pyramid";
    float hw = width * 0.5f;
    m.vertices = {
        { -hw, 0.0f, -hw }, {  hw, 0.0f, -hw }, {  hw, 0.0f,  hw }, { -hw, 0.0f,  hw },
        { 0.0f, height, 0.0f }
    };
    m.indices = {
        0, 1, 4,
        1, 2, 4,
        2, 3, 4,
        3, 0, 4,
        0, 2, 1,  0, 3, 2
    };
    m.faceColors.assign(6, color);
    return m;
}

Mesh3D ObjectController::makeCoin(float radius, float thickness, int segments, const Color& color) {
    Mesh3D m;
    m.name = "Coin";
    float ht = thickness * 0.5f;

    m.vertices.push_back({ 0.0f, 0.0f, ht });
    m.vertices.push_back({ 0.0f, 0.0f, -ht });

    float step = 6.2831853f / static_cast<float>(segments);
    for (int i = 0; i < segments; ++i) {
        float angle = i * step;
        float x = std::cos(angle) * radius;
        float y = std::sin(angle) * radius;
        m.vertices.push_back({ x, y, ht });
        m.vertices.push_back({ x, y, -ht });
    }

    for (int i = 0; i < segments; ++i) {
        int next = (i + 1) % segments;
        int t1 = 2 + i * 2;
        int b1 = 2 + i * 2 + 1;
        int t2 = 2 + next * 2;
        int b2 = 2 + next * 2 + 1;

        m.indices.push_back(0); m.indices.push_back(t1); m.indices.push_back(t2);
        m.indices.push_back(1); m.indices.push_back(b2); m.indices.push_back(b1);
        m.indices.push_back(t1); m.indices.push_back(b1); m.indices.push_back(t2);
        m.indices.push_back(t2); m.indices.push_back(b1); m.indices.push_back(b2);
    }
    m.faceColors.assign(segments * 4, color);
    return m;
}

Mesh3D ObjectController::makeCrystal(float radius, float height, const Color& color) {
    Mesh3D m;
    m.name = "Crystal";
    float h2 = height * 0.5f;

    m.vertices.push_back({ 0.0f, 0.0f, h2 });
    m.vertices.push_back({ 0.0f, 0.0f, -h2 });

    const int segments = 6;
    float step = 6.2831853f / static_cast<float>(segments);
    for (int i = 0; i < segments; ++i) {
        float angle = i * step;
        m.vertices.push_back({ std::cos(angle) * radius, std::sin(angle) * radius, 0.0f });
    }

    for (int i = 0; i < segments; ++i) {
        int next = (i + 1) % segments;
        int v1 = 2 + i;
        int v2 = 2 + next;
        m.indices.push_back(0); m.indices.push_back(v1); m.indices.push_back(v2);
        m.indices.push_back(1); m.indices.push_back(v2); m.indices.push_back(v1);
    }
    m.faceColors.assign(segments * 2, color);
    return m;
}

Mesh3D ObjectController::makeArch(float width, float height, float thickness, const Color& color) {
    Mesh3D m;
    m.name = "Arch";
    float hw = width * 0.5f;
    float t = thickness;

    m.vertices = {
        // Left pillar
        { -hw, 0.0f, 0.0f }, { -hw + t, 0.0f, 0.0f }, { -hw + t, height, 0.0f }, { -hw, height, 0.0f },
        { -hw, 0.0f, t },    { -hw + t, 0.0f, t },    { -hw + t, height, t },    { -hw, height, t },
        // Right pillar
        { hw - t, 0.0f, 0.0f }, { hw, 0.0f, 0.0f }, { hw, height, 0.0f }, { hw - t, height, 0.0f },
        { hw - t, 0.0f, t },    { hw, 0.0f, t },    { hw, height, t },    { hw - t, height, t },
        // Lintel top bar
        { -hw, height, 0.0f }, { hw, height, 0.0f }, { hw, height + t, 0.0f }, { -hw, height + t, 0.0f },
        { -hw, height, t },    { hw, height, t },    { hw, height + t, t },    { -hw, height + t, t }
    };

    auto addBox = [&](int base) {
        int idx[] = {
            base+4, base+5, base+6,  base+4, base+6, base+7,
            base+1, base+0, base+3,  base+1, base+3, base+2,
            base+0, base+4, base+7,  base+0, base+7, base+3,
            base+5, base+1, base+2,  base+5, base+2, base+6,
            base+7, base+6, base+2,  base+7, base+2, base+3,
            base+0, base+1, base+5,  base+0, base+5, base+4
        };
        for (int i = 0; i < 36; ++i) m.indices.push_back(idx[i]);
    };
    addBox(0);
    addBox(8);
    addBox(16);
    m.faceColors.assign(36, color);
    return m;
}

Mesh3D ObjectController::loadObj(const std::string& filepath, const Color& defaultColor) {
    Mesh3D m;
    m.name = filepath;
    std::ifstream file(filepath);
    if (!file) return m;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream iss(line);
        std::string token;
        iss >> token;
        if (token == "v") {
            float x, y, z;
            iss >> x >> y >> z;
            m.vertices.push_back({ x, y, z });
        } else if (token == "f") {
            std::vector<int> fIndices;
            std::string vSpec;
            while (iss >> vSpec) {
                int vi = std::stoi(vSpec.substr(0, vSpec.find('/')));
                if (vi < 0) vi = static_cast<int>(m.vertices.size()) + vi + 1;
                fIndices.push_back(vi - 1);
            }
            if (fIndices.size() >= 3) {
                for (size_t i = 1; i + 1 < fIndices.size(); ++i) {
                    m.indices.push_back(fIndices[0]);
                    m.indices.push_back(fIndices[i]);
                    m.indices.push_back(fIndices[i + 1]);
                    m.faceColors.push_back(defaultColor);
                }
            }
        }
    }
    return m;
}

// Lifecycle: Update & Render
void ObjectController::update(float deltaTime) {
    auto& player = PlayerController::instance();
    bool playerValid = player.isValid();
    float playerY = playerValid ? static_cast<float>(player.getY()) : 0.0f;

    std::vector<int> entIds;
    entIds.reserve(m_entities2D.size());
    for (const auto& e : m_entities2D) entIds.push_back(e.id);

    for (int id : entIds) {
        auto* e = getEntity2D(id);
        if (!e || !e->visible) continue;
        if (e->onUpdate) e->onUpdate(*e, deltaTime);

        if (playerValid && e->hasCollision && e->onCollideWithPlayer) {
            float dy = std::abs(e->worldY - playerY);
            if (dy < e->collisionRadius) {
                e->onCollideWithPlayer(*e);
            }
        }
    }

    std::vector<int> objIds;
    objIds.reserve(m_objects3D.size());
    for (const auto& o : m_objects3D) objIds.push_back(o.id);

    for (int id : objIds) {
        auto* o = getObject3D(id);
        if (!o || !o->visible) continue;

        o->rotation.x += o->rotationSpeed.x * deltaTime;
        o->rotation.y += o->rotationSpeed.y * deltaTime;
        o->rotation.z += o->rotationSpeed.z * deltaTime;

        if (o->onUpdate) o->onUpdate(*o, deltaTime);

        if (playerValid && o->hasCollision && o->onCollideWithPlayer) {
            float dy = std::abs(o->position.y - playerY);
            if (dy < o->collisionRadius) {
                o->onCollideWithPlayer(*o);
            }
        }
    }
}

struct ProjectedTriangle {
    PointF p[3];
    Color color;
    float avgDepth;
    bool wireframe;
};

void ObjectController::render(SDL_Renderer* renderer, int screenW, int screenH) {
    if (!renderer) return;

    auto& player = PlayerController::instance();
    float playerY = player.isValid() ? static_cast<float>(player.getY()) : 0.0f;
    auto& rc = RenderController::instance();

    float screenCenterX = screenW * 0.5f;
    float horizonY = 220.0f;
    float baseY = 530.0f;

    // 1. Render 2D World Entities
    for (const auto& e : m_entities2D) {
        if (!e.visible) continue;
        float distY = e.worldY - playerY;
        if (distY < -100.0f || distY > 2000.0f) continue;

        float sFactor = 1.0f / (1.0f + (std::max)(0.0f, distY) * 0.0022f);
        float normY = (std::min)(1.0f, (std::max)(0.0f, distY) / 2000.0f);
        float screenY = horizonY + (baseY - horizonY) * (1.0f - normY);
        float screenX = screenCenterX + e.worldX * sFactor;

        int w = static_cast<int>(e.width * sFactor);
        int h = static_cast<int>(e.height * sFactor);
        Rect rect(static_cast<int>(screenX - w * 0.5f), static_cast<int>(screenY - h), w, h);

        if (e.texture) {
            PointF p0(static_cast<float>(rect.x), static_cast<float>(rect.y));
            PointF p1(static_cast<float>(rect.x + rect.w), static_cast<float>(rect.y));
            PointF p2(static_cast<float>(rect.x + rect.w), static_cast<float>(rect.y + rect.h));
            PointF p3(static_cast<float>(rect.x), static_cast<float>(rect.y + rect.h));
            rc.drawTrapezoid(renderer, e.texture, p0, p1, p2, p3, e.color);
        } else {
            rc.fillRect(renderer, rect, e.color);
            rc.drawRect(renderer, rect, Color::White());
            if (!e.label.empty() && sFactor > 0.35f) {
                rc.drawText(renderer, e.label, rect.x + 6, rect.y + 4, Color::White(), sFactor * 0.85f);
            }
        }
    }

    // 2. Render 3D World Objects
    std::vector<ProjectedTriangle> renderQueue;
    Vec3 lightDir = Vec3(0.4f, 0.7f, 0.6f).normalized();

    for (const auto& obj : m_objects3D) {
        if (!obj.visible || obj.mesh.vertices.empty()) continue;

        float distY = obj.position.y - playerY;
        if (distY < -150.0f || distY > 2000.0f) continue;

        float radX = obj.rotation.x * 0.01745329f;
        float radY = obj.rotation.y * 0.01745329f;
        float radZ = obj.rotation.z * 0.01745329f;

        float cx = std::cos(radX), sx = std::sin(radX);
        float cy = std::cos(radY), sy = std::sin(radY);
        float cz = std::cos(radZ), sz = std::sin(radZ);

        auto transformVertex = [&](const Vec3& v) -> Vec3 {
            Vec3 s = { v.x * obj.scale.x, v.y * obj.scale.y, v.z * obj.scale.z };
            Vec3 r1 = { s.x, s.y * cx - s.z * sx, s.y * sx + s.z * cx };
            Vec3 r2 = { r1.x * cy + r1.z * sy, r1.y, -r1.x * sy + r1.z * cy };
            Vec3 r3 = { r2.x * cz - r2.y * sz, r2.x * sz + r2.y * cz, r2.z };
            return Vec3(obj.position.x + r3.x, obj.position.y + r3.y, obj.position.z + r3.z);
        };

        const auto& verts = obj.mesh.vertices;
        const auto& indices = obj.mesh.indices;

        for (size_t i = 0; i + 2 < indices.size(); i += 3) {
            Vec3 w0 = transformVertex(verts[indices[i]]);
            Vec3 w1 = transformVertex(verts[indices[i + 1]]);
            Vec3 w2 = transformVertex(verts[indices[i + 2]]);

            auto project = [&](const Vec3& w) -> PointF {
                float dY = w.y - playerY;
                float sFactor = 1.0f / (1.0f + (std::max)(0.0f, dY) * 0.0022f);
                float px = screenCenterX + w.x * sFactor;
                float py = horizonY + (baseY - horizonY) * (1.0f - (std::min)(1.0f, (std::max)(0.0f, dY) / 2000.0f)) - w.z * sFactor;
                return PointF(px, py);
            };

            PointF p0 = project(w0);
            PointF p1 = project(w1);
            PointF p2 = project(w2);

            float crossZ = (p1.x - p0.x) * (p2.y - p0.y) - (p1.y - p0.y) * (p2.x - p0.x);
            if (!obj.wireframe && crossZ >= 0.0f) {
                continue;
            }

            Vec3 edge1 = w1 - w0;
            Vec3 edge2 = w2 - w0;
            Vec3 normal = Vec3::cross(edge1, edge2).normalized();
            float diff = (std::max)(0.0f, Vec3::dot(normal, lightDir));
            float light = 0.35f + diff * 0.65f;

            Color baseCol = (i / 3 < obj.mesh.faceColors.size()) ? obj.mesh.faceColors[i / 3] : obj.tint;
            Color shadedColor(
                static_cast<uint8_t>((std::min)(255.0f, baseCol.r * light)),
                static_cast<uint8_t>((std::min)(255.0f, baseCol.g * light)),
                static_cast<uint8_t>((std::min)(255.0f, baseCol.b * light)),
                baseCol.a
            );

            ProjectedTriangle tri;
            tri.p[0] = p0;
            tri.p[1] = p1;
            tri.p[2] = p2;
            tri.color = shadedColor;
            tri.avgDepth = (w0.y + w1.y + w2.y) / 3.0f;
            tri.wireframe = obj.wireframe;
            renderQueue.push_back(tri);
        }
    }

    // 3. Painter's Algorithm
    std::sort(renderQueue.begin(), renderQueue.end(), [](const ProjectedTriangle& a, const ProjectedTriangle& b) {
        return a.avgDepth > b.avgDepth;
    });

    // 4. Rasterize Triangles
    for (const auto& tri : renderQueue) {
        if (tri.wireframe) {
            rc.drawLine(renderer, static_cast<int>(tri.p[0].x), static_cast<int>(tri.p[0].y), static_cast<int>(tri.p[1].x), static_cast<int>(tri.p[1].y), tri.color);
            rc.drawLine(renderer, static_cast<int>(tri.p[1].x), static_cast<int>(tri.p[1].y), static_cast<int>(tri.p[2].x), static_cast<int>(tri.p[2].y), tri.color);
            rc.drawLine(renderer, static_cast<int>(tri.p[2].x), static_cast<int>(tri.p[2].y), static_cast<int>(tri.p[0].x), static_cast<int>(tri.p[0].y), tri.color);
        } else {
            rc.drawTriangle(renderer, tri.p[0], tri.p[1], tri.p[2], tri.color);
        }
    }
}

} // namespace sense

