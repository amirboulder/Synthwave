#version 460 core
#extension GL_OES_standard_derivatives : enable
out vec4 fragColor;

in vec3 vertex;
in vec3 cameraPos;

void main() {
    vec2 coord = vertex.xz;
    vec2 cameraPosXZ = cameraPos.xz; 

    vec2 gridSize = vec2(4.0, 4.0);

    // Calculate distance from camera position for fade-out effect
    float distFromCamera = length(coord - cameraPosXZ);

    // Create fade-out factor (1.0 at camera position, 0.0 at fadeDistance and beyond)
    float fadeDistance = 250.0; // Adjust this value to control fade distance
    float fadeOut = 1.0 - smoothstep(fadeDistance * 0.4, fadeDistance, distFromCamera);

    vec2 scaledCoord = coord / gridSize;

    //grid calculation
    vec2 grid = abs(fract(scaledCoord - 0.4) - 0.5) / fwidth(scaledCoord);
    float line = min(grid.x, grid.y);


    float baseLineIntensity = 1.0 - min(line, 1.0);

    

    // Create bloom effect - softer falloff from the line
    float bloomIntensity = 1.0 - min(line * 0.35, 1.0);

    bloomIntensity *= fadeOut;

    bloomIntensity = pow(bloomIntensity, 2.0); // Control bloom shape

    

    // Combine base line and bloom
    float finalIntensity = max(baseLineIntensity, bloomIntensity * 0.8 * fadeOut);

   // finalIntensity *= fadeOut / 2;

    // Apply gamma correction
    // This seems to be doing the antialiasing
    finalIntensity = pow(finalIntensity, 1.0 / 2.2);

    float red = 0;
    float green = 0;
    float blue = 0;

    red = 1 * finalIntensity;
    green = 0.341 * finalIntensity;
    blue = 1 * finalIntensity;

    // Final color with fade-out applied to alpha as well
    fragColor = vec4(red, green, blue, 1.0 * fadeOut);
}