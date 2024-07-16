#include "Radar.h"

#include <unordered_set>
#include <osg/BlendFunc>
#include <osg/BlendEquation>
#include <osg/BlendColor>

// ui part
// color and drawway

static osg::ref_ptr<osg::Uniform> colorUniform = new osg::Uniform("mainColor", osg::Vec4f(78.0 / 255, 201.0 / 255, 176.0 / 255, 0.6));
//// for overlap part
static osg::ref_ptr<osg::BlendColor> blendColor = new osg::BlendColor();
static osg::Vec4 color = { 0.5,0.5,0.3,0.2 };
static osg::ref_ptr<osg::LineWidth> lineWidth = new osg::LineWidth(2);
enum DrawWay
{
	surface,
	line
};
static DrawWay usePattern = surface ;

void Radar::Radar::updateR(double value) {
	osg::Vec4 color;
	colorUniform->get(color);
	color.r() = value;
	rt->getOrCreateStateSet()->getUniform("mainColor")->set(color);
};
void Radar::Radar::updateG(double value) {
	osg::Vec4 color;
	colorUniform->get(color);
	color.g() = value;
	rt->getOrCreateStateSet()->getUniform("mainColor")->set(color);
};
void Radar::Radar::updateB(double value) {
	osg::Vec4 color;
	colorUniform->get(color);
	color.b() = value;
	rt->getOrCreateStateSet()->getUniform("mainColor")->set(color);
};
void Radar::Radar::updateA(double value) {
	osg::Vec4 color;
	colorUniform->get(color);
	color.a() = value;
	rt->getOrCreateStateSet()->getUniform("mainColor")->set(color);
};
void Radar::Radar::updateLineWidth(double value) {
	lineWidth->setWidth(value);
};
void Radar::Radar::updateDrawStyle(int index) {
	if (index == 0) {
		osg::ref_ptr<osg::PolygonMode> polyMode = new osg::PolygonMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::FILL);
		rt->getOrCreateStateSet()->setAttribute(polyMode);
		

	}
	else {
		osg::ref_ptr<osg::PolygonMode> polyMode = new osg::PolygonMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE);
			rt->getOrCreateStateSet()->setAttribute(polyMode);
			rt->getOrCreateStateSet()->setAttributeAndModes(lineWidth, osg::StateAttribute::ON);
	}
}
void Radar::Radar::updateOverlapR(double value)
{

	color.r() = value;
	blendColor->setConstantColor(color);
	rt->getOrCreateStateSet()->setAttribute(blendColor, osg::StateAttribute::ON);
}
void Radar::Radar::updateOverlapG(double value)
{

	color.g() = value;

	blendColor->setConstantColor(color);
	rt->getOrCreateStateSet()->setAttribute(blendColor, osg::StateAttribute::ON);
}
void Radar::Radar::updateOverlapB(double value)
{

	color.b() = value;

	blendColor->setConstantColor(color);
	rt->getOrCreateStateSet()->setAttribute(blendColor, osg::StateAttribute::ON);
}
void Radar::Radar::updateOverlapA(double value)
{
	color.a() = value;

	blendColor->setConstantColor(color);
	rt->getOrCreateStateSet()->setAttribute(blendColor, osg::StateAttribute::ON);

}

namespace lapCa {
	inline bool isOverlapping(const llhRange& range1, const llhRange& range2) {
		return !(range1.maxLatitude <= range2.minLatitude || range1.minLatitude >= range2.maxLatitude ||
			range1.maxLongtitude <= range2.minLongtitude || range1.minLongtitude >= range2.maxLongtitude ||
			range1.maxHeight <= range2.minHeight || range1.minHeight >= range2.maxHeight);
	}

	inline llhRange getOverlap(const llhRange& range1, const llhRange& range2) {
		llhRange ret;
		ret.minLatitude = max(range1.minLatitude, range2.minLatitude);
		ret.maxLatitude = min(range1.maxLatitude, range2.maxLatitude);
		ret.minLongtitude = max(range1.minLongtitude, range2.minLongtitude);
		ret.maxLongtitude = min(range1.maxLongtitude, range2.maxLongtitude);
		ret.minHeight = max(range1.minHeight, range2.minHeight);
		ret.maxHeight = min(range1.maxHeight, range2.maxHeight);

		return ret;
	}
	llhRange overlap_range(double min1, double max1, double min2, double max2) {
		return { max(min1, min2), min(max1, max2), 0, 0, 0, 0,1 }; // 只用前两个值，后面是占位符
	}

	void subtract_cubes(llhRange big_cube, llhRange small_cube, vector<llhRange>& remaining_parts) {
		double overlap_lat_min, overlap_lat_max, overlap_long_min, overlap_long_max, overlap_height_min, overlap_height_max;

		// 计算重叠区域
		llhRange lat_overlap = overlap_range(big_cube.minLatitude, big_cube.maxLatitude, small_cube.minLatitude, small_cube.maxLatitude);
		llhRange long_overlap = overlap_range(big_cube.minLongtitude, big_cube.maxLongtitude, small_cube.minLongtitude, small_cube.maxLongtitude);
		llhRange height_overlap = overlap_range(big_cube.minHeight, big_cube.maxHeight, small_cube.minHeight, small_cube.maxHeight);

		overlap_lat_min = lat_overlap.minLatitude;
		overlap_lat_max = lat_overlap.maxLatitude;
		overlap_long_min = long_overlap.minLatitude; // 使用minLatitude代替minLongtitude
		overlap_long_max = long_overlap.maxLatitude; // 使用maxLatitude代替maxLongtitude
		overlap_height_min = height_overlap.minLatitude; // 使用minLatitude代替minHeight
		overlap_height_max = height_overlap.maxLatitude; // 使用maxLatitude代替maxHeight



		// 上部
		if (big_cube.maxHeight > overlap_height_max) {
			remaining_parts.push_back({ big_cube.minLatitude, big_cube.maxLatitude, big_cube.minLongtitude, big_cube.maxLongtitude, overlap_height_max, big_cube.maxHeight,1 });
		}

		// 下部
		if (big_cube.minHeight < overlap_height_min) {
			remaining_parts.push_back({ big_cube.minLatitude, big_cube.maxLatitude, big_cube.minLongtitude, big_cube.maxLongtitude, big_cube.minHeight, overlap_height_min,1 });
		}

		// 前部
		if (big_cube.maxLongtitude > overlap_long_max) {
			remaining_parts.push_back({ big_cube.minLatitude, big_cube.maxLatitude, overlap_long_max, big_cube.maxLongtitude, overlap_height_min, overlap_height_max,1 });
		}

		// 后部
		if (big_cube.minLongtitude < overlap_long_min) {
			remaining_parts.push_back({ big_cube.minLatitude, big_cube.maxLatitude, big_cube.minLongtitude, overlap_long_min, overlap_height_min, overlap_height_max,1 });
		}

		// 左部
		if (big_cube.minLatitude < overlap_lat_min) {
			remaining_parts.push_back({ big_cube.minLatitude, overlap_lat_min, overlap_long_min, overlap_long_max, overlap_height_min, overlap_height_max,1 });
		}

		// 右部
		if (big_cube.maxLatitude > overlap_lat_max) {
			remaining_parts.push_back({ overlap_lat_max, big_cube.maxLatitude, overlap_long_min, overlap_long_max, overlap_height_min, overlap_height_max,1 });
		}
	}

}


osg::ref_ptr<osg::Camera> Radar::Radar::addRadarDrawPass()
{
	osg::ref_ptr<osg::Camera> ret = new osg::Camera;

	rt = new osg::Geode;
	for (int i = 0; i < ranges.size(); ++i) {
		rt->addDrawable(Geos[i]);
	}

	osg::ref_ptr<osg::Shader> surfaceVertexShader = new osg::Shader(osg::Shader::VERTEX);
	osg::ref_ptr<osg::Shader> surfaceFragmentShader = new osg::Shader(osg::Shader::FRAGMENT);
	surfaceVertexShader->loadShaderSourceFromFile(std::string(OSG_3D_VIS_SHADER_PREFIX) + "RadarRoundScanVS.glsl");
	surfaceFragmentShader->loadShaderSourceFromFile(std::string(OSG_3D_VIS_SHADER_PREFIX) + "RadarRoundScanPS.glsl");
	osg::ref_ptr<osg::Program> surfaceProgram = new osg::Program;
	surfaceProgram->addShader(surfaceVertexShader);
	surfaceProgram->addShader(surfaceFragmentShader);

	osg::ref_ptr<osg::StateSet> surfaceStateSet = rt->getOrCreateStateSet();
	surfaceStateSet->setAttributeAndModes(surfaceProgram);

	// set color
	surfaceStateSet->addUniform(colorUniform);

	surfaceStateSet->addUniform(mvpUniform);

	// Blend Rendering Related 使用颜色的ALPHA通道进行透明材质渲染
	surfaceStateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
	surfaceStateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);


	osg::ref_ptr<osg::BlendColor> blendColor = new osg::BlendColor();
	blendColor->setConstantColor(color); // 设置混合颜色
	surfaceStateSet->setAttribute(blendColor, osg::StateAttribute::ON);

	ret->addChild(rt);


	ret->setClearColor(osg::Vec4());
	ret->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	ret->setViewport(0, 0, width, height);

	ret->setRenderOrder(osg::Camera::PRE_RENDER, 1);

	ret->attach(osg::Camera::COLOR_BUFFER, radarColorTexture);
	ret->attach(osg::Camera::DEPTH_BUFFER, radarDepthTexture);
	ret->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
	return ret;
}

osg::ref_ptr<osg::Camera> Radar::Radar::addBlendPass()
{
	osg::ref_ptr<osg::Camera> ret = new osg::Camera;

	auto geod = createFullScreenQuad();

	osg::ref_ptr<osg::Shader> VertexShader = new osg::Shader(osg::Shader::VERTEX);
	osg::ref_ptr<osg::Shader> FragmentShader = new osg::Shader(osg::Shader::FRAGMENT);
	VertexShader->loadShaderSourceFromFile(std::string(OSG_3D_VIS_SHADER_PREFIX) + "fullscreenQuad.vs");
	FragmentShader->loadShaderSourceFromFile(std::string(OSG_3D_VIS_SHADER_PREFIX) + "FullScreenPS.glsl");
	osg::ref_ptr<osg::Program> Program = new osg::Program;
	Program->addBindAttribLocation("Vertex", 0);
	Program->addBindAttribLocation("TexCoord", 1);
	Program->addShader(VertexShader);
	Program->addShader(FragmentShader);

	auto stateset = geod->getOrCreateStateSet();
	stateset->setAttributeAndModes(Program);

	osg::ref_ptr<osg::Uniform> uniformsceneColorTexture = new osg::Uniform("ScreenTexture", 0);
	stateset->setTextureAttributeAndModes(0, sceneColorTexture.get(), osg::StateAttribute::ON);
	stateset->addUniform(uniformsceneColorTexture);
	 
	osg::ref_ptr<osg::Uniform> uniformradarColorTexture = new osg::Uniform("RadarTexture", 1);
	stateset->setTextureAttributeAndModes(1, radarColorTexture.get(), osg::StateAttribute::ON);
	stateset->addUniform(uniformradarColorTexture);
	 
	osg::ref_ptr<osg::Uniform> uniformsceneDepthTexture = new osg::Uniform("ScreenDepth", 2);
	stateset->setTextureAttributeAndModes(2, sceneDepthTexture.get(), osg::StateAttribute::ON);
	stateset->addUniform(uniformsceneDepthTexture);
	 
	osg::ref_ptr<osg::Uniform> uniformradarDepthTexture = new osg::Uniform("RadarDepth", 3);
	stateset->setTextureAttributeAndModes(3, radarDepthTexture.get(), osg::StateAttribute::ON);
	stateset->addUniform(uniformradarDepthTexture);



	osg::ref_ptr<osg::Geode> tmpGeode = new osg::Geode;
	tmpGeode->addDrawable(geod);

	osg::ref_ptr<osg::Transform> tmpNode = new osg::Transform;
	tmpNode->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
	tmpNode->addChild(tmpGeode);


	ret->addChild(tmpGeode);
	return ret;
}

void Radar::Radar::submit(osgViewer::Viewer& viewer, osg::Camera* mainCamera, osg::ref_ptr<osg::Group> root)
{
	mainCamera->attach(osg::Camera::COLOR_BUFFER, sceneColorTexture);
	mainCamera->attach(osg::Camera::DEPTH_BUFFER, sceneDepthTexture);

	auto SlaveCamera = addRadarDrawPass();
	SlaveCamera->setGraphicsContext(mainCamera->getGraphicsContext());
	viewer.addSlave(SlaveCamera,false);

	root->addChild(addBlendPass());
}



osg::ref_ptr<osg::Geometry>  Radar::Radar::Generate( llhRange range )
{
		std::ofstream outFile("output.txt");
		if (!outFile.is_open()) {
			std::cerr << "Unable to open file";
			return nullptr;
		}

		osg::ref_ptr<osg::Vec2Array> m_lines_normals_pos = new osg::Vec2Array();
		osg::ref_ptr<osg::Vec2Array> m_lines_normals_nor = new osg::Vec2Array();


		// 求不同i的情况下，_radarrender->m_lines_normals->at(i).x()中最大值
		double maxX = 0;
		for (int i = 0; i < _radarrender->m_lines_normals->size(); i++)
			if (maxX < _radarrender->m_lines_normals->at(i).x())
				maxX = _radarrender->m_lines_normals->at(i).x();

		for (int i = 0; i < _radarrender->m_lines_normals->size(); i++)
		{
			double transX, transY, transZ;
			llh2xyz_Ellipsoid(range, _radarrender->m_lines_normals->at(i).x() / maxX, 0, _radarrender->m_lines_normals->at(i).y() / maxX, transX, transY, transZ);
			m_lines_normals_pos->push_back(osg::Vec2(_radarrender->m_lines_normals->at(i).x() / maxX, _radarrender->m_lines_normals->at(i).y() / maxX));
			m_lines_normals_nor->push_back(osg::Vec2(_radarrender->m_lines_normals->at(i).z(), _radarrender->m_lines_normals->at(i).w()));

			// 将数据写入文本文件
			outFile << m_lines_normals_pos->at(i).x() << ", " << m_lines_normals_pos->at(i).y() << "\n";
			outFile << m_lines_normals_nor->at(i).x() << ", " << m_lines_normals_nor->at(i).y() << "\n\n";
		}
		outFile.close();

			osg::ref_ptr<osg::Geometry> surfaceGeometry = new osg::Geometry;

			osg::ref_ptr<osg::Vec4Array> surfacep = new osg::Vec4Array;
			// 设置角度
			const float startAngle = 0;
			const float endAngle = 2 * M_PI;
			const float stepAngle = 2 * M_PI / 20.0;
			for (int i = 0; i + 1 < m_lines_normals_pos->size(); i += 2) {
				osg::Vec4 v0 = osg::Vec4(m_lines_normals_pos->at(i).x(), m_lines_normals_pos->at(i).y(), m_lines_normals_nor->at(i).x(), m_lines_normals_nor->at(i).y());
				osg::Vec4 v1 = osg::Vec4(m_lines_normals_pos->at(i + 1).x(), m_lines_normals_pos->at(i + 1).y(), m_lines_normals_nor->at(i + 1).x(), m_lines_normals_nor->at(i + 1).y());

				for (int j = 0; j < 21; ++j) {
					float curangle = startAngle + stepAngle * i;
					float sinthita = sin(curangle);
					float costhita = cos(curangle);
					osg::Vec4 v00 = osg::Vec4(v0.x() * costhita, v0.y(), v0.x() * sinthita, 1.0);
					double x, y, z;
					llh2xyz_Ellipsoid(range, v00.x() * 1.0, v00.y() * 1.0, v00.z() * 1.0, x, y, z);
					v00 = osg::Vec4(x, y, z, 1);
					surfacep->push_back(v00);
					osg::Vec4 v11 = osg::Vec4(v1.x() * costhita, v1.x() * sinthita, v1.y(), 1.0);
					llh2xyz_Ellipsoid(range, v11.x() * 1.0, v11.y() * 1.0, v11.z() * 1.0, x, y, z);
					v00 = osg::Vec4(x, y, z, 1);
					surfacep->push_back(v00);
				}
			}

		surfaceGeometry->setVertexAttribArray(0, surfacep, osg::Array::BIND_PER_VERTEX);


		osg::ref_ptr<osg::DrawArrays> radar_lines_to_surfaces = new osg::DrawArrays(osg::PrimitiveSet::TRIANGLE_STRIP, 0, surfacep->size() - 2);
		surfaceGeometry->addPrimitiveSet(radar_lines_to_surfaces);
		surfaceGeometry->getOrCreateStateSet()->addUniform(colorUniform);

		return surfaceGeometry;
}
