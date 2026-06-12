#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;

// Input uniform values
uniform sampler2D texture0;

// Output fragment color
out vec4 finalColor;

void main()
{
    vec4 texColor = texture(texture0, fragTexCoord);
    finalColor = vec4(texColor.rgb, 1.0);
}
