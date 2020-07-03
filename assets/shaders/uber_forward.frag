#version 330 core

#define LIGHTTYPE_SPOT 0
#define LIGHTTYPE_DIRECTION 1
#define LIGHTTYPE_POINT 2
#define NUM_LIGHTS 20

struct VertOutput {
        vec3 texCoord;
        vec4 position;
        vec3 position_vs;
        vec3 tangent_vs;
        vec3 binormal_vs;
        vec3 normal_vs;
};

struct Material {
        vec4 globalAmbient;
        
        vec4 ambientColor;
        vec4 emissiveColor;
        vec4 diffuseColor;
        vec4 specularColor;
        
        vec4 reflectance;
        
        float opacity;
        float specularPower;
        float indexOfRefraction;
        
        bool hasAmbientTexture;
        bool hasEmissiveTexture;
        bool hasDiffuseTexture;
        bool hasSpecularTexture;
        bool hasSpecularPowerTexture;
        bool hasNormalTexture;
        bool hasBumpTexture;
        bool hasOpacityTexture;
        
        float bumpIntensity;
        float specularScale;
        
        float alphaThreshold;
};

struct Light {
        bool enabled;
        
        vec4 color;
        float range;
        float intensity;

        uint type;
        
        vec4 position_ws;
        vec4 position_vs;
        vec4 direction_ws;
        vec4 direction_vs;
        float angle;
};

struct LightingResult {
        vec4 diffuse;
        vec4 specular;
}

in VertOutput vertOutput;

out vec4 FragColor;

uniform sampler2D ambientTexture;
uniform sampler2D emissiveTexture;
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;
uniform sampler2D specularPowerTexture;
uniform sampler2D normalTexture;
uniform sampler2D bumpTexture;
uniform sampler2D opacityTexture;

uniform Material material;
uniform Light lights[NUM_LIGHTS];

vec3 expandNormal(vec3 normal) {
        return normal * 2.0f - 1.0f;
}

vec4 doNormalMapping(mat3 TBN, sampler2D tex, vec2 uv) {
        vec3 normal = texture(tex, uv).xyz;
        normal = expandNormal(normal);
        normal *= TBN;
        return normalize(vec4(normal, 0));
}

// Might be incorrect way to do bump mapping
vec4 doBumpMapping(mat3 TBN, sampler2D tex, vec2 uv, float bumpScale) {
        float height = texture(tex, uv).x * bumpScale;
        float heightU = textureOffset(tex, uv, vec2(1, 0)).x * bumpScale;
        float heightV = textureOffset(tex, uv, vec2(0, 1)).x * bumpScale;

        vec3 p = vec3(0, 0, height);
        vec3 pU = vec3(1, 0, heightU);
        vec4 pV = vec3(0, 1, heightV);

        vec3 normal = cross(normalize(pU-p), normalize(pV-p));
        normal *= TBN;
        return vec4(normal, 0);
}

vec4 doDiffuse(Light light, vec4 L, vec4 N) {
        return light.color * max(dot(N, L), 0);
}

vec4 doSpecular(Light light, Material mat, vec4 L, vec4 N) {
        vec4 R = normalize(reflect(-L, N));
        return light.color * pow(max(dot(R, V), 0), material.SpecularPower);
}

float doAttenuation(Light light, float d) {
        return 1.0f - smoothstep(light.range * 0.75f, light.range, d);
}

LightingResult doPointLight(Light light, Material mat,
                            vec4 V, vec4 P, vec4 N) {
        vec4 L = light.position_vs - P;
        float dist = length(L);
        L /= dist;
        
        float attenuation = doAttenuation(light, dist);

        LightingResult result;
        result.diffuse = doDiffuse(light, L, N)
                * attenuation * light.intensity;
        result.specular = doSpecular(light, mat, V, L, N)
                * attenuation * light.intensity;
        return result;
}

float doSpotCone(Light light, float4 L) {
        float minCos = cos(radians(light.angle));
        float maxCos = mix(minCos, 1, 0.5f);
        float cosAngle = dot(light.direction_vs, -L);
        return smoothstep(minCos, maxCos, cosAngle);
}

LightingResult doSpotLight(Light light, Material mat,
                           vec4 V, vec4 P, vec4 N) {
        vec4 L = light.position_vs - P;
        float dist = length(L);
        L /= dist;

        float attenuation = doAttenuation(light, dist);
        float spotIntensity = doSpotCone(light, L);

        LightingResult result;
        result.diffuse = doDiffuse(light, L, N) *
                attenuation * spotIntensity * light.intensity;
        result.specular = doSpecular(light, mat, V, L, N) *
                attenuation * spotIntensity * light.intensity;
        return result;
}

LightingResult doDirectionalLight(Light light, Material mat,
                                  vec4 V, vec4 P, vec4 N) {
        vec4 L = normalize(-light.direction_vs);

        LightingResult result;
        result.diffuse = doDiffuse(light, L, N) * light.intensity;
        result.specular = doSpecular(light, mat, V, L, N) * light.intensity;
        return result;
}

LightingResult doLighting(Material mat, vec4 eyePos, vec4 P, vec4 N) {
        LightingResult totalResult;
        totalResult.difuse = 0;
        totalResult.specular = 0;
        
        vec4 V = normalize(eyePos - P);

        for (int i=0; i<NUM_LIGHTS; i++) {
                if (!lights[i].enabled) {
                        continue;
                }
                if (lights[i].type != LIGHTTYPE_DIRECTION &&
                    length(lights[i].position_vs - P) > lights[i].range) {
                        continue;
                }

                LightingResult result;
                
                switch (lights[i].type) {
                case LIGHTTYPE_SPOT:
                        result = DoSpotLight(lights[i], mat, V, P, N);
                        break;
                case LIGHTTYPE_DIRECTION:
                        result = DoDirectionalLight(lights[i], mat, V, P, N);
                        break;
                case LIGHTTYPE_POINT:
                        result = DoPointLight(lights[i], mat, V, P, N);
                        break;
                default:
                        result.diffuse = 0;
                        result.specular = 0;
                        break;
                }

                totalResults.diffuse += result.diffuse;
                totalResults.specular += result.specular;
        }

        return totalResults;
}

void main() {
        vec4 eyePos = vec4(0, 0, 0, 1);
        Material mat = material;

        vec4 diffuse = mat.diffuseColor;
        if (mat.hasDiffuseTexture) {
                vec4 diffuseTexture =
                        texture(diffuseTexture, vertOutput.texCoord);
                if (diffuse.xyz == vec3(0, 0, 0)) {
                        diffuse = diffuseTexture;
                } else {
                        diffuse *= diffuseTexture;
                }
        }

        float alpha = diffuse.w;
        if (mat.hasOpacityTexture) {
                alpha = texture(opacityTexture, vertOutput.texCoord).x;
        }

        vec4 ambient = mat.ambientColor;
        if (mat.hasAmbientTexture) {
                vec4 ambientTexture =
                        texture(ambientTexture, vertOutput.texCoord);
                if (ambient.xyz == vec3(0, 0, 0)) {
                        ambient = ambientTexture;
                } else {
                        ambient *= ambientTexture;
                }
        }
        ambient *= mat.globalAmbient;

        vec4 emissive = mat.emissiveColor;
        if (mat.hasEmissiveTexture) {
                vec4 emissiveTexture =
                        texture(emissiveTexture, vertOutput.texCoord);
                if (emissive.xyz == vec3(0, 0, 0)) {
                        emissive = emissiveTexture;
                } else {
                        emissive *= emissiveTexture;
                }
        }

        if (mat.hasSpecularPowerTexture) {
                mat.specularPower =
                        texture(specularPowerTexture, vertOutput.texCoord).x
                        * mat.specularScale;
        }

        vec4 N;
        if (mat.hasNormalTexture) {
                mat3 TBN = mat3(normalize(vertOutput.tangent_vs),
                                normalize(vertOutput.binormal_vs),
                                normalize(vertOutput.normal_vs));
                N = doNormalMapping(TBN, normalTexture, vertOutput.texCoord);
        } else if (mat.hasBumpTexture) {
                mat3 TBN = mat3(normalize(vertOutput.tangent_vs),
                                normalize(-vertOutput.binormal_vs),
                                normalize(vertOuput.normal_vs));
                N = doBumpMapping(TBN, bumpTexture, vertOutput.texCoord,
                                  mat.bumpIntensity);
        } else {
                N = normalize(vec4(vertOutput.normal_vs, 0));
        }

        vec4 P = vec4(vertOutput.position_vs, 1);
        LightingResult lit = doLighting(lights, mat, eyePos, P, N);

        diffuse *= vec4(lit.diffuse.xyz, 1.0f);

        vec4 specular = vec4(0, 0, 0, 0);
        if (mat.specularPower > 1.0f) {
                specular = mat.specularColor;
                if (mat.hasSpecularTexture) {
                        vec4 specularTex =
                                texture(specularTexture, vertOutput.texCoord);
                        if (specular.xyz == vec3(0, 0, 0)) {
                                specular = specularTex;
                        } else {
                                specular *= specularTex;
                        }
                }
                specular *= lit.specular;
        }

        FragColor = vec4((ambient + emissive + diffuse + specular).xyz,
                         alpha * mat.opacity)
}
