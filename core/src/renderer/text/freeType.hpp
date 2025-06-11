//#include <ft2build.h>
//#include FT_FREETYPE_H
#include "../Shader.hpp"
#include <map>
#include <iostream>

/*

// Character info struct
struct TextCharacter {
    GLuint textureID;
    glm::ivec2 size;
    glm::ivec2 bearing;
    GLuint advance;
};


// TODO Find out why text 
class TextRenderer {
private:

    GLuint VAO, VBO;
    glm::mat4 projection;
    std::map<char, TextCharacter> characters;

    Shader shader;

public:

    TextRenderer(int width, int height, const char* fontPath) 
        : shader("shaders/textShaders/textShader.vs", "shaders/textShaders/textShader.fs")
    {

        // Set up projection
        projection = glm::ortho(0.0f, static_cast<float>(width), 0.0f, static_cast<float>(height));

        // Set up VAO/VBO
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        

        // Initialize FreeType
        FT_Library ft;
        if (FT_Init_FreeType(&ft)) {
            std::cerr << "ERROR: Could not init FreeType Library" << std::endl;
            return;
        }

        // Load font
        FT_Face face;
        if (FT_New_Face(ft, fontPath, 0, &face)) {
            std::cerr << "ERROR: Failed to load font" << std::endl;
            FT_Done_FreeType(ft);
            return;
        }

        // Set font size
        FT_Set_Pixel_Sizes(face, 0, 48);

        // Disable byte-alignment restriction
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        // Load ASCII characters (32-126)
        for (unsigned char c = 32; c < 127; c++) {
            if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
                std::cerr << "ERROR: Failed to load glyph '" << c << "'" << std::endl;
                continue;
            }

            // Generate texture
            GLuint texture;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, face->glyph->bitmap.width, face->glyph->bitmap.rows,
                0, GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);

            // Set texture options
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            // Store character
            TextCharacter character = {
                texture,
                glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
                static_cast<GLuint>(face->glyph->advance.x)
            };
            characters[c] = character;
        }
        


        // Clean up FreeType
        FT_Done_Face(face);
        FT_Done_FreeType(ft);
    }

    void updateProjection(int width, int height) {
        projection = glm::ortho(0.0f, static_cast<float>(width), 0.0f, static_cast<float>(height));
    }


    //TODO look into making sure that text is always drawn on top of everything else.
    void renderText(const std::string& text, float x, float y, float scale, glm::vec3 color) {

        glUseProgram(shader.m_shaderID);
        glUniform3f(glGetUniformLocation(shader.m_shaderID, "textColor"), color.x, color.y, color.z);
        //glUniformMatrix4fv(glGetUniformLocation(shader.m_shaderID, "projection"), 5, GL_FALSE, &projection[0][0]);
        shader.setMat4("projection", projection);
        glActiveTexture(GL_TEXTURE0);
        glBindVertexArray(VAO);

        // Iterate through characters
        for (char c : text) {
            TextCharacter ch = characters[c];

            float xpos = x + ch.bearing.x * scale;
            float ypos = y - (ch.size.y - ch.bearing.y) * scale;
            float w = ch.size.x * scale;
            float h = ch.size.y * scale;

            // Update VBO for each character
            float vertices[6][4] = {
                {xpos, ypos + h, 0.0f, 0.0f},
                {xpos, ypos, 0.0f, 1.0f},
                {xpos + w, ypos, 1.0f, 1.0f},
                {xpos, ypos + h, 0.0f, 0.0f},
                {xpos + w, ypos, 1.0f, 1.0f},
                {xpos + w, ypos + h, 1.0f, 0.0f}
            };

            // Render glyph texture
            glBindTexture(GL_TEXTURE_2D, ch.textureID);
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            // Advance cursor
            x += (ch.advance >> 6) * scale; // Bitshift by 6 to get pixels
        }

        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);

    }

    ~TextRenderer() {
        for (auto& ch : characters) {
            glDeleteTextures(1, &ch.second.textureID);
        }
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteProgram(shader.m_shaderID);
    }

};


*/


