#version 330 core

struct AppData {
        vec3 position;
        vec3 texCoord;
        vec3 normal;
        vec3 tangent;
        vec3 binormal;
};

struct VertOutput {
        vec3 texCoord;
        vec4 position;
        vec3 position_vs;
        vec3 tangent_vs;
        vec3 binormal_vs;
        vec3 normal_vs;
};

layout (location = 0) in AppData appData;

out VertOutput vertOutput;

uniform mat4 modelViewProjection;
uniform mat4 modelView;

void main() {
        vertOuput.texCoord = appData.texCoord;
        vertOutput.position = modelViewProjection * vec4(appData.position, 1);
        vertOutput.position_vs = (modelView * vec4(appData.position, 1)).xyz;
        vertOutput.tangent_vs = mat3(modelView) * appData.tangent;
        vertOutput.binormal_vs = mat3(modelView) * appData.binormal;
        vertOutput.normal = mat3(modelView) * appData.normal;
}
