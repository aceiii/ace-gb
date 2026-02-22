#version 330

in vec2 fragTexCoord;

out vec4 finalColor;

uniform sampler2D texture0;

const float renderHeight = 144;
float offset = 0.0;

void main() {
    float frequency = renderHeight/2.0;
    float globalPos = (fragTexCoord.y + offset)*frequency;
    float wavePos = cos((fract(globalPos) - 0.5)*3.14);
    vec4 texelColor = texture(texture0, fragTexCoord);
    finalColor = mix(vec4(0.2, 0.5, 0.2, 0.0), texelColor, wavePos);
}
