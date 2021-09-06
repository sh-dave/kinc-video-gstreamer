#version 450

// uniform mat3 mvp;

// in vec3 pos;
// out vec2 texcoord;
// in vec2 tex;

// void main() {
//     gl_Position = vec4(mvp * vec3(pos.x, pos.y, 0.5), 1.0);
//     texcoord = tex;
// }

in vec3 vertexPosition;
in vec2 texPosition;
in vec4 vertexColor;
uniform mat4 projectionMatrix;
out vec2 texCoord;
out vec4 color;

void main() {
	gl_Position = projectionMatrix * vec4(vertexPosition, 1.0);
	texCoord = texPosition;
	color = vertexColor;
}
