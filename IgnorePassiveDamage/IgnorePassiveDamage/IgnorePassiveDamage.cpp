#include <API/ARK/Ark.h>
#include <Logger/Logger.h>

#pragma comment(lib, "ArkApi.lib")

DECLARE_HOOK(APrimalStructure_TakeDamage, float, APrimalStructure*, float, FDamageEvent*, AController*, AActor*);

FString GetObjectFullName(UObject* uobject)
{
	FString full_name = "";

	if (uobject != nullptr) {
		if (uobject->ClassField()
			&& uobject->ClassField()->GetDefaultObject(true)
			&& uobject->ClassField()->GetDefaultObject(true)->GetFullName(&full_name, nullptr)) {

			return full_name;
		}
	}
	return full_name;
}

float Hook_APrimalStructure_TakeDamage(APrimalStructure* Structure, float Damage, FDamageEvent* DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (DamageEvent) {
		auto& damage_type_class = DamageEvent->DamageTypeClassField();
		if (damage_type_class.uClass) {

			auto* damage_type = static_cast<UObject*>(damage_type_class.uClass->ClassDefaultObjectField());
			if (damage_type)			{

				const auto damage_type_full_name = GetObjectFullName(damage_type);

				//DmgType_Passive_C /Game/PrimalEarth/CoreBlueprints/DamageTypes/DmgType_Passive.Default
				//__DmgType_Passive_C
				if (damage_type_full_name.Contains("DmgType_Passive_C")) {
					return 0.f;
				}
			}
		}
	}

	return APrimalStructure_TakeDamage_original(Structure, Damage, DamageEvent, EventInstigator, DamageCauser);
}

void Load()
{
	Log::Get().Init("IgnorePassiveDamage");

	ArkApi::GetHooks().SetHook("APrimalStructure.TakeDamage", &Hook_APrimalStructure_TakeDamage, &APrimalStructure_TakeDamage_original);
}

void Unload()
{
	ArkApi::GetHooks().DisableHook("APrimalStructure.TakeDamage", &Hook_APrimalStructure_TakeDamage);
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