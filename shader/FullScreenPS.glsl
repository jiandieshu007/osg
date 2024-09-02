#version 330 core

in vec2 v_textureCoordinates;
out vec4 FragColor;

uniform sampler2D ScreenTexture;
uniform sampler2D RadarTexture;
uniform sampler2D ScreenDepth;
uniform sampler2D RadarDepth;

void main() {

    if( texture(ScreenDepth,v_textureCoordinates).r < texture(RadarDepth,v_textureCoordinates).r){
        FragColor = texture(ScreenTexture,v_textureCoordinates);
    }else{
        FragColor = texture(RadarTexture,v_textureCoordinates);
    }


}
