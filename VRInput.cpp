#include "VRInput.h"

void VRInput::Init() {
	for (int code = vr::EVRButtonId::k_EButton_System; code < vr::EVRButtonId::k_EButton_Max; code++) {
		controller_states[static_cast<vr::EVRButtonId>(code)] = INPUT_STATE_UP;
	}
}

void VRInput::ProcessEvent(const vr::VREvent_t& e)
{
	std::string str_td_class = GetTrackedDeviceClassString(openvr_utils::vrSystem->GetTrackedDeviceClass(e.trackedDeviceIndex));

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
			<< openvr_utils::vrSystem->GetButtonIdNameFromEnum((vr::EVRButtonId)controller_data.button)
			<< " of device " << e.trackedDeviceIndex << " (" << str_td_class << ")" << std::endl;
		controller_states[(vr::EVRButtonId)controller_data.button] = INPUT_STATE_JUST_BECAME_DOWN;
		just_became_down_buttoncodes.push_back((vr::EVRButtonId)controller_data.button);
	}
	break;
	case vr::VREvent_ButtonUnpress:
	{
		vr::VREvent_Controller_t controller_data = e.data.controller;
		std::cout << "Unpressed button "
			<< openvr_utils::vrSystem->GetButtonIdNameFromEnum((vr::EVRButtonId)controller_data.button)
			<< " of device " << e.trackedDeviceIndex << " (" << str_td_class << ")" << std::endl;
		controller_states[(vr::EVRButtonId)controller_data.button] = INPUT_STATE_JUST_BECAME_UP;
		just_became_up_buttoncodes.push_back((vr::EVRButtonId)controller_data.button);
	}
	break;
	case vr::VREvent_ButtonTouch:
	{
		vr::VREvent_Controller_t controller_data = e.data.controller;
		std::cout << "Touched button "
			<< openvr_utils::vrSystem->GetButtonIdNameFromEnum((vr::EVRButtonId)controller_data.button)
			<< " of device " << e.trackedDeviceIndex << " (" << str_td_class << ")" << std::endl;
		controller_states[(vr::EVRButtonId)controller_data.button] = INPUT_STATE_TOUCHED;
		just_became_touchedcodes.push_back((vr::EVRButtonId)controller_data.button);
	}
	break;
	case vr::VREvent_ButtonUntouch:
	{
		vr::VREvent_Controller_t controller_data = e.data.controller;
		std::cout << "Untouched button "
			<< openvr_utils::vrSystem->GetButtonIdNameFromEnum((vr::EVRButtonId)controller_data.button)
			<< " of device " << e.trackedDeviceIndex << " (" << str_td_class << ")" << std::endl;
		controller_states[(vr::EVRButtonId)controller_data.button] = INPUT_STATE_UNTOUCHED;
		just_became_untouchedcodes.push_back((vr::EVRButtonId)controller_data.button);
	}
	break;
	case vr::VREvent_Input_HapticVibration:
	{
		//   do nothing
	}
	break;
	//default:
	//	std::cout << e.eventType << std::endl;
	//	vr::VREvent_Controller_t controller_data = e.data.controller;
	//	std::cout << "NOT FOUND " << openvr_utils::vrSystem->GetButtonIdNameFromEnum((vr::EVRButtonId)controller_data.button) << std::endl;
	}
}

void VRInput::LateUpdate() {
	for (const vr::EVRButtonId& code : just_became_down_buttoncodes) {
		controller_states[code] = INPUT_STATE_DOWN;
	}
	just_became_down_buttoncodes.clear();
	for (const vr::EVRButtonId& code : just_became_up_buttoncodes) {
		controller_states[code] = INPUT_STATE_UP;
	}
	just_became_up_buttoncodes.clear();
}

bool VRInput::GetKey(const std::string& keyName) {
	auto it = vr_button_map.find(keyName);
	if (it != vr_button_map.end()) {
		auto keyState = controller_states.find(it->second);
		if (keyState != controller_states.end()) {
			INPUT_STATE state = keyState->second;
			return state == INPUT_STATE_DOWN || state == INPUT_STATE_JUST_BECAME_DOWN;
		}

	}
	return false;
}

bool VRInput::GetKeyDown(const std::string& keyName) {
	auto it = vr_button_map.find(keyName);
	if (it != vr_button_map.end()) {
		auto keyState = controller_states.find(it->second);
		if (keyState != controller_states.end()) {
			INPUT_STATE state = keyState->second;
			return state == INPUT_STATE_JUST_BECAME_DOWN;
		}
	}
	return false;
}

bool VRInput::GetKeyUp(const std::string& keyName) {
	auto it = vr_button_map.find(keyName);
	if (it != vr_button_map.end()) {
		auto keyState = controller_states.find(it->second);
		if (keyState != controller_states.end()) {
			INPUT_STATE state = keyState->second;
			return state == INPUT_STATE_JUST_BECAME_UP;
		}
	}
	return false;
}


std::string VRInput::GetTrackedDeviceClassString(vr::ETrackedDeviceClass td_class) {

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