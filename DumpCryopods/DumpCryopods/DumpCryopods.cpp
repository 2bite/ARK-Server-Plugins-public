#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include <API/ARK/Ark.h>

#pragma comment(lib, "ArkApi.lib")

#define DEBUG          true
#define DEBUG_INFO     if(DEBUG) Log::GetLog()->info
#define DEBUG_WARN     if(DEBUG) Log::GetLog()->warn
#define DEBUG_ERROR    if(DEBUG) Log::GetLog()->error

template <class T>
static bool IsValid(T* object)
{
	return object != nullptr && object->IsValidLowLevelFast(false);
}

void CreateDir(std::string folder)
{
	if (CreateDirectoryA(folder.c_str(), NULL) ||
		ERROR_ALREADY_EXISTS == GetLastError())
	{
		// CopyFile(...)
	}
	else
	{
		// Failed to create directory.
	}
}

bool WriteBinary(std::string const& filename, const char* data, size_t const bytes)
{
	std::ofstream b_stream(filename.c_str(),
		std::fstream::out | std::fstream::binary);
	if (b_stream)
	{
		b_stream.write(data, bytes);
		return (b_stream.good());
	}
	return false;
}

std::vector<unsigned char> ReadBinary(std::string const& filename)
{
	std::ifstream file(filename, std::ios::binary);

	file.seekg(0, std::ios::end);
	const std::streampos file_size = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<unsigned char> file_data(file_size);
	file.read(reinterpret_cast<char*>(&file_data[0]), file_size);
	return file_data;
}

uint64 CombinedNumber(unsigned int ItemID1, unsigned int ItemID2)
{
	return ItemID2 | (static_cast<uint64>(ItemID1) << 32);
}

void DumpCryopods(AShooterPlayerController* player_controller, const FString& cmd)
{
	static const std::string dump_root = ArkApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/DumpCryopods/dump/";
	CreateDir(dump_root); // create folder for dump

	static const FName cryopod_with_dino_fname("Dino", EFindName::FNAME_Add);

	TArray<UObject*> inventory_objects;
	Globals::GetObjectsOfClass(UPrimalInventoryComponent::GetPrivateStaticClass(L""), &inventory_objects, true, EObjectFlags::RF_NoFlags);
	DEBUG_INFO("({}) inventories: {}", __FUNCTION__, inventory_objects.Num());

	for (auto* inventory_object : inventory_objects) {
		auto* inventory = static_cast<UPrimalInventoryComponent*>(inventory_object);
		if (IsValid(inventory) && inventory->GetCurrentNumInventoryItems() > 0) {

			auto items = inventory->InventoryItemsField();
			for (auto* item : items) {

				if (IsValid(item)) {

					if (item->HasCustomItemData(cryopod_with_dino_fname)) {

						// we have cryopod with dino data
						TArray<unsigned char> item_bytes;
						item->GetItemBytes(&item_bytes);


						// save bytes to file
						if (item_bytes.Num() > 0) {

							std::vector<unsigned char> custom_data_bytes;
							for (auto item_byte : item_bytes) {
								custom_data_bytes.push_back(item_byte);
							}

							// create full path
							const auto cryopod_id = CombinedNumber(item->ItemIDField().ItemID1, item->ItemIDField().ItemID2);
							std::string dump_path = dump_root + std::to_string(cryopod_id) + ".bin";

							DEBUG_INFO("({}) cryopod dump path: {}", __FUNCTION__, dump_path);

							auto* custom_data_bytes_ptr = &custom_data_bytes[0];

							// save to file
							WriteBinary(dump_path, (char*)(custom_data_bytes_ptr), custom_data_bytes.size());
						}
					}
				}
			}
		}
	}

	DEBUG_INFO("({}) dump completed!", __FUNCTION__);
}

void GetCryopodsFromDump(AShooterPlayerController* player_controller, const FString& cmd)
{
	static const std::string dump_root = ArkApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/DumpCryopods/dump/";

	// create directories path
	std::vector<std::string> files;
	for (const auto& entry : std::filesystem::directory_iterator(dump_root)) {
		auto entry_path = entry.path().string();
		files.push_back(entry_path);
		DEBUG_INFO("({}) entry_path: {}", __FUNCTION__, entry_path);
	}

	// loop files
	for (auto& file : files) {

		// copy data from file to array
		auto file_bytes = ReadBinary(file);
		TArray<unsigned char> item_bytes;
		for (auto file_byte : file_bytes) {
			item_bytes.Add(file_byte);
		}
		DEBUG_INFO("({}) loaded {} bytes from file dump!", __FUNCTION__, item_bytes.Num());

		// create item from dump
		auto* item = UPrimalItem::CreateFromBytes(&item_bytes);

		// add item to player inventory
		if (player_controller->GetPlayerInventoryComponent()) {
			UPrimalItem* item2 = player_controller->GetPlayerInventoryComponent()->AddItemObject(item);

			if (item2) {
				DEBUG_INFO("({}) item added to inventory!", __FUNCTION__);
			}
		}
	}
}

void GetCryopodsFromDumpCmd(APlayerController* player_controller, FString* cmd, bool /*unused*/)
{
	auto* shooter_controller = static_cast<AShooterPlayerController*>(player_controller);
	GetCryopodsFromDump(shooter_controller, *cmd);
}

void DumpCryopodsCmd(APlayerController* player_controller, FString* cmd, bool /*unused*/)
{
	auto* shooter_controller = static_cast<AShooterPlayerController*>(player_controller);
	DumpCryopods(shooter_controller, *cmd);
}

void Load()
{
	Log::Get().Init("DumpCryopods");

	ArkApi::GetCommands().AddConsoleCommand("DumpCryopods", &DumpCryopodsCmd);
	ArkApi::GetCommands().AddConsoleCommand("GetCryopodsFromDump", &GetCryopodsFromDumpCmd);
}

void Unload()
{
	ArkApi::GetCommands().RemoveConsoleCommand("DumpCryopods");
	ArkApi::GetCommands().RemoveConsoleCommand("GetCryopodsFromDump");
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