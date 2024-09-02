#version 330 core
 
uniform sampler2D inputRadar;

in vec2 v_textureCoordinates;

uniform vec4 mainColor;
uniform vec4 blendColor;

out vec4 fragColor;

bool approximatelyEqual(vec4 a, vec4 b, float epsilon) {
    return all(lessThan(abs(a - b), vec4(epsilon)));
}

void main(){
	if(approximatelyEqual(texture(inputRadar,v_textureCoordinates) , vec4(0,0,0,0),0.005) ){
		fragColor = vec4(0,0,0,0);
	}else if(approximatelyEqual(texture(inputRadar,v_textureCoordinates) , mainColor, 0.005 ) ){
		fragColor = mainColor;
	}else{
		fragColor = blendColor;
	}
}