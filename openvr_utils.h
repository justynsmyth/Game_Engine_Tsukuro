#pragma once
#include <string>
#include <iostream>
#include <vector>
#include <iomanip> // setprecision
#include <sstream> // stringstream
#include "openvr-master/headers/openvr.h"
#ifdef _WIN32
	#include "ThirdParty/SDL/include/SDL.h"
	#include "ThirdParty/SDL_image/include/SDL_image.h"
	#include "ThirdParty/SDL_ttf/include/SDL_ttf.h"
#endif
#include "TextRenderRequest.h"
#include "Renderer.h"
#include "ComponentManager.h"
#include "VRInput.h"
#include "glad/glad.h"
#include "shared/lodepng.h"
#include "shared/Matrices.h"
#include "shared/pathtools.h"
#include "CGLRenderModel.h"

struct Position {
	float x;
	float y;
	float z;
};

enum class TrackingUniverse {
	Seated,
	Standing
};

struct Quaternion {
	float w;
	float x;
	float y;
	float z;

	// Default constructor initializing to no rotation
	Quaternion() : w(1.0f), x(0.0f), y(0.0f), z(0.0f) {}

	Quaternion(float w, float x, float y, float z) : w(w), x(x), y(y), z(z) {}


};

struct VertexDataScene
{
	b2Vec3 position;
	b2Vec2 texCoord;
};

struct VertexDataWindow
{
	b2Vec2 position;
	b2Vec2 texCoord;

	VertexDataWindow(const b2Vec2& pos, const b2Vec2 tex) : position(pos), texCoord(tex) {	}
};

struct FramebufferDesc
{
	GLuint m_nDepthBufferId;
	GLuint m_nRenderTextureId;
	GLuint m_nRenderFramebufferId;
	GLuint m_nResolveTextureId;
	GLuint m_nResolveFramebufferId;
};

struct ControllerInfo_t
{
	vr::VRInputValueHandle_t m_source = vr::k_ulInvalidInputValueHandle;
	vr::VRActionHandle_t m_actionPose = vr::k_ulInvalidActionHandle;
	vr::VRActionHandle_t m_actionHaptic = vr::k_ulInvalidActionHandle;
	Matrix4 m_rmat4Pose;
	CGLRenderModel* m_pRenderModel = nullptr;
	std::string m_sRenderModelName;
	bool m_bShowController;
};


/// <summary>
/// https://skarredghost.com/2018/03/15/introduction-to-openvr-101-series-what-is-openvr-and-how-to-get-started-with-its-apis/
/// This helped!
/// </summary>
class openvr_utils
{
public:
	openvr_utils();
	static void Initialize();
	static std::string ftos(float f, int precision);
	std::string GetTrackedDeviceString(vr::IVRSystem* pHmd, vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError* peError);
	std::string GetTrackedDeviceString(vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError* peError);
	std::string GetTrackedDeviceClassString(vr::ETrackedDeviceClass td_class);
	bool SetupTexturemaps();
	void SetupScene();
	void SetupCameras();
	bool SetupStereoRenderTargets();
	void InitiateCompanionWindow(SDL_Window* window, int window_w, int window_h);
	void SetupCompanionWindow();
	bool CreateFrameBuffer(int nWidth, int nHeight, FramebufferDesc& framebufferDesc);
	bool CreateAllShaders();
	GLuint CompileGLShader(const char* pchShaderName, const char* pchVertexShader, const char* pchFragmentShader);
	void AddCubeToScene(Matrix4 mat, std::vector<float>& vertdata);
	void AddCubeVertex(float fl0, float fl1, float fl2, float fl3, float fl4, std::vector<float>& vertdata);
	Matrix4 GetHMDMatrixProjectionEye(vr::Hmd_Eye nEye);
	Matrix4 GetHMDMatrixPoseEye(vr::Hmd_Eye nEye);
	void UpdateHMDMatrixPose();
	Matrix4 GetCurrentViewProjectionMatrix(vr::Hmd_Eye nEye);
	Matrix4 ConvertSteamVRMatrixToMatrix4(const vr::HmdMatrix34_t& matPose);
	void CheckDevices();
	void HandleInput();
	bool GetDigitalActionRisingEdge(vr::VRActionHandle_t action, vr::VRInputValueHandle_t* pDevicePath);
	bool GetDigitalActionFallingEdge(vr::VRActionHandle_t action, vr::VRInputValueHandle_t* pDevicePath);
	bool GetDigitalActionState(vr::VRActionHandle_t action, vr::VRInputValueHandle_t* pDevicePath);
	CGLRenderModel* FindOrLoadRenderModel(const char* pchRenderModelName);
	void TrackDevicePoses();
	static bool VRActivated();
	static Position GetDevicePosition(int device_n, std::string track_universe);
	static b2Vec3 GetDeviceForwardVector(int device_n, std::string track_universe);
	static b2Vec2 GetJoystickPosition(int device_n);
	static void TriggerHaptic(int device_n, uint32_t axisId, unsigned short durationMicroSec);
	void RenderFrame();
	static inline vr::IVRSystem* vrSystem;
	void RenderScene(vr::Hmd_Eye nEye);
	void RenderCompanionWindow();
	static bool initCompositor();

private:
	static inline bool vr_mode;
	void RenderControllerAxes();
	void RenderStereoTargets();
	void ProcessVREvent(vr::VREvent_t& e);
	static std::string vftos(float v[3], int precision);
	static inline std::vector<std::string> tracked_devices;

	Matrix4 m_rmat4DevicePose[vr::k_unMaxTrackedDeviceCount];
	vr::TrackedDevicePose_t m_rTrackedDevicePose[vr::k_unMaxTrackedDeviceCount];

	GLuint m_unSceneProgramID;
	GLuint m_unCompanionWindowProgramID;
	GLuint m_unControllerTransformProgramID;
	GLuint m_unRenderModelProgramID;

	GLint m_nSceneMatrixLocation;
	GLint m_nControllerMatrixLocation;
	GLint m_nRenderModelMatrixLocation;

	GLuint m_glControllerVertBuffer;
	GLuint m_unControllerVAO;
	unsigned int m_uiControllerVertcount;

	int m_iSceneVolumeInit;
	int m_iSceneVolumeWidth;
	int m_iSceneVolumeHeight;
	int m_iSceneVolumeDepth;

	float m_fScaleSpacing;
	float m_fScale;

	Matrix4 m_mat4HMDPose;
	Matrix4 m_mat4eyePosLeft;
	Matrix4 m_mat4eyePosRight;

	Matrix4 m_mat4ProjectionCenter;
	Matrix4 m_mat4ProjectionLeft;
	Matrix4 m_mat4ProjectionRight;

	GLuint m_glSceneVertBuffer;
	GLuint m_unSceneVAO;
	GLuint m_unCompanionWindowVAO;
	GLuint m_glCompanionWindowIDVertBuffer;
	GLuint m_glCompanionWindowIDIndexBuffer;
	unsigned int m_uiCompanionWindowIndexSize;

	FramebufferDesc leftEyeDesc;
	FramebufferDesc rightEyeDesc;

	uint32_t m_nRenderWidth;
	uint32_t m_nRenderHeight;

	unsigned int m_uiVertcount;

	float m_fNearClip;
	float m_fFarClip;

	GLuint m_iTexture;

	bool m_bVblank;
	bool m_bGlFinishHack;

	// SDL marking
	SDL_Window* m_pCompanionWindow;

	int m_iValidPoseCount;

	enum EHand
	{
		Left = 0,
		Right = 1,
	};
	ControllerInfo_t m_rHand[2];

	int m_iTrackedControllerCount;

	int window_w;
	int window_h;

	bool m_bShowCubes;
	
	std::vector< CGLRenderModel* > m_vecRenderModels;

	Vector2 m_vAnalogValue;

	std::string m_strPoseClasses;                            // what classes we saw poses for this frame
	char m_rDevClassChar[vr::k_unMaxTrackedDeviceCount];   // for each device, a character representing its class

	vr::VRActionHandle_t m_actionHideCubes = vr::k_ulInvalidActionHandle;
	vr::VRActionHandle_t m_actionHideThisController = vr::k_ulInvalidActionHandle;
	vr::VRActionHandle_t m_actionTriggerHaptic = vr::k_ulInvalidActionHandle;
	vr::VRActionHandle_t m_actionAnalongInput = vr::k_ulInvalidActionHandle;

	vr::VRActionSetHandle_t m_actionsetDemo = vr::k_ulInvalidActionSetHandle;
};

