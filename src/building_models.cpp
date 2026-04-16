#include "building_models.h"

#include <array>
#include <cmath>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>

#include "constants.h"

namespace BuildingModels {
namespace {

constexpr std::array<const char*, BUILDING_MODEL_COUNT> kTowerModelPaths = {
    "models/building-sample-tower-a.obj",
    "models/building-sample-tower-b.obj",
    "models/building-sample-tower-c.obj",
    "models/building-sample-tower-d.obj",
};

struct ObjVertexKey {
    int positionIndex;
    int texCoordIndex;
    int normalIndex;

    bool operator==(const ObjVertexKey& other) const {
        return positionIndex == other.positionIndex
            && texCoordIndex == other.texCoordIndex
            && normalIndex == other.normalIndex;
    }
};

struct ObjVertexKeyHash {
    std::size_t operator()(const ObjVertexKey& key) const {
        const std::size_t h1 = std::hash<int>{}(key.positionIndex);
        const std::size_t h2 = std::hash<int>{}(key.texCoordIndex);
        const std::size_t h3 = std::hash<int>{}(key.normalIndex);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

struct LoadedMeshData {
    Mesh mesh;
    glm::vec3 localMin;
    glm::vec3 localMax;
};

struct TriangleKey {
    unsigned int a;
    unsigned int b;
    unsigned int c;

    bool operator==(const TriangleKey& other) const {
        return a == other.a && b == other.b && c == other.c;
    }
};

struct TriangleKeyHash {
    std::size_t operator()(const TriangleKey& key) const {
        const std::size_t h1 = std::hash<unsigned int>{}(key.a);
        const std::size_t h2 = std::hash<unsigned int>{}(key.b);
        const std::size_t h3 = std::hash<unsigned int>{}(key.c);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

int resolveObjIndex(int rawIndex, std::size_t sourceCount) {
    if (rawIndex == 0) {
        return -1;
    }

    if (rawIndex > 0) {
        return rawIndex - 1;
    }

    return static_cast<int>(sourceCount) + rawIndex;
}

bool parseFaceToken(const std::string& token,
                    std::size_t positionCount,
                    std::size_t texCoordCount,
                    std::size_t normalCount,
                    ObjVertexKey& outKey) {
    outKey = {-1, -1, -1};

    std::size_t firstSlash = token.find('/');
    if (firstSlash == std::string::npos) {
        int v = std::stoi(token);
        outKey.positionIndex = resolveObjIndex(v, positionCount);
        return outKey.positionIndex >= 0 && outKey.positionIndex < static_cast<int>(positionCount);
    }

    const std::string posPart = token.substr(0, firstSlash);
    if (posPart.empty()) {
        return false;
    }

    int v = std::stoi(posPart);
    outKey.positionIndex = resolveObjIndex(v, positionCount);

    std::size_t secondSlash = token.find('/', firstSlash + 1);

    if (secondSlash == std::string::npos) {
        const std::string vtPart = token.substr(firstSlash + 1);
        if (!vtPart.empty()) {
            int vt = std::stoi(vtPart);
            outKey.texCoordIndex = resolveObjIndex(vt, texCoordCount);
        }
    } else {
        const std::string vtPart = token.substr(firstSlash + 1, secondSlash - firstSlash - 1);
        const std::string vnPart = token.substr(secondSlash + 1);

        if (!vtPart.empty()) {
            int vt = std::stoi(vtPart);
            outKey.texCoordIndex = resolveObjIndex(vt, texCoordCount);
        }
        if (!vnPart.empty()) {
            int vn = std::stoi(vnPart);
            outKey.normalIndex = resolveObjIndex(vn, normalCount);
        }
    }

    const bool validPos = outKey.positionIndex >= 0 && outKey.positionIndex < static_cast<int>(positionCount);
    const bool validTex = outKey.texCoordIndex == -1
        || (outKey.texCoordIndex >= 0 && outKey.texCoordIndex < static_cast<int>(texCoordCount));
    const bool validNorm = outKey.normalIndex == -1
        || (outKey.normalIndex >= 0 && outKey.normalIndex < static_cast<int>(normalCount));

    return validPos && validTex && validNorm;
}

LoadedMeshData loadObjMesh(const char* filePath, unsigned int textureID) {
    std::ifstream in(filePath);
    if (!in.is_open()) {
        throw std::runtime_error(std::string("Failed to open OBJ file: ") + filePath);
    }

    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> texCoords;
    std::vector<glm::vec3> normals;

    positions.reserve(1024);
    texCoords.reserve(1024);
    normals.reserve(1024);

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::unordered_map<ObjVertexKey, unsigned int, ObjVertexKeyHash> vertexLookup;
    std::unordered_set<TriangleKey, TriangleKeyHash> emittedTriangles;

    glm::vec3 localMin(std::numeric_limits<float>::max());
    glm::vec3 localMax(std::numeric_limits<float>::lowest());

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::istringstream stream(line);
        std::string tag;
        stream >> tag;

        if (tag == "v") {
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;
            stream >> x >> y >> z;
            positions.push_back(glm::vec3(x, y, z));

            localMin = glm::min(localMin, glm::vec3(x, y, z));
            localMax = glm::max(localMax, glm::vec3(x, y, z));
            continue;
        }

        if (tag == "vt") {
            float u = 0.0f;
            float v = 0.0f;
            stream >> u >> v;
            texCoords.push_back(glm::vec2(u, v));
            continue;
        }

        if (tag == "vn") {
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;
            stream >> x >> y >> z;
            normals.push_back(glm::vec3(x, y, z));
            continue;
        }

        if (tag == "f") {
            std::vector<ObjVertexKey> face;
            std::string token;
            while (stream >> token) {
                ObjVertexKey key;
                if (parseFaceToken(token, positions.size(), texCoords.size(), normals.size(), key)) {
                    face.push_back(key);
                }
            }

            if (face.size() < 3) {
                continue;
            }

            auto vertexIndexForKey = [&](const ObjVertexKey& key) {
                const auto found = vertexLookup.find(key);
                if (found != vertexLookup.end()) {
                    return found->second;
                }

                Vertex vertex{};
                vertex.Position = positions[key.positionIndex];
                vertex.TexCoords = (key.texCoordIndex >= 0) ? texCoords[key.texCoordIndex] : glm::vec2(0.0f, 0.0f);
                vertex.Normal = (key.normalIndex >= 0) ? normals[key.normalIndex] : glm::vec3(0.0f, 1.0f, 0.0f);

                const unsigned int newIndex = static_cast<unsigned int>(vertices.size());
                vertices.push_back(vertex);
                vertexLookup.emplace(key, newIndex);
                return newIndex;
            };

            for (std::size_t i = 1; i + 1 < face.size(); ++i) {
                const unsigned int i0 = vertexIndexForKey(face[0]);
                const unsigned int i1 = vertexIndexForKey(face[i]);
                const unsigned int i2 = vertexIndexForKey(face[i + 1]);

                const TriangleKey triangle = {i0, i1, i2};
                if (!emittedTriangles.insert(triangle).second) {
                    continue;
                }

                indices.push_back(i0);
                indices.push_back(i1);
                indices.push_back(i2);
            }
        }
    }

    if (positions.empty() || indices.empty()) {
        throw std::runtime_error(std::string("OBJ data is empty or has no triangles: ") + filePath);
    }

    LoadedMeshData loaded = {
        Mesh(vertices, indices, textureID),
        localMin,
        localMax,
    };

    return loaded;
}

std::vector<Scene::Prototype> loadTowerPrototypes(unsigned int colormapTextureID) {
    std::vector<Scene::Prototype> prototypes;
    prototypes.reserve(kTowerModelPaths.size());

    for (const char* modelPath : kTowerModelPaths) {
        LoadedMeshData loaded = loadObjMesh(modelPath, colormapTextureID);
        const float localWidth = loaded.localMax.x - loaded.localMin.x;
        const float localDepth = loaded.localMax.z - loaded.localMin.z;
        const float dominantFootprint = std::max(localWidth, localDepth);

        if (dominantFootprint <= 0.0f) {
            throw std::runtime_error(std::string("Tower footprint is invalid: ") + modelPath);
        }

        prototypes.push_back({
            std::move(loaded.mesh),
            loaded.localMin,
            loaded.localMax,
            BUILDING_TARGET_FOOTPRINT / dominantFootprint,
        });
    }

    return prototypes;
}

std::vector<Instance> createTowerLayout(const std::vector<Scene::Prototype>& prototypes) {
    std::vector<Instance> instances;
    instances.reserve(BUILDING_COUNT);

    if (prototypes.empty()) {
        return instances;
    }

    float maxHalfWidth = 0.0f;
    for (std::size_t i = 0; i < static_cast<std::size_t>(BUILDING_COUNT); ++i) {
        const std::size_t prototypeIndex = i % prototypes.size();
        const Scene::Prototype& prototype = prototypes[prototypeIndex];
        const float halfWidth = 0.5f * (prototype.localMax.x - prototype.localMin.x) * prototype.scale;
        maxHalfWidth = std::max(maxHalfWidth, halfWidth);
    }

    const float xOffset = TRACK_RADIUS_X + BUILDING_SIDE_CLEARANCE + maxHalfWidth;
    const float zStart = -0.5f * BUILDING_SIDE_Z_SPACING * static_cast<float>(BUILDINGS_PER_SIDE - 1);

    for (int side = 0; side < BUILDING_SIDES; ++side) {
        const float sideSign = (side == 0) ? -1.0f : 1.0f;
        const float xPos = sideSign * xOffset;

        for (int slot = 0; slot < BUILDINGS_PER_SIDE; ++slot) {
            const int buildingIndex = side * BUILDINGS_PER_SIDE + slot;
            const std::size_t prototypeIndex = static_cast<std::size_t>(buildingIndex) % prototypes.size();
            const Scene::Prototype& prototype = prototypes[prototypeIndex];

            const float zPos = zStart + (static_cast<float>(slot) * BUILDING_SIDE_Z_SPACING);
            const float height = (prototype.localMax.y - prototype.localMin.y) * prototype.scale;
            const float halfWidth = 0.5f * (prototype.localMax.x - prototype.localMin.x) * prototype.scale;
            const float halfDepth = 0.5f * (prototype.localMax.z - prototype.localMin.z) * prototype.scale;

            instances.push_back({
                glm::vec3(xPos, 0.0f, zPos),
                prototypeIndex,
                prototype.scale,
                height,
                halfWidth,
                halfDepth,
            });
        }
    }

    return instances;
}

}  // namespace

Scene loadTowerScene(unsigned int colormapTextureID) {
    Scene scene;
    scene.prototypes = loadTowerPrototypes(colormapTextureID);
    scene.instances = createTowerLayout(scene.prototypes);
    return scene;
}

const Mesh& meshFor(const Scene& scene, const Instance& instance) {
    return scene.prototypes[instance.prototypeIndex].mesh;
}

Mesh& meshFor(Scene& scene, const Instance& instance) {
    return scene.prototypes[instance.prototypeIndex].mesh;
}

glm::mat4 modelTransformFor(const Scene& scene, const Instance& instance) {
    const Scene::Prototype& prototype = scene.prototypes[instance.prototypeIndex];
    const float localCenterX = 0.5f * (prototype.localMin.x + prototype.localMax.x);
    const float localCenterZ = 0.5f * (prototype.localMin.z + prototype.localMax.z);
    const float roadFacingYaw = roadFacingYawRadians(instance);

    glm::mat4 transform(1.0f);
    transform = glm::translate(transform, instance.position);
    transform = glm::rotate(transform, roadFacingYaw, glm::vec3(0.0f, 1.0f, 0.0f));
    transform = glm::scale(transform, glm::vec3(instance.scale));
    transform = glm::translate(transform, glm::vec3(-localCenterX, -prototype.localMin.y, -localCenterZ));
    return transform;
}

std::vector<LightRigConfig> createLightRigConfigs(const Scene& scene) {
    std::vector<LightRigConfig> configs;

    const std::size_t maxLights = std::min(scene.instances.size(), static_cast<std::size_t>(MAX_BUILDING_LIGHTS));
    configs.reserve(maxLights);

    for (std::size_t i = 0; i < maxLights; ++i) {
        const Instance& instance = scene.instances[i];

        LightRigConfig config;
        config.baseDirection = glm::normalize(glm::vec3(roadFacingSignX(instance), BUILDING_LIGHT_BASE_DIRECTION_Y, 0.0f));

        const glm::vec3 roofAnchor = roofCenter(instance) + glm::vec3(0.0f, BUILDING_LIGHT_HEIGHT_OFFSET, 0.0f);
        const glm::vec3 horizontalRoadForward = glm::normalize(glm::vec3(config.baseDirection.x, 0.0f, config.baseDirection.z));

        config.basePosition = roofAnchor
            + glm::vec3(0.0f,
                        BUILDING_LIGHT_GIMBAL_PIVOT_HEIGHT + BUILDING_LIGHT_LAMP_CENTER_Y,
                        0.0f)
            + (horizontalRoadForward * (BUILDING_LIGHT_LAMP_DEPTH * BUILDING_LIGHT_LAMP_NOZZLE_X_FACTOR));

        config.color = glm::vec3(
            BUILDING_LIGHT_COLORS[i % BUILDING_LIGHT_COLOR_COUNT][0],
            BUILDING_LIGHT_COLORS[i % BUILDING_LIGHT_COLOR_COUNT][1],
            BUILDING_LIGHT_COLORS[i % BUILDING_LIGHT_COLOR_COUNT][2]);

        config.swingSpeed = BUILDING_LIGHT_SWING_BASE_SPEED + (static_cast<float>(i) * BUILDING_LIGHT_SWING_SPEED_STEP);
        configs.push_back(config);
    }

    return configs;
}

float roadFacingSignX(const Instance& instance) {
    return (instance.position.x > 0.0f) ? -1.0f : 1.0f;
}

float roadFacingYawRadians(const Instance& instance) {
    // Tower, gimbal, and windmill assets are authored with +Z as forward.
    // Rotate around Y so forward aligns with road-facing +/-X.
    const float yawSign = roadFacingSignX(instance);
    return yawSign * glm::half_pi<float>();
}

glm::vec3 roofCenter(const Instance& instance) {
    return instance.position + glm::vec3(0.0f, instance.roofY, 0.0f);
}

void cleanup(Scene& scene) {
    for (Scene::Prototype& prototype : scene.prototypes) {
        prototype.mesh.cleanup();
    }
}

}  // namespace BuildingModels