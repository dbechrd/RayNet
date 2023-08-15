#version 330 core
precision mediump float;

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
//uniform vec4 colDiffuse;

// Output fragment color
out vec4 finalColor;

void main()
{
    // Texel color fetching from texture sampler
    float sample = texture(texture0, fragTexCoord).a;

    // Calculate alpha using signed distance field (SDF)
    float distanceFromOutline = sample - 0.5;
    float distanceChangePerFragment = length(vec2(dFdx(distanceFromOutline), dFdy(distanceFromOutline)));
    float smoothing = 0.5;
    float alpha = smoothstep(-distanceChangePerFragment*smoothing, distanceChangePerFragment*smoothing, distanceFromOutline);

    // Calculate final fragment color
    finalColor.rgb = fragColor.rgb;
    finalColor.a = fragColor.a * alpha;
}
