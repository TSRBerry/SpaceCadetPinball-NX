#include "pch.h"
#include "options.h"

#include "fullscrn.h"
#include "midi.h"
#include "pb.h"
#include "render.h"
#include "Sound.h"
#include "winmain.h"
#include "translations.h"

constexpr int options::MaxUps, options::MaxFps, options::MinUps, options::MinFps, options::DefUps, options::DefFps;
constexpr int options::MaxSoundChannels, options::MinSoundChannels, options::DefSoundChannels;
constexpr int options::MaxVolume, options::MinVolume, options::DefVolume;

std::unordered_map<std::string, std::string> options::settings{};
bool options::ShowDialog = false;
GameInput* options::ControlWaitingForInput = nullptr;
std::vector<OptionBase*> options::AllOptions{};

optionsStruct options::Options
{
	{
		{
			"Left Flipper key",
			{InputTypes::Keyboard, SDLK_z},
			{InputTypes::Mouse, SDL_BUTTON_LEFT},
#ifndef __SWITCH__
                    {InputTypes::GameController, SDL_CONTROLLER_BUTTON_LEFTSHOULDER},
#else
            {InputTypes::GameController, 8}, // HidNpadButton_ZL
#endif
		},
		{
			"Right Flipper key",
			{InputTypes::Keyboard, SDLK_SLASH},
			{InputTypes::Mouse,SDL_BUTTON_RIGHT},
#ifndef __SWITCH__
                    {InputTypes::GameController, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER},
#else
            {InputTypes::GameController, 9}, // HidNpadButton_ZR
#endif
		},
		{
			"Plunger key",
			{InputTypes::Keyboard, SDLK_SPACE},
			{InputTypes::Mouse,SDL_BUTTON_MIDDLE},
#ifndef __SWITCH__
                    {InputTypes::GameController, SDL_CONTROLLER_BUTTON_A},
#else
            {InputTypes::GameController, 0}, // HidNpadButton_A
#endif
		},
		{
			"Left Table Bump key",
			{InputTypes::Keyboard, SDLK_x},
			{InputTypes::Mouse,SDL_BUTTON_X1},
#ifndef __SWITCH__
                    {InputTypes::GameController, SDL_CONTROLLER_BUTTON_DPAD_LEFT},
#else
            {InputTypes::GameController, 12}, // HidNpadButton_Left
#endif
		},
		{
			"Right Table Bump key",
			{InputTypes::Keyboard, SDLK_PERIOD},
			{InputTypes::Mouse,SDL_BUTTON_X2},
#ifndef __SWITCH__
                    {InputTypes::GameController, SDL_CONTROLLER_BUTTON_DPAD_RIGHT},
#else
            {InputTypes::GameController, 14}, // HidNpadButton_Right
#endif
		},
		{
			"Bottom Table Bump key",
			{InputTypes::Keyboard, SDLK_UP},
			{InputTypes::Mouse,SDL_BUTTON_X2 + 1},
#ifndef __SWITCH__
                    {InputTypes::GameController, SDL_CONTROLLER_BUTTON_DPAD_UP},
#else
            {InputTypes::GameController, 15}, // HidNpadButton_Down
#endif
		},
	},
	{"Sounds", true},
	{"Music", false},
	{"FullScreen", false},
	{"Players", 1},
	{"Screen Resolution", -1 },
	{"UI Scale", 1.0f},
	{"Uniform scaling", true},
	{"Linear Filtering", true },
	{"Frames Per Second", DefFps },
	{"Updates Per Second", DefUps },
	{"ShowMenu", true },
	{"Uncapped Updates Per Second", false },
	{"Sound Channels", DefSoundChannels },
	{"HybridSleep", false },
	{"Prefer 3DPB Game Data", false },
	{"Integer Scaling", false },
	{"Sound Volume", DefVolume },
	{"Music Volume", DefVolume },
	{"Stereo Sound Effects", false },
	{"Debug Overlay", false },
	{"Debug Overlay Grid", true },
	{"Debug Overlay All Edges", true },
	{"Debug Overlay Ball Position", true },
	{"Debug Overlay Ball Edges", true },
	{"Debug Overlay Collision Mask", true },
	{"Debug Overlay Sprites", true },
	{"Debug Overlay Sounds", true },
	{"Debug Overlay Ball Depth Grid", true },
	{"Debug Overlay AABB", true },
	{"FontFileName", "" },
	{"Language",  translations::GetCurrentLanguage()->ShortName },
};

void options::InitPrimary()
{
	auto imContext = ImGui::GetCurrentContext();
	ImGuiSettingsHandler ini_handler;
	ini_handler.TypeName = "Pinball";
	ini_handler.TypeHash = ImHashStr(ini_handler.TypeName);
	ini_handler.ReadOpenFn = MyUserData_ReadOpen;
	ini_handler.ReadLineFn = MyUserData_ReadLine;
	ini_handler.WriteAllFn = MyUserData_WriteAll;
	imContext->SettingsHandlers.push_back(ini_handler);

	// Settings are loaded from disk on the first frame
	if (!imContext->SettingsLoaded)
	{
		ImGui::LoadIniSettingsFromDisk(imContext->IO.IniFilename);
		imContext->SettingsLoaded = true;
	}

	for(const auto opt : AllOptions)
	{
		opt->Load();
	}

	winmain::ImIO->FontGlobalScale = Options.UIScale;
	Options.FramesPerSecond = Clamp(Options.FramesPerSecond.V, MinFps, MaxFps);
	Options.UpdatesPerSecond = Clamp(Options.UpdatesPerSecond.V, MinUps, MaxUps);
	Options.UpdatesPerSecond = std::max(Options.UpdatesPerSecond.V, Options.FramesPerSecond.V);
	Options.SoundChannels = Clamp(Options.SoundChannels.V, MinSoundChannels, MaxSoundChannels);
	Options.SoundVolume = Clamp(Options.SoundVolume.V, MinVolume, MaxVolume);
	Options.MusicVolume = Clamp(Options.MusicVolume.V, MinVolume, MaxVolume);
	translations::SetCurrentLanguage(Options.Language.V.c_str());
}

void options::InitSecondary()
{
	winmain::UpdateFrameRate();

	auto maxRes = fullscrn::GetMaxResolution();
	if (Options.Resolution >= 0 && Options.Resolution > maxRes)
		Options.Resolution = maxRes;
	fullscrn::SetResolution(Options.Resolution == -1 ? maxRes : Options.Resolution);
}

void options::uninit()
{
	Options.Language.V = translations::GetCurrentLanguage()->ShortName;
	for (const auto opt : AllOptions)
	{
		opt->Save();
	}
}


int options::get_int(LPCSTR lpValueName, int defaultValue)
{
	auto value = GetSetting(lpValueName, std::to_string(defaultValue));
	return std::stoi(value);
}

void options::set_int(LPCSTR lpValueName, int data)
{
	SetSetting(lpValueName, std::to_string(data));
}

float options::get_float(LPCSTR lpValueName, float defaultValue)
{
	auto value = GetSetting(lpValueName, std::to_string(defaultValue));
	return std::stof(value);
}

void options::set_float(LPCSTR lpValueName, float data)
{
	SetSetting(lpValueName, std::to_string(data));
}

void options::GetInput(const std::string& rowName, GameInput (&values)[3])
{
	for (auto i = 0u; i <= 2; i++)
	{
		auto name = rowName + " " + std::to_string(i);
		auto inputType = static_cast<InputTypes>(get_int((name + " type").c_str(), -1));
		auto input = get_int((name + " input").c_str(), -1);
		if (inputType <= InputTypes::GameController && input != -1)
			values[i] = {inputType, input};
	}
}

void options::SetInput(const std::string& rowName, GameInput (&values)[3])
{
	for (auto i = 0u; i <= 2; i++)
	{
		auto input = values[i];
		auto name = rowName + " " + std::to_string(i);
		set_int((name + " type").c_str(), static_cast<int>(input.Type));
		set_int((name + " input").c_str(), input.Value);
	}
}

void options::toggle(Menu1 uIDCheckItem)
{
	switch (uIDCheckItem)
	{
	case Menu1::Sounds:
		Options.Sounds ^= true;
		Sound::Enable(Options.Sounds);
		return;
	case Menu1::SoundStereo:
		Options.SoundStereo ^= true;
		return;
	case Menu1::Music:
		Options.Music ^= true;
		if (!Options.Music)
			midi::music_stop();
		else
			midi::music_play();
		return;
	case Menu1::Show_Menu:
		Options.ShowMenu ^= true;
		fullscrn::window_size_changed();
		return;
	case Menu1::Full_Screen:
		Options.FullScreen ^= true;
		fullscrn::set_screen_mode(Options.FullScreen);
		return;
	case Menu1::OnePlayer:
	case Menu1::TwoPlayers:
	case Menu1::ThreePlayers:
	case Menu1::FourPlayers:
		Options.Players = static_cast<int>(uIDCheckItem) - static_cast<int>(Menu1::OnePlayer) + 1;
		break;
	case Menu1::MaximumResolution:
	case Menu1::R640x480:
	case Menu1::R800x600:
	case Menu1::R1024x768:
		{
			auto restart = false;
			int newResolution = static_cast<int>(uIDCheckItem) - static_cast<int>(Menu1::R640x480);
			if (uIDCheckItem == Menu1::MaximumResolution)
			{
				restart = fullscrn::GetResolution() != fullscrn::GetMaxResolution();
				Options.Resolution = -1;
			}
			else if (newResolution <= fullscrn::GetMaxResolution())
			{
				restart = newResolution != (Options.Resolution == -1
					                            ? fullscrn::GetMaxResolution()
					                            : fullscrn::GetResolution());
				Options.Resolution = newResolution;
			}

			if (restart)
				winmain::Restart();
			break;
		}
	case Menu1::WindowUniformScale:
		Options.UniformScaling ^= true;
		fullscrn::window_size_changed();
		break;
	case Menu1::WindowLinearFilter:
		Options.LinearFiltering ^= true;
		render::recreate_screen_texture();
		break;
	case Menu1::Prefer3DPBGameData:
		Options.Prefer3DPBGameData ^= true;
		winmain::Restart();
		break;
	case Menu1::WindowIntegerScale:
		Options.IntegerScaling ^= true;
		fullscrn::window_size_changed();
		break;
	default:
		break;
	}
}

void options::InputDown(GameInput input)
{
	if (ControlWaitingForInput)
	{
		// Skip function keys, just in case.
		if (input.Type == InputTypes::Keyboard && input.Value >= SDLK_F1 && input.Value <= SDLK_F12)
			return;

		// Start is reserved for pause
		if (input.Type == InputTypes::GameController && input.Value == SDL_CONTROLLER_BUTTON_START)
			return;

		*ControlWaitingForInput = input;
		ControlWaitingForInput = nullptr;
	}
}

void options::ShowControlDialog()
{
	if (!ShowDialog)
	{
		ControlWaitingForInput = nullptr;
		ShowDialog = true;
		// Save previous controls in KVP storage.
		for(const auto& control: Options.Key)
		{
			control.Save();
		}
	}
}

void options::RenderControlDialog()
{
	static const Msg controlDescriptions[6]
	{
		Msg::KEYMAPPER_FlipperL,
		Msg::KEYMAPPER_FlipperR,
		Msg::KEYMAPPER_BumpLeft,
		Msg::KEYMAPPER_BumpRight,
		Msg::KEYMAPPER_BumpBottom,
		Msg::KEYMAPPER_Plunger,
	};

	if (!ShowDialog)
		return;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2{550, 550});
	if (ImGui::Begin(pb::get_rc_string(Msg::KEYMAPPER_Caption), &ShowDialog))
	{
		ImGui::TextUnformatted(pb::get_rc_string(Msg::KEYMAPPER_Groupbox2));
		ImGui::Separator();

		ImGui::TextWrapped("%s", pb::get_rc_string(Msg::KEYMAPPER_Help1));
		ImGui::TextWrapped("%s", pb::get_rc_string(Msg::KEYMAPPER_Help2));
		ImGui::Spacing();

		ImGui::TextUnformatted(pb::get_rc_string(Msg::KEYMAPPER_Groupbox1));

		ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2{5, 10});
		if (ImGui::BeginTable("Controls", 4, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders| ImGuiTableFlags_SizingStretchSame))
		{
			ImGui::TableSetupColumn("Control");
			ImGui::TableSetupColumn("Binding 1");
			ImGui::TableSetupColumn("Binding 2");
			ImGui::TableSetupColumn("Binding 3");
			ImGui::TableHeadersRow();

			int rowHash = 0;
			for (auto inputId = GameBindings::Min; inputId < GameBindings::Max; inputId++)
			{
				auto& option = Options.Key[~inputId];

				ImGui::TableNextColumn();
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.5, 0, 0, 1});
				if (ImGui::Button(pb::get_rc_string(controlDescriptions[~inputId])))
				{
					for (auto& input : option.Inputs)
						input = {};
				}
				ImGui::PopStyleColor(1);

				for (auto& input : option.Inputs)
				{
					ImGui::TableNextColumn();
					if (ControlWaitingForInput == &input)
					{
						if(ImGui::Button("Press the key", ImVec2(-1, 0)))
						{
							ControlWaitingForInput = &input;
						}
					}
					else
					{
						auto inputDescription = input.GetInputDescription();
						if (ImGui::Button((inputDescription + "##" + std::to_string(rowHash++)).c_str(),
						                  ImVec2(-1, 0)))
						{
							ControlWaitingForInput = &input;
						}
					}
				}
			}
			ImGui::EndTable();
		}
		ImGui::PopStyleVar();
		ImGui::Spacing();

		if (ImGui::Button(pb::get_rc_string(Msg::KEYMAPPER_Ok)))
		{
			ShowDialog = false;
		}

		ImGui::SameLine();
		if (ImGui::Button(pb::get_rc_string(Msg::KEYMAPPER_Cancel)))
		{
			for (auto& control : Options.Key)
			{
				control.Load();
			}
			ShowDialog = false;
		}

		ImGui::SameLine();
		if (ImGui::Button(pb::get_rc_string(Msg::KEYMAPPER_Default)))
		{
			for (auto& control : Options.Key)
			{
				control.Reset();
			}
			ControlWaitingForInput = nullptr;
		}
	}
	ImGui::End();
	ImGui::PopStyleVar();

	if (!ShowDialog)
		ControlWaitingForInput = nullptr;
}

std::vector<GameBindings> options::MapGameInput(GameInput key)
{
	std::vector<GameBindings> result;
	for (auto inputId = GameBindings::Min; inputId < GameBindings::Max; inputId++)
	{
		for (auto& inputValue : Options.Key[~inputId].Inputs)
		{
			if (key == inputValue)
			{
				result.push_back(inputId);
				break;
			}
		}
	}
	return result;
}

void options::MyUserData_ReadLine(ImGuiContext* ctx, ImGuiSettingsHandler* handler, void* entry, const char* line)
{
	auto& keyValueStore = *static_cast<std::unordered_map<std::string, std::string>*>(entry);
	std::string keyValue = line;
	auto separatorPos = keyValue.find('=');
	if (separatorPos != std::string::npos)
	{
		auto key = keyValue.substr(0, separatorPos);
		auto value = keyValue.substr(separatorPos + 1, keyValue.size());
		keyValueStore[key] = value;
	}
}

void* options::MyUserData_ReadOpen(ImGuiContext* ctx, ImGuiSettingsHandler* handler, const char* name)
{
	// There is only one custom entry
	return strcmp(name, "Settings") == 0 ? &settings : nullptr;
}

void options::MyUserData_WriteAll(ImGuiContext* ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf)
{
	buf->appendf("[%s][%s]\n", handler->TypeName, "Settings");
	for (const auto& setting : settings)
	{
		buf->appendf("%s=%s\n", setting.first.c_str(), setting.second.c_str());
	}
	buf->append("\n");
}

std::string GameInput::GetInputDescription() const
{
	static LPCSTR mouseButtons[]
	{
		nullptr,
		"Left",
		"Middle",
		"Right",
		"X1",
		"X2",
	};

	static LPCSTR controllerButtons[] =
	{
		"A",
		"B",
		"X",
		"Y",
		"Back",
		"Guide",
		"Start",
		"LeftStick",
		"RightStick",
		"LeftShoulder",
		"RightShoulder",
		"DpUp",
		"DpDown",
		"DpLeft",
		"DpRight",
		"Misc1",
		"Paddle1",
		"Paddle2",
		"Paddle3",
		"Paddle4",
		"Touchpad",
	};

	std::string keyName;
	switch (Type)
	{
	case InputTypes::Keyboard:
		keyName = "Keyboard\n";
		keyName += SDL_GetKeyName(Value);
		break;
	case InputTypes::Mouse:
		keyName = "Mouse\n";
		if (Value >= SDL_BUTTON_LEFT && Value <= SDL_BUTTON_X2)
			keyName += mouseButtons[Value];
		else
			keyName += std::to_string(Value);
		break;
	case InputTypes::GameController:
		keyName = "Controller\n";
		if (Value >= SDL_CONTROLLER_BUTTON_A && Value <= SDL_CONTROLLER_BUTTON_TOUCHPAD)
			keyName += controllerButtons[Value];
		else
			keyName += std::to_string(Value);
		break;
	case InputTypes::None:
	default:
		keyName = "Unused";
	}

	return keyName;
}

const std::string& options::GetSetting(const std::string& key, const std::string& defaultValue)
{
	auto setting = settings.find(key);
	if (setting == settings.end())
	{
		settings[key] = defaultValue;
		if (ImGui::GetCurrentContext())
			ImGui::MarkIniSettingsDirty();
		return defaultValue;
	}
	return setting->second;
}

void options::SetSetting(const std::string& key, const std::string& value)
{
	settings[key] = value;
	if (ImGui::GetCurrentContext())
		ImGui::MarkIniSettingsDirty();
}

OptionBase::OptionBase(LPCSTR name): Name(name)
{
	options::AllOptions.push_back(this);
}

OptionBase::~OptionBase()
{
	auto& vec = options::AllOptions;
	auto position = std::find(vec.begin(), vec.end(), this);
	if (position != vec.end())
		vec.erase(position);
}
