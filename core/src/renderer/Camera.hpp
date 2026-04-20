#pragma once 


#include "RendererConfig.hpp"
#include "Mesh.hpp"


class Camera {

public:

    glm::vec3 position;
    glm::vec3 front = glm::vec3(1.0f);// Direction camera is looking (normalized target - position)
   
    glm::vec3 up;        // Up vector
    glm::vec3 right;     // Right vector

    float aspectRatio = 1;

    // Euler angles
    float yaw;           // Rotation around Y-axis
    float pitch;         // Rotation around X-axis

    // Camera parameters
    float fov;           // Field of view
    float nearPlane;     // Near clipping plane
    float farPlane;      // Far clipping plane

    // Camera movement
    float movementSpeed = 0.5f;
    float mouseSensitivity;


    Camera(const RenderConfig& config)
        : position(config.FreeCamPos),
        yaw(config.FreeCamYaw),
        pitch(config.FreeCamPitch),
        fov(config.freeCamFov),
        nearPlane(config.freeCamNearPlane),
        farPlane(config.freeCamFarPlane),
        mouseSensitivity(config.FreeCamMouseSensitivity),
        aspectRatio(static_cast<float>(config.windowWidth) / static_cast<float>(config.windowHeight))
    {

        updateVectors();

    }


    void updateVectors() {

        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(front);

        // World up vector is (0,1,0)
        right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
        up = glm::normalize(glm::cross(right, front));
    }

    glm::mat4 generateView() {

        glm::mat4 view = glm::lookAtRH(position, position + front, up);
        return view;
    }
    glm::mat4 generateProj() {

        glm::mat4 proj = glm::perspectiveRH_ZO(glm::radians(fov), aspectRatio, nearPlane, farPlane);
        return proj;
    }

    glm::mat4 generateViewProj() {

        return generateProj() * generateView();
    }

    glm::quat getRotationQuat() const {
        return glm::quat(glm::vec3(
            glm::radians(pitch),
            glm::radians(yaw),
            0.0f
        ));
    }

    void rotateCamera(float xoffset, float yoffset, bool constrainPitch = true) {

        // Constraint for pitch to prevent camera flipping
        const float PITCH_LIMIT = 89.0f;
        xoffset *= mouseSensitivity;
        yoffset *= mouseSensitivity;

        yaw += xoffset;
        pitch -= yoffset;

        // Apply constraints after changing pitch
        if (constrainPitch) {
            if (pitch > PITCH_LIMIT)
                pitch = PITCH_LIMIT;
            if (pitch < -PITCH_LIMIT)
                pitch = -PITCH_LIMIT;
        }

    }

    static MeshStandalone createMesh(
        float fovY = glm::radians(39.6f),
        float aspect = 16.f / 9.f,
        float nearDist = 0.1f,
        float farDist = 1.0f,
        float upTriScale = 0.3f)
    {
        const float hn = glm::tan(fovY * 0.5f) * nearDist;
        const float wn = hn * aspect;
        const float hf = glm::tan(fovY * 0.5f) * farDist;
        const float wf = hf * aspect;

        auto vert = [](float x, float y, float z) -> Vertex {
            return { .position = {x, y, z} };
        };

        MeshStandalone mesh;

        mesh.vertices.emplace_back(vert(-wn, hn, -nearDist)); // 0 near top-left
        mesh.vertices.emplace_back(vert(wn, hn, -nearDist)); // 1 near top-right
        mesh.vertices.emplace_back(vert(wn, -hn, -nearDist)); // 2 near bottom-right
        mesh.vertices.emplace_back(vert(-wn, -hn, -nearDist)); // 3 near bottom-left
        mesh.vertices.emplace_back(vert(-wf, hf, -farDist));  // 4 far top-left
        mesh.vertices.emplace_back(vert(wf, hf, -farDist));  // 5 far top-right
        mesh.vertices.emplace_back(vert(wf, -hf, -farDist));  // 6 far bottom-right
        mesh.vertices.emplace_back(vert(-wf, -hf, -farDist));  // 7 far bottom-left
        mesh.vertices.emplace_back(vert(0.f, hn * (1.f + upTriScale), -nearDist)); // 8 up apex

        mesh.indices = {
            // Near face — viewed from outside = from -Z direction, so reverse winding
            0, 2, 1,  0, 3, 2,

            // Far face — viewed from outside = from +Z direction (behind it)
            4, 6, 5,  4, 7, 6,

            // Top side — outside is above
            0, 5, 1,  0, 4, 5,

            // Bottom side — outside is below
            3, 6, 2,  3, 7, 6,

            // Left side — outside is to the left
            0, 3, 7,  0, 7, 4,

            // Right side — outside is to the right
            1, 6, 2,  1, 5, 6,

            // Up triangle — outside is above/front
            0, 1, 8,
        };

        return mesh;
    }
};