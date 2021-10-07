#include <fstream>

#include <API/ARK/Ark.h>
#include <Logger/Logger.h>

#include "json.hpp"

#pragma comment(lib, "ArkApi.lib")

nlohmann::json config;
int adult_to_baby_chance;
int adult_max_level;

float baby_min_age;
float baby_max_age;

DECLARE_HOOK(ANPCZoneManager_SpawnNPC, APrimalDinoCharacter*, ANPCZoneManager*, TSubclassOf<APrimalDinoCharacter>, FVector*, bool, int, float, bool, float, float, float, float, TSubclassOf<UPrimalColorSet>);

template <class T>
static bool IsValid(T* object)
{
	return object != nullptr && object->IsValidLowLevel();
}

int GetRandomInt(int min, int max)
{
	const int n = max - min + 1;
	const int remainder = RAND_MAX % n;
	int x;

	do
	{
		x = rand();
	} while (x >= RAND_MAX - remainder);

	return min + x % n;
}

bool GetRandomBool(int prob)
{
	const int n = GetRandomInt(0, 100);
	auto const result = n >= 100 - prob;

	return result;
}

float GetRandomChance(int min, int max)
{
	return static_cast<float>(GetRandomInt(min, max)) / 100.f;
}

APrimalDinoCharacter* Hook_ANPCZoneManager_SpawnNPC(ANPCZoneManager* _this, TSubclassOf<APrimalDinoCharacter> PawnTemplate, FVector* SpawnLoc,
	bool bOverrideNPCLevel, int NPCLevelOffset, float NPCLevelMultiplier, bool bAddLevelOffsetBeforeMultiplier,
	float WaterOnlySpawnMinimumWaterHeight, float SpawnVolumeStartExtentZ, float OverrideYaw, float MaximumWaterHeight, TSubclassOf<UPrimalColorSet> ColorSets)
{
	auto* dino = ANPCZoneManager_SpawnNPC_original(_this, PawnTemplate, SpawnLoc, bOverrideNPCLevel, NPCLevelOffset, NPCLevelMultiplier, bAddLevelOffsetBeforeMultiplier,
		WaterOnlySpawnMinimumWaterHeight, SpawnVolumeStartExtentZ, OverrideYaw, MaximumWaterHeight, ColorSets);



	if (PawnTemplate.uClass && dino) {
		if (dino->AbsoluteBaseLevelField() <= adult_max_level) {
			if (GetRandomBool(adult_to_baby_chance)) {

				//0.2 - juvenile 
				//0.5 - adolescent 
				//1 is full adult
				const auto baby_age = GetRandomChance(baby_min_age, baby_max_age);
				dino->SetBabyAge(baby_age);
			}
		}
	}

	return dino;
}

void ReadConfig()
{
	const std::string config_path = ArkApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/WildBaby/config.json";
	std::ifstream file{ config_path };
	if (!file.is_open())
	{
		throw std::runtime_error("Can't open config.json");
	}

	file >> config;

	adult_to_baby_chance = config.value("AdultToBabyChance", 40);
	adult_max_level = config.value("AdultMaxLevel", 100);

	baby_min_age = config.value("BabyMinAge", 0.1f);
	baby_max_age = config.value("BabyMaxAge", 0.9f);

	file.close();
}

void Load()
{
	Log::Get().Init("WildBaby");

	try
	{
		ReadConfig();
	}
	catch (const std::exception& error)
	{
		Log::GetLog()->error(error.what());
		throw;
	}

	ArkApi::GetHooks().SetHook("ANPCZoneManager.SpawnNPC", &Hook_ANPCZoneManager_SpawnNPC, &ANPCZoneManager_SpawnNPC_original);

}

void Unload()
{
	ArkApi::GetHooks().DisableHook("ANPCZoneManager.SpawnNPC", &Hook_ANPCZoneManager_SpawnNPC);

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