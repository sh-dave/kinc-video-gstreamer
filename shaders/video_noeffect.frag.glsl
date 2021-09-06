#version 450

uniform sampler2D tex;
in vec2 texCoord;
in vec4 color;
float u_yWidth; // TODO (DK) make uniform?
float u_yHeight; // TODO (DK) make uniform?
out vec4 fragColor;

vec4 yuv2rgb() {
    vec2 texc = texCoord;
    vec3 yuv;

// r
    yuv.x = texture(tex, texc).x;
    // return vec4(yuv, 1.0);
    // return vec4(1.0, 1.0, 0.0, 1.0);

// g
    texc.x /= 2.0;
    if ((texc.y - floor(texc.y)) == 0.0) {
        texc.x += (u_yWidth / 2.0);
    }
    texc.y = u_yHeight + (texc.y / 4.0);
    yuv.y = texture(tex, texc).x;

// b
    texc.y += (u_yHeight / 4.0);
    yuv.z = texture(tex, texc).x;

    yuv += vec3(-0.0625, -0.5, -0.5);

    vec3 rgb = vec3(
        dot(yuv, vec3(1.164, 0.0, 1.596)),
        dot(yuv, vec3(1.164, -0.391, -0.813)),
        dot(yuv, vec3(1.164, 2.018, 0.0))
    );

    return vec4(rgb, 1.0);// * color;
}

void main() {
    u_yWidth = 1;//854.0;
    u_yHeight = 0.666666667;//720; //480.0; //  * 1.5;
    fragColor = yuv2rgb();
}
