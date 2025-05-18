#pragma once 

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Camera {

public:

    glm::vec3 position;

    // Camera orientation vectors
    glm::vec3 front;     // Direction camera is looking
    glm::vec3 up;        // Up vector
    glm::vec3 right;     // Right vector
   // glm::vec3 worldUp;   // World up vector (usually 0,1,0)

    // Euler angles
    float yaw;           // Rotation around Y-axis
    float pitch;         // Rotation around X-axis

    // Camera parameters
    float fov;           // Field of view
    float nearPlane;     // Near clipping plane
    float farPlane;      // Far clipping plane

    // Camera movement
    //float movementSpeed;
    float mouseSensitivity;


private:

public:
    Camera(glm::vec3 position = glm::vec3(-10.0f, 0.0f, -10.0f))
        : position(position),
        front(glm::vec3(7.0f, 0.0f, -1.0f)),
       // worldUp(glm::vec3(0.0f, 1.0f, 0.0f)),
        yaw(45.0f),
        pitch(0.0f),
        fov(45.0f),
        nearPlane(0.1f),
        farPlane(1000.0f),
      //  movementSpeed(0.1),
        mouseSensitivity(0.5f)
    {
        
        updateCameraVectors();
    }


    glm::mat4 getViewMatrix() {
        return glm::lookAt(position, position + front, up);
    }

    glm::mat4 getProjectionMatrix(float aspectRatio) {

        return glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
    }


    void rotateCamera(float xoffset, float yoffset, bool constrainPitch = true) {
        xoffset *= mouseSensitivity;
        yoffset *= mouseSensitivity;

        yaw += xoffset;
        pitch += yoffset;

        // Apply constraints after changing pitch
        if (constrainPitch) {
            if (pitch > PITCH_LIMIT)
                pitch = PITCH_LIMIT;
            if (pitch < -PITCH_LIMIT)
                pitch = -PITCH_LIMIT;
        }

        updateCameraVectors();
    }

    // Constraints for pitch to prevent camera flipping
    const float PITCH_LIMIT = 89.0f;

    // Calculate the front, right and up vectors from euler angles
    void updateCameraVectors() {
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(front);

        right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
        up = glm::normalize(glm::cross(right, front));
    }
};
