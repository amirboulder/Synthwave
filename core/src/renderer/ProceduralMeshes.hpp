#pragma once

//#include "../ecs/components.hpp"

namespace MeshGen {

    // Generates a centered axis-aligned cube.
    void generateCube(float scale, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {

        if (scale < 0.01f) {
            LogError(LOG_APP, "Cannot generate cube mesh: scale %f too small", scale);
            return;
        }

        const float h = 0.5f * scale;

        vertices.clear();
        indices.clear();
        vertices.reserve(24);
        indices.reserve(36);

        auto addFace = [&](const glm::vec3& normal,
            const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3) {
            const uint32_t base = static_cast<uint32_t>(vertices.size());
            const glm::vec3 positions[4] = { v0, v1, v2, v3 };
            const glm::vec2 uvs[4] = {
                { 0.0f, 0.0f },
                { 1.0f, 0.0f },
                { 1.0f, 1.0f },
                { 0.0f, 1.0f },
            };

            for (int i = 0; i < 4; ++i) {

                vertices.emplace_back(Vertex{
                    positions[i],
                    normal,
                    uvs[i],
                    glm::vec4(1.0f) // default white color
                    });
            }

            // Counter-Clockwise (CCW) Index Winding
            indices.push_back(base + 0);
            indices.push_back(base + 1);
            indices.push_back(base + 2);

            indices.push_back(base + 0);
            indices.push_back(base + 2);
            indices.push_back(base + 3);
        };

        // Winding is CCW when viewed from outside each respective face.
        // +Y
        addFace({ 0.0f,  1.0f,  0.0f }, { -h,  h, -h }, { -h,  h,  h }, { h,  h,  h }, { h,  h, -h });
        // -Y
        addFace({ 0.0f, -1.0f,  0.0f }, { -h, -h,  h }, { -h, -h, -h }, { h, -h, -h }, { h, -h,  h });
        // +Z 
        addFace({ 0.0f,  0.0f,  1.0f }, { -h, -h,  h }, { h, -h,  h }, { h,  h,  h }, { -h,  h,  h });
        // -Z
        addFace({ 0.0f,  0.0f, -1.0f }, { h, -h, -h }, { -h, -h, -h }, { -h,  h, -h }, { h,  h, -h });
        // +X 
        addFace({ 1.0f,  0.0f,  0.0f }, { h, -h,  h }, { h, -h, -h }, { h,  h, -h }, { h,  h,  h });
        // -X
        addFace({ -1.0f,  0.0f,  0.0f }, { -h, -h, -h }, { -h, -h,  h }, { -h,  h,  h }, { -h,  h, -h });
    }


    void generateGrid(int size, std::vector<Vertex>& vertices, std::vector<unsigned int>& indices) {
        if (size <= 0) return;

        vertices.clear();
        indices.clear();

        // Adding one to rows and cols because the loops go to <= size
        vertices.reserve((size + 1) * (size + 1));

        for (int r = 0; r <= size; r++) {
            for (int c = 0; c <= size; c++) {

                float x = static_cast<float>(c) - (size / 2.0f);
                float y = 0.0f;
                float z = static_cast<float>(r) - (size / 2.0f);

                // Calculate UV coordinates scaled from 0.0 to 1.0 across the grid
                float u = static_cast<float>(c) / static_cast<float>(size);
                float v = static_cast<float>(r) / static_cast<float>(size);

                // Construct and push the Vertex safely in one go
                vertices.emplace_back(Vertex{
                    glm::vec3(x, y, z),            // position
                    glm::vec3(0.0f, 1.0f, 0.0f),   // normal (pointing up)
                    glm::vec2(u, v),               // texCoord (UVs added)
                    glm::vec4(1.0f)                // color (default white)
                    });
            }
        }

        indices.reserve(size * size * 6);

        // Generate indices 
        for (int r = 0; r < size; r++) {
            for (int c = 0; c < size; c++) {
                // Define two triangles for each grid cell
                int topLeft = r * (size + 1) + c;
                int topRight = topLeft + 1;
                int bottomLeft = (r + 1) * (size + 1) + c;
                int bottomRight = bottomLeft + 1;

                // First triangle (top-left, bottom-left, top-right)
                indices.emplace_back(topLeft);
                indices.emplace_back(bottomLeft);
                indices.emplace_back(topRight);

                // Second triangle (top-right, bottom-left, bottom-right)
                indices.emplace_back(topRight);
                indices.emplace_back(bottomLeft);
                indices.emplace_back(bottomRight);
            }
        }
    }


    // Generates a UV Sphere centered at (0, 0, 0)
    // Generates a UV Sphere centered at (0, 0, 0) with CCW outer winding
    void generateSphere(float radius, int sectors, int stacks, std::vector<Vertex>& vertices, std::vector<unsigned int>& indices) {
        if (radius <= 0.0f || sectors < 3 || stacks < 2) return;

        vertices.clear();
        indices.clear();

        vertices.reserve((stacks + 1) * (sectors + 1));
        indices.reserve(stacks * sectors * 6);

        // 1. Generate Vertices, Normals, and UVs
        for (int i = 0; i <= stacks; ++i) {
            float v = static_cast<float>(i) / static_cast<float>(stacks);
            float phi = v * glm::pi<float>();

            for (int j = 0; j <= sectors; ++j) {
                float u = static_cast<float>(j) / static_cast<float>(sectors);
                float theta = u * 2.0f * glm::pi<float>();

                float x = std::sin(phi) * std::cos(theta);
                float y = std::cos(phi);
                float z = std::sin(phi) * std::sin(theta);

                glm::vec3 normal(x, y, z);
                glm::vec3 position = normal * radius;

                vertices.emplace_back(Vertex{
                    position,
                    normal,
                    glm::vec2(u, v),
                    glm::vec4(1.0f)
                    });
            }
        }

        // 2. Generate Indices (Flipped to face OUTWARD)
        for (int i = 0; i < stacks; ++i) {
            int k1 = i * (sectors + 1);     // Beginning of current stack
            int k2 = k1 + sectors + 1;      // Beginning of next stack

            for (int j = 0; j < sectors; ++j, ++k1, ++k2) {

                // Triangle 1: k1 -> k1+1 -> k2 (Swapped k2 and k1+1 for CCW)
                if (i != 0) {
                    indices.emplace_back(k1);
                    indices.emplace_back(k1 + 1);
                    indices.emplace_back(k2);
                }

                // Triangle 2: k1+1 -> k2+1 -> k2 (Swapped k2 and k2+1 for CCW)
                if (i != (stacks - 1)) {
                    indices.emplace_back(k1 + 1);
                    indices.emplace_back(k2 + 1);
                    indices.emplace_back(k2);
                }
            }
        }
    }

    // Generates a centered cylinder along the Y-axis
    void generateCylinder(float radius, float height, int segments, std::vector<Vertex>& vertices, std::vector<unsigned int>& indices) {
        if (radius <= 0.0f || height <= 0.0f || segments < 3) return;

        vertices.clear();
        indices.clear();

        vertices.reserve((segments + 1) * 4);
        indices.reserve((segments * 6) + (segments * 6));

        float halfH = height * 0.5f;

        // ==========================================
        // 1. TUBE SIDE GENERATION
        // ==========================================
        uint32_t tubeBaseIndex = static_cast<uint32_t>(vertices.size());

        for (int i = 0; i <= segments; ++i) {
            float u = static_cast<float>(i) / static_cast<float>(segments);
            float theta = u * 2.0f * glm::pi<float>();

            float cosTheta = std::cos(theta);
            float sinTheta = std::sin(theta);

            glm::vec3 normal(cosTheta, 0.0f, sinTheta);
            glm::vec3 posTop(cosTheta * radius, halfH, sinTheta * radius);
            glm::vec3 posBot(cosTheta * radius, -halfH, sinTheta * radius);

            vertices.emplace_back(Vertex{ posTop, normal, glm::vec2(u, 0.0f), glm::vec4(1.0f) });
            vertices.emplace_back(Vertex{ posBot, normal, glm::vec2(u, 1.0f), glm::vec4(1.0f) });
        }

        // Tube Indices - Flipped to face OUTWARD
        for (int i = 0; i < segments; ++i) {
            uint32_t currentIdx = tubeBaseIndex + (i * 2);
            uint32_t nextIdx = currentIdx + 2;

            // Triangle 1 (Swapped nextIdx and currentIdx + 1)
            indices.emplace_back(currentIdx);
            indices.emplace_back(nextIdx);
            indices.emplace_back(currentIdx + 1);

            // Triangle 2 (Swapped nextIdx + 1 and currentIdx + 1)
            indices.emplace_back(nextIdx);
            indices.emplace_back(nextIdx + 1);
            indices.emplace_back(currentIdx + 1);
        }

        // ==========================================
        // 2. TOP CAP GENERATION (Y = +halfH)
        // ==========================================
        uint32_t topCapBaseIndex = static_cast<uint32_t>(vertices.size());

        vertices.emplace_back(Vertex{
            glm::vec3(0.0f, halfH, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f),
            glm::vec2(0.5f, 0.5f),
            glm::vec4(1.0f)
            });

        for (int i = 0; i <= segments; ++i) {
            float theta = (static_cast<float>(i) / static_cast<float>(segments)) * 2.0f * glm::pi<float>();
            float cosTheta = std::cos(theta);
            float sinTheta = std::sin(theta);

            float u = cosTheta * 0.5f + 0.5f;
            float v = sinTheta * 0.5f + 0.5f;

            vertices.emplace_back(Vertex{
                glm::vec3(cosTheta * radius, halfH, sinTheta * radius),
                glm::vec3(0.0f, 1.0f, 0.0f),
                glm::vec2(u, v),
                glm::vec4(1.0f)
                });
        }

       
        for (int i = 1; i <= segments; ++i) {
            indices.emplace_back(topCapBaseIndex);
            indices.emplace_back(topCapBaseIndex + i + 1); 
            indices.emplace_back(topCapBaseIndex + i);     
        }

        // ==========================================
        // 3. BOTTOM CAP GENERATION (Y = -halfH)
        // ==========================================
        uint32_t botCapBaseIndex = static_cast<uint32_t>(vertices.size());

        vertices.emplace_back(Vertex{
            glm::vec3(0.0f, -halfH, 0.0f),
            glm::vec3(0.0f, -1.0f, 0.0f),
            glm::vec2(0.5f, 0.5f),
            glm::vec4(1.0f)
            });

        for (int i = 0; i <= segments; ++i) {
            float theta = (static_cast<float>(i) / static_cast<float>(segments)) * 2.0f * glm::pi<float>();
            float cosTheta = std::cos(theta);
            float sinTheta = std::sin(theta);

            float u = -cosTheta * 0.5f + 0.5f;
            float v = sinTheta * 0.5f + 0.5f;

            vertices.emplace_back(Vertex{
                glm::vec3(cosTheta * radius, -halfH, sinTheta * radius),
                glm::vec3(0.0f, -1.0f, 0.0f),
                glm::vec2(u, v),
                glm::vec4(1.0f)
                });
        }

        // Bottom Cap Indices - Flipped to face OUTWARD (Looking up)
        for (int i = 1; i <= segments; ++i) {
            indices.emplace_back(botCapBaseIndex);
            indices.emplace_back(botCapBaseIndex + i);    
            indices.emplace_back(botCapBaseIndex + i + 1);
        }
    }


    // Generates a capsule centered at (0, 0, 0) aligned along the Y-axis.
    // Total tip-to-tip height is 'height'. Inner cylinder height is (height - 2*radius).
    void generateCapsule(float radius, float cylHalfHeight, int sectors, int ringsPerDome, std::vector<Vertex>& vertices, std::vector<unsigned int>& indices) {
        if (radius <= 0.0f || cylHalfHeight < 0.0f || sectors < 3 || ringsPerDome < 1) return;

        vertices.clear();
        indices.clear();

        // Sizing allocations
        int totalRings = (ringsPerDome * 2) + 2;
        vertices.reserve((totalRings) * (sectors + 1));
        indices.reserve((totalRings - 1) * sectors * 6);

        // 1. Generate Vertices, Normals, and UVs
        for (int r = 0; r < totalRings; ++r) {
            float yPos = 0.0f;
            float ringRadius = 0.0f;
            float v = static_cast<float>(r) / static_cast<float>(totalRings - 1);

            // Top Hemisphere Dome
            if (r <= ringsPerDome) {
                float phi = (static_cast<float>(r) / static_cast<float>(ringsPerDome)) * (glm::pi<float>() * 0.5f);
                yPos = cylHalfHeight + radius * std::cos(phi);
                ringRadius = radius * std::sin(phi);
            }
            // Bottom Hemisphere Dome
            else {
                int adjustedRing = r - 1;
                float phi = (static_cast<float>(adjustedRing - ringsPerDome) / static_cast<float>(ringsPerDome)) * (glm::pi<float>() * 0.5f) + (glm::pi<float>() * 0.5f);
                yPos = -cylHalfHeight + radius * std::cos(phi);
                ringRadius = radius * std::sin(phi);
            }

            // Generate vertices around the current ring level
            for (int s = 0; s <= sectors; ++s) {
                float u = static_cast<float>(s) / static_cast<float>(sectors);
                float theta = u * 2.0f * glm::pi<float>();

                float cosTheta = std::cos(theta);
                float sinTheta = std::sin(theta);

                glm::vec3 position(cosTheta * ringRadius, yPos, sinTheta * ringRadius);

                // Normals calculation
                glm::vec3 normal;
                if (r <= ringsPerDome || r >= ringsPerDome + 1) {
                    float domeCenterY = (r <= ringsPerDome) ? cylHalfHeight : -cylHalfHeight;
                    normal = glm::normalize(position - glm::vec3(0.0f, domeCenterY, 0.0f));
                }
                else {
                    normal = glm::vec3(cosTheta, 0.0f, sinTheta);
                }

                vertices.emplace_back(Vertex{
                    position,
                    normal,
                    glm::vec2(u, v),
                    glm::vec4(1.0f)
                    });
            }
        }

        // 2. Generate Indices 
        for (int r = 0; r < totalRings - 1; ++r) {
            int k1 = r * (sectors + 1);
            int k2 = k1 + sectors + 1;

            for (int s = 0; s < sectors; ++s, ++k1, ++k2) {
                // Triangle 1 
                indices.emplace_back(k1);
                indices.emplace_back(k1 + 1);
                indices.emplace_back(k2);

                // Triangle 2 
                indices.emplace_back(k2);
                indices.emplace_back(k1 + 1);
                indices.emplace_back(k2 + 1);
            }
        }
    }

} 
