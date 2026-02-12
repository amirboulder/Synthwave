#pragma once 


#include "RendererConfig.hpp"



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

    glm::mat4 generateview() {

        glm::mat4 view = glm::lookAtRH(position, position + front, up);
        return view;
    }
    glm::mat4 generateProj() {

        glm::mat4 proj = glm::perspectiveRH_ZO(glm::radians(fov), aspectRatio, nearPlane, farPlane);
        return proj;
    }

    glm::mat4 generateviewProj() {

        return generateProj() * generateview();
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


    
};