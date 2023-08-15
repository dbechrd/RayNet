#version 330

in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec4 vertexColor;
out vec2 fragTexCoord;
out vec4 fragColor;

uniform mat4 mvp;
uniform vec2 screenSize;

vec4 AlignToPixel(vec4 pos)
{
    vec2 halfScreen = vec2(screenSize.x, screenSize.y).xy * 0.5;
    pos.xy = round((pos.xy / pos.w) * halfScreen) / halfScreen * pos.w;
    return pos;
}

void main()
{
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    gl_Position = AlignToPixel(mvp*vec4(vertexPosition, 1.0));
}