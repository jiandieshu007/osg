#pragma once
#include <osgViewer/Viewer>
#include <functional>
#include <optional>
using namespace std;

struct llhRange{
	double minLatitude;
	double maxLatitude;
	double minLongitude;
	double maxLongitude;
	double minHeight;
	double maxHeight;
	llhRange() {
		minLatitude = osg::DegreesToRadians(-90.0);
		maxLatitude = osg::DegreesToRadians(+90.0);
		minLongitude = osg::DegreesToRadians(-180.0);
		maxLongitude = osg::DegreesToRadians(+180.0);
		minHeight = 0.0;
		maxHeight = 100000.0;
	}
	llhRange(double _minLatitude, double _maxLatitude,
		double _minLongitute, double _maxLongitute, 
		double _minHeight, double _maxHeight)
	:minLatitude(osg::DegreesToRadians(_minLatitude)), maxLatitude(osg::DegreesToRadians(_maxLatitude)),
		minLongitude(osg::DegreesToRadians(_minLongitute+180)), maxLongitude(osg::DegreesToRadians(_maxLongitute+180)),
		minHeight(_minHeight), maxHeight(_maxHeight){}
	llhRange(double _minLatitude, double _maxLatitude,
		double _minLongitute, double _maxLongitute,
		double _minHeight, double _maxHeight,int )
		:minLatitude(_minLatitude), maxLatitude(_maxLatitude),
		minLongitude(_minLongitute ), maxLongitude(_maxLongitute ),
		minHeight(_minHeight), maxHeight(_maxHeight) {}

	bool operator==(const llhRange& other) const {
		return minLatitude == other.minLatitude && maxLatitude == other.maxLatitude &&
			minLongitude == other.minLongitude && maxLongitude == other.maxLongitude &&
			minHeight == other.minHeight && maxHeight == other.maxHeight;
	}
	// 重载输出运算符
	friend ostream& operator<<(ostream& os, const llhRange& range) {
		os << "llhRange("
			<< "minLatitude: " << range.minLatitude << ", "
			<< "maxLatitude: " << range.maxLatitude << ", "
			<< "minLongitude: " << range.minLongitude << ", "
			<< "maxLongitude: " << range.maxLongitude << ", "
			<< "minHeight: " << range.minHeight << ", "
			<< "maxHeight: " << range.maxHeight << ")"<<endl;
		return os;
	}

};
// 计算两个 llhRange 区域的交集
inline std::optional<llhRange> calculateIntersection(const llhRange& a, const llhRange& b) {
	llhRange intersection;
	intersection.minLatitude = std::max(a.minLatitude, b.minLatitude);
	intersection.maxLatitude = std::min(a.maxLatitude, b.maxLatitude);
	intersection.minLongitude = std::max(a.minLongitude, b.minLongitude);
	intersection.maxLongitude = std::min(a.maxLongitude, b.maxLongitude);
	intersection.minHeight = std::max(a.minHeight, b.minHeight);
	intersection.maxHeight = std::min(a.maxHeight, b.maxHeight);

	// 检查是否存在重叠
	if (intersection.minLatitude > intersection.maxLatitude ||
		intersection.minLongitude > intersection.maxLongitude ||
		intersection.minHeight > intersection.maxHeight) {
		// 如果没有重叠，返回一个无效区域
		return nullopt;
	}

	return intersection;
}

struct llhRangeHash {
	size_t operator()(const llhRange& c) const {
		return hash<double>()(c.minLatitude) ^ hash<double>()(c.maxLatitude) ^
			hash<double>()(c.minLongitude) ^ hash<double>()(c.maxLongitude) ^
			hash<double>()(c.minHeight) ^ hash<double>()(c.maxHeight);
	}
};

inline void llh2xyz_Sphere(llhRange llh,
	float _lat, float _lon, float _h, float& x, float& y, float& z) {
	double hDlt = llh.maxHeight - llh.minHeight;
	double latDlt = llh.maxLatitude - llh.minLatitude;
	double lonDlt = llh.maxLongitude - llh.minLongitude;

	double lat = llh.minLatitude + latDlt * _lat;
	double lon = llh.minLongitude + lonDlt * _lon;
	double h = llh.minHeight + hDlt * _h + osg::WGS_84_RADIUS_EQUATOR;

	z = h * sin(lat);
	float r = h * cos(lat);
	x = r * cos(lon);
	y = r * sin(lon);
}

// transform to llhRange
inline void llh2xyz_Ellipsoid(llhRange llh,
	double _lat, double _lon, double _h, double& x, double& y, double& z) {
	double hDlt = llh.maxHeight - llh.minHeight;
	double latDlt = llh.maxLatitude - llh.minLatitude;
	double lonDlt = llh.maxLongitude - llh.minLongitude;

	double lat = llh.minLatitude + latDlt * _lat;
	double lon = llh.minLongitude + lonDlt * _lon;
	double h = llh.minHeight + hDlt * _h;

	auto pEllModel = new osg::EllipsoidModel();
	pEllModel->convertLatLongHeightToXYZ(lat, lon, h, x, y, z);
}

inline void llh2xyz_Ellipsoid(double _lat, double _lon, double _h, double& x, double& y, double& z) {

	double lat = _lat;
	double lon = _lon;
	double h = _h;
	auto pEllModel = new osg::EllipsoidModel();
	pEllModel->convertLatLongHeightToXYZ(lat, lon, h, x, y, z);
}

struct ModelViewProjectionMatrixCallback : public osg::Uniform::Callback
{
	ModelViewProjectionMatrixCallback(osg::Camera* camera) :
		_camera(camera) {
	}

	virtual void operator()(osg::Uniform* uniform, osg::NodeVisitor* nv) {
		osg::Matrixd viewMatrix = _camera->getViewMatrix();
		osg::Matrixd modelMatrix = osg::computeLocalToWorld(nv->getNodePath());
		osg::Matrixd modelViewProjectionMatrix = modelMatrix * viewMatrix * _camera->getProjectionMatrix();
		uniform->set(modelViewProjectionMatrix);
	}

	osg::Camera* _camera;
};

struct ColorCallBack : public osg::Uniform::Callback {
	ColorCallBack(osg::Vec4* color) 
		: mainColor(color)
	{}

	virtual void operator()(osg::Uniform* uniform, osg::NodeVisitor* nv) {
		uniform->set(mainColor);
	}
	osg::Vec4* mainColor;
};

struct CameraEyeCallback : public osg::Uniform::Callback
{
	CameraEyeCallback(osg::Camera* camera) :
		_camera(camera) {
	}

	virtual void operator()(osg::Uniform* uniform, osg::NodeVisitor* /*nv*/) {
		osg::Vec3f eye, center, up;
		_camera->getViewMatrixAsLookAt(eye, center, up);
		osg::Vec4f eye_vec = osg::Vec4f(eye.x(), eye.y(), eye.z(), 1);
		uniform->set(eye_vec);
	}
	osg::Camera* _camera;
};