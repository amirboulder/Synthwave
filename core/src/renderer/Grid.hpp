#pragma once
#include <vector>
#include <glm/glm.hpp>

struct Grid {

	std::vector<glm::vec3> gridVertices;
	std::vector<unsigned int> GridIndices;

	Shader gridShader;
	GLuint VBO, VAO, EBO;
	int rows,cols;


	Grid(const char* vertexShaderPath, const char* fragmentShaderPath,int rows, int cols, const char* GeometryShaderPath = nullptr)
		:	gridShader(vertexShaderPath,fragmentShaderPath, GeometryShaderPath),
			rows(rows),
			cols(cols)

	{

			generateGrid(rows, cols);
	}

	void generateGrid(int rows, int cols) {

		

		// put a vertex in each integer
		for (int r = 0; r <= rows; r++) {
			for (int c = 0; c <= cols; c++) {
				float x = static_cast<float>(c)  *4;
				float y = 0.0f; // Y is up, grid is on XZ plane
				float z = static_cast<float>(r) * 4; // Z instead of Y for depth
				gridVertices.push_back({ x, y, z });
			}
		}

		// Step 2: Generate indices for GL_TRIANGLE_STRIP
		for (int r = 0; r < rows; r++) {
			if (r > 0) {
				// Insert a degenerate triangle (repeat the last vertex of the previous row)
				GridIndices.push_back((r * (cols + 1)) + cols);
				GridIndices.push_back((r * (cols + 1)));
			}

			for (int c = 0; c <= cols; c++) {
				int top = r * (cols + 1) + c;
				int bottom = (r + 1) * (cols + 1) + c;

				GridIndices.push_back(top);
				GridIndices.push_back(bottom);
			}
		}
		// Debugging: Print the vertex and index data
		/*
		std::cout << "Vertices: \n";
		for (size_t i = 0; i < gridVertices.size(); i++) {
			std::cout << "Vertex " << i << ": " << gridVertices[i].x << " "
				<< gridVertices[i].y << " " << gridVertices[i].z << "\n";
		}


		std::cout << "Indices: \n";
		for (size_t i = 0; i < GridIndices.size(); i++) {
			std::cout << "Index " << i << ": " << GridIndices[i] << "\n";
		}

		*/
		//std::cout << "Size of Vertices: " << gridVertices.size() << '\n';
		// std::cout << "Size of Indices: " << GridIndices.size() << "\n";

		//TODO MAKE THIS A SEPRATE FUNCTION
		for (int i = 0; i < gridVertices.size(); i++) {

			gridVertices[i].x -= 50;
			//gridVertices[i].y -= 10;
			gridVertices[i].z -= 50;
		}

		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glGenBuffers(1, &EBO);

		// BIND the Vertex Array object
		glBindVertexArray(VAO);

		// bind the vertex buffer object
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		//configure the currently bound buffer

		glBufferData(GL_ARRAY_BUFFER, gridVertices.size() * sizeof(glm::vec3), &gridVertices[0], GL_STATIC_DRAW);
		//BiND ELEMENT buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, GridIndices.size() * sizeof(unsigned int), &GridIndices[0], GL_STATIC_DRAW);
		//tell open gl how to interprate the data
		//position arrtibute
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);


	}

	
	void draw() {

		gridShader.use();

		glBindVertexArray(VAO);

		// Use GL_TRIANGLE_STRIP instead of GL_TRIANGLES
		glDrawElements(GL_TRIANGLE_STRIP, GridIndices.size(), GL_UNSIGNED_INT, 0);

		glBindVertexArray(0);

	}

};




