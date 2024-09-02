#include "Radar.h"

#include <unordered_set>
#include <osg/BlendFunc>
#include <osg/BlendEquation>
#include <osg/BlendColor>

#include "cmath"
#include "OSGPCDLoader.h"

// ui part
// color and drawway

static osg::ref_ptr<osg::Uniform> colorUniform = new osg::Uniform("mainColor", osg::Vec4f(1,0.6,0.6,0.4));
//// for overlap part

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
		osg::ref_ptr<osg::PolygonMode> polyMode = new osg::PolygonMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE);
		rt->getOrCreateStateSet()->setAttribute(polyMode);
		rt->getOrCreateStateSet()->setAttributeAndModes(lineWidth, osg::StateAttribute::OFF);

	}
	else {
		osg::ref_ptr<osg::PolygonMode> polyMode = new osg::PolygonMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE);
		rt->getOrCreateStateSet()->setAttribute(polyMode);
		rt->getOrCreateStateSet()->setAttributeAndModes(lineWidth, osg::StateAttribute::ON);
	}
}

osg::ref_ptr<osg::Camera> Radar::Radar::addRadarDrawPass()
{
	osg::ref_ptr<osg::Camera> ret = new osg::Camera;

	rt = new osg::Geode;
	for (int i = 0; i < ranges.size(); ++i) {
		rt->addChild(Geos[i]);
		
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


	//// 设置混合函数
	osg::ref_ptr<osg::BlendFunc> blendFunc = new osg::BlendFunc(osg::BlendFunc::ONE, osg::BlendFunc::ONE);
	surfaceStateSet->setAttributeAndModes(blendFunc,osg::StateAttribute::ON);
	osg::ref_ptr<osg::BlendEquation> blendEquation = new osg::BlendEquation(osg::BlendEquation::FUNC_ADD);
	surfaceStateSet->setAttributeAndModes(blendEquation);


	ret->addChild(rt);



	ret->setClearColor(osg::Vec4(0,0,0,0));
	
	ret->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	ret->setViewport(0, 0, width, height);

	ret->setRenderOrder(osg::Camera::PRE_RENDER, 1);

	ret->attach(osg::Camera::COLOR_BUFFER, radarColorTexture);
	ret->attach(osg::Camera::DEPTH_BUFFER, radarDepthTexture);
	ret->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);

	return ret;
}

osg::ref_ptr<osg::Camera> Radar::Radar::addtrueRadarDrawPass()
{
	osg::ref_ptr<osg::Camera> ret = new osg::Camera;

	forTureRadarColor = createFullScreenQuad();

	osg::ref_ptr<osg::Shader> VertexShader = new osg::Shader(osg::Shader::VERTEX);
	osg::ref_ptr<osg::Shader> FragmentShader = new osg::Shader(osg::Shader::FRAGMENT);
	VertexShader->loadShaderSourceFromFile(std::string(OSG_3D_VIS_SHADER_PREFIX) + "fullscreenQuad.vs");
	FragmentShader->loadShaderSourceFromFile(std::string(OSG_3D_VIS_SHADER_PREFIX) + "radarColorps.glsl");
	osg::ref_ptr<osg::Program> Program = new osg::Program;
	Program->addBindAttribLocation("Vertex", 0);
	Program->addBindAttribLocation("TexCoord", 1);
	Program->addShader(VertexShader);
	Program->addShader(FragmentShader);

	auto stateset = forTureRadarColor->getOrCreateStateSet();
	stateset->setAttributeAndModes(Program);

	osg::ref_ptr<osg::Uniform> uniformsceneColorTexture = new osg::Uniform("inputRadar", 0);
	stateset->setTextureAttributeAndModes(0, radarColorTexture.get(), osg::StateAttribute::ON);
	stateset->addUniform(uniformsceneColorTexture);

	stateset->addUniform(colorUniform);
	//stateset->addUniform(blendUniform);


	osg::ref_ptr<osg::Geode> tmpGeode = new osg::Geode;
	tmpGeode->addDrawable(forTureRadarColor);

	osg::ref_ptr<osg::Transform> tmpNode = new osg::Transform;
	tmpNode->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
	tmpNode->addChild(tmpGeode);


	ret->addChild(tmpNode);

	ret->setClearColor(osg::Vec4(0, 0, 0, 0));

	ret->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	ret->setViewport(0, 0, width, height);

	ret->setRenderOrder(osg::Camera::PRE_RENDER, 1);

	ret->attach(osg::Camera::COLOR_BUFFER, trueradarColorTexture);
	//ret->attach(osg::Camera::DEPTH_BUFFER, tmpDepthTexture);
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
	// 
	osg::ref_ptr<osg::Uniform> uniformradarDepthTexture = new osg::Uniform("RadarDepth", 3);
	stateset->setTextureAttributeAndModes(3, radarDepthTexture.get(), osg::StateAttribute::ON);
	stateset->addUniform(uniformradarDepthTexture);



	osg::ref_ptr<osg::Geode> tmpGeode = new osg::Geode;
	tmpGeode->addDrawable(geod);

	osg::ref_ptr<osg::Transform> tmpNode = new osg::Transform;
	tmpNode->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
	tmpNode->addChild(tmpGeode);
	ret->addChild(tmpNode);
	return ret;
}

void Radar::Radar::submit(osgViewer::Viewer& viewer, osg::Camera* mainCamera, osg::ref_ptr<osg::Group> root)
{
	//mainCamera->attach(osg::Camera::COLOR_BUFFER, sceneColorTexture);
	//mainCamera->attach(osg::Camera::DEPTH_BUFFER, sceneDepthTexture);
	//mainCamera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);



	//auto SlaveCamera = addRadarDrawPass();
	//SlaveCamera->setGraphicsContext(mainCamera->getGraphicsContext());
	//viewer.addSlave(SlaveCamera,false);
	//root->addChild(addRadarDrawPass());

	//root->addChild(addtrueRadarDrawPass());

	//root->addChild(addBlendPass());
	rt = new osg::Geode;
	for (int i = 0; i < ranges.size(); ++i) {
		rt->addDrawable(Geos[i]);

	}

	osg::ref_ptr<osg::Shader> VertexShader = new osg::Shader(osg::Shader::VERTEX);
	osg::ref_ptr<osg::Shader> FragmentShader = new osg::Shader(osg::Shader::FRAGMENT);
	VertexShader->loadShaderSourceFromFile(std::string(OSG_3D_VIS_SHADER_PREFIX) + "RadarRoundScanVS.glsl");
	FragmentShader->loadShaderSourceFromFile(std::string(OSG_3D_VIS_SHADER_PREFIX) + "RadarRoundScanPS.glsl");
	osg::ref_ptr<osg::Program> Program = new osg::Program;
	Program->addShader(VertexShader);
	Program->addShader(FragmentShader);

	osg::ref_ptr<osg::StateSet> StateSet = rt->getOrCreateStateSet();
	StateSet->setAttributeAndModes(Program);
	StateSet->addUniform(colorUniform);
	StateSet->addUniform(mvpUniform);
	StateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
	StateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);

	root->addChild(rt);
}

//osg::ref_ptr<osg::Geometry>  Radar::Radar::Generate( llhRange range )
//{
//		osg::ref_ptr<osg::Vec2Array> m_lines_normals_pos = new osg::Vec2Array();
//		osg::ref_ptr<osg::Vec2Array> m_lines_normals_nor = new osg::Vec2Array();
//
//
//		// 求不同i的情况下，_radarrender->m_lines_normals->at(i).x()中最大值
//		double maxX = 0;
//		for (int i = 0; i < _radarrender->m_lines_normals->size(); i++)
//			if (maxX < _radarrender->m_lines_normals->at(i).x())
//				maxX = _radarrender->m_lines_normals->at(i).x();
//
//		for (int i = 0; i < _radarrender->m_lines_normals->size(); i++)
//		{
//			m_lines_normals_pos->push_back(osg::Vec2(_radarrender->m_lines_normals->at(i).x() / maxX, _radarrender->m_lines_normals->at(i).y() / maxX));
//			m_lines_normals_nor->push_back(osg::Vec2(_radarrender->m_lines_normals->at(i).z(), _radarrender->m_lines_normals->at(i).w()));
//		}
//
//		osg::ref_ptr<osg::Geode> ret = new osg::Geode;
//
//		std::vector<osg::ref_ptr<osg::Vec4Array>> Vec4arrays;
//
//		// 设置角度
//		const float startAngle = 0;
//		const float endAngle = 2 * M_PI;
//		const float stepAngle = 2 * M_PI / 20.0;
//		for (int i = 0; i + 1 < m_lines_normals_pos->size(); i ++) {
//			osg::Vec4 v0 = osg::Vec4(m_lines_normals_pos->at(i).x(), m_lines_normals_pos->at(i).y(), m_lines_normals_nor->at(i).x(), m_lines_normals_nor->at(i).y());
//			osg::Vec4 v1 = osg::Vec4(m_lines_normals_pos->at(i + 1).x(), m_lines_normals_pos->at(i + 1).y(), m_lines_normals_nor->at(i + 1).x(), m_lines_normals_nor->at(i + 1).y());
//			osg::ref_ptr<osg::Vec4Array> surfacep = new osg::Vec4Array;
//			for (int j = 0; j < 21; ++j) {
//				float curangle = startAngle + stepAngle * i;
//				float sinthita = sin(curangle);
//				float costhita = cos(curangle);
//				osg::Vec4 v00 = osg::Vec4(v0.x() * costhita, v0.y(), v0.x() * sinthita, 1.0);
//				double x, y, z;
//
//				llh2xyz_Ellipsoid(range, v00.x() * 1.0, v00.y() * 1.0, v00.z() * 1.0, x, y, z);
//				v00 = osg::Vec4(x, y, z, 1);
//
//				surfacep->push_back(v00);
//				osg::Vec4 v11 = osg::Vec4(v1.x() * costhita, v1.x() * sinthita, v1.y(), 1.0);
//				llh2xyz_Ellipsoid(range, v11.x() * 1.0, v11.y() * 1.0, v11.z() * 1.0, x, y, z);
//				v00 = osg::Vec4(x, y, z, 1);
//
//				surfacep->push_back(v00);
//			}
//			Vec4arrays.push_back(surfacep);
//		}
//
//	for(auto surfacep : Vec4arrays)
//	{
//		osg::ref_ptr<osg::Geometry> surfaceGeometry = new osg::Geometry;
//		surfaceGeometry->setVertexAttribArray(0, surfacep, osg::Array::BIND_PER_VERTEX);
//		osg::ref_ptr<osg::DrawArrays> radar_lines_to_surfaces = new osg::DrawArrays(osg::PrimitiveSet::TRIANGLE_STRIP, 0, surfacep->size() - 2);
//		surfaceGeometry->addPrimitiveSet(radar_lines_to_surfaces);
//		surfaceGeometry->getOrCreateStateSet()->addUniform(colorUniform);
//
//		ret->addDrawable(surfaceGeometry);
//	}
//
//	return ret;
//}

//osg::ref_ptr<osg::Geometry>  Radar::Radar::Generate(llhRange range)
//{
//	double lowLo = range.minLongitude;
//	double highLo = range.maxLongitude;
//	double lowLa = range.minLatitude;
//	double highLa = range.maxLatitude;
//	double lowH = range.minHeight;
//	double highH = range.maxHeight;
//	double a = (highLa - lowLa) / 2;
//	double b = (highLo - lowLo) / 2;
//	osg::ref_ptr<osg::Vec4Array> Vec4arrays = new osg::Vec4Array;
//
//	osg::Vec2 center ((lowLa + highLa) / 2, (lowLo + highLo) / 2);
//
//	for(double i = lowH; i<highH;  i += 10)
//	{
//		for(double theta = 0; theta <= 2*osg::PI; theta += 0.0174533 )
//		{
//			double k = center.x() + a * cos(theta);
//			double j = center.y() + b * sin(theta);
//			double x, y, z;
//			llh2xyz_Ellipsoid(k, j, i, x, y, z);
//			osg::Vec4 p(x, y, z, 1.);
//			Vec4arrays->push_back(p);
//		}
//
//	}
//	osg::ref_ptr<osg::Geometry> ret = new osg::Geometry;
//	ret->setVertexAttribArray(0, Vec4arrays, osg::Array::BIND_PER_VERTEX);
//
//	osg::ref_ptr<osg::DrawArrays> lines = new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP, 0, Vec4arrays->size() - 1);
//	ret->addPrimitiveSet(lines);
//	return ret;
//}

osg::ref_ptr<osg::Geometry>  Radar::Radar::Generate(llhRange range)
{
	double lowLo = range.minLongitude;
	double highLo = range.maxLongitude;
	double lowLa = range.minLatitude;
	double highLa = range.maxLatitude;
	double lowH = range.minHeight;
	double highH = range.maxHeight;
	double a = (highLa - lowLa) / 2;
	double b = (highLo - lowLo) / 2;
	osg::ref_ptr<osg::Vec4Array> Vec4arrays = new osg::Vec4Array;

	osg::Vec2 center((lowLa + highLa) / 2, (lowLo + highLo) / 2);

	double h = (lowH + highH) / 2;
	for (double theta = 0; theta <= 2 * osg::PI; theta += 0.0174533)
	{
		double k = center.x() + a * cos(theta);
		double j = center.y() + b * sin(theta);
		double x, y, z;
		llh2xyz_Ellipsoid(k, j, h, x, y, z);
		osg::Vec4 p(x, y, z, 1.);
		Vec4arrays->push_back(p);
	}

	double x, y, z;
	llh2xyz_Ellipsoid(center.x(), center.y(), h, x, y, z);
	osg::Vec4 p(x, y, z, 1.);
	Vec4arrays->push_back(p);
	const int cIndex = Vec4arrays->size() - 1;
		osg::ref_ptr<osg::DrawElementsUInt> indices = new osg::DrawElementsUInt(GL_TRIANGLES);
		for(int i=0; i<Vec4arrays->size()-1; ++i)
		{
			indices->push_back(i);
			indices->push_back(i + 1);
			indices->push_back(cIndex);
		}
		indices->push_back(cIndex - 1);
		indices->push_back(cIndex);
		indices->push_back(0);

	osg::ref_ptr<osg::Geometry> ret = new osg::Geometry;
	ret->setVertexAttribArray(0, Vec4arrays, osg::Array::BIND_PER_VERTEX);
	ret->addPrimitiveSet(indices);
	return ret;
}