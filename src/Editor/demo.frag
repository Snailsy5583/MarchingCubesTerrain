#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

// Light + camera uniforms
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;
uniform vec3 baseColor = vec3(1.f, 0.f, 1.f);

// Material
uniform bool useTexture = false;
uniform sampler2D texture1;

void main()
{
    // Normalize inputs
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    vec3 viewDir = normalize(viewPos - FragPos);

    // --- Ambient ---
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;

    // --- Diffuse ---
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // --- Blinn-Phong Specular ---
    float specularStrength = 0.5;
    vec3 halfwayDir = normalize(lightDir + viewDir);

    float shininess = 32.0;// higher = tighter highlight
    float spec = pow(max(dot(norm, halfwayDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * lightColor;


    // Final color
    vec3 color = useTexture ? texture(texture1, TexCoord).rgb : baseColor;
    vec3 result = (ambient + diffuse + specular) * color;
    FragColor = vec4(result, 1.0);
    FragColor = vec4(1, 0, 1, 1.0);
}