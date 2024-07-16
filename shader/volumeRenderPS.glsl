#version 130

#define SkipAlpha (.95f)
#define PI (3.14159f)

uniform sampler3D volTex;
uniform sampler1D tfTex;
uniform vec3 eyePos;
uniform mat3 rotMat;
uniform float dt;
uniform float minLatitute;
uniform float maxLatitute;
uniform float minLongtitute;
uniform float maxLongtitute;
uniform float minHeight;
uniform float maxHeight;
uniform int maxStepCnt;

varying vec3 vertex;

struct Hit {
	int isHit;
	float tEntry;
	float tExit;
};
/*
* 函数: intersectSphere
* 功能: 返回视线与球相交的位置
* 参数:
* -- d: 视点出发的方向
* -- r: 球半径
*/
Hit intersectSphere(vec3 d, float r) {
	Hit hit = Hit(0, 0.f, 0.f);

	float tVert = -dot(eyePos, d);
	vec3 pVert = eyePos + tVert * d;

	float r2 = r * r;
	float pVert2 = pVert.x * pVert.x + pVert.y * pVert.y + pVert.z * pVert.z;
	if (pVert2 >= r2) return hit;
	float l = sqrt(r2 - pVert2);

	hit.isHit = 1;
	hit.tEntry = tVert - l;
	hit.tExit = tVert + l;
	return hit;
}

void main() {
	vec3 d = normalize(vertex - eyePos);
	Hit hit = intersectSphere(d, maxHeight);
	if (hit.isHit == 0)
		discard;
	float tEntry = hit.tEntry;
	vec3 outerX = eyePos + tEntry * d;

	vec3 pos = outerX;
	float r = sqrt(pos.x * pos.x + pos.y * pos.y);
	float lat = atan(pos.z / r);
	r = length(pos);
	float lon = atan(pos.y, pos.x);
	// 判断视线与外球第一个交点（即进入体的位置）所在象限
	int entryOutOfRng = 0;
	if (lat < minLatitute)
		entryOutOfRng |= 1;
	if (lat > maxLatitute)
		entryOutOfRng |= 2;
	if (lon < minLongtitute)
		entryOutOfRng |= 4;
	if (lon > maxLongtitute)
		entryOutOfRng |= 8;
	float tExit = hit.tExit;
	hit = intersectSphere(d, minHeight);
	if (hit.isHit != 0)
		tExit = hit.tEntry;
	// 判断视线离开体的位置所在象限
	pos = eyePos + tExit * d;
	r = sqrt(pos.x * pos.x + pos.y * pos.y);
	lat = atan(pos.z / r);
	lon = atan(pos.y, pos.x);
	// 若两个位置均不在范围内，且所在象限相同，则不需要计算该视线
	if ((entryOutOfRng & 1) != 0 && lat < minLatitute)
		discard;
	if ((entryOutOfRng & 2) != 0 && lat > maxLatitute)
		discard;
	if ((entryOutOfRng & 4) != 0 && lon < minLongtitute)
		discard;
	if ((entryOutOfRng & 8) != 0 && lon > maxLongtitute)
		discard;

	float hDlt = maxHeight - minHeight;
	float latDlt = maxLatitute - minLatitute;
	float lonDlt = maxLongtitute - minLongtitute;
	
	// 执行光线传播算法
	vec4 color = vec4(0, 0, 0, 0);
	float tAcc = 0.f;
	int stepCnt = 0;
	int realMaxStepCnt = maxStepCnt;
	pos = outerX;
	tExit -= tEntry;
	do {
		r = sqrt(pos.x * pos.x + pos.y * pos.y);
		lat = atan(pos.z / r);
		r = length(pos);
		lon = atan(pos.y, pos.x);

		if (lat < minLatitute || lat > maxLatitute || lon < minLongtitute || lon > maxLongtitute) {	}
		else {
			r = (r - minHeight) / hDlt;
			lat = (lat - minLatitute) / latDlt;
			lon = (lon - minLongtitute) / lonDlt;

			
			float scalar = texture(volTex, vec3(lon, lat, r)).r;
			vec4 tfCol;
			tfCol = vec4(48.0 / 256, 126.0 / 256, 106.0 / 256, 0.01) * scalar * 256;
			// if (scalar > 1.0 / 255)
			// 	tfCol = vec4(201.0 / 255, 78.0 / 255, 78.0 / 255, 0.01);
			// else if (scalar > 0.0)
			// 	tfCol = vec4(78.0 / 255, 201.0 / 255, 176.0 / 255, 0.01);
			// else
			// 	tfCol = vec4(0, 0, 0, 0);
			if (tfCol.a > 0.f) {
				color.rgb = color.rgb + (1.f - color.a) * tfCol.a * tfCol.rgb;
				 color.a = color.a + (1.f - color.a) * tfCol.a;
				if (color.a > SkipAlpha)
					break;
			}
			
		}

		pos += dt * d;
		tAcc += dt;
		++stepCnt;
	} while (tAcc < tExit && stepCnt <= realMaxStepCnt);
	gl_FragColor = color;
}