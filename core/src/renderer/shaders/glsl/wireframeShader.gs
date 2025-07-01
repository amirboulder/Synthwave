// based on https://developer.download.nvidia.com/SDK/10/direct3d/Source/SolidWireframe/Doc/SolidWireframe.pdf ai generated
#version 460 core
layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

out vec3 distances;  // Distance to edges for each vertex

// Screen space conversion
vec2 toScreenSpace(vec4 pos) {
    vec2 screenPos = pos.xy / pos.w;
    return (screenPos * 0.5 + 0.5) * vec2(1280.0, 720.0); // Adjust for your screen resolution
}

void main() {
    // Get vertex positions in screen space
    vec2 p0 = toScreenSpace(gl_in[0].gl_Position);
    vec2 p1 = toScreenSpace(gl_in[1].gl_Position);
    vec2 p2 = toScreenSpace(gl_in[2].gl_Position);

    // Calculate triangle heights (distance from vertex to opposite edge)
    float area = abs((p1.x - p0.x) * (p2.y - p0.y) - (p2.x - p0.x) * (p1.y - p0.y));
    float h0 = area / length(p1 - p2);  // Height from vertex 0
    float h1 = area / length(p2 - p0);  // Height from vertex 1
    float h2 = area / length(p0 - p1);  // Height from vertex 2

    // Emit vertices with their corresponding edge distances
    gl_Position = gl_in[0].gl_Position;
    distances = vec3(h0, 0.0, 0.0);
    EmitVertex();

    gl_Position = gl_in[1].gl_Position;
    distances = vec3(0.0, h1, 0.0);
    EmitVertex();

    gl_Position = gl_in[2].gl_Position;
    distances = vec3(0.0, 0.0, h2);
    EmitVertex();

    EndPrimitive();
}