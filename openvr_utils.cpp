#include "openvr_utils.h"


//void ThreadSleep(unsigned long nMilliseconds)
//{
//#if defined(_WIN32)
//	::Sleep(nMilliseconds);
//#elif defined(POSIX)
//	usleep(nMilliseconds * 1000);
//#endif
//}

openvr_utils::openvr_utils()
	: m_pCompanionWindow(NULL)
	, m_unSceneProgramID(0)
	, m_unCompanionWindowProgramID(0)
	, m_unControllerTransformProgramID(0)
	, m_unRenderModelProgramID(0)
	, m_bVblank(false)
	, m_bGlFinishHack(true)
	, m_glControllerVertBuffer(0)
	, m_unControllerVAO(0)
	, m_unSceneVAO(0)
	, m_nSceneMatrixLocation(-1)
	, m_nControllerMatrixLocation(-1)
	, m_nRenderModelMatrixLocation(-1)
	, m_iTrackedControllerCount(0)
	, m_iValidPoseCount(0)
	, m_iSceneVolumeInit(20)
	, m_strPoseClasses("")
	, m_bShowCubes(true)
{

	vr_mode = false;
	memset(m_rDevClassChar, 0, sizeof(m_rDevClassChar));

	// HARDCODED
	m_fScale = 0.3f;
	m_fScaleSpacing = 4.0f;

	m_fNearClip = 0.1f;
	m_fFarClip = 30.0f;
	m_iSceneVolumeWidth = m_iSceneVolumeInit;
	m_iSceneVolumeHeight = m_iSceneVolumeInit;
	m_iSceneVolumeDepth = m_iSceneVolumeInit;
	m_iTexture = 0;
	m_uiVertcount = 0;
}

/// <summary>
/// Checks for HMD device and activates vr_mode for game loop
/// </summary>
void openvr_utils::Initialize()
{
	// Check whether there is an HMD plugged-in
	if (!vr::VR_IsHmdPresent())
	{
		std::cout << "No HMD was found in the system" << std::endl;
		return;
	}
	vrSystem = nullptr;

	vr::HmdError hmdError = vr::VRInitError_None;
	vrSystem = vr::VR_Init(&hmdError, vr::VRApplication_Scene);

	if (hmdError != vr::VRInitError_None) {
		vrSystem = nullptr;
		std::cerr << "Failed to initialize OpenVR with error code: "
			<< vr::VR_GetVRInitErrorAsEnglishDescription(hmdError) << std::endl;
		return;
	}
	vr_mode = true;

	// initialize vector
	tracked_devices.resize(vr::k_unMaxTrackedDeviceCount);

	if (!initCompositor()) {
		std::cout << "Failed to initialize VR Compositor!\n" << std::endl;
	}

}

/// <summary>
/// Finds all devices and stores them in a map for future access
/// </summary>
void openvr_utils::CheckDevices() {
	int base_stations_count = 0;
	for (uint32_t td = vr::k_unTrackedDeviceIndex_Hmd; td < vr::k_unMaxTrackedDeviceCount; td++) {
		std::string tracked_device_type[vr::k_unMaxTrackedDeviceCount];
		std::string driver_name, driver_serial;
		if (vrSystem->IsTrackedDeviceConnected(td))
		{
			vr::ETrackedDeviceClass tracked_device_class = vrSystem->GetTrackedDeviceClass(td);

			std::string td_type = GetTrackedDeviceClassString(tracked_device_class);
			tracked_devices[td] = td_type;
			std::cout << "Tracking device " << td << " is connected " << std::endl;
			std::cout << "  Device type: " << td_type << ". Name: " << GetTrackedDeviceString(vrSystem, td, vr::Prop_TrackingSystemName_String, nullptr) << std::endl;

			if (tracked_device_class == vr::ETrackedDeviceClass::TrackedDeviceClass_TrackingReference) base_stations_count++;

			if (td == vr::k_unTrackedDeviceIndex_Hmd)
			{
				// Fill variables used for obtaining the device name and serial ID (used later for naming the SDL window)
				driver_name = GetTrackedDeviceString(vrSystem, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_TrackingSystemName_String, nullptr);
				driver_serial = GetTrackedDeviceString(vrSystem, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SerialNumber_String, nullptr);
			}
		}
		else {
			break;
		}
	}
}

void openvr_utils::HandleInput() {
	// Process SteamVR events
	if (vrSystem != NULL)
	{
		vr::VREvent_t event;
		while (vrSystem->PollNextEvent(&event, sizeof(event)))
		{
			VRInput::ProcessEvent(event);
		}
		//TrackDevicePoses();
	}

	vr::VRActiveActionSet_t actionSet = { 0 };
	actionSet.ulActionSet = m_actionsetDemo;
	vr::VRInput()->UpdateActionState(&actionSet, sizeof(actionSet), 1);

	m_bShowCubes = !GetDigitalActionState(m_actionHideCubes, nullptr);

	vr::VRInputValueHandle_t ulHapticDevice;
	if (GetDigitalActionRisingEdge(m_actionTriggerHaptic, &ulHapticDevice))
	{
		if (ulHapticDevice == m_rHand[Left].m_source)
		{
			vr::VRInput()->TriggerHapticVibrationAction(m_rHand[Left].m_actionHaptic, 0, 1, 4.f, 1.0f, vr::k_ulInvalidInputValueHandle);
		}
		if (ulHapticDevice == m_rHand[Right].m_source)
		{
			vr::VRInput()->TriggerHapticVibrationAction(m_rHand[Right].m_actionHaptic, 0, 1, 4.f, 1.0f, vr::k_ulInvalidInputValueHandle);
		}
	}

	vr::InputAnalogActionData_t analogData;
	if (vr::VRInput()->GetAnalogActionData(m_actionAnalongInput, &analogData, sizeof(analogData), vr::k_ulInvalidInputValueHandle) == vr::VRInputError_None && analogData.bActive)
	{
		m_vAnalogValue[0] = analogData.x;
		m_vAnalogValue[1] = analogData.y;
	}

	m_rHand[Left].m_bShowController = true;
	m_rHand[Right].m_bShowController = true;

	vr::VRInputValueHandle_t ulHideDevice;
	if (GetDigitalActionState(m_actionHideThisController, &ulHideDevice))
	{
		if (ulHideDevice == m_rHand[Left].m_source)
		{
			m_rHand[Left].m_bShowController = false;
		}
		if (ulHideDevice == m_rHand[Right].m_source)
		{
			m_rHand[Right].m_bShowController = false;
		}
	}

	for (EHand eHand = Left; eHand <= Right; ((int&)eHand)++)
	{
		vr::InputPoseActionData_t poseData;
		if (vr::VRInput()->GetPoseActionDataForNextFrame(m_rHand[eHand].m_actionPose, vr::TrackingUniverseStanding, &poseData, sizeof(poseData), vr::k_ulInvalidInputValueHandle) != vr::VRInputError_None
			|| !poseData.bActive || !poseData.pose.bPoseIsValid)
		{
			m_rHand[eHand].m_bShowController = false;
		}
		else
		{
			m_rHand[eHand].m_rmat4Pose = ConvertSteamVRMatrixToMatrix4(poseData.pose.mDeviceToAbsoluteTracking);

			vr::InputOriginInfo_t originInfo;
			if (vr::VRInput()->GetOriginTrackedDeviceInfo(poseData.activeOrigin, &originInfo, sizeof(originInfo)) == vr::VRInputError_None
				&& originInfo.trackedDeviceIndex != vr::k_unTrackedDeviceIndexInvalid)
			{
				std::string sRenderModelName = GetTrackedDeviceString(originInfo.trackedDeviceIndex, vr::Prop_RenderModelName_String, NULL);
				if (sRenderModelName != m_rHand[eHand].m_sRenderModelName)
				{
					m_rHand[eHand].m_pRenderModel = FindOrLoadRenderModel(sRenderModelName.c_str());
					m_rHand[eHand].m_sRenderModelName = sRenderModelName;
				}
			}
		}
	}
}

//---------------------------------------------------------------------------------------------------------------------
// Purpose: Returns true if the action is active and had a rising edge
//---------------------------------------------------------------------------------------------------------------------
bool openvr_utils::GetDigitalActionRisingEdge(vr::VRActionHandle_t action, vr::VRInputValueHandle_t* pDevicePath = nullptr)
{
	vr::InputDigitalActionData_t actionData;
	vr::VRInput()->GetDigitalActionData(action, &actionData, sizeof(actionData), vr::k_ulInvalidInputValueHandle);
	if (pDevicePath)
	{
		*pDevicePath = vr::k_ulInvalidInputValueHandle;
		if (actionData.bActive)
		{
			vr::InputOriginInfo_t originInfo;
			if (vr::VRInputError_None == vr::VRInput()->GetOriginTrackedDeviceInfo(actionData.activeOrigin, &originInfo, sizeof(originInfo)))
			{
				*pDevicePath = originInfo.devicePath;
			}
		}
	}
	return actionData.bActive && actionData.bChanged && actionData.bState;
}


//---------------------------------------------------------------------------------------------------------------------
// Purpose: Returns true if the action is active and had a falling edge
//---------------------------------------------------------------------------------------------------------------------
bool openvr_utils::GetDigitalActionFallingEdge(vr::VRActionHandle_t action, vr::VRInputValueHandle_t* pDevicePath = nullptr)
{
	vr::InputDigitalActionData_t actionData;
	vr::VRInput()->GetDigitalActionData(action, &actionData, sizeof(actionData), vr::k_ulInvalidInputValueHandle);
	if (pDevicePath)
	{
		*pDevicePath = vr::k_ulInvalidInputValueHandle;
		if (actionData.bActive)
		{
			vr::InputOriginInfo_t originInfo;
			if (vr::VRInputError_None == vr::VRInput()->GetOriginTrackedDeviceInfo(actionData.activeOrigin, &originInfo, sizeof(originInfo)))
			{
				*pDevicePath = originInfo.devicePath;
			}
		}
	}
	return actionData.bActive && actionData.bChanged && !actionData.bState;
}


//---------------------------------------------------------------------------------------------------------------------
// Purpose: Returns true if the action is active and its state is true
//---------------------------------------------------------------------------------------------------------------------
bool GetDigitalActionState(vr::VRActionHandle_t action, vr::VRInputValueHandle_t* pDevicePath = nullptr)
{
	vr::InputDigitalActionData_t actionData;
	vr::VRInput()->GetDigitalActionData(action, &actionData, sizeof(actionData), vr::k_ulInvalidInputValueHandle);
	if (pDevicePath)
	{
		*pDevicePath = vr::k_ulInvalidInputValueHandle;
		if (actionData.bActive)
		{
			vr::InputOriginInfo_t originInfo;
			if (vr::VRInputError_None == vr::VRInput()->GetOriginTrackedDeviceInfo(actionData.activeOrigin, &originInfo, sizeof(originInfo)))
			{
				*pDevicePath = originInfo.devicePath;
			}
		}
	}
	return actionData.bActive && actionData.bState;
}


//---------------------------------------------------------------------------------------------------------------------
// Purpose: Returns true if the action is active and its state is true
//---------------------------------------------------------------------------------------------------------------------
bool openvr_utils::GetDigitalActionState(vr::VRActionHandle_t action, vr::VRInputValueHandle_t* pDevicePath = nullptr)
{
	vr::InputDigitalActionData_t actionData;
	vr::VRInput()->GetDigitalActionData(action, &actionData, sizeof(actionData), vr::k_ulInvalidInputValueHandle);
	if (pDevicePath)
	{
		*pDevicePath = vr::k_ulInvalidInputValueHandle;
		if (actionData.bActive)
		{
			vr::InputOriginInfo_t originInfo;
			if (vr::VRInputError_None == vr::VRInput()->GetOriginTrackedDeviceInfo(actionData.activeOrigin, &originInfo, sizeof(originInfo)))
			{
				*pDevicePath = originInfo.devicePath;
			}
		}
	}
	return actionData.bActive && actionData.bState;
}

//-----------------------------------------------------------------------------
// Purpose: Finds a render model we've already loaded or loads a new one
//-----------------------------------------------------------------------------
CGLRenderModel* openvr_utils::FindOrLoadRenderModel(const char* pchRenderModelName)
{
	CGLRenderModel* pRenderModel = NULL;
	for (std::vector< CGLRenderModel* >::iterator i = m_vecRenderModels.begin(); i != m_vecRenderModels.end(); i++)
	{
		if (!_stricmp((*i)->GetName().c_str(), pchRenderModelName))
		{
			pRenderModel = *i;
			break;
		}
	}

	// load the model if we didn't find one
	if (!pRenderModel)
	{
		vr::RenderModel_t* pModel;
		vr::EVRRenderModelError error;
		while (1)
		{
			error = vr::VRRenderModels()->LoadRenderModel_Async(pchRenderModelName, &pModel);
			if (error != vr::VRRenderModelError_Loading)
				break;

			//ThreadSleep(1);
		}

		if (error != vr::VRRenderModelError_None)
		{
			std::cout << "Unable to load render model " << pchRenderModelName << " - "
				<< vr::VRRenderModels()->GetRenderModelErrorNameFromEnum(error) << "\n";
			return NULL; // move on to the next tracked device
		}

		vr::RenderModel_TextureMap_t* pTexture;
		while (1)
		{
			error = vr::VRRenderModels()->LoadTexture_Async(pModel->diffuseTextureId, &pTexture);
			if (error != vr::VRRenderModelError_Loading)
				break;

			//ThreadSleep(1);
		}

		if (error != vr::VRRenderModelError_None)
		{
			std::cout << "Unable to load render texture id:" << pModel->diffuseTextureId << " for render model " << pchRenderModelName << "\n";
			vr::VRRenderModels()->FreeRenderModel(pModel);
			return NULL; // move on to the next tracked device
		}

		pRenderModel = new CGLRenderModel(pchRenderModelName);
		if (!pRenderModel->BInit(*pModel, *pTexture))
		{
			std::cout << "Unable to create GL model from render model " << pchRenderModelName << "\n";
			delete pRenderModel;
			pRenderModel = NULL;
		}
		else
		{
			m_vecRenderModels.push_back(pRenderModel);
		}
		vr::VRRenderModels()->FreeRenderModel(pModel);
		vr::VRRenderModels()->FreeTexture(pTexture);
	}
	return pRenderModel;
}

void openvr_utils::TrackDevicePoses()
{
	vr::TrackedDevicePose_t tracked_device_pose[vr::k_unMaxTrackedDeviceCount];

	vrSystem->GetDeviceToAbsoluteTrackingPose(vr::ETrackingUniverseOrigin::TrackingUniverseSeated, 0, tracked_device_pose, vr::k_unMaxTrackedDeviceCount);

	int actual_y = 110, tracked_device_count = 0;
	for (int nDevice = 0; nDevice < vr::k_unMaxTrackedDeviceCount; nDevice++)
	{
		if ((tracked_device_pose[nDevice].bDeviceIsConnected) && (tracked_device_pose[nDevice].bPoseIsValid))
		{
			SDL_Color color = { 0, 200, 0, 255 };

			// Check whether the tracked device is a controller. If so, set text color based on the trigger button state
			vr::VRControllerState_t controller_state;
			if (vrSystem->GetControllerState(nDevice, &controller_state, sizeof(controller_state)))
				((vr::ButtonMaskFromId(vr::EVRButtonId::k_EButton_Axis1) & controller_state.ulButtonPressed) == 0) ? color = { 0, 200, 0, 255 } : color = { 0, 0, 200, 255 };

			std::string str_content = ("Tracked device #" + ftos((float)nDevice, 0) + " (" + tracked_devices[nDevice] + ")").c_str();
			int x_int = static_cast<int>(10);
			int y_int = static_cast<int>(actual_y);
			int font_size_int = static_cast<int>(12);
			std::string font_name = "NotoSans-Regular";
			TextRenderRequest t(str_content, font_name, color, font_size_int, x_int, y_int);
			Renderer::addTextRequest(t);

			// We take just the translation part of the matrix (actual position of tracked device, not orientation)
			float v[3] = { tracked_device_pose[nDevice].mDeviceToAbsoluteTracking.m[0][3], tracked_device_pose[nDevice].mDeviceToAbsoluteTracking.m[1][3], tracked_device_pose[nDevice].mDeviceToAbsoluteTracking.m[2][3] };

			str_content = vftos(v, 2).c_str();
			x_int = static_cast<int>(50);
			y_int = static_cast<int>(actual_y + 25);
			TextRenderRequest t2(str_content, font_name, color, font_size_int, x_int, y_int);
			Renderer::addTextRequest(t2);

			actual_y += 60;
			tracked_device_count++;
		}
	}
	std::string str_content = ("Tracked devices: " + ftos((float)tracked_device_count, 0)).c_str();
	int x_int = static_cast<int>(10);
	int y_int = static_cast<int>(70);
	int font_size_int = static_cast<int>(12);
	std::string font_name = "NotoSans-Regular";
	TextRenderRequest t(str_content, font_name, { 0, 0, 0, 255 }, font_size_int, x_int, y_int);
	Renderer::addTextRequest(t);
}

bool openvr_utils::VRActivated()
{
	return vr_mode;
}

Position openvr_utils::GetDevicePosition(int device_n, std::string track_universe)
{
	// Array filled with pose data for all tracked devices after GetDeviceToAbsoluteTrackingPose() call
	// unMaxTrackedDeviceCount refers to maximum number of devices the system will track
	vr::TrackedDevicePose_t tracked_device_pose[vr::k_unMaxTrackedDeviceCount];
	int time_offset_for_pose = 0;
	vr::ETrackingUniverseOrigin origin = vr::ETrackingUniverseOrigin::TrackingUniverseStanding;
	if (track_universe == "Seated") {
		origin = vr::ETrackingUniverseOrigin::TrackingUniverseSeated;
	}
	else if (track_universe == "Standing") {
		origin = vr::ETrackingUniverseOrigin::TrackingUniverseStanding;
	}
	vrSystem->GetDeviceToAbsoluteTrackingPose(origin, time_offset_for_pose, tracked_device_pose, vr::k_unMaxTrackedDeviceCount);
	/* Matrix format [R3x3 t3x1] :

	| R00 R01 R02 Tx |
	| R10 R11 R12 Ty |
	| R20 R21 R22 Tz |

	*/
	Position position;
	int actual_y = 110, tracked_device_count = 0;

	if ((tracked_device_pose[device_n].bDeviceIsConnected) && (tracked_device_pose[device_n].bPoseIsValid))
    {
        position.x = tracked_device_pose[device_n].mDeviceToAbsoluteTracking.m[0][3];
        position.y = tracked_device_pose[device_n].mDeviceToAbsoluteTracking.m[1][3];
        position.z = tracked_device_pose[device_n].mDeviceToAbsoluteTracking.m[2][3];
    }
    else
    {
        position.x = position.y = position.z = 0;
    }
	return position;
}

b2Vec3 openvr_utils::GetDeviceForwardVector(int device_n, std::string track_universe)
{
	b2Vec3 vector;
	vr::TrackedDevicePose_t tracked_device_pose[vr::k_unMaxTrackedDeviceCount];
	int time_offset_for_pose = 0;
	vr::ETrackingUniverseOrigin origin = vr::ETrackingUniverseOrigin::TrackingUniverseStanding;
	if (track_universe == "Seated") {
		origin = vr::ETrackingUniverseOrigin::TrackingUniverseSeated;
	}
	else if (track_universe == "Standing") {
		origin = vr::ETrackingUniverseOrigin::TrackingUniverseStanding;
	}
	vrSystem->GetDeviceToAbsoluteTrackingPose(origin, time_offset_for_pose, tracked_device_pose, vr::k_unMaxTrackedDeviceCount);
	if ((tracked_device_pose[device_n].bDeviceIsConnected) && (tracked_device_pose[device_n].bPoseIsValid)) {
		const auto& mat = tracked_device_pose[device_n].mDeviceToAbsoluteTracking;
		
		float m13 = mat.m[0][2];
		float m23 = mat.m[1][2];
		float m33 = mat.m[2][2];
		//std::cout << "R matrix: " << std::endl << "[" << m11 << ", " << m12 << ", " << m13 << "]" << std::endl
		//	<< "[" << m21 << ", " << m22 << ", " << m23 << "]" << std::endl
		//	<< "[" << m31 << ", " << m32 << ", " << m33 << "]" << std::endl;
		vector = b2Vec3(m13, m23, m33);
	}
	else {
		vector = b2Vec3(0.0f, 0.0f, 0.0f);
	}
	return vector;
}

b2Vec2 openvr_utils::GetJoystickPosition(int device_n)
{
	b2Vec2 vector(0, 0);
	vr::VRControllerState_t controllerState;
	if (vrSystem->GetControllerState(device_n, &controllerState, vr::k_unMaxTrackedDeviceCount))
	{
		vector.x = controllerState.rAxis[0].x; // Assuming axis 0 is the joystick x-axis
		vector.y = controllerState.rAxis[0].y; // Assuming axis 0 is the joystick y-axis
	}
	return vector;
}

/// <summary>
/// 
/// </summary>
/// <param name="device_n"></param>
/// <param name="axisId"></param>
/// <param name="durationMicroSec"> 5000 microseconds == 5 ms </param>
void openvr_utils::TriggerHaptic(int device_n, uint32_t axisId, unsigned short durationMicroSec)
{
	if (vrSystem != nullptr) {
		vrSystem->TriggerHapticPulse(device_n, axisId, durationMicroSec);
	}
}

void openvr_utils::RenderFrame()
{
	// Vertex Shader source code
	const char* vertexShaderSource = "#version 330 core\n"
		"layout (location = 0) in vec3 aPos;\n"
		"void main()\n"
		"{\n"
		"   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
		"}\0";
	//Fragment Shader source code
	const char* fragmentShaderSource = "#version 330 core\n"
		"out vec4 FragColor;\n"
		"void main()\n"
		"{\n"
		"   FragColor = vec4(0.8f, 0.3f, 0.02f, 1.0f);\n"
		"}\n\0";

	float vertices[] = {
		-0.5f, -0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
		0.0f, 0.5f, 0.0f
	};
	if (vrSystem)
	{
		RenderControllerAxes();
		RenderStereoTargets();
		RenderCompanionWindow();

		vr::Texture_t leftEyeTexture = { (void*)(uintptr_t)leftEyeDesc.m_nResolveTextureId, vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
		vr::VRCompositor()->Submit(vr::Eye_Left, &leftEyeTexture);
		vr::Texture_t rightEyeTexture = { (void*)(uintptr_t)rightEyeDesc.m_nResolveTextureId, vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
		vr::VRCompositor()->Submit(vr::Eye_Right, &rightEyeTexture);
	}

	if (m_bVblank && m_bGlFinishHack)
	{
		//$ HACKHACK. From gpuview profiling, it looks like there is a bug where two renders and a present
		// happen right before and after the vsync causing all kinds of jittering issues. This glFinish()
		// appears to clear that up. Temporary fix while I try to get nvidia to investigate this problem.
		// 1/29/2014 mikesart
		glFinish();
	}

	// SwapWindow
	{
		SDL_GL_SwapWindow(m_pCompanionWindow);
	}

	// Clear
	{
		// We want to make sure the glFinish waits for the entire present to complete, not just the submission
		// of the command. So, we do a clear here right here so the glFinish will wait fully for the swap.
		glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	// Flush and wait for swap.
	if (m_bVblank)
	{
		glFlush();
		glFinish();
	}

	UpdateHMDMatrixPose();
}

//-----------------------------------------------------------------------------
// Purpose: Draw all of the controllers as X/Y/Z lines
//-----------------------------------------------------------------------------
void openvr_utils::RenderControllerAxes()
{
	// Don't attempt to update controllers if input is not available
	if (!vrSystem->IsInputAvailable())
		return;

	std::vector<float> vertdataarray;

	m_uiControllerVertcount = 0;
	m_iTrackedControllerCount = 0;

	for (EHand eHand = Left; eHand <= Right; ((int&)eHand)++)
	{
		if (!m_rHand[eHand].m_bShowController)
			continue;

		const Matrix4& mat = m_rHand[eHand].m_rmat4Pose;

		Vector4 center = mat * Vector4(0, 0, 0, 1);

		for (int i = 0; i < 3; ++i)
		{
			Vector3 color(0, 0, 0);
			Vector4 point(0, 0, 0, 1);
			point[i] += 0.05f;  // offset in X, Y, Z
			color[i] = 1.0;  // R, G, B
			point = mat * point;
			vertdataarray.push_back(center.x);
			vertdataarray.push_back(center.y);
			vertdataarray.push_back(center.z);

			vertdataarray.push_back(color.x);
			vertdataarray.push_back(color.y);
			vertdataarray.push_back(color.z);

			vertdataarray.push_back(point.x);
			vertdataarray.push_back(point.y);
			vertdataarray.push_back(point.z);

			vertdataarray.push_back(color.x);
			vertdataarray.push_back(color.y);
			vertdataarray.push_back(color.z);

			m_uiControllerVertcount += 2;
		}

		Vector4 start = mat * Vector4(0, 0, -0.02f, 1);
		Vector4 end = mat * Vector4(0, 0, -39.f, 1);
		Vector3 color(.92f, .92f, .71f);

		vertdataarray.push_back(start.x);vertdataarray.push_back(start.y);vertdataarray.push_back(start.z);
		vertdataarray.push_back(color.x);vertdataarray.push_back(color.y);vertdataarray.push_back(color.z);

		vertdataarray.push_back(end.x);vertdataarray.push_back(end.y);vertdataarray.push_back(end.z);
		vertdataarray.push_back(color.x);vertdataarray.push_back(color.y);vertdataarray.push_back(color.z);
		m_uiControllerVertcount += 2;
	}

	// Setup the VAO the first time through.
	if (m_unControllerVAO == 0)
	{
		glGenVertexArrays(1, &m_unControllerVAO);
		glBindVertexArray(m_unControllerVAO);

		glGenBuffers(1, &m_glControllerVertBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, m_glControllerVertBuffer);

		GLuint stride = 2 * 3 * sizeof(float);
		uintptr_t offset = 0;

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (const void*)offset);

		offset += sizeof(Vector3);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (const void*)offset);

		glBindVertexArray(0);
	}

	glBindBuffer(GL_ARRAY_BUFFER, m_glControllerVertBuffer);

	// set vertex data if we have some
	if (vertdataarray.size() > 0)
	{
		//$ TODO: Use glBufferSubData for this...
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertdataarray.size(), &vertdataarray[0], GL_STREAM_DRAW);
	}
}

void openvr_utils::RenderStereoTargets()
{
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glEnable(GL_MULTISAMPLE);

	// Left Eye
	glBindFramebuffer(GL_FRAMEBUFFER, leftEyeDesc.m_nRenderFramebufferId);
	glViewport(0, 0, m_nRenderWidth, m_nRenderHeight);
	RenderScene(vr::Eye_Left);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glDisable(GL_MULTISAMPLE);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, leftEyeDesc.m_nRenderFramebufferId);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, leftEyeDesc.m_nResolveFramebufferId);

	glBlitFramebuffer(0, 0, m_nRenderWidth, m_nRenderHeight, 0, 0, m_nRenderWidth, m_nRenderHeight,
		GL_COLOR_BUFFER_BIT,
		GL_LINEAR);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	glEnable(GL_MULTISAMPLE);

	// Right Eye
	glBindFramebuffer(GL_FRAMEBUFFER, rightEyeDesc.m_nRenderFramebufferId);
	glViewport(0, 0, m_nRenderWidth, m_nRenderHeight);
	RenderScene(vr::Eye_Right);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glDisable(GL_MULTISAMPLE);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, rightEyeDesc.m_nRenderFramebufferId);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, rightEyeDesc.m_nResolveFramebufferId);

	glBlitFramebuffer(0, 0, m_nRenderWidth, m_nRenderHeight, 0, 0, m_nRenderWidth, m_nRenderHeight,
		GL_COLOR_BUFFER_BIT,
		GL_LINEAR);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

//-----------------------------------------------------------------------------
// Purpose: Renders a scene with respect to nEye.
//-----------------------------------------------------------------------------
void openvr_utils::RenderScene(vr::Hmd_Eye nEye)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	if (m_bShowCubes)
	{
		glUseProgram(m_unSceneProgramID);
		glUniformMatrix4fv(m_nSceneMatrixLocation, 1, GL_FALSE, GetCurrentViewProjectionMatrix(nEye).get());
		glBindVertexArray(m_unSceneVAO);
		glBindTexture(GL_TEXTURE_2D, m_iTexture);
		glDrawArrays(GL_TRIANGLES, 0, m_uiVertcount);
		glBindVertexArray(0);
	}

	bool bIsInputAvailable = vrSystem->IsInputAvailable();

	if (bIsInputAvailable)
	{
		// draw the controller axis lines
		glUseProgram(m_unControllerTransformProgramID);
		glUniformMatrix4fv(m_nControllerMatrixLocation, 1, GL_FALSE, GetCurrentViewProjectionMatrix(nEye).get());
		glBindVertexArray(m_unControllerVAO);
		glDrawArrays(GL_LINES, 0, m_uiControllerVertcount);
		glBindVertexArray(0);
	}

	// ----- Render Model rendering -----
	glUseProgram(m_unRenderModelProgramID);

	for (EHand eHand = Left; eHand <= Right; ((int&)eHand)++)
	{
		if (!m_rHand[eHand].m_bShowController || !m_rHand[eHand].m_pRenderModel)
			continue;

		const Matrix4& matDeviceToTracking = m_rHand[eHand].m_rmat4Pose;
		Matrix4 matMVP = GetCurrentViewProjectionMatrix(nEye) * matDeviceToTracking;
		glUniformMatrix4fv(m_nRenderModelMatrixLocation, 1, GL_FALSE, matMVP.get());

		m_rHand[eHand].m_pRenderModel->Draw();
	}

	glUseProgram(0);
}

void openvr_utils::RenderCompanionWindow()
{
	glDisable(GL_DEPTH_TEST);
	glViewport(0, 0, window_w, window_h);

	glBindVertexArray(m_unCompanionWindowVAO);
	glUseProgram(m_unCompanionWindowProgramID);

	// render left eye (first half of index array )
	glBindTexture(GL_TEXTURE_2D, leftEyeDesc.m_nResolveTextureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glDrawElements(GL_TRIANGLES, m_uiCompanionWindowIndexSize / 2, GL_UNSIGNED_SHORT, 0);

	// render right eye (second half of index array )
	glBindTexture(GL_TEXTURE_2D, rightEyeDesc.m_nResolveTextureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glDrawElements(GL_TRIANGLES, m_uiCompanionWindowIndexSize / 2, GL_UNSIGNED_SHORT, (const void*)(uintptr_t)(m_uiCompanionWindowIndexSize));

	glBindVertexArray(0);
	glUseProgram(0);
}

bool openvr_utils::initCompositor()
{
	vr::EVRInitError peError = vr::VRInitError_None;

	if (!vr::VRCompositor())
	{
		printf("Compositor initialization failed. See log file for details\n");
		return false;
	}

	return true;
}


void openvr_utils::ProcessVREvent(vr::VREvent_t& e)
{
	std::string str_td_class = GetTrackedDeviceClassString(vrSystem->GetTrackedDeviceClass(e.trackedDeviceIndex));

	switch (e.eventType)
	{
	case vr::VREvent_TrackedDeviceActivated:
	{
		std::cout << "Device " << e.trackedDeviceIndex << " attached (" << str_td_class << ")" << std::endl;
	}
	break;
	case vr::VREvent_TrackedDeviceDeactivated:
	{
		std::cout << "Device " << e.trackedDeviceIndex << " detached (" << str_td_class << ")" << std::endl;
	}
	break;
	case vr::VREvent_TrackedDeviceUpdated:
	{
		std::cout << "Device " << e.trackedDeviceIndex << " updated (" << str_td_class << ")" << std::endl;
	}
	break;
	case vr::VREvent_ButtonPress:
	{
		vr::VREvent_Controller_t controller_data = e.data.controller;
		std::cout << "Pressed button " 
				  << vrSystem->GetButtonIdNameFromEnum((vr::EVRButtonId)controller_data.button) 
				  << " of device " << e.trackedDeviceIndex << " (" << str_td_class << ")" << std::endl;
	}
	break;
	case vr::VREvent_ButtonUnpress:
	{
		vr::VREvent_Controller_t controller_data = e.data.controller;
		std::cout << "Unpressed button " 
				  << vrSystem->GetButtonIdNameFromEnum((vr::EVRButtonId)controller_data.button) 
				  << " of device " << e.trackedDeviceIndex << " (" << str_td_class << ")" << std::endl;
	}
	break;
	case vr::VREvent_ButtonTouch:
	{
		vr::VREvent_Controller_t controller_data = e.data.controller;
		std::cout << "Touched button " 
				  << vrSystem->GetButtonIdNameFromEnum((vr::EVRButtonId)controller_data.button) 
				  << " of device " << e.trackedDeviceIndex << " (" << str_td_class << ")" << std::endl;
	}
	break;
	case vr::VREvent_ButtonUntouch:
	{
		vr::VREvent_Controller_t controller_data = e.data.controller;
		std::cout << "Untouched button " 
				  << vrSystem->GetButtonIdNameFromEnum((vr::EVRButtonId)controller_data.button) 
				  << " of device " << e.trackedDeviceIndex << " (" << str_td_class << ")" << std::endl;
	}
	break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: helper to convert an 3-float array into a vector string
//-----------------------------------------------------------------------------
std::string openvr_utils::vftos(float v[3], int precision)
{
	std::stringstream stream;
	stream << "(" << ftos(v[0], precision) << "," << ftos(v[1], precision) << "," << ftos(v[2], precision) << ")";
	std::string f_str = stream.str();

	return f_str;
}

//-----------------------------------------------------------------------------
// Purpose: helper to convert an float into a string
//-----------------------------------------------------------------------------
std::string openvr_utils::ftos(float f, int precision)
{
	std::stringstream stream;
	stream << std::fixed << std::setprecision(precision) << f;
	std::string f_str = stream.str();

	return f_str;
}


std::string openvr_utils::GetTrackedDeviceString(vr::IVRSystem* pHmd, vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError* peError)
{
	uint32_t requiredBufferLen = pHmd->GetStringTrackedDeviceProperty(unDevice, prop, NULL, 0, peError);
	if (requiredBufferLen == 0)
		return "";

	char* pchBuffer = new char[requiredBufferLen];
	requiredBufferLen = pHmd->GetStringTrackedDeviceProperty(unDevice, prop, pchBuffer, requiredBufferLen, peError);
	std::string sResult = pchBuffer;
	delete[] pchBuffer;

	return sResult;
}

//-----------------------------------------------------------------------------
// Purpose: Helper to get a string from a tracked device property and turn it
//			into a std::string
//-----------------------------------------------------------------------------
std::string openvr_utils::GetTrackedDeviceString(vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError* peError = NULL)
{
	uint32_t unRequiredBufferLen = vr::VRSystem()->GetStringTrackedDeviceProperty(unDevice, prop, NULL, 0, peError);
	if (unRequiredBufferLen == 0)
		return "";

	char* pchBuffer = new char[unRequiredBufferLen];
	unRequiredBufferLen = vr::VRSystem()->GetStringTrackedDeviceProperty(unDevice, prop, pchBuffer, unRequiredBufferLen, peError);
	std::string sResult = pchBuffer;
	delete[] pchBuffer;
	return sResult;
}

std::string openvr_utils::GetTrackedDeviceClassString(vr::ETrackedDeviceClass td_class) {

	std::string str_td_class = "Unknown class";

	switch (td_class)
	{
	case vr::TrackedDeviceClass_Invalid:			// = 0, the ID was not valid.
		str_td_class = "invalid";
		break;
	case vr::TrackedDeviceClass_HMD:				// = 1, Head-Mounted Displays
		str_td_class = "hmd";
		break;
	case vr::TrackedDeviceClass_Controller:			// = 2, Tracked controllers
		str_td_class = "controller";
		break;
	case vr::TrackedDeviceClass_GenericTracker:		// = 3, Generic trackers, similar to controllers
		str_td_class = "generic tracker";
		break;
	case vr::TrackedDeviceClass_TrackingReference:	// = 4, Camera and base stations that serve as tracking reference points
		str_td_class = "base station";
		break;
	case vr::TrackedDeviceClass_DisplayRedirect:	// = 5, Accessories that aren't necessarily tracked themselves, but may redirect video output from other tracked devices
		str_td_class = "display redirect";
		break;
	}

	return str_td_class;
}

bool openvr_utils::SetupTexturemaps()
{
	std::string sExecutableDirectory = Path_StripFilename(Path_GetExecutablePath());
	std::string strFullPath = Path_MakeAbsolute("../cube_texture.png", sExecutableDirectory);

	std::vector<unsigned char> imageRGBA;
	unsigned nImageWidth, nImageHeight;
	unsigned nError = lodepng::decode(imageRGBA, nImageWidth, nImageHeight, strFullPath.c_str());

	if (nError != 0)
		return false;

	glGenTextures(1, &m_iTexture);
	glBindTexture(GL_TEXTURE_2D, m_iTexture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, nImageWidth, nImageHeight,
		0, GL_RGBA, GL_UNSIGNED_BYTE, &imageRGBA[0]);

	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	GLfloat fLargest;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &fLargest);
	glTexParameterf(GL_TEXTURE_2D, GL_MAX_TEXTURE_MAX_ANISOTROPY, fLargest);

	glBindTexture(GL_TEXTURE_2D, 0);

	return (m_iTexture != 0);
}



//-----------------------------------------------------------------------------
// Purpose: create a sea of cubes
//-----------------------------------------------------------------------------
void openvr_utils::SetupScene()
{
	if (!vrSystem)
		return;

	std::vector<float> vertdataarray;

	Matrix4 matScale;
	matScale.scale(m_fScale, m_fScale, m_fScale);
	Matrix4 matTransform;
	matTransform.translate(
		-((float)m_iSceneVolumeWidth * m_fScaleSpacing) / 2.f,
		-((float)m_iSceneVolumeHeight * m_fScaleSpacing) / 2.f,
		-((float)m_iSceneVolumeDepth * m_fScaleSpacing) / 2.f);

	Matrix4 mat = matScale * matTransform;

	for (int z = 0; z < m_iSceneVolumeDepth; z++)
	{
		for (int y = 0; y < m_iSceneVolumeHeight; y++)
		{
			for (int x = 0; x < m_iSceneVolumeWidth; x++)
			{
				AddCubeToScene(mat, vertdataarray);
				mat = mat * Matrix4().translate(m_fScaleSpacing, 0, 0);
			}
			mat = mat * Matrix4().translate(-((float)m_iSceneVolumeWidth) * m_fScaleSpacing, m_fScaleSpacing, 0);
		}
		mat = mat * Matrix4().translate(0, -((float)m_iSceneVolumeHeight) * m_fScaleSpacing, m_fScaleSpacing);
	}
	m_uiVertcount = vertdataarray.size() / 5;

	glGenVertexArrays(1, &m_unSceneVAO);
	glBindVertexArray(m_unSceneVAO);

	glGenBuffers(1, &m_glSceneVertBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, m_glSceneVertBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertdataarray.size(), &vertdataarray[0], GL_STATIC_DRAW);

	GLsizei stride = sizeof(VertexDataScene);
	uintptr_t offset = 0;

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (const void*)offset);

	offset += sizeof(Vector3);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (const void*)offset);

	glBindVertexArray(0);
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);

}

void openvr_utils::SetupCameras()
{
	m_mat4ProjectionLeft = GetHMDMatrixProjectionEye(vr::Eye_Left);
	m_mat4ProjectionRight = GetHMDMatrixProjectionEye(vr::Eye_Right);
	m_mat4eyePosLeft = GetHMDMatrixPoseEye(vr::Eye_Left);
	m_mat4eyePosRight = GetHMDMatrixPoseEye(vr::Eye_Right);
}

bool openvr_utils::SetupStereoRenderTargets()
{
	if (!vrSystem)
		return false;

	vrSystem->GetRecommendedRenderTargetSize(&m_nRenderWidth, &m_nRenderHeight);

	CreateFrameBuffer(m_nRenderWidth, m_nRenderHeight, leftEyeDesc);
	CreateFrameBuffer(m_nRenderWidth, m_nRenderHeight, rightEyeDesc);

	return true;
}

void openvr_utils::InitiateCompanionWindow(SDL_Window* window, int window_w, int window_h)
{
	this->window_w = window_w;
	this->window_h = window_h;
	m_pCompanionWindow = window;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void openvr_utils::SetupCompanionWindow()
{
	if (!vrSystem)
		return;

	std::vector<VertexDataWindow> vVerts;

	// left eye verts
	vVerts.push_back(VertexDataWindow(b2Vec2(-1, -1), b2Vec2(0, 1)));
	vVerts.push_back(VertexDataWindow(b2Vec2(0, -1), b2Vec2(1, 1)));
	vVerts.push_back(VertexDataWindow(b2Vec2(-1, 1), b2Vec2(0, 0)));
	vVerts.push_back(VertexDataWindow(b2Vec2(0, 1), b2Vec2(1, 0)));

	// right eye verts
	vVerts.push_back(VertexDataWindow(b2Vec2(0, -1), b2Vec2(0, 1)));
	vVerts.push_back(VertexDataWindow(b2Vec2(1, -1), b2Vec2(1, 1)));
	vVerts.push_back(VertexDataWindow(b2Vec2(0, 1), b2Vec2(0, 0)));
	vVerts.push_back(VertexDataWindow(b2Vec2(1, 1), b2Vec2(1, 0)));

	GLushort vIndices[] = { 0, 1, 3,   0, 3, 2,   4, 5, 7,   4, 7, 6 };
	m_uiCompanionWindowIndexSize = _countof(vIndices);

	glGenVertexArrays(1, &m_unCompanionWindowVAO);
	glBindVertexArray(m_unCompanionWindowVAO);

	glGenBuffers(1, &m_glCompanionWindowIDVertBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, m_glCompanionWindowIDVertBuffer);
	glBufferData(GL_ARRAY_BUFFER, vVerts.size() * sizeof(VertexDataWindow), &vVerts[0], GL_STATIC_DRAW);

	glGenBuffers(1, &m_glCompanionWindowIDIndexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_glCompanionWindowIDIndexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_uiCompanionWindowIndexSize * sizeof(GLushort), &vIndices[0], GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(VertexDataWindow), (void*)offsetof(VertexDataWindow, position));

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexDataWindow), (void*)offsetof(VertexDataWindow, texCoord));

	glBindVertexArray(0);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

//-----------------------------------------------------------------------------
// Purpose: Creates a frame buffer. Returns true if the buffer was set up.
//          Returns false if the setup failed.
//-----------------------------------------------------------------------------
bool openvr_utils::CreateFrameBuffer(int nWidth, int nHeight, FramebufferDesc& framebufferDesc)
{
	glGenFramebuffers(1, &framebufferDesc.m_nRenderFramebufferId);
	glBindFramebuffer(GL_FRAMEBUFFER, framebufferDesc.m_nRenderFramebufferId);

	glGenRenderbuffers(1, &framebufferDesc.m_nDepthBufferId);
	glBindRenderbuffer(GL_RENDERBUFFER, framebufferDesc.m_nDepthBufferId);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT, nWidth, nHeight);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, framebufferDesc.m_nDepthBufferId);

	glGenTextures(1, &framebufferDesc.m_nRenderTextureId);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, framebufferDesc.m_nRenderTextureId);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA8, nWidth, nHeight, true);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, framebufferDesc.m_nRenderTextureId, 0);

	glGenFramebuffers(1, &framebufferDesc.m_nResolveFramebufferId);
	glBindFramebuffer(GL_FRAMEBUFFER, framebufferDesc.m_nResolveFramebufferId);

	glGenTextures(1, &framebufferDesc.m_nResolveTextureId);
	glBindTexture(GL_TEXTURE_2D, framebufferDesc.m_nResolveTextureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, nWidth, nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebufferDesc.m_nResolveTextureId, 0);

	// check FBO status
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		return false;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return true;
}

bool openvr_utils::CreateAllShaders()
{
	m_unSceneProgramID = CompileGLShader(
		"Scene",

		// Vertex Shader
		"#version 410\n"
		"uniform mat4 matrix;\n"
		"layout(location = 0) in vec4 position;\n"
		"layout(location = 1) in vec2 v2UVcoordsIn;\n"
		"layout(location = 2) in vec3 v3NormalIn;\n"
		"out vec2 v2UVcoords;\n"
		"void main()\n"
		"{\n"
		"	v2UVcoords = v2UVcoordsIn;\n"
		"	gl_Position = matrix * position;\n"
		"}\n",

		// Fragment Shader
		"#version 410 core\n"
		"uniform sampler2D mytexture;\n"
		"in vec2 v2UVcoords;\n"
		"out vec4 outputColor;\n"
		"void main()\n"
		"{\n"
		"   outputColor = texture(mytexture, v2UVcoords);\n"
		"}\n"
	);
	m_nSceneMatrixLocation = glGetUniformLocation(m_unSceneProgramID, "matrix");
	if (m_nSceneMatrixLocation == -1)
	{
		std::cout << "Unable to find matrix uniform in scene shader\n";
		return false;
	}

	m_unControllerTransformProgramID = CompileGLShader(
		"Controller",

		// vertex shader
		"#version 410\n"
		"uniform mat4 matrix;\n"
		"layout(location = 0) in vec4 position;\n"
		"layout(location = 1) in vec3 v3ColorIn;\n"
		"out vec4 v4Color;\n"
		"void main()\n"
		"{\n"
		"	v4Color.xyz = v3ColorIn; v4Color.a = 1.0;\n"
		"	gl_Position = matrix * position;\n"
		"}\n",

		// fragment shader
		"#version 410\n"
		"in vec4 v4Color;\n"
		"out vec4 outputColor;\n"
		"void main()\n"
		"{\n"
		"   outputColor = v4Color;\n"
		"}\n"
	);
	m_nControllerMatrixLocation = glGetUniformLocation(m_unControllerTransformProgramID, "matrix");
	if (m_nControllerMatrixLocation == -1)
	{
		std::cout << "Unable to find matrix uniform in controller shader\n";
		return false;
	}

	m_unRenderModelProgramID = CompileGLShader(
		"render model",

		// vertex shader
		"#version 410\n"
		"uniform mat4 matrix;\n"
		"layout(location = 0) in vec4 position;\n"
		"layout(location = 1) in vec3 v3NormalIn;\n"
		"layout(location = 2) in vec2 v2TexCoordsIn;\n"
		"out vec2 v2TexCoord;\n"
		"void main()\n"
		"{\n"
		"	v2TexCoord = v2TexCoordsIn;\n"
		"	gl_Position = matrix * vec4(position.xyz, 1);\n"
		"}\n",

		//fragment shader
		"#version 410 core\n"
		"uniform sampler2D diffuse;\n"
		"in vec2 v2TexCoord;\n"
		"out vec4 outputColor;\n"
		"void main()\n"
		"{\n"
		"   outputColor = texture( diffuse, v2TexCoord);\n"
		"}\n"

	);
	m_nRenderModelMatrixLocation = glGetUniformLocation(m_unRenderModelProgramID, "matrix");
	if (m_nRenderModelMatrixLocation == -1)
	{
		std::cout << "Unable to find matrix uniform in render model shader\n";
		return false;
	}

	m_unCompanionWindowProgramID = CompileGLShader(
		"CompanionWindow",

		// vertex shader
		"#version 410 core\n"
		"layout(location = 0) in vec4 position;\n"
		"layout(location = 1) in vec2 v2UVIn;\n"
		"noperspective out vec2 v2UV;\n"
		"void main()\n"
		"{\n"
		"	v2UV = v2UVIn;\n"
		"	gl_Position = position;\n"
		"}\n",

		// fragment shader
		"#version 410 core\n"
		"uniform sampler2D mytexture;\n"
		"noperspective in vec2 v2UV;\n"
		"out vec4 outputColor;\n"
		"void main()\n"
		"{\n"
		"		outputColor = texture(mytexture, v2UV);\n"
		"}\n"
	);

	return m_unSceneProgramID != 0
		&& m_unControllerTransformProgramID != 0
		&& m_unRenderModelProgramID != 0
		&& m_unCompanionWindowProgramID != 0;
}

//-----------------------------------------------------------------------------
// Purpose: Compiles a GL shader program and returns the handle. Returns 0 if
//			the shader couldn't be compiled for some reason.
//-----------------------------------------------------------------------------
GLuint openvr_utils::CompileGLShader(const char* pchShaderName, const char* pchVertexShader, const char* pchFragmentShader)
{
	GLuint unProgramID = glCreateProgram();

	GLuint nSceneVertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(nSceneVertexShader, 1, &pchVertexShader, NULL);
	glCompileShader(nSceneVertexShader);

	GLint vShaderCompiled = GL_FALSE;
	glGetShaderiv(nSceneVertexShader, GL_COMPILE_STATUS, &vShaderCompiled);
	if (vShaderCompiled != GL_TRUE)
	{
		std::cout << pchShaderName << " - Unable to compile vertex shader " << nSceneVertexShader << "!\n";
		glDeleteProgram(unProgramID);
		glDeleteShader(nSceneVertexShader);
		return 0;
	}
	glAttachShader(unProgramID, nSceneVertexShader);
	glDeleteShader(nSceneVertexShader); // the program hangs onto this once it's attached

	GLuint  nSceneFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(nSceneFragmentShader, 1, &pchFragmentShader, NULL);
	glCompileShader(nSceneFragmentShader);

	GLint fShaderCompiled = GL_FALSE;
	glGetShaderiv(nSceneFragmentShader, GL_COMPILE_STATUS, &fShaderCompiled);
	if (fShaderCompiled != GL_TRUE)
	{
		std::cout << pchShaderName << " - Unable to compile fragment shader " << nSceneFragmentShader << "!\n";
		glDeleteProgram(unProgramID);
		glDeleteShader(nSceneFragmentShader);
		return 0;
	}

	glAttachShader(unProgramID, nSceneFragmentShader);
	glDeleteShader(nSceneFragmentShader); // the program hangs onto this once it's attached

	glLinkProgram(unProgramID);

	GLint programSuccess = GL_TRUE;
	glGetProgramiv(unProgramID, GL_LINK_STATUS, &programSuccess);
	if (programSuccess != GL_TRUE)
	{
		std::cout << pchShaderName << " - Error linking program " << unProgramID << "!\n";
		glDeleteProgram(unProgramID);
		return 0;
	}

	glUseProgram(unProgramID);
	glUseProgram(0);

	return unProgramID;
}



void openvr_utils::AddCubeToScene(Matrix4 mat, std::vector<float>& vertdata)
{
	// Matrix4 mat( outermat.data() );

	Vector4 A = mat * Vector4(0, 0, 0, 1);
	Vector4 B = mat * Vector4(1, 0, 0, 1);
	Vector4 C = mat * Vector4(1, 1, 0, 1);
	Vector4 D = mat * Vector4(0, 1, 0, 1);
	Vector4 E = mat * Vector4(0, 0, 1, 1);
	Vector4 F = mat * Vector4(1, 0, 1, 1);
	Vector4 G = mat * Vector4(1, 1, 1, 1);
	Vector4 H = mat * Vector4(0, 1, 1, 1);

	// triangles instead of quads
	AddCubeVertex(E.x, E.y, E.z, 0, 1, vertdata); //Front
	AddCubeVertex(F.x, F.y, F.z, 1, 1, vertdata);
	AddCubeVertex(G.x, G.y, G.z, 1, 0, vertdata);
	AddCubeVertex(G.x, G.y, G.z, 1, 0, vertdata);
	AddCubeVertex(H.x, H.y, H.z, 0, 0, vertdata);
	AddCubeVertex(E.x, E.y, E.z, 0, 1, vertdata);

	AddCubeVertex(B.x, B.y, B.z, 0, 1, vertdata); //Back
	AddCubeVertex(A.x, A.y, A.z, 1, 1, vertdata);
	AddCubeVertex(D.x, D.y, D.z, 1, 0, vertdata);
	AddCubeVertex(D.x, D.y, D.z, 1, 0, vertdata);
	AddCubeVertex(C.x, C.y, C.z, 0, 0, vertdata);
	AddCubeVertex(B.x, B.y, B.z, 0, 1, vertdata);

	AddCubeVertex(H.x, H.y, H.z, 0, 1, vertdata); //Top
	AddCubeVertex(G.x, G.y, G.z, 1, 1, vertdata);
	AddCubeVertex(C.x, C.y, C.z, 1, 0, vertdata);
	AddCubeVertex(C.x, C.y, C.z, 1, 0, vertdata);
	AddCubeVertex(D.x, D.y, D.z, 0, 0, vertdata);
	AddCubeVertex(H.x, H.y, H.z, 0, 1, vertdata);

	AddCubeVertex(A.x, A.y, A.z, 0, 1, vertdata); //Bottom
	AddCubeVertex(B.x, B.y, B.z, 1, 1, vertdata);
	AddCubeVertex(F.x, F.y, F.z, 1, 0, vertdata);
	AddCubeVertex(F.x, F.y, F.z, 1, 0, vertdata);
	AddCubeVertex(E.x, E.y, E.z, 0, 0, vertdata);
	AddCubeVertex(A.x, A.y, A.z, 0, 1, vertdata);

	AddCubeVertex(A.x, A.y, A.z, 0, 1, vertdata); //Left
	AddCubeVertex(E.x, E.y, E.z, 1, 1, vertdata);
	AddCubeVertex(H.x, H.y, H.z, 1, 0, vertdata);
	AddCubeVertex(H.x, H.y, H.z, 1, 0, vertdata);
	AddCubeVertex(D.x, D.y, D.z, 0, 0, vertdata);
	AddCubeVertex(A.x, A.y, A.z, 0, 1, vertdata);

	AddCubeVertex(F.x, F.y, F.z, 0, 1, vertdata); //Right
	AddCubeVertex(B.x, B.y, B.z, 1, 1, vertdata);
	AddCubeVertex(C.x, C.y, C.z, 1, 0, vertdata);
	AddCubeVertex(C.x, C.y, C.z, 1, 0, vertdata);
	AddCubeVertex(G.x, G.y, G.z, 0, 0, vertdata);
	AddCubeVertex(F.x, F.y, F.z, 0, 1, vertdata);
}

void openvr_utils::AddCubeVertex(float fl0, float fl1, float fl2, float fl3, float fl4, std::vector<float>& vertdata)
{
	vertdata.push_back(fl0);
	vertdata.push_back(fl1);
	vertdata.push_back(fl2);
	vertdata.push_back(fl3);
	vertdata.push_back(fl4);
}

//-----------------------------------------------------------------------------
// Purpose: Gets a Matrix Projection Eye with respect to nEye.
//-----------------------------------------------------------------------------
Matrix4 openvr_utils::GetHMDMatrixProjectionEye(vr::Hmd_Eye nEye)
{
	if (!vrSystem)
		return Matrix4();

	vr::HmdMatrix44_t mat = vrSystem->GetProjectionMatrix(nEye, m_fNearClip, m_fFarClip);

	return Matrix4(
		mat.m[0][0], mat.m[1][0], mat.m[2][0], mat.m[3][0],
		mat.m[0][1], mat.m[1][1], mat.m[2][1], mat.m[3][1],
		mat.m[0][2], mat.m[1][2], mat.m[2][2], mat.m[3][2],
		mat.m[0][3], mat.m[1][3], mat.m[2][3], mat.m[3][3]
	);
}


//-----------------------------------------------------------------------------
// Purpose: Gets an HMDMatrixPoseEye with respect to nEye.
//-----------------------------------------------------------------------------
Matrix4 openvr_utils::GetHMDMatrixPoseEye(vr::Hmd_Eye nEye)
{
	if (!vrSystem)
		return Matrix4();

	vr::HmdMatrix34_t matEyeRight = vrSystem->GetEyeToHeadTransform(nEye);
	Matrix4 matrixObj(
		matEyeRight.m[0][0], matEyeRight.m[1][0], matEyeRight.m[2][0], 0.0,
		matEyeRight.m[0][1], matEyeRight.m[1][1], matEyeRight.m[2][1], 0.0,
		matEyeRight.m[0][2], matEyeRight.m[1][2], matEyeRight.m[2][2], 0.0,
		matEyeRight.m[0][3], matEyeRight.m[1][3], matEyeRight.m[2][3], 1.0f
	);

	return matrixObj.invert();
}

void openvr_utils::UpdateHMDMatrixPose()
{
	if (!vrSystem)
		return;

	vr::VRCompositor()->WaitGetPoses(m_rTrackedDevicePose, vr::k_unMaxTrackedDeviceCount, NULL, 0);

	m_iValidPoseCount = 0;
	m_strPoseClasses = "";
	for (int nDevice = 0; nDevice < vr::k_unMaxTrackedDeviceCount; ++nDevice)
	{
		if (m_rTrackedDevicePose[nDevice].bPoseIsValid)
		{
			m_iValidPoseCount++;
			m_rmat4DevicePose[nDevice] = ConvertSteamVRMatrixToMatrix4(m_rTrackedDevicePose[nDevice].mDeviceToAbsoluteTracking);
			if (m_rDevClassChar[nDevice] == 0)
			{
				switch (vrSystem->GetTrackedDeviceClass(nDevice))
				{
				case vr::TrackedDeviceClass_Controller:        m_rDevClassChar[nDevice] = 'C'; break;
				case vr::TrackedDeviceClass_HMD:               m_rDevClassChar[nDevice] = 'H'; break;
				case vr::TrackedDeviceClass_Invalid:           m_rDevClassChar[nDevice] = 'I'; break;
				case vr::TrackedDeviceClass_GenericTracker:    m_rDevClassChar[nDevice] = 'G'; break;
				case vr::TrackedDeviceClass_TrackingReference: m_rDevClassChar[nDevice] = 'T'; break;
				default:                                       m_rDevClassChar[nDevice] = '?'; break;
				}
			}
			m_strPoseClasses += m_rDevClassChar[nDevice];
		}
	}

	if (m_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid)
	{
		m_mat4HMDPose = m_rmat4DevicePose[vr::k_unTrackedDeviceIndex_Hmd];
		m_mat4HMDPose.invert();
	}
}

Matrix4 openvr_utils::GetCurrentViewProjectionMatrix(vr::Hmd_Eye nEye)
{
	Matrix4 matMVP;
	if (nEye == vr::Eye_Left)
	{
		matMVP = m_mat4ProjectionLeft * m_mat4eyePosLeft * m_mat4HMDPose;
	}
	else if (nEye == vr::Eye_Right)
	{
		matMVP = m_mat4ProjectionRight * m_mat4eyePosRight * m_mat4HMDPose;
	}

	return matMVP;
}

//-----------------------------------------------------------------------------
// Purpose: Converts a SteamVR matrix to our local matrix class
//-----------------------------------------------------------------------------
Matrix4 openvr_utils::ConvertSteamVRMatrixToMatrix4(const vr::HmdMatrix34_t& matPose)
{
	Matrix4 matrixObj(
		matPose.m[0][0], matPose.m[1][0], matPose.m[2][0], 0.0,
		matPose.m[0][1], matPose.m[1][1], matPose.m[2][1], 0.0,
		matPose.m[0][2], matPose.m[1][2], matPose.m[2][2], 0.0,
		matPose.m[0][3], matPose.m[1][3], matPose.m[2][3], 1.0f
	);
	return matrixObj;
}