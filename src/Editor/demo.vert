#version 330 core

// Inputs (from vertex buffer)
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

// Outputs (to fragment shader)
out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

// Uniforms (set from CPU)
uniform mat4 model = mat4(
1, 0, 0, 0,
0, 1, 0, 0,
0, 0, 1, 0,
0, 0, 0, 1
);
uniform mat4 view;
uniform mat4 projection;

void main()
{
    // Compute world position of the vertex
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos = worldPos.xyz;

    // Transform normal (handle scaling correctly)
    Normal = mat3(transpose(inverse(model))) * aNormal;

    // Pass through texture coordinates
    TexCoord = aTexCoord;

    // Final clip space position
    gl_Position = projection * view * worldPos;
}