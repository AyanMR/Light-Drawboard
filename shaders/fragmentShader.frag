#version 330 core

in vec2  texCoord;
out vec4 FragColor;

uniform sampler2D drawboardTexture;

void main()
{
    vec4 color = texture(drawboardTexture, vec2(texCoord.x, 1.0 - texCoord.y));

    if (color.a == 0.0)
        discard;

    FragColor = color;
}
