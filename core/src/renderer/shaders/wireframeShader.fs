// based on https://developer.download.nvidia.com/SDK/10/direct3d/Source/SolidWireframe/Doc/SolidWireframe.pdf ai generated
#version 460 core
in vec3 distances;
out vec4 FragColor;

uniform float lineWidth = 2.0;
uniform vec4 lineColor = vec4(0.188, 0.486, 0.976, 1.0);
uniform vec4 fillColor = vec4(0.0, 0.0, 0.0, 1.0);

float smoothEdge(float d, float w) {
    float edge = w * 0.5;
    if (d > edge) return 0.0;
    float x = d / edge;
    return exp2(-2.0 * x * x);  // Smoothing function from the paper
}

void main() {
    // Find minimum distance to any edge
    float minDistance = min(min(distances.x, distances.y), distances.z);

    // Calculate wireframe alpha using smoothing function
    float lineAlpha = smoothEdge(minDistance, lineWidth);

    // If fragment is beyond line width, use fill color only
    if (minDistance > lineWidth * 0.5) {
        FragColor = fillColor;
    }
    // Blend line color with fill color near edges
    else {
        // Blend between fill color and line color based on distance
        vec4 baseColor = mix(fillColor, lineColor, lineAlpha);
        FragColor = vec4(baseColor.rgb, max(fillColor.a, lineColor.a * lineAlpha));
    }
}