#pragma once

#include "../ecs/components.hpp"

class GridGenerator  {
public:

    GridGenerator(){}

    static void generateGrid(int size, std::vector<Vertex> & vertices, std::vector <unsigned int> & indices) {

        // Generate vertices
       // adding one to rows and cols becasuet the loops go to <=
       vertices.reserve((size +1) * (size + 1));

        for (int r = 0; r <= size; r++) {
            for (int c = 0; c <= size; c++) {

                float x = static_cast<float>(c) - (size / 2.0f);
                float y = 0.0f;
                float z = static_cast<float>(r)  - (size / 2.0f);

                vertices.emplace_back();
                Vertex& currentVertex = vertices.back();

                currentVertex.position.x = x;
                currentVertex.position.y = y;
                currentVertex.position.z = z;

            }
        }

        indices.reserve(size * size * 6);

        // Generate indices for GL_TRIANGLES
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

};
