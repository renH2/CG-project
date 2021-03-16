#version 330 core

out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D pic;

void main()
{             
     vec3 Color = texture(pic, TexCoords).rgb;
     //FragColor = vec4(Color, 1.0);
    FragColor = texture(pic, TexCoords);
}