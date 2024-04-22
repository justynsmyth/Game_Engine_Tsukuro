#ifndef VRINPUT_H
#define VRINPUT_H

#include "ThirdParty/SDL/include/SDL.h"
#include <unordered_map>
#include <iostream>
#include <vector>
#include "ThirdParty/glm-0.9.9.8/glm/glm.hpp"
#include "ThirdParty/glm-0.9.9.8/glm/gtx/hash.hpp"
#include "openvr-master/headers/openvr.h"
#include "Input.h"
#include "openvr_utils.h"


class VRInput
{
public:
	static void Init(); // Call before main loop begins.
	static void ProcessEvent(const vr::VREvent_t& e); // Call every frame at start of event loop.
	static void LateUpdate();

	///* Interface */

	// Check if certain key is down currently
	static bool GetKey(const std::string& keyName);
	// Check if certain key is pressed down on this frame
	static bool GetKeyDown(const std::string& keyName);
	// Check if certain key was released this very frame
	static bool GetKeyUp(const std::string& keyName);
	static std::string GetTrackedDeviceClassString(vr::ETrackedDeviceClass td_class);


private:
	static inline std::unordered_map<vr::EVRButtonId, INPUT_STATE> controller_states;
	static inline std::vector<vr::EVRButtonId> just_became_down_buttoncodes;
	static inline std::vector<vr::EVRButtonId> just_became_up_buttoncodes;
	static inline std::vector<vr::EVRButtonId> just_became_touchedcodes;
	static inline std::vector<vr::EVRButtonId> just_became_untouchedcodes;

	static inline std::unordered_map<int, INPUT_STATE> mouse_button_states;
	static inline std::vector<int> just_became_down_buttons;
	static inline std::vector<int> just_became_up_buttons;
	static inline std::vector<int> just_became_touched;
	static inline std::vector<int> just_became_untouched;
    static inline const std::unordered_map<std::string, vr::EVRButtonId> vr_button_map = {
        // System buttons
        {"system", vr::k_EButton_System},
        {"menu", vr::k_EButton_ApplicationMenu}, //  This is the B Button on Oculus
        {"grip", vr::k_EButton_Grip},

        // D-Pad directions (not all controllers have these)
        {"dpad_left", vr::k_EButton_DPad_Left},
        {"dpad_up", vr::k_EButton_DPad_Up},
        {"dpad_right", vr::k_EButton_DPad_Right},
        {"dpad_down", vr::k_EButton_DPad_Down},

        // Other buttons
        {"button_a", vr::k_EButton_A},

        // Axes for thumbsticks or touchpads (not all have these)
        // aliases for well known controllers 
        {"touchpad", vr::k_EButton_SteamVR_Touchpad}, // Alias to k_EButton_Axis0
        {"trigger", vr::k_EButton_SteamVR_Trigger}, // Alias to k_EButton_Axis1

        // Alias for joystick or thumbstick
        {"joystick", vr::k_EButton_Axis3},

        // Generic axis aliases
        {"axis0", vr::k_EButton_Axis0}, // joystick for Oculus
        {"axis1", vr::k_EButton_Axis1}, // top trigger for Oculus
        {"axis2", vr::k_EButton_Axis2}, // bottom trigger for Oculus
        {"axis3", vr::k_EButton_Axis3},
        {"axis4", vr::k_EButton_Axis4},

        // Reserved and others
        {"reserved0", vr::k_EButton_Reserved0},
        {"reserved1", vr::k_EButton_Reserved1},

        // Special cases (back button usually not common on VR controllers)
        {"dashboard_back", vr::k_EButton_Dashboard_Back}
    };
	
};

#endif
