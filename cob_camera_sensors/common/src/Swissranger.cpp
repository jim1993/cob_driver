/*
 * Copyright 2017 Fraunhofer Institute for Manufacturing Engineering and Automation (IPA)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0

 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include "../include/cob_camera_sensors/StdAfx.h"
#ifdef __LINUX__
	#include "cob_camera_sensors/Swissranger.h"
	#include "cob_vision_utils/VisionUtils.h"
	#include "tinyxml.h"
#else
	#include "cob_driver/cob_camera_sensors/common/include/cob_camera_sensors/Swissranger.h"
	#include "cob_common/cob_vision_utils/common/include/cob_vision_utils/VisionUtils.h"
	#include "cob_vision/windows/src/extern/TinyXml/tinyxml.h"
#endif

using namespace ipa_CameraSensors;

__DLL_LIBCAMERASENSORS__ AbstractRangeImagingSensorPtr ipa_CameraSensors::CreateRangeImagingSensor_Swissranger()
{
	return AbstractRangeImagingSensorPtr(new Swissranger());
}

Swissranger::Swissranger()
{
	m_initialized = false;
	m_open = false;

	m_SRCam = 0;
	m_DataBuffer = 0;
	m_BufferSize = 1;

	m_CoeffsInitialized = false;
}


Swissranger::~Swissranger()
{
	if (isOpen())
	{
		Close();
	}
}

int ipa_CameraSensors::LibMesaCallback(SRCAM srCam, unsigned int msg, unsigned int param, void* data)
{
	switch(msg)
	{
		case CM_MSG_DISPLAY: // Redirects all output to console
		{
			if (param==MC_ETH)
			{
				// Do nothing
				return 0;
			}
			else
			{
				return SR_GetDefaultCallback()(0,msg,param,data);
			}
			break;
		}
		default:
		{
			// Default handling
			return SR_GetDefaultCallback()(0,msg,param,data);
		}
	}
	return 0;
}


unsigned long Swissranger::Init(std::string directory, int cameraIndex)
{
	if (isInitialized())
	{
		return (RET_OK | RET_CAMERA_ALREADY_INITIALIZED);
	}

	m_CameraType = ipa_CameraSensors::CAM_SWISSRANGER;

	// Load SR parameters from xml-file
	if (LoadParameters((directory + "cameraSensorsIni.xml").c_str(), cameraIndex) & RET_FAILED)
	{
		std::cerr << "ERROR - Swissranger::Init:" << std::endl;
		std::cerr << "\t ... Parsing xml configuration file failed." << std::endl;
		return (RET_FAILED | RET_INIT_CAMERA_FAILED);
	}

	m_CoeffsInitialized = true;

	// Set callback function, to catch annoying ethernet messages
	SR_SetCallback(LibMesaCallback);

	if (m_CalibrationMethod == MATLAB)
	{
		// Load z-calibration files
		std::string filename = directory + "MatlabCalibrationData/PMD/ZCoeffsA0.xml";
		CvMat* c_mat = (CvMat*)cvLoad(filename.c_str());
		if (! c_mat)
		{
			std::cerr << "ERROR - PMDCamCube::LoadParameters:" << std::endl;
			std::cerr << "\t ... Error while loading " << directory + "MatlabCalibrationData/ZcoeffsA0.txt" << "." << std::endl;
			std::cerr << "\t ... Data is necessary for z-calibration of swissranger camera" << std::endl;
			m_CoeffsInitialized = false;
			// no RET_FAILED, as we might want to calibrate the camera to create these files
		}
		else
		{
			m_CoeffsA0 = c_mat;
			cvReleaseMat(&c_mat);
		}

		filename = directory + "MatlabCalibrationData/PMD/ZCoeffsA1.xml";
		c_mat = (CvMat*)cvLoad(filename.c_str());
		if (! c_mat)
		{
			std::cerr << "ERROR - PMDCamCube::LoadParameters:" << std::endl;
			std::cerr << "\t ... Error while loading " << directory + "MatlabCalibrationData/ZcoeffsA1.txt" << "." << std::endl;
			std::cerr << "\t ... Data is necessary for z-calibration of swissranger camera" << std::endl;
			m_CoeffsInitialized = false;
			// no RET_FAILED, as we might want to calibrate the camera to create these files
		}
		else
		{
			m_CoeffsA1 = c_mat;
			cvReleaseMat(&c_mat);
		}

		filename = directory + "MatlabCalibrationData/PMD/ZCoeffsA2.xml";
		c_mat = (CvMat*)cvLoad(filename.c_str());
		if (! c_mat)
		{
			std::cerr << "ERROR - PMDCamCube::LoadParameters:" << std::endl;
			std::cerr << "\t ... Error while loading " << directory + "MatlabCalibrationData/ZcoeffsA2.txt" << "." << std::endl;
			std::cerr << "\t ... Data is necessary for z-calibration of swissranger camera" << std::endl;
			m_CoeffsInitialized = false;
			// no RET_FAILED, as we might want to calibrate the camera to create these files
		}
		else
		{
			m_CoeffsA2 = c_mat;
			cvReleaseMat(&c_mat);
		}

		filename = directory + "MatlabCalibrationData/PMD/ZCoeffsA3.xml";
		c_mat = (CvMat*)cvLoad(filename.c_str());
		if (! c_mat)
		{
			std::cerr << "ERROR - PMDCamCube::LoadParameters:" << std::endl;
			std::cerr << "\t ... Error while loading " << directory + "MatlabCalibrationData/ZcoeffsA3.txt" << "." << std::endl;
			std::cerr << "\t ... Data is necessary for z-calibration of swissranger camera" << std::endl;
			m_CoeffsInitialized = false;
			// no RET_FAILED, as we might want to calibrate the camera to create these files
		}
		else
		{
			m_CoeffsA3 = c_mat;
			cvReleaseMat(&c_mat);
		}

		filename = directory + "MatlabCalibrationData/PMD/ZCoeffsA4.xml";
		c_mat = (CvMat*)cvLoad(filename.c_str());
		if (! c_mat)
		{
			std::cerr << "ERROR - PMDCamCube::LoadParameters:" << std::endl;
			std::cerr << "\t ... Error while loading " << directory + "MatlabCalibrationData/ZcoeffsA4.txt" << "." << std::endl;
			std::cerr << "\t ... Data is necessary for z-calibration of swissranger camera" << std::endl;
			m_CoeffsInitialized = false;
			// no RET_FAILED, as we might want to calibrate the camera to create these files
		}
		else
		{
			m_CoeffsA4 = c_mat;
			cvReleaseMat(&c_mat);
		}

		filename = directory + "MatlabCalibrationData/PMD/ZCoeffsA5.xml";
		c_mat = (CvMat*)cvLoad(filename.c_str());
		if (! c_mat)
		{
			std::cerr << "ERROR - PMDCamCube::LoadParameters:" << std::endl;
			std::cerr << "\t ... Error while loading " << directory + "MatlabCalibrationData/ZcoeffsA5.txt" << "." << std::endl;
			std::cerr << "\t ... Data is necessary for z-calibration of swissranger camera" << std::endl;
			m_CoeffsInitialized = false;
			// no RET_FAILED, as we might want to calibrate the camera to create these files
		}
		else
		{
			m_CoeffsA5 = c_mat;
			cvReleaseMat(&c_mat);
		}

		filename = directory + "MatlabCalibrationData/PMD/ZCoeffsA6.xml";
		c_mat = (CvMat*)cvLoad(filename.c_str());
		if (! c_mat)
		{
			std::cerr << "ERROR - PMDCamCube::LoadParameters:" << std::endl;
			std::cerr << "\t ... Error while loading " << directory + "MatlabCalibrationData/ZcoeffsA6.txt" << "." << std::endl;
			std::cerr << "\t ... Data is necessary for z-calibration of swissranger camera" << std::endl;
			m_CoeffsInitialized = false;
			// no RET_FAILED, as we might want to calibrate the camera to create these files
		}
		else
		{
			m_CoeffsA6 = c_mat;
			cvReleaseMat(&c_mat);
		}
	}

	// set init flag
	m_initialized = true;
	m_GrayImageAcquireCalled = false;

	return RET_OK;
}


unsigned long Swissranger::Open()
{
	if (!isInitialized())
	{
		return (RET_FAILED | RET_CAMERA_NOT_INITIALIZED);
	}

	if (isOpen())
	{
		return (RET_OK | RET_CAMERA_ALREADY_OPEN);
	}

	std::string sInterface = "";
	m_RangeCameraParameters.m_Interface.clear(); // Clear flags
	m_RangeCameraParameters.m_Interface.seekg(0); // Set Pointer to position 0 within stringstream
	m_RangeCameraParameters.m_Interface >> sInterface;

	if (sInterface == "USB")
	{
		if(SR_OpenUSB(&m_SRCam, 0)<=0)
		{
			std::cerr << "ERROR - Swissranger::Open():" << std::endl;
			std::cerr << "\t ... Could not open swissranger camera on USB port" << std::endl;
			std::cerr << "\t ... Unplug and Replugin camera power cable.\n";
			return RET_FAILED;
		}
	}
	else if (sInterface == "ETHERNET")
	{
		std::string sIP = "";
		m_RangeCameraParameters.m_IP.clear(); // Clear flags
		m_RangeCameraParameters.m_IP.seekg(0); // Set Pointer to position 0 within stringstream
		m_RangeCameraParameters.m_IP >> sIP;
		if(SR_OpenETH(&m_SRCam, sIP.c_str())<=0)
		{
			std::cerr << "ERROR - Swissranger::Open():" << std::endl;
			std::cerr << "\t ... Could not open swissranger camera on ETHERNET port" << std::endl;
			std::cerr << "\t ... with ip '" << sIP << "'." << std::endl;
			std::cerr << "\t ... Unplug and Replugin camera power cable to fix problem\n";
			return RET_FAILED;
		}
	}
	else
	{
		std::cerr << "ERROR - Swissranger::Open():" << std::endl;
		std::cerr << "\t ... Unknown interface type '" << sInterface << "'" << std::endl;
		return RET_FAILED;
	}

	//char DevStr[1024];
	//SR_GetDeviceString(m_SRCam, DevStr, 1024);
	//std::cout << "Swissranger::Open(): INFO" << std::endl;
	//std::cout << "\t ... " << DevStr << std::endl;

	if (SetParameters() & ipa_CameraSensors::RET_FAILED)
	{
		std::cerr << "ERROR - AVTPikeCam::Open:" << std::endl;
		std::cerr << "\t ... Could not set parameters" << std::endl;
		return RET_FAILED;
	}

	std::cout << "**************************************************" << std::endl;
	std::cout << "Swissranger::Open: Swissranger camera device OPEN" << std::endl;
	std::cout << "**************************************************" << std::endl << std::endl;
	m_open = true;

	return RET_OK;
}


unsigned long Swissranger::Close()
{
	if (!isOpen())
	{
		return (RET_OK);
	}

	if(SR_Close(m_SRCam)<0)
	{
		std::cout << "ERROR - Swissranger::Close():" << std::endl;
		std::cerr << "\t ... Could not close swissranger camera." << std::endl;
		return RET_FAILED;
	}
	m_SRCam = 0;

	m_open = false;
	return RET_OK;

}


unsigned long Swissranger::SetProperty(t_cameraProperty* cameraProperty)
{
	if (!m_SRCam)
	{
		return (RET_FAILED | RET_CAMERA_NOT_OPEN);
	}

	int err = 0;
	switch (cameraProperty->propertyID)
	{
		case PROP_AMPLITUDE_THRESHOLD:
			if (cameraProperty->propertyType & ipa_CameraSensors::TYPE_SPECIAL)
			{
				if(cameraProperty->specialValue == ipa_CameraSensors::VALUE_AUTO)
				{
					unsigned short val = 0;
					err =SR_SetAmplitudeThreshold(m_SRCam, val);
					if(err<0)
					{
						std::cerr << "ERROR - Swissranger::SetProperty:" << std::endl;
						std::cerr << "\t ... Could not set amplitude threshold to AUTO mode" << std::endl;
						return RET_FAILED;
					}
				}
				else if(cameraProperty->specialValue == ipa_CameraSensors::VALUE_DEFAULT)
				{
					// Void
				}
				else
				{
					std::cerr << "ERROR - Swissranger::SetProperty:" << std::endl;
					std::cerr << "\t ... Special value 'VALUE_AUTO' or 'VALUE_DEFAULT' expected." << std::endl;
					return RET_FAILED;
				}
			}
			else if (cameraProperty->propertyType & (ipa_CameraSensors::TYPE_SHORT | ipa_CameraSensors::TYPE_UNSIGNED))
			{
				err =SR_SetAmplitudeThreshold(m_SRCam, cameraProperty->u_shortData);
				if(err<0)
				{
					std::cerr << "ERROR - Swissranger::SetProperty:" << std::endl;
					std::cerr << "\t ... Could not set amplitude threshold to AUTO mode" << std::endl;
					return RET_FAILED;
				}
			}
			else
			{
				std::cerr << "ERROR - Swissranger::SetProperty:" << std::endl;
				std::cerr << "\t ... Wrong property type. '(TYPE_SHORT|TYPE_UNSIGNED)' or special value 'TYPE_SPECIAL' expected." << std::endl;
				return RET_FAILED;
			}
			break;
		case PROP_INTEGRATION_TIME:
			if (cameraProperty->propertyType & ipa_CameraSensors::TYPE_SPECIAL)
			{
				if(cameraProperty->specialValue == ipa_CameraSensors::VALUE_AUTO)
				{
					err = SR_SetAutoExposure(m_SRCam, 1, 150, 5, 40);
					if(err<0)
					{
						std::cerr << "ERROR - Swissranger::SetProperty:" << std::endl;
						std::cerr << "\t ... Could not set integration time to AUTO mode" << std::endl;
						return RET_FAILED;
					}
				}
				else if(cameraProperty->specialValue == ipa_CameraSensors::VALUE_DEFAULT)
				{
					// Void
				}
				else
				{
					std::cerr << "ERROR - Swissranger::SetProperty:" << std::endl;
					std::cerr << "\t ... Special value 'VALUE_AUTO' or 'VALUE_DEFAULT' expected." << std::endl;
					return RET_FAILED;
				}
			}
			else if (cameraProperty->propertyType & (ipa_CameraSensors::TYPE_CHARACTER | ipa_CameraSensors::TYPE_UNSIGNED))
			{
				err = SR_SetAutoExposure(m_SRCam, 0xff, 150, 5, 70);
				if(err<0)
				{
					std::cerr << "ERROR - Swissranger::SetProperty:" << std::endl;
					std::cerr << "\t ... Could not turn off auto exposure" << std::endl;
					return RET_FAILED;
				}
				err = SR_SetIntegrationTime(m_SRCam, cameraProperty->u_charData);
				if(err<0)
				{
					std::cerr << "ERROR - Swissranger::SetProperty:" << std::endl;
					std::cerr << "\t ... Could not set amplitude threshold to '" << cameraProperty->u_charData << "'" << std::endl;
					return RET_FAILED;
				}
			}
			else
			{
				std::cerr << "ERROR - Swissranger::SetProperty:" << std::endl;
				std::cerr << "\t ... Wrong property type. '(TYPE_LONG|TYPE_UNSIGNED)' or special value 'TYPE_SPECIAL' expected." << std::endl;
				return RET_FAILED;
			}
			break;
		case PROP_MODULATION_FREQUENCY:
			if (cameraProperty->propertyType & ipa_CameraSensors::TYPE_SPECIAL)
			{
				if(cameraProperty->specialValue == ipa_CameraSensors::VALUE_AUTO)
				{
					err = SR_SetModulationFrequency(m_SRCam, MF_LAST);
					if(err<0)
					{
						std::cerr << "ERROR - Swissranger::SetProperty:" << std::endl;
						std::cerr << "\t ... Could not set modulation frequency to AUTO mode" << std::endl;
						return RET_FAILED;
					}
				}
				else if(cameraProperty->specialValue == ipa_CameraSensors::VALUE_DEFAULT)
				{
					// Void
				}
				else
				{
					std::cerr << "ERROR - Swissranger::SetProperty:" << std::endl;
					std::cerr << "\t ... Special value 'VALUE_AUTO' or 'VALUE_DEFAULT' expected." << std::endl;
					return RET_FAILED;
				}
			}
			else if (cameraProperty->propertyType & (ipa_CameraSensors::TYPE_STRING))
			{
				// MF_40MHz, SR3k: maximal range 3.75m
                // MF_30MHz, SR3k, SR4k: maximal range 5m
                // MF_21MHz, SR3k: maximal range 7.14m
                // MF_20MHz, SR3k: maximal range 7.5m
                // MF_19MHz, SR3k: maximal range 7.89m
                // MF_60MHz, SR4k: maximal range 2.5m
                // MF_15MHz, SR4k: maximal range 10m
                // MF_10MHz, SR4k: maximal range 15m
                // MF_29MHz, SR4k: maximal range 5.17m
                // MF_31MHz
				if (cameraProperty->stringData == "MF_40MHz")
				{
					err = SR_SetModulationFrequency(m_SRCam, MF_40MHz);
				}
				else if (cameraProperty->stringData == "MF_30MHz")
				{
					err = SR_SetModulationFrequency(m_SRCam, MF_30MHz);
				}
				else if (cameraProperty->stringData == "MF_21MHz")
				{
					err = SR_SetModulationFrequency(m_SRCam, MF_21MHz);
				}
				else if (cameraProperty->stringData == "MF_20MHz")
				{
					err = SR_SetModulationFrequency(m_SRCam, MF_20MHz);
				}
				else if (cameraProperty->stringData == "MF_19MHz")
				{
					err = SR_SetModulationFrequency(m_SRCam, MF_19MHz);
				}
				else if (cameraProperty->stringData == "MF_60MHz")
				{
					err = SR_SetModulationFrequency(m_SRCam, MF_60MHz);
				}
				else if (cameraProperty->stringData == "MF_15MHz")
				{
					err = SR_SetModulationFrequency(m_SRCam, MF_15MHz);
				}
				else if (cameraProperty->stringData == "MF_10MHz")
				{
					err = SR_SetModulationFrequency(m_SRCam, MF_10MHz);
				}
				else if (cameraProperty->stringData == "MF_29MHz")
				{
					err = SR_SetModulationFrequency(m_SRCam, MF_29MHz);
				}
				else if (cameraProperty->stringData == "MF_31MHz")
				{
					err = SR_SetModulationFrequency(m_SRCam, MF_31MHz);
				}
				else
				{
					std::cerr << "ERROR - Swissranger::SetProperty:" << std::endl;
					std::cerr << "\t ... Modulation frequency " << cameraProperty->stringData << " unknown" << std::endl;
				}

				if(err<0)
				{
					std::cerr << "ERROR - Swissranger::SetProperty:" << std::endl;
					std::cerr << "\t ... Could not set modulation frequency " << cameraProperty->stringData << std::endl;
					return RET_FAILED;
				}

			}
			else
			{
				std::cerr << "ERROR - Swissranger::SetProperty:" << std::endl;
				std::cerr << "\t ... Wrong property type. '(TYPE_LONG|TYPE_UNSIGNED)' or special value 'TYPE_SPECIAL' expected." << std::endl;
				return RET_FAILED;
			}
			break;
		case PROP_ACQUIRE_MODE:
			if (cameraProperty->propertyType & ipa_CameraSensors::TYPE_INTEGER)
			{
				err = SR_SetMode(m_SRCam, cameraProperty->integerData);
				if(err<0)
				{
					std::cerr << "ERROR - Swissranger::SetProperty:" << std::endl;
					std::cerr << "\t ... Could not set acquire mode" << std::endl;
					return RET_FAILED;
				}
			}
			else
			{
				std::cerr << "ERROR - Swissranger::SetProperty:" << std::endl;
				std::cerr << "\t ... Wrong property type. 'TYPE_INTEGER' expected." << std::endl;
				return RET_FAILED;
			}
			break;
		default:
			std::cout << "Swissranger::SetProperty: Property " << cameraProperty->propertyID << " unspecified.\n";
			return RET_FAILED;
			break;
	}

	return RET_OK;
}


unsigned long Swissranger::SetPropertyDefaults()
{
	return RET_FUNCTION_NOT_IMPLEMENTED;
}


unsigned long Swissranger::GetProperty(t_cameraProperty* cameraProperty)
{
	switch (cameraProperty->propertyID)
	{
		case PROP_DMA_BUFFER_SIZE:
			cameraProperty->u_integerData = m_BufferSize;
			return RET_OK;
			break;
		case PROP_AMPLITUDE_THRESHOLD:
			if (isOpen())
			{
				cameraProperty->u_shortData = SR_GetAmplitudeThreshold(m_SRCam);
				cameraProperty->propertyType = (TYPE_UNSIGNED | TYPE_SHORT);
			}
			else
			{
				return (RET_FAILED | RET_CAMERA_NOT_OPEN);
			}
			break;

		case PROP_INTEGRATION_TIME:
			if (isOpen())
			{
				cameraProperty->u_charData = SR_GetIntegrationTime(m_SRCam);
				cameraProperty->propertyType = (TYPE_UNSIGNED | TYPE_CHARACTER);
			}
			else
			{
				return (RET_FAILED | RET_CAMERA_NOT_OPEN);
			}
			break;

		case PROP_ACQUIRE_MODE:
			if (isOpen())
			{
				cameraProperty->integerData = SR_GetMode(m_SRCam);
				cameraProperty->propertyType = TYPE_INTEGER;
			}
			else
			{
				return (RET_FAILED | RET_CAMERA_NOT_OPEN);
			}
			break;

		case PROP_CAMERA_RESOLUTION:
			if (isOpen())
			{
				cameraProperty->cameraResolution.xResolution = (int)SR_GetCols(m_SRCam);
				cameraProperty->cameraResolution.yResolution = (int)SR_GetRows(m_SRCam);
				cameraProperty->propertyType = TYPE_CAMERA_RESOLUTION;
			}
			else
			{
				cameraProperty->cameraResolution.xResolution = 176;
				cameraProperty->cameraResolution.yResolution = 144;
				cameraProperty->propertyType = TYPE_CAMERA_RESOLUTION;
			}
			break;

		default:
			std::cout << "ERROR - Swissranger::GetProperty:" << std::endl;
			std::cout << "\t ... Property " << cameraProperty->propertyID << " unspecified.";
			return RET_FAILED;
			break;

	}

	return RET_OK;
}


// Wrapper for IplImage retrival from AcquireImage
// Images have to be initialized prior to calling this function
unsigned long Swissranger::AcquireImages(cv::Mat* rangeImage, cv::Mat* grayImage, cv::Mat* cartesianImage,
										 bool getLatestFrame, bool undistort, ipa_CameraSensors::t_ToFGrayImageType grayImageType)
{
	char* rangeImageData = 0;
	char* grayImageData = 0;
	char* cartesianImageData = 0;
	int widthStepRange = -1;
	int widthStepGray = -1;
	int widthStepCartesian = -1;

	int width = -1;
	int height = -1;
	ipa_CameraSensors::t_cameraProperty cameraProperty;
	cameraProperty.propertyID = PROP_CAMERA_RESOLUTION;
	GetProperty(&cameraProperty);
	width = cameraProperty.cameraResolution.xResolution;
	height = cameraProperty.cameraResolution.yResolution;

	if(rangeImage)
	{
		rangeImage->create(height, width, CV_32FC(1));
		rangeImageData = rangeImage->ptr<char>(0);
		widthStepRange = rangeImage->step;
	}

	if(grayImage)
	{
		grayImage->create(height, width, CV_32FC(1));
		grayImageData = grayImage->ptr<char>(0);
		widthStepGray = grayImage->step;
	}

	if(cartesianImage)
	{
		cartesianImage->create(height, width, CV_32FC(3));
		cartesianImageData = cartesianImage->ptr<char>(0);
		widthStepCartesian = cartesianImage->step;
	}

	if (!rangeImage && !grayImage && !cartesianImage)
	{
		return RET_OK;
	}

	return AcquireImages(widthStepRange, widthStepGray, widthStepCartesian, rangeImageData, grayImageData, cartesianImageData, getLatestFrame, undistort, grayImageType);

}

// Enables faster image retrival than AcquireImage
unsigned long Swissranger::AcquireImages(int widthStepRange, int widthStepGray, int widthStepCartesian, char* rangeImageData, char* grayImageData, char* cartesianImageData,
										 bool getLatestFrame, bool undistort, ipa_CameraSensors::t_ToFGrayImageType grayImageType)
{
///***********************************************************************
// Get data from camera
///***********************************************************************
	if (!m_open)
	{
		std::cerr << "ERROR - Swissranger::AcquireImages:" << std::endl;
		std::cerr << "t ... Camera not open." << std::endl;
		return (RET_FAILED | RET_CAMERA_NOT_OPEN);
	}

	//unsigned int c = SR_GetIntegrationTime(m_SRCam);
	//unsigned short a = SR_GetAmplitudeThreshold(m_SRCam);
	//std::cout << "\t ... Integration time is '" << c << "'" << std::endl;
	//std::cout << "\t ... Amplitude threshold is '" << a << "'" << std::endl;

	int width = -1;
	int height = -1;
	ipa_CameraSensors::t_cameraProperty cameraProperty;
	cameraProperty.propertyID = PROP_CAMERA_RESOLUTION;
	GetProperty(&cameraProperty);
	width = cameraProperty.cameraResolution.xResolution;
	height = cameraProperty.cameraResolution.yResolution;

	unsigned int bytesRead	= 0;
	bytesRead = SR_Acquire(m_SRCam);
	if(bytesRead <= 0)
	{
		std::cerr << "ERROR - Swissranger::AcquireImages:" << std::endl;
		std::cerr << "\t ... Could not acquire image!" << std::endl;
		return RET_FAILED;
	}

	if (getLatestFrame == true)
	{
		bytesRead = SR_Acquire(m_SRCam);
		if(bytesRead <= 0)
		{
			std::cerr << "ERROR - Swissranger::AcquireImages:" << std::endl;
			std::cerr << "\t ... Could not acquire image!" << std::endl;
			return RET_FAILED;
		}
	}
	WORD* pixels =(WORD*) SR_GetImage(m_SRCam, 0);
///***********************************************************************
// Range image (distorted or undistorted)
///***********************************************************************
	if (rangeImageData)
	{
		int imageStep = -1;
		float* f_ptr = 0;

		// put data in corresponding IPLImage structures
		for(unsigned int row=0; row<(unsigned int)height; row++)
		{
			imageStep = row*width;
			f_ptr = (float*)(rangeImageData + row*widthStepRange);

			for (unsigned int col=0; col<(unsigned int)width; col++)
			{
				f_ptr[col] = (float)(pixels[imageStep + col]);
			}
		}

		if (undistort)
		{
			cv::Mat undistortedData(height, width, CV_32FC(1), (float*) rangeImageData);
			cv::Mat distortedData;

			assert (!m_undistortMapX.empty() && !m_undistortMapY.empty());
			cv::remap(distortedData, undistortedData, m_undistortMapX, m_undistortMapY, cv::INTER_LINEAR);
		}

	} // End if (rangeImage)

///***********************************************************************
// Intensity/Amplitude image
// ATTENTION: SR provides only amplitude information
///***********************************************************************
	if(grayImageData)
	{
		if (grayImageType == ipa_CameraSensors::INTENSITY_32F1 &&
			m_GrayImageAcquireCalled == false)
		{
			std::cout << "WARNING - Swissranger::AcquireImages:" << std::endl;
			std::cout << "\t ... Intensity image for swissranger not available" << std::endl;
			std::cout << "\t ... falling back to amplitude image" << std::endl;
			m_GrayImageAcquireCalled = true;
		}

		int imageSize = width*height;
		int imageStep = 0;
		float* f_ptr = 0;

		for(unsigned int row=0; row<(unsigned int)height-1; row++)
		{
			imageStep = imageSize+row*width;
			f_ptr = (float*)(grayImageData + row*widthStepGray);

			for (unsigned int col=0; col<(unsigned int)width-1; col++)
			{
				f_ptr[col] = (float)(pixels[imageStep+col]);
			}
		}

		if (undistort)
		{
			cv::Mat undistortedData( height, width, CV_32FC1, (float*) grayImageData);
			cv::Mat distortedData;

			assert (!m_undistortMapX.empty() && !m_undistortMapY.empty());
			cv::remap(distortedData, undistortedData, m_undistortMapX, m_undistortMapY, cv::INTER_LINEAR);
		}

	}

///***********************************************************************
// Cartesian image (always undistorted)
///***********************************************************************
	if(cartesianImageData)
	{
		float x = -1;
		float y = -1;
		float zRaw = -1;
		float* zCalibratedPtr = 0;
		float zCalibrated = -1;
		float* f_ptr = 0;

		if(m_CalibrationMethod==MATLAB)
		{
			if (m_CoeffsInitialized)
			{
				// Calculate calibrated z values (in meter) based on 6 degree polynomial approximation
				cv::Mat distortedData( height, width, CV_32FC1 );
				for(unsigned int row=0; row<(unsigned int)height; row++)
				{
					f_ptr = distortedData.ptr<float>(row);
					for (unsigned int col=0; col<(unsigned int)width; col++)
					{
						zRaw = (float)(pixels[width*row + col]);
						GetCalibratedZMatlab(col, row, zRaw, zCalibrated);
						f_ptr[col] = zCalibrated;
					}
				}
				/*IplImage dummy;
				IplImage *z = cvGetImage(distortedData, &dummy);
				IplImage *image = cvCreateImage(cvGetSize(z), IPL_DEPTH_8U, 3);
				ipa_Utils::ConvertToShowImage(z, image, 1);
				cvNamedWindow("Z");
				cvShowImage("Z", image);
				cvWaitKey();
				*/

				// Undistort
				cv::Mat undistortedData;
	 			assert (!m_undistortMapX.empty() && !m_undistortMapY.empty());
				cv::remap(distortedData, undistortedData, m_undistortMapX, m_undistortMapY, cv::INTER_LINEAR);

				/*IplImage dummy;
				IplImage* z = cvGetImage(undistortedData, &dummy);
				IplImage* image = cvCreateImage(cvGetSize(z), IPL_DEPTH_8U, 3);
				ipa_utils::ConvertToShowImage(z, image, 1);
				cvNamedWindow("Z");
				cvShowImage("Z", image);
				cvWaitKey();*/

				// Calculate X and Y based on instrinsic rotation and translation
				for(unsigned int row=0; row<(unsigned int)height; row++)
				{
					zCalibratedPtr = undistortedData.ptr<float>(row);
					f_ptr = (float*)(cartesianImageData + row*widthStepCartesian);

					for (unsigned int col=0; col<(unsigned int)width; col++)
					{
						int colTimes3 = 3*col;
						GetCalibratedXYMatlab(col, row, zCalibratedPtr[col], x, y);

						f_ptr[colTimes3] = x;
						f_ptr[colTimes3 + 1] = y;
						f_ptr[colTimes3 + 2] = zCalibratedPtr[col];
					}
				}
			}
			else
			{
				std::cerr << "ERROR - Swissranger::AcquireImages: \n";
				std::cerr << "\t ... At least one of m_CoeffsA0 ... m_CoeffsA6 not initialized.\n";
				return RET_FAILED;
			}

		}
		else if(m_CalibrationMethod==MATLAB_NO_Z)
		{
			SR_CoordTrfFlt(m_SRCam, m_X, m_Y, m_Z, sizeof(float), sizeof(float), sizeof(float));
			// Calculate calibrated z values (in meter) based on 6 degree polynomial approximation
			cv::Mat distortedData( height, width, CV_32FC1 );
			for(unsigned int row=0; row<(unsigned int)height; row++)
			{
				f_ptr = distortedData.ptr<float>(row);
				for (unsigned int col=0; col<(unsigned int)width; col++)
				{
					GetCalibratedZSwissranger(col, row, width, zCalibrated);
					f_ptr[col] = zCalibrated;
				}
			}

			// Undistort
			cv::Mat undistortedData;
 			assert (!m_undistortMapX.empty() && !m_undistortMapY.empty());
			cv::remap(distortedData, undistortedData, m_undistortMapX, m_undistortMapY, cv::INTER_LINEAR);

			// Calculate X and Y based on instrinsic rotation and translation
			for(unsigned int row=0; row<(unsigned int)height; row++)
			{
				zCalibratedPtr = undistortedData.ptr<float>(row);
				f_ptr = (float*)(cartesianImageData + row*widthStepCartesian);

				for (unsigned int col=0; col<(unsigned int)width; col++)
				{
					int colTimes3 = 3*col;
					GetCalibratedXYMatlab(col, row, zCalibratedPtr[col], x, y);

					f_ptr[colTimes3] = x;
					f_ptr[colTimes3 + 1] = y;
					f_ptr[colTimes3 + 2] = zCalibratedPtr[col];
				}
			}
		}
		else if(m_CalibrationMethod==NATIVE)
		{
			SR_CoordTrfFlt(m_SRCam, m_X, m_Y, m_Z, sizeof(float), sizeof(float), sizeof(float));

			for(unsigned int row=0; row<(unsigned int)height; row++)
			{
				f_ptr = (float*)(cartesianImageData + row*widthStepCartesian);

				for (unsigned int col=0; col<(unsigned int)width; col++)
				{
					int colTimes3 = 3*col;

					GetCalibratedZSwissranger(col, row, width, zCalibrated);
					GetCalibratedXYSwissranger(col, row, width, x, y);

					f_ptr[colTimes3] = x;
					f_ptr[colTimes3 + 1] = y;
					f_ptr[colTimes3 + 2] = zCalibrated;
				}
			}
		}
		else
		{
			std::cerr << "ERROR - Swissranger::AcquireImages:" << std::endl;
			std::cerr << "\t ... Calibration method unknown.\n";
			return RET_FAILED;
		}
	}
	return RET_OK;
}

unsigned long Swissranger::SaveParameters(const char* filename)
{
	return RET_FUNCTION_NOT_IMPLEMENTED;
}

unsigned long Swissranger::GetCalibratedZMatlab(int u, int v, float zRaw, float& zCalibrated)
{

	double c[7] = {m_CoeffsA0.at<double>(v,u), m_CoeffsA1.at<double>(v,u), m_CoeffsA2.at<double>(v,u),
		m_CoeffsA3.at<double>(v,u), m_CoeffsA4.at<double>(v,u), m_CoeffsA5.at<double>(v,u), m_CoeffsA6.at<double>(v,u)};
	double y = 0;
	ipa_Utils::EvaluatePolynomial((double) zRaw, 6, &c[0], &y);
	zCalibrated = (float) y;

	return RET_OK;
}

// Return value is in m
unsigned long Swissranger::GetCalibratedZSwissranger(int u, int v, int width, float& zCalibrated)
{
	zCalibrated = (float) m_Z[v*width + u];

	return RET_OK;
}

// u and v are assumed to be distorted coordinates
unsigned long Swissranger::GetCalibratedXYMatlab(int u, int v, float z, float& x, float& y)
{
	// Conversion form m to mm
	z *= 1000;

	// Use intrinsic camera parameters
	double fx, fy, cx, cy;

	fx = m_intrinsicMatrix.at<double>(0, 0);
	fy = m_intrinsicMatrix.at<double>(1, 1);

	cx = m_intrinsicMatrix.at<double>(0, 2);
	cy = m_intrinsicMatrix.at<double>(1, 2);

	// Fundamental equation: u = (fx*x)/z + cx
	if (fx == 0)
	{
		std::cerr << "ERROR - Swissranger::GetCalibratedXYZ:" << std::endl;
		std::cerr << "\t ... fx is 0.\n";
		return RET_FAILED;
	}
	x = (float) (z*(u-cx)/fx) ;

	// Fundamental equation: v = (fy*y)/z + cy
	if (fy == 0)
	{
		std::cerr << "ERROR - Swissranger::GetCalibratedXYZ:" << std::endl;
		std::cerr << "\t ... fy is 0.\n";
		return RET_FAILED;
	}
	y = (float) (z*(v-cy)/fy);

	// Conversion from mm to m
	x /= 1000;
	y /= 1000;

	return RET_OK;
}

unsigned long Swissranger::GetCalibratedXYSwissranger(int u, int v, int width, float& x, float& y)
{
	// make sure, that m_X, m_Y and m_Z have been initialized by Acquire image
	int i = v*width + u;
	x = (float)m_X[i];
	y = (float)m_Y[i];

	return RET_OK;
}

unsigned long Swissranger::SetParameters()
{
	ipa_CameraSensors::t_cameraProperty cameraProperty;


// -----------------------------------------------------------------
// Set amplitude threshold
// -----------------------------------------------------------------
	cameraProperty.propertyID = ipa_CameraSensors::PROP_AMPLITUDE_THRESHOLD;
	std::string sAmplitudeThreshold = "";
	m_RangeCameraParameters.m_AmplitudeThreshold.clear(); // Clear flags
	m_RangeCameraParameters.m_AmplitudeThreshold.seekg(0); // Set Pointer to position 0 within stringstream
	m_RangeCameraParameters.m_AmplitudeThreshold >> sAmplitudeThreshold;
	if (sAmplitudeThreshold == "AUTO")
	{
		cameraProperty.propertyType = ipa_CameraSensors::TYPE_SPECIAL;
		cameraProperty.specialValue = VALUE_AUTO;
	}
	else if (sAmplitudeThreshold == "DEFAULT")
	{
		cameraProperty.propertyType = ipa_CameraSensors::TYPE_SPECIAL;
		cameraProperty.specialValue = VALUE_DEFAULT;
	}
	else
	{
		cameraProperty.propertyType = (ipa_CameraSensors::TYPE_UNSIGNED | ipa_CameraSensors::TYPE_SHORT);
		m_RangeCameraParameters.m_AmplitudeThreshold.clear(); // Clear flags
		m_RangeCameraParameters.m_AmplitudeThreshold.seekg(0); // Set Pointer to position 0 within stringstream
		m_RangeCameraParameters.m_AmplitudeThreshold >> cameraProperty.u_shortData;
	}

	if (SetProperty(&cameraProperty) & ipa_CameraSensors::RET_FAILED)
	{
		std::cout << "WARNING - Swissranger::SetParameters:" << std::endl;
		std::cout << "\t ... Could not set amplitude threshold" << std::endl;
	}

// -----------------------------------------------------------------
// Set integration time
// -----------------------------------------------------------------
	cameraProperty.propertyID = ipa_CameraSensors::PROP_INTEGRATION_TIME;
	std::string sIntegrationTime = "";
	m_RangeCameraParameters.m_IntegrationTime.clear(); // Clear flags
	m_RangeCameraParameters.m_IntegrationTime.seekg(0); // Set Pointer to position 0 within stringstream
	m_RangeCameraParameters.m_IntegrationTime >> sIntegrationTime;
	if (sIntegrationTime == "AUTO")
	{
		cameraProperty.propertyType = ipa_CameraSensors::TYPE_SPECIAL;
		cameraProperty.specialValue = VALUE_AUTO;
	}
	else if (sIntegrationTime == "DEFAULT")
	{
		cameraProperty.propertyType = ipa_CameraSensors::TYPE_SPECIAL;
		cameraProperty.specialValue = VALUE_DEFAULT;
	}
	else
	{
		std::string tempValue;
		cameraProperty.propertyType = (ipa_CameraSensors::TYPE_UNSIGNED | ipa_CameraSensors::TYPE_CHARACTER);
		m_RangeCameraParameters.m_IntegrationTime.clear(); // Clear flags
		m_RangeCameraParameters.m_IntegrationTime.seekg(0); // Set Pointer to position 0 within stringstream
		m_RangeCameraParameters.m_IntegrationTime >> tempValue;
		cameraProperty.u_charData = (unsigned char)atoi(tempValue.c_str());
	}

	if (SetProperty(&cameraProperty) & ipa_CameraSensors::RET_FAILED)
	{
		std::cout << "WARNING - Swissranger::SetParameters:" << std::endl;
		std::cout << "\t ... Could not set integration time" << std::endl;
	}

// -----------------------------------------------------------------
// Set modulation frequency
// -----------------------------------------------------------------
	cameraProperty.propertyID = ipa_CameraSensors::PROP_MODULATION_FREQUENCY;
	std::string sModulationFrequency = "";
	m_RangeCameraParameters.m_ModulationFrequency.clear(); // Clear flags
	m_RangeCameraParameters.m_ModulationFrequency.seekg(0); // Set Pointer to position 0 within stringstream
	m_RangeCameraParameters.m_ModulationFrequency >> sModulationFrequency;
	if (sModulationFrequency == "AUTO")
	{
		cameraProperty.propertyType = ipa_CameraSensors::TYPE_SPECIAL;
		cameraProperty.specialValue = VALUE_AUTO;
	}
	else if (sModulationFrequency == "DEFAULT")
	{
		cameraProperty.propertyType = ipa_CameraSensors::TYPE_SPECIAL;
		cameraProperty.specialValue = VALUE_DEFAULT;
	}
	else
	{
		cameraProperty.propertyType = (ipa_CameraSensors::TYPE_STRING);
		m_RangeCameraParameters.m_ModulationFrequency.clear(); // Clear flags
		m_RangeCameraParameters.m_ModulationFrequency.seekg(0); // Set Pointer to position 0 within stringstream
		m_RangeCameraParameters.m_ModulationFrequency >> cameraProperty.stringData;
	}

	if (SetProperty(&cameraProperty) & ipa_CameraSensors::RET_FAILED)
	{
		std::cout << "WARNING - Swissranger::SetParameters:" << std::endl;
		std::cout << "\t ... Could not set modulation frequency" << std::endl;
	}

// -----------------------------------------------------------------
// Set acquire mode
// -----------------------------------------------------------------
	cameraProperty.propertyID = ipa_CameraSensors::PROP_ACQUIRE_MODE;
	cameraProperty.propertyType = ipa_CameraSensors::TYPE_INTEGER;
	m_RangeCameraParameters.m_AcquireMode.clear(); // Set Pointer to position 0 within stringstream
	m_RangeCameraParameters.m_AcquireMode.seekg(0); // Set Pointer to position 0 within stringstream
	m_RangeCameraParameters.m_AcquireMode >> cameraProperty.integerData;
	if (SetProperty(&cameraProperty) & ipa_CameraSensors::RET_FAILED)
	{
		std::cout << "WARNING - Swissranger::SetParameters:" << std::endl;
		std::cout << "\t ... Could not set acquire mode" << std::endl;
	}

	return RET_OK;
}

unsigned long Swissranger::LoadParameters(const char* filename, int cameraIndex)
{
	// Load SwissRanger parameters.
	boost::shared_ptr<TiXmlDocument> p_configXmlDocument (new TiXmlDocument( filename ));

	if (!p_configXmlDocument->LoadFile())
	{
		std::cerr << "ERROR - Swissranger::LoadParameters:" << std::endl;
		std::cerr << "\t ... Error while loading xml configuration file \n";
		std::cerr << "\t ... (Check filename and syntax of the file):\n";
		std::cerr << "\t ... '" << filename << "'" << std::endl;
		return (RET_FAILED | RET_FAILED_OPEN_FILE);
	}
	std::cout << "INFO - Swissranger::LoadParameters:" << std::endl;
	std::cout << "\t ... Parsing xml configuration file:" << std::endl;
	std::cout << "\t ... '" << filename << "'" << std::endl;

	std::string tempString;
	if ( p_configXmlDocument )
	{

//************************************************************************************
//	BEGIN LibCameraSensors
//************************************************************************************
		// Tag element "LibCameraSensors" of Xml Inifile
		TiXmlElement *p_xmlElement_Root = NULL;
		p_xmlElement_Root = p_configXmlDocument->FirstChildElement( "LibCameraSensors" );

		if ( p_xmlElement_Root )
		{

//************************************************************************************
//	BEGIN LibCameraSensors->Swissranger
//************************************************************************************
			// Tag element "Swissranger3000" of Xml Inifile
			TiXmlElement *p_xmlElement_Root_SR31 = NULL;
			std::stringstream ss;
			ss << "Swissranger_" << cameraIndex;
			p_xmlElement_Root_SR31 = p_xmlElement_Root->FirstChildElement( ss.str() );
			if ( p_xmlElement_Root_SR31 )
			{

//************************************************************************************
//	BEGIN LibCameraSensors->Swissranger->Role
//************************************************************************************
				// Subtag element "Role" of Xml Inifile
				TiXmlElement* p_xmlElement_Child = NULL;
				p_xmlElement_Child = p_xmlElement_Root_SR31->FirstChildElement( "Role" );
				if ( p_xmlElement_Child )
				{
					// read and save value of attribute
					if ( p_xmlElement_Child->QueryValueAttribute( "value", &tempString ) != TIXML_SUCCESS)
					{
						std::cerr << "ERROR - Swissranger::LoadParameters:" << std::endl;
						std::cerr << "\t ... Can't find attribute 'value' of tag 'Role'." << std::endl;
						return (RET_FAILED | RET_XML_ATTR_NOT_FOUND);
					}

					if (tempString == "MASTER") m_RangeCameraParameters.m_CameraRole = MASTER;
					else if (tempString == "SLAVE") m_RangeCameraParameters.m_CameraRole = SLAVE;
					else
					{
						std::cerr << "ERROR - Swissranger::LoadParameters:" << std::endl;
						std::cerr << "\t ... Role " << tempString << " unspecified." << std::endl;
						return (RET_FAILED);
					}

				}
				else
				{
					std::cerr << "ERROR - Swissranger::LoadParameters:" << std::endl;
					std::cerr << "\t ... Can't find tag 'Role'." << std::endl;
					return (RET_FAILED | RET_XML_TAG_NOT_FOUND);
				}

//************************************************************************************
//	BEGIN LibCameraSensors->Swissranger->Interface
//************************************************************************************
				// Subtag element "OperationMode" of Xml Inifile
				p_xmlElement_Child = NULL;
				p_xmlElement_Child = p_xmlElement_Root_SR31->FirstChildElement( "Interface" );
				std::string tempString;
				if ( p_xmlElement_Child )
				{
					// read and save value of attribute
					if ( p_xmlElement_Child->QueryValueAttribute( "value", &tempString ) != TIXML_SUCCESS)
					{
						std::cerr << "ERROR - Swissranger::LoadParameters:" << std::endl;
						std::cerr << "\t ... Can't find attribute 'value' of tag 'Interface'." << std::endl;
						return (RET_FAILED | RET_XML_ATTR_NOT_FOUND);
					}
					if (tempString == "USB")
					{
						m_RangeCameraParameters.m_Interface.str( " " );	// Clear stringstream
						m_RangeCameraParameters.m_Interface.clear();		// Reset flags
						m_RangeCameraParameters.m_Interface << tempString;
					}
					else if (tempString == "ETHERNET")
					{
						m_RangeCameraParameters.m_Interface.str( " " );	// Clear stringstream
						m_RangeCameraParameters.m_Interface.clear();		// Reset flags
						m_RangeCameraParameters.m_Interface << tempString;
						// read and save value of attribute
						if ( p_xmlElement_Child->QueryValueAttribute( "ip", &tempString ) != TIXML_SUCCESS)
						{
							std::cerr << "ERROR - Swissranger::LoadParameters:" << std::endl;
							std::cerr << "\t ... Can't find attribute 'ip' of tag 'Interface'." << std::endl;
							return (RET_FAILED | RET_XML_ATTR_NOT_FOUND);
						}
						m_RangeCameraParameters.m_IP.str( " " );	// Clear stringstream
						m_RangeCameraParameters.m_IP.clear();		// Reset flags
						m_RangeCameraParameters.m_IP << tempString;
					}
					else
					{
						std::cerr << "ERROR - Swissranger::LoadParameters:" << std::endl;
						std::cerr << "\t ... Interface " << tempString << " unspecified." << std::endl;
						return (RET_FAILED);
					}
				}
				else
				{
					std::cerr << "ERROR - Swissranger::LoadParameters:" << std::endl;
					std::cerr << "\t ... Can't find tag 'Interface'." << std::endl;
					return (RET_FAILED | RET_XML_TAG_NOT_FOUND);
				}

//************************************************************************************
//	BEGIN LibCameraSensors->Swissranger->AmplitudeThreshold
//************************************************************************************
				// Subtag element "IntegrationTime" of Xml Inifile
				p_xmlElement_Child = NULL;
				p_xmlElement_Child = p_xmlElement_Root_SR31->FirstChildElement( "AmplitudeThreshold" );
				if ( p_xmlElement_Child )
				{
					// read and save value of attribute
					if ( p_xmlElement_Child->QueryValueAttribute( "value", &tempString) != TIXML_SUCCESS)
					{
						std::cerr << "ERROR - Swissranger::LoadParameters:" << std::endl;
						std::cerr << "\t ... Can't find attribute 'value' of tag 'AmplitudeThreshold'." << std::endl;
						return (RET_FAILED | RET_XML_ATTR_NOT_FOUND);
					}
					else
					{
						m_RangeCameraParameters.m_AmplitudeThreshold.str( " " );	// Clear stringstream
						m_RangeCameraParameters.m_AmplitudeThreshold.clear();		// Reset flags
						m_RangeCameraParameters.m_AmplitudeThreshold << tempString;
					}
				}
				else
				{
					std::cerr << "ERROR - Swissranger::LoadParameters:" << std::endl;
					std::cerr << "\t ... Can't find tag 'AmplitudeThreshold'." << std::endl;
					return (RET_FAILED | RET_XML_TAG_NOT_FOUND);
				}

//************************************************************************************
//	BEGIN LibCameraSensors->Swissranger->IntegrationTime
//************************************************************************************
				// Subtag element "IntegrationTime" of Xml Inifile
				p_xmlElement_Child = NULL;
				p_xmlElement_Child = p_xmlElement_Root_SR31->FirstChildElement( "IntegrationTime" );
				if ( p_xmlElement_Child )
				{
					// read and save value of attribute
					if ( p_xmlElement_Child->QueryValueAttribute( "value", &tempString) != TIXML_SUCCESS)
					{
						std::cerr << "ERROR - Swissranger::LoadParameters:" << std::endl;
						std::cerr << "\t ... Can't find attribute 'value' of tag 'IntegrationTime'." << std::endl;
						return (RET_FAILED | RET_XML_ATTR_NOT_FOUND);
					}
					else
					{
						m_RangeCameraParameters.m_IntegrationTime.str( " " );	// Clear stringstream
						m_RangeCameraParameters.m_IntegrationTime.clear();		// Reset flags
						m_RangeCameraParameters.m_IntegrationTime << tempString;
					}
				}
				else
				{
					std::cerr << "ERROR - Swissranger::LoadParameters:" << std::endl;
					std::cerr << "\t ... Can't find tag 'IntegrationTime'." << std::endl;
					return (RET_FAILED | RET_XML_TAG_NOT_FOUND);
				}

//************************************************************************************
//	BEGIN LibCameraSensors->Swissranger->Modulation
//************************************************************************************
				// Subtag element "IntegrationTime" of Xml Inifile
				p_xmlElement_Child = NULL;
				p_xmlElement_Child = p_xmlElement_Root_SR31->FirstChildElement( "Modulation" );
				if ( p_xmlElement_Child )
				{
					// read and save value of attribute
					if ( p_xmlElement_Child->QueryValueAttribute( "frequency", &tempString) != TIXML_SUCCESS)
					{
						std::cerr << "ERROR - Swissranger::LoadParameters:" << std::endl;
						std::cerr << "\t ... Can't find attribute 'frequency' of tag 'Modulation'." << std::endl;
						return (RET_FAILED | RET_XML_ATTR_NOT_FOUND);
					}
					else
					{
						m_RangeCameraParameters.m_ModulationFrequency.str( " " );	// Clear stringstream
						m_RangeCameraParameters.m_ModulationFrequency.clear();		// Reset flags
						m_RangeCameraParameters.m_ModulationFrequency << tempString;
					}
				}
				else
				{
					std::cerr << "ERROR - Swissranger::LoadParameters:" << std::endl;
					std::cerr << "\t ... Can't find tag 'Modulation'." << std::endl;
					return (RET_FAILED | RET_XML_TAG_NOT_FOUND);
				}

//************************************************************************************
//	BEGIN LibCameraSensors->Swissranger->AcquireMode
//************************************************************************************
				// Subtag element "IntegrationTime" of Xml Inifile
				p_xmlElement_Child = NULL;
				p_xmlElement_Child = p_xmlElement_Root_SR31->FirstChildElement( "AcquireMode" );
				if ( p_xmlElement_Child )
				{
					int acquireMode = 0;
					// read and save value of attribute
					if ( p_xmlElement_Child->QueryValueAttribute( "AM_COR_FIX_PTRN", &tempString) != TIXML_SUCCESS)
					{
						std::cerr << "ERROR - Swissranger::LoadParameters:" << std::endl;
						std::cerr << "\t ... Can't find attribute 'AM_COR_FIX_PTRN' of tag 'AcquireMode'." << std::endl;
						return (RET_FAILED | RET_XML_ATTR_NOT_FOUND);
					}
					else
					{
						if (tempString == "ON") acquireMode |= AM_COR_FIX_PTRN;
					}

					if ( p_xmlElement_Child->QueryValueAttribute( "AM_MEDIAN", &tempString) != TIXML_SUCCESS)
					{
						std::cerr << "ERROR - Swissranger::LoadParameters:" << std::endl;
						std::cerr << "\t ... Can't find attribute 'AM_MEDIAN' of tag 'AcquireMode'." << std::endl;
						return (RET_FAILED | RET_XML_ATTR_NOT_FOUND);
					}
					else
					{
						if (tempString == "ON") acquireMode |= AM_MEDIAN;
					}

					if ( p_xmlElement_Child->QueryValueAttribute( "AM_TOGGLE_FRQ", &tempString) != TIXML_SUCCESS)
					{
						std::cerr << "ERROR - Swissranger::LoadParameters:" << std::endl;
						std::cerr << "\t ... Can't find attribute 'AM_TOGGLE_FRQ' of tag 'AcquireMode'." << std::endl;
						return (RET_FAILED | RET_XML_ATTR_NOT_FOUND);
					}
					else
					{
						if (tempString == "ON") acquireMode |= AM_TOGGLE_FRQ;
					}

					if ( p_xmlElement_Child->QueryValueAttribute( "AM_CONV_GRAY", &tempString) != TIXML_SUCCESS)
					{
						std::cerr << "ERROR - Swissranger::LoadParameters:" << std::endl;
						std::cerr << "\t ... Can't find attribute 'AM_CONV_GRAY' of tag 'AcquireMode'." << std::endl;
						return (RET_FAILED | RET_XML_ATTR_NOT_FOUND);
					}
					else
					{
						if (tempString == "ON") acquireMode |= AM_CONV_GRAY;
					}

					if ( p_xmlElement_Child->QueryValueAttribute( "AM_SW_ANF", &tempString) != TIXML_SUCCESS)
					{
						std::cerr << "ERROR - Swissranger::LoadParameters:" << std::endl;
						std::cerr << "\t ... Can't find attribute 'AM_SW_ANF' of tag 'AcquireMode'." << std::endl;
						return (RET_FAILED | RET_XML_ATTR_NOT_FOUND);
					}
					else
					{
						if (tempString == "ON") acquireMode |= AM_SW_ANF;
					}

					if ( p_xmlElement_Child->QueryValueAttribute( "AM_SR3K_2TAP_PROC", &tempString) != TIXML_SUCCESS)
					{
						std::cerr << "ERROR - Swissranger::LoadParameters:" << std::endl;
						std::cerr << "\t ... Can't find attribute 'AM_SR3K_2TAP_PROC' of tag 'AcquireMode'." << std::endl;
						return (RET_FAILED | RET_XML_ATTR_NOT_FOUND);
					}
					else
					{
						//if (tempString == "ON") acquireMode |= AM_RESERVED0;
					}

					if ( p_xmlElement_Child->QueryValueAttribute( "AM_SHORT_RANGE", &tempString) != TIXML_SUCCESS)
					{
						std::cerr << "ERROR - Swissranger::LoadParameters:" << std::endl;
						std::cerr << "\t ... Can't find attribute 'AM_SHORT_RANGE' of tag 'AcquireMode'." << std::endl;
						return (RET_FAILED | RET_XML_ATTR_NOT_FOUND);
					}
					else
					{
						//if (tempString == "ON") acquireMode |= AM_RESERVED1;
					}

					if ( p_xmlElement_Child->QueryValueAttribute( "AM_CONF_MAP", &tempString) != TIXML_SUCCESS)
					{
						std::cerr << "ERROR - Swissranger::LoadParameters:" << std::endl;
						std::cerr << "\t ... Can't find attribute 'AM_CONF_MAP' of tag 'AcquireMode'." << std::endl;
						return (RET_FAILED | RET_XML_ATTR_NOT_FOUND);
					}
					else
					{
						if (tempString == "ON") acquireMode |= AM_CONF_MAP;
					}

					if ( p_xmlElement_Child->QueryValueAttribute( "AM_HW_TRIGGER", &tempString) != TIXML_SUCCESS)
					{
						std::cerr << "ERROR - Swissranger::LoadParameters:" << std::endl;
						std::cerr << "\t ... Can't find attribute 'AM_HW_TRIGGER' of tag 'AcquireMode'." << std::endl;
						return (RET_FAILED | RET_XML_ATTR_NOT_FOUND);
					}
					else
					{
						if (tempString == "ON") acquireMode |= AM_HW_TRIGGER;
					}

					if ( p_xmlElement_Child->QueryValueAttribute( "AM_SW_TRIGGER", &tempString) != TIXML_SUCCESS)
					{
						std::cerr << "ERROR - Swissranger::LoadParameters:" << std::endl;
						std::cerr << "\t ... Can't find attribute 'AM_SW_TRIGGER' of tag 'AcquireMode'." << std::endl;
						return (RET_FAILED | RET_XML_ATTR_NOT_FOUND);
					}
					else
					{
						if (tempString == "ON") acquireMode |= AM_SW_TRIGGER;
					}

					if ( p_xmlElement_Child->QueryValueAttribute( "AM_DENOISE_ANF", &tempString) != TIXML_SUCCESS)
					{
						std::cerr << "ERROR - Swissranger::LoadParameters:" << std::endl;
						std::cerr << "\t ... Can't find attribute 'AM_DENOISE_ANF' of tag 'AcquireMode'." << std::endl;
						return (RET_FAILED | RET_XML_ATTR_NOT_FOUND);
					}
					else
					{
						if (tempString == "ON") acquireMode |= AM_DENOISE_ANF;
					}

					if ( p_xmlElement_Child->QueryValueAttribute( "AM_MEDIANCROSS", &tempString) != TIXML_SUCCESS)
					{
						std::cerr << "ERROR - Swissranger::LoadParameters:" << std::endl;
						std::cerr << "\t ... Can't find attribute 'AM_MEDIANCROSS' of tag 'AcquireMode'." << std::endl;
						return (RET_FAILED | RET_XML_ATTR_NOT_FOUND);
					}
					else
					{
						if (tempString == "ON") acquireMode |= AM_MEDIANCROSS;
					}

					m_RangeCameraParameters.m_AcquireMode.str( " " );	// Clear stringstream
					m_RangeCameraParameters.m_AcquireMode.clear();		// Reset flags
					m_RangeCameraParameters.m_AcquireMode << acquireMode;
				}
				else
				{
					std::cerr << "ERROR - Swissranger::LoadParameters:" << std::endl;
					std::cerr << "\t ... Can't find tag 'AcquireMode'." << std::endl;
					return (RET_FAILED | RET_XML_TAG_NOT_FOUND);
				}

//************************************************************************************
//	BEGIN LibCameraSensors->Swissranger->CalibrationMethod
//************************************************************************************
				// Subtag element "OperationMode" of Xml Inifile
				p_xmlElement_Child = NULL;
				p_xmlElement_Child = p_xmlElement_Root_SR31->FirstChildElement( "CalibrationMethod" );
				if ( p_xmlElement_Child )
				{
					// read and save value of attribute
					if ( p_xmlElement_Child->QueryValueAttribute( "name", &tempString ) != TIXML_SUCCESS)
					{
						std::cerr << "ERROR - Swissranger::LoadParameters:" << std::endl;
						std::cerr << "\t ... Can't find attribute 'name' of tag 'CalibrationMethod'." << std::endl;
						return (RET_FAILED | RET_XML_ATTR_NOT_FOUND);
					}
					if (tempString == "MATLAB") m_CalibrationMethod = MATLAB;
					else if (tempString == "MATLAB_NO_Z") m_CalibrationMethod = MATLAB_NO_Z;
					else if (tempString == "NATIVE") m_CalibrationMethod = NATIVE;
					else
					{
						std::cerr << "ERROR - Swissranger::LoadParameters:" << std::endl;
						std::cerr << "\t ... Calibration mode " << tempString << " unspecified." << std::endl;
						return (RET_FAILED);
					}
				}
				else
				{
					std::cerr << "ERROR - Swissranger::LoadParameters:" << std::endl;
					std::cerr << "\t ... Can't find tag 'CalibrationMethod'." << std::endl;
					return (RET_FAILED | RET_XML_TAG_NOT_FOUND);
				}

			}
//************************************************************************************
//	END LibCameraSensors->Swissranger
//************************************************************************************
			else
			{
				std::cerr << "ERROR - Swissranger::LoadParameters:" << std::endl;
				std::cerr << "\t ... Can't find tag '" << ss.str() << "'" << std::endl;
				return (RET_FAILED | RET_XML_TAG_NOT_FOUND);
			}
		}

//************************************************************************************
//	END LibCameraSensors
//************************************************************************************
		else
		{
			std::cerr << "ERROR - Swissranger::LoadParameters:" << std::endl;
			std::cerr << "\t ... Can't find tag 'LibCameraSensors'." << std::endl;
			return (RET_FAILED | RET_XML_TAG_NOT_FOUND);
		}
	}


	std::cout << "INFO - Swissranger::LoadParameters:" << std::endl;
	std::cout << "\t ... Parsing xml calibration file: Done.\n";



	return RET_OK;
}
