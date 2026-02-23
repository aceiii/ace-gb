#version 100

precision mediump float;

varying vec2 fragTexCoord;

uniform sampler2D texture0;

const float renderHeight = 144;
float offset = 0.0;

void main() {
    float frequency = renderHeight/2.0;
    float globalPos = (fragTexCoord.y + offset)*frequency;
    float wavePos = cos((fract(globalPos) - 0.5)*3.14);
    vec4 texelColor = texture2D(texture0, fragTexCoord);
    gl_FragColor = mix(vec4(0.2, 0.5, 0.2, 0.0), texelColor, wavePos);
}
