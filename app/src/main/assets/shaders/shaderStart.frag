#version 300 es
precision highp float;
in vec3 fNormal;
in vec4 fPosEye;
in vec2 fTexCoords;
in vec4 fragPosLightSpace;

out vec4 fColor;

// Lighting Uniforms
uniform vec3 lightDir;
uniform vec3 lightColor;

// Textures
uniform sampler2D texture_diffuse1;  // This handles Base Color / Diffuse
uniform sampler2D texture_specular1;
uniform sampler2D shadowMap;

uniform float fogD;

// Global Variables
vec3 ambient;
float ambientStrength = 0.5f; // Increased ambient so you can see the model even without light
vec3 diffuse;
vec3 specular;
float specularStrength = 0.5f;
float shininess = 32.0f;
float shadowFactor;

float computeFog()
{
    float fogDensity = fogD;
    float fragmentDistance = length(fPosEye);
    float fogFactor = exp(-pow(fragmentDistance * fogDensity, 2.0)); // Use 2.0 (float)
    return clamp(fogFactor, 0.0f, 1.0f);
}

float computeShadow()
{
    // If you haven't implemented the shadow map generation yet, return 0.0
    // otherwise the whole model will be black.
    // vec3 normalizedCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // ... (rest of shadow logic) ...
    return 0.0f; // Force no shadow for now to confirm texture works
}

void computeLightComponents()
{
    vec3 cameraPosEye = vec3(0.0f);
    vec3 normalEye = normalize(fNormal);
    vec3 lightDirN = normalize(lightDir);
    vec3 viewDirN = normalize(cameraPosEye - fPosEye.xyz);

    ambient = ambientStrength * lightColor;
    diffuse = max(dot(normalEye, lightDirN), 0.0f) * lightColor;

    vec3 reflection = reflect(-lightDirN, normalEye);
    float specCoeff = pow(max(dot(viewDirN, reflection), 0.0f), shininess);
    specular = specularStrength * specCoeff * lightColor;
}

void main()
{
    computeLightComponents();

    // 1. Get Texture Color
    vec4 texColor = texture(texture_diffuse1, fTexCoords);

    // DEBUG: If texture is transparent or missing, show PINK.
    if(texColor.a < 0.1) {
        // Uncomment this line to debug UV mapping if the model is black
        // fColor = vec4(1.0, 0.0, 1.0, 1.0);
        // return;
    }

    // 2. Apply Texture
    ambient *= texColor.rgb;
    diffuse *= texColor.rgb;
    specular *= texture(texture_specular1, fTexCoords).rgb;

    // 3. Compute Shadow
    shadowFactor = computeShadow();

    // 4. Combine Lighting
    // Ambient is NOT shadowed. Diffuse/Specular ARE shadowed.
    vec3 lighting = ambient + (1.0 - shadowFactor) * (diffuse + specular);

    vec3 finalColor = min(lighting, 1.0);

    // 5. Fog
    float fogFactor = computeFog();
    vec4 fogColor = vec4(0.5f, 0.5f, 0.5f, 1.0f);

    fColor = mix(fogColor, vec4(finalColor, 1.0), fogFactor);
}