#pragma once
#include <vector>
#include <glm/glm.hpp>

#include "../ecs/components.hpp"

// used for testing
class Grid  {
public:

    std::vector<glm::vec3> gridVertices;
    std::vector<unsigned int> gridIndices;

    GLuint shaderID;
    GLuint VAO;
  
    Grid(GLuint shaderID , int rows, int cols)
       : shaderID(shaderID)
    {
        generateGrid(rows, cols);
    }

    void generateGrid(int rows, int cols) {

        gridVertices.reserve((rows + 1) * (cols + 1));

        // Generate vertices
        for (int r = 0; r <= rows; r++) {
            for (int c = 0; c <= cols; c++) {
                float x = static_cast<float>(c) * 4;
                float y = 0.0f; // Y is up, grid is on XZ plane
                float z = static_cast<float>(r) * 4; // Z instead of Y for depth
                gridVertices.emplace_back(glm::vec3(x,y,z));
            }
        }

        gridIndices.reserve(rows * cols * 6);

        // Generate indices for GL_TRIANGLES
        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < cols; c++) {
                // Define two triangles for each grid cell
                int topLeft = r * (cols + 1) + c;
                int topRight = topLeft + 1;
                int bottomLeft = (r + 1) * (cols + 1) + c;
                int bottomRight = bottomLeft + 1;

                // First triangle (top-left, bottom-left, top-right)
                gridIndices.emplace_back(topLeft);
                gridIndices.emplace_back(bottomLeft);
                gridIndices.emplace_back(topRight);

                // Second triangle (top-right, bottom-left, bottom-right)
                gridIndices.emplace_back(topRight);
                gridIndices.emplace_back(bottomLeft);
                gridIndices.emplace_back(bottomRight);
            }
        }

        // Center the grid
        //TODO turn these offsets into parameters
        for (int i = 0; i < gridVertices.size(); i++) {
            gridVertices[i].x -= 50;
            gridVertices[i].z -= 50;
        }

        GLuint VBO, EBO;

        // Create buffers
        glCreateVertexArrays(1, &VAO);
        glCreateBuffers(1, &VBO);
        glCreateBuffers(1, &EBO);

        // Upload vertex data to VBO
        glNamedBufferData(VBO, gridVertices.size() * sizeof(glm::vec3), gridVertices.data(), GL_STATIC_DRAW);
        // Upload index data to EBO
        glNamedBufferData(EBO, gridIndices.size() * sizeof(unsigned int), gridIndices.data(), GL_STATIC_DRAW);

        glEnableVertexArrayAttrib(VAO, 0);
        glVertexArrayAttribBinding(VAO, 0, 0);
        glVertexArrayAttribFormat(VAO, 0, 3, GL_FLOAT, GL_FALSE, 0);

        // Bind VBO to VAO for vertex data
        glVertexArrayVertexBuffer(VAO, 0, VBO, 0, sizeof(glm::vec3));

        // Bind EBO to VAO for index data
        glVertexArrayElementBuffer(VAO, EBO);

    }

    void draw() {
        glUseProgram(shaderID);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, gridIndices.size(), GL_UNSIGNED_INT, 0);
    }
   
};


class GridGenerator  {
public:

    GridGenerator(){}

    static void generateGrid(int rows, int cols, std::vector<VertexData> & vertices, std::vector <unsigned int> & indices) {

        // Generate vertices
       // adding one to rows and cols becasuet the loops go to <=
       vertices.reserve((rows+1) * (cols + 1));

        for (int r = 0; r <= rows; r++) {
            for (int c = 0; c <= cols; c++) {

                float x = static_cast<float>(c) - (cols  / 2.0f);
                float y = 0.0f;
                float z = static_cast<float>(r)  - (rows  / 2.0f);

                vertices.emplace_back();
                VertexData & currentVertex = vertices.back();

                currentVertex.vertex.x = x;
                currentVertex.vertex.y = y;
                currentVertex.vertex.z = z;

            }
        }

        indices.reserve(rows * cols * 6);

        // Generate indices for GL_TRIANGLES
        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < cols; c++) {
                // Define two triangles for each grid cell
                int topLeft = r * (cols + 1) + c;
                int topRight = topLeft + 1;
                int bottomLeft = (r + 1) * (cols + 1) + c;
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

};
