﻿#pragma once
#include <osg/ShapeDrawable>
#include <osg/Texture3D>
#include <osg/LineWidth>
#include <osg/PrimitiveSet>
#include<osg/PolygonMode>
#include <osg/Texture2D>
#include <osgViewer/Viewer>
#include <osg/Group>
#include <osgUtil/PolytopeIntersector>

#include "Util.h"
#include "APMRadarRender.h"
#include "volumeRender.cpp"
#include <iostream>
#include <osgDB/WriteFile>

class CaptureCallback : public osg::Camera::DrawCallback {
public:
	CaptureCallback(osg::Texture2D* texture, string str) : _texture(texture),file_name(str) {}

	virtual void operator()(osg::RenderInfo& renderInfo) const {
		osg::Image* image = new osg::Image;
		image->readPixels(0, 0, _texture->getTextureWidth(), _texture->getTextureHeight(), GL_RGBA, GL_FLOAT);
		osgDB::writeImageFile(*image, file_name);
	}

private:
	osg::ref_ptr<osg::Texture2D> _texture;
	string file_name;
};


namespace VoxelRader {

	inline RadarRender* _radarrender;
	inline int voxels[256][256][256];
	inline unsigned char* voxelsData = new unsigned char[sizeof(float) * 256 * 256 * 256];
	inline llhRange _range;
	inline void initRadar(llhRange llh) { _range = llh; }
	inline void transPointsToVoxels(osg::ref_ptr<osg::Vec3Array> points, float curd, llhRange smallRange)
	{
		int mid = 128;
		std::vector<std::vector<std::vector<int>>> localVoxels(256, std::vector<std::vector<int>>(256, std::vector<int>(256, 0)));

		// 求smallRange在_range中的比例，得到中心与半径
		int centerX = ((smallRange.maxLatitude + smallRange.minLatitude) / 2 - _range.minLatitude) / (_range.maxLatitude - _range.minLatitude) * 256;
		int centerY = ((smallRange.maxLongitude + smallRange.minLongitude) / 2 - _range.minLongitude) / (_range.maxLongitude - _range.minLongitude) * 256;
		float rRatio = (smallRange.maxLatitude - smallRange.minLatitude) / (_range.maxLatitude - _range.minLatitude);

		// 清空localVoxels为false
		for (int i = 0; i < 256; i++)
			for (int j = 0; j < 256; j++)
				for (int k = 0; k < 256; k++)
					localVoxels[i][j][k] = 0;

		for (int i = 0; i < points->size(); i++)
		{
			if (points->at(i).z() < curd)
				continue;
			int x = points->at(i).x() / 5 * rRatio;
			int y = points->at(i).y();
			if (localVoxels[centerX + x][centerY][y] == 1)
				continue;
			localVoxels[centerX + x][centerY][y] = 1;
			float stepAngle = (360.0 / 180.0) * M_PI / 3600.0;
			for (int i = 1; i < 3600; i++)
			{
				float curangle = stepAngle * i;
				float sinthita = sin(curangle);
				float costhita = cos(curangle);
				localVoxels[centerX + int(x * costhita)][centerY + int(x * sinthita)][y] = 1;
			}
		}

		for (int i = 0; i < 256; i++)
			for (int j = 0; j < 256; j++)
				for (int k = 0; k < 256; k++)
					voxels[i][j][k] += localVoxels[i][j][k];

	}

	inline void submitRadar(osg::Group* grp)
	{
		osg::ref_ptr<osg::Texture3D> voxelsTexture = new osg::Texture3D;
		osg::ref_ptr<osg::Image> voxelsImage = new osg::Image;
		voxelsImage->setImage(256, 256, 256, GL_LUMINANCE, GL_LUMINANCE, GL_UNSIGNED_BYTE, voxelsData, osg::Image::USE_NEW_DELETE);
		//voxelsTexture->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
		//voxelsTexture->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
		voxelsTexture->setImage(voxelsImage);


		static const std::array<uint32_t, 3> dim = { 256, 256, 256 };
		static const std::array<float, 2> lonRng = { float(_range.minLongitude / M_PI * 180.f - 360.f), float(_range.maxLongitude / M_PI * 180.f - 360.f) };
		static const std::array<float, 2> latRng = { float(_range.minLatitude / M_PI * 180.f), float(_range.maxLatitude / M_PI * 180.f) };
		static const std::array<float, 2> hRng = { float(_range.minHeight), float(_range.maxHeight) };
		static const float hScale = 150.f;

		auto dvr = std::make_shared<SciVis::ScalarViser::DirectVolumeRenderer>();
		dvr->SetDeltaT(hScale * (hRng[1] - hRng[0]) / dim[2] * .3f);
		dvr->SetMaxStepCount(8000);
		dvr->AddVolume("name1", voxelsTexture, nullptr, dim, false);
		auto vol = dvr->GetVolume("name1");
		vol->SetLongtituteRange(lonRng[0], lonRng[1]);
		vol->SetLatituteRange(latRng[0], latRng[1]);
		vol->SetHeightFromCenterRange(
			static_cast<float>(osg::WGS_84_RADIUS_EQUATOR) + hScale * hRng[0],
			static_cast<float>(osg::WGS_84_RADIUS_EQUATOR) + hScale * hRng[1]);

		dvr->DisplayVolume("name1");
		grp->addChild(dvr->GetGroup());
	}

	inline void addRadar(llhRange range = llhRange())
	{
		float maxAngle = 91;
		float maxLength = 20;
		float maxSearchRange = 1;
		float minSearchRange = 1;
		float DetectionProbability = 99.9;
		_radarrender = new RadarRender();
		_radarrender->radar->MakeRGridFromFile(std::string(OSG_3D_VIS_DATA_PREFIX) + "cosec2.out");
		_radarrender->radar->SetMaxAngle_length_box(maxAngle, maxLength, maxSearchRange, minSearchRange);
		_radarrender->radar->UpdatePara();
		_radarrender->radar->UpdateDValue();
		_radarrender->SetNewPd(DetectionProbability * 0.01);

		// 将point信息放到空间体素中	
		transPointsToVoxels(_radarrender->m_grid_points, _radarrender->radar->m_curd, range);
		// 将空间体素信息放到Image3D中
		for (int i = 0; i < 256; i++)
			for (int j = 0; j < 256; j++)
				for (int k = 0; k < 256; k++)
					voxelsData[i * 256 * 256 + j * 256 + k] = voxels[j][k][i];

	}

	inline void ExportRadar()
	{
		std::string filePath = std::string(OSG_3D_VIS_DATA_PREFIX) + "OutPutVoxelData.binary";
		// 打开文件进行二进制写入
		std::ofstream outFile(filePath, std::ios::binary);
		if (!outFile) {
			std::cerr << "Failed to open file: " << filePath << std::endl;
			return;
		}

		// 写入数据到文件
		outFile.write(reinterpret_cast<const char*>(voxelsData), sizeof(float) * 256 * 256 * 256);

		// 关闭文件
		outFile.close();

		if (outFile.good()) {
			std::cout << "Voxel data successfully saved to " << filePath << std::endl;
		}
		else {
			std::cerr << "Failed to write voxel data to " << filePath << std::endl;
		}


	}
};

namespace Radar {
	class Radar {
	public:
		Radar() {}

		osg::Uniform* mvpUniform;
		osg::ref_ptr<osg::Camera> maincamera;
		void setCamera(osg::Camera* cam) {
			maincamera = cam;
			mvpUniform = new osg::Uniform(osg::Uniform::FLOAT_MAT4, "mvp");
			mvpUniform->setUpdateCallback(new ModelViewProjectionMatrixCallback(maincamera));
		};


		//6,375,827   6356752.3142;
		std::vector<llhRange> ranges;
		std::vector<osg::ref_ptr<osg::Geometry>> Geos;

		//for actualy draw 
		osg::ref_ptr<osg::Geode> rt;


		void submit(osgViewer::Viewer& viewer, osg::Camera* mainCamera, osg::ref_ptr<osg::Group> root);

		void Addllh(llhRange range)
		{
			ranges.push_back(range);
			Geos.push_back(Generate(ranges.back()));
		}
		osg::ref_ptr<osg::Geometry>  Generate(llhRange range);

		void updateR(double value);
		void updateG(double value);
		void updateB(double value);
		void updateA(double value);
		void updateLineWidth(double value);
		void updateDrawStyle(int index);

		osg::ref_ptr<osg::Camera> addRadarDrawPass();


	};

};