#version 330 core
out vec4 FragColor;
  
in vec3 vertexColor; // the input variable from the vertex shader (same name and same type)  
in vec2 TexCoord;

uniform sampler2D texture1;
uniform sampler2D texture2;

void main()
{

    //FragColor = texture(texture1, TexCoord) * vec4(vertexColor, 1.0f);

   //FragColor = mix(texture(texture1, TexCoord), texture(texture2, TexCoord), 0.2);
   FragColor = mix(texture(texture1, TexCoord), texture(texture2, TexCoord), 0.2);
} 