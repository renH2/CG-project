#version 330 core
out vec4 FragColor;
uniform vec4 pcolor;

void main()
{
    FragColor = vec4(1.0)*pcolor; // set alle 4 vector values to 1.0
}