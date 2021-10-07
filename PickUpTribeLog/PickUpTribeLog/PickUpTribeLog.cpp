#include <fstream>
#include <API/ARK/Ark.h>

#include "json.hpp"

#pragma comment(lib, "ArkApi.lib")

bool enable_debug;
#define DEBUG          enable_debug
#define DEBUG_INFO     if(DEBUG) Log::GetLog()->info
#define DEBUG_WARN     if(DEBUG) Log::GetLog()->warn
#define DEBUG_ERROR    if(DEBUG) Log::GetLog()->error

DECLARE_HOOK(APrimalStructure_PickupStructure, UPrimalItem*, APrimalStructure*, bool, AShooterPlayerController*);

nlohmann::json config;
std::string log_format;

void AddToTribeLog(AShooterPlayerController* player_controller, FString* log)
{
	auto* player_state = player_controller->GetShooterPlayerState();
	if (player_state) {
		const int tribe_id = player_state->GetTribeId();

		auto* game_mode = ArkApi::GetApiUtils().GetShooterGameMode();
		if (game_mode) {
			game_mode->AddToTribeLog(tribe_id, log);
		}
	}
}

UPrimalItem* Hook_APrimalStructure_PickupStructure(APrimalStructure* _this, bool bIsQuickPickup, AShooterPlayerController* pc)
{

	auto* result = APrimalStructure_PickupStructure_original(_this, bIsQuickPickup, pc);
	if (result)	{

		FString structure_name;
		_this->GetDescriptiveName(&structure_name);

		const auto character_name = ArkApi::IApiUtils::GetCharacterName(pc);

		auto new_log_line = FString(fmt::format(log_format, character_name.ToString(), structure_name.ToString()));
		AddToTribeLog(pc, &new_log_line);

		DEBUG_INFO("{} has been picked up by {}!", structure_name.ToString(), character_name.ToString());
	}
	
	return result;
}

void ReadConfig()
{
	const std::string config_path = ArkApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/PickUpTribeLog/config.json";
	std::ifstream file{ config_path };
	if (!file.is_open())
	{
		throw std::runtime_error("Can't open config.json");
	}

	file >> config;

	enable_debug = config.value("Debug", true);
	log_format = config.value("LogFormat", "{} picked up a '{}'");

	file.close();
}

void Load()
{
	Log::Get().Init("PickUpTribeLog");

	try
	{
		ReadConfig();
	}
	catch (const std::exception& error)
	{
		Log::GetLog()->error(error.what());
		throw;
	}
	
	ArkApi::GetHooks().SetHook("APrimalStructure.PickupStructure", &Hook_APrimalStructure_PickupStructure, &APrimalStructure_PickupStructure_original);
}

void Unload()
{
	ArkApi::GetHooks().DisableHook("APrimalStructure.PickupStructure", &Hook_APrimalStructure_PickupStructure);
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD ul_reason_for_call, LPVOID /*lpReserved*/)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		Load();
		break;

	case DLL_PROCESS_DETACH:
		Unload();
		break;
	}
	return TRUE;
}