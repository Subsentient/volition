#include "../libvolition/include/common.h"
#include "../libvolition/include/utils.h"
#include <string.h>

//Globals
static const char TumorFilenameStart[] = "VLLTUMOR_FN_DELIM";
static const char TumorFilenameEnd[] = "VLLTUMOR_FN_EDELIM";

//Prototypes
static bool StripTumor(const VLString &BinaryPath);
static bool WriteTumor(const VLString &Filename, const VLString &BinaryPath, const VLString &TumorPath);
static bool ExtractTumor(const VLString &BinaryPath, const VLString &TumorOutDir);
static uint8_t *GetTumorStart(std::vector<uint8_t> *const Binary);
static uint8_t *GetTumorData(std::vector<uint8_t> *const Binary);
static VLString GetTumorFilename(std::vector<uint8_t> *const Binary);

//Definitions

static VLString GetTumorFilename(std::vector<uint8_t> *const Binary)
{
	uint8_t *Start = GetTumorStart(Binary);
	uint8_t *End = (uint8_t*)strstr((const char*)Start, TumorFilenameEnd);
	
	if (!Start || !End)
	{
		VLERROR("No valid tumor filename detected");
		return false;
	}
	
	VLString Buffer((End - Start) + 1);
	
	memcpy(Buffer.GetBuffer(), (const char*)Start + (sizeof TumorFilenameStart - 1), End - Start - (sizeof TumorFilenameStart - 1));
	
	return Buffer;
}

	
	
static bool ExtractTumor(const VLString &BinaryPath, const VLString &TumorOutDir)
{
	VLScopedPtr<std::vector<uint8_t>* > Binary { Utils::Slurp(BinaryPath, true) };
	
	if (!Binary)
	{
		VLERROR("Unable to load binary " + BinaryPath);
		return false;
	}
	
	const uint8_t *const TumorData = GetTumorData(Binary);
	
	if (!TumorData)
	{
		VLERROR("No tumor present in binary file " + BinaryPath);
		return false;
	}
	
	if (!Utils::WriteFile(TumorOutDir + PATH_DIVIDER_STRING + GetTumorFilename(Binary), TumorData, (((Binary->data() + Binary->size()) - 1) - TumorData - 1)))
	{
		VLERROR("Error extracting tumor to " + TumorOutDir);
		return false;
	}
	
	std::cout << "Extract success" << std::endl;

	return true;
}
	
static uint8_t *GetTumorData(std::vector<uint8_t> *const Binary)
{
	const char *Search = (char*)Binary->data();
	const char *const Stopper = (const char*)Binary->data() + Binary->size();
	
	for (; Search != Stopper; ++Search)
	{
		if (!memcmp(Search, TumorFilenameEnd, sizeof TumorFilenameEnd - 1))
		{
			return (uint8_t*)Search + (sizeof TumorFilenameEnd - 1);
		}
	}
	
	return nullptr;
}

static uint8_t *GetTumorStart(std::vector<uint8_t> *const Binary)
{
	const char *Search = (char*)Binary->data();
	const char *const Stopper = (const char*)Binary->data() + Binary->size();
	
	for (; Search != Stopper; ++Search)
	{
		if (!memcmp(Search, TumorFilenameStart, sizeof TumorFilenameStart - 1))
		{
			return (uint8_t*)Search;
		}
	}
	
	return nullptr;
}


static bool WriteTumor(const VLString &Filename, const VLString &BinaryPath, const VLString &TumorPath)
{	
	VLScopedPtr<std::vector<uint8_t>* > Binary { Utils::Slurp(BinaryPath, true) };

	if (!Binary)
	{
		VLERROR("Failed to slurp binary " + BinaryPath + " for writing!");
		return false;
	}
	
	const uint64_t StartOffset = Binary->size();
	
	VLScopedPtr<std::vector<uint8_t>* > NewTumor { Utils::Slurp(TumorPath, true) };

	if (!NewTumor)
	{
		VLERROR("Failed to load new tumor file at path " + TumorPath);
		return false;
	}
	
	if (GetTumorStart(Binary))
	{
		std::cout << +(VLString("Erasing existing tumor from ") + BinaryPath) << std::endl;
		if (!StripTumor(BinaryPath))
		{
			VLERROR("Failed to erase existing tumor from " + BinaryPath);
			return false;
		}
		
		return WriteTumor(Filename, BinaryPath, TumorPath);
	}
	
	Binary->resize(Binary->size() +
					(sizeof TumorFilenameStart - 1) +
					Filename.Length() +
					(sizeof TumorFilenameEnd - 1) +
					NewTumor->size());
	
	uint8_t *Worker = Binary->data() + StartOffset;
	
	memcpy(Worker, TumorFilenameStart, sizeof TumorFilenameStart - 1);
	Worker += sizeof TumorFilenameStart - 1;
	
	memcpy(Worker, +Filename, Filename.Length());
	Worker += Filename.Length();
	
	memcpy(Worker, TumorFilenameEnd, sizeof TumorFilenameEnd - 1);
	Worker += sizeof TumorFilenameEnd - 1;
	
	memcpy(Worker, NewTumor->data(), NewTumor->size());
	
	if (!Utils::WriteFile(BinaryPath, Binary->data(), Binary->size()))
	{
		VLERROR("Failed to write tumored binary back to path " + BinaryPath);
		return false;
	}
	
	std::cout << "Write success" << std::endl;
	
	return true;
}

static bool StripTumor(const VLString &BinaryPath)
{
	VLScopedPtr<std::vector<uint8_t>* > Binary { Utils::Slurp(BinaryPath, true) };
	
	if (!Binary)
	{
		VLERROR("Failed to read binary " + BinaryPath);
		return false;
	}
	
	const uint8_t *const Offset = GetTumorStart(Binary);
	
	if (!Offset)
	{
		VLERROR("No tumor present in binary " + BinaryPath);
		return false;
	}
	
	Binary->resize(Offset - Binary->data());

	if (!Utils::WriteFile(BinaryPath, Binary->data(), Binary->size()))
	{
		VLERROR("Failed to write binary " + BinaryPath + " back to disk!");
		return false;
	}
	
	std::cout << "Strip success" << std::endl;

	return true;
}


int main(const int argc, const char **const argv)
{
	if (argc < 3)
	{
		VLERROR("Incorrect parameters");
		return 1;
	}
	
	const VLString Action { argv[1] };
	
	if (Action == "--strip")
	{
		return !StripTumor(argv[2]);
	}
	else if (Action == "--write")
	{
		if (argc < 4)
		{
			VLERROR("Requires a binary path, and a tumor object path");
			return 1;
		}
		
		
		return !WriteTumor(Utils::StripPathFromFilename(argv[3]), argv[2], argv[3]);
	}
	else if (Action == "--extract")
	{
		if (argc < 4)
		{
			VLERROR("Requires a binary path, and an output directory for the tumor.");
			return 1;
		}
		return !ExtractTumor(argv[2], argv[3]);
	}

	VLERROR("Unrecognized action " + Action);
	return 1;
}

	
	
