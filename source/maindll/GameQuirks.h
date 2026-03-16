#pragma once

#include <Windows.h>
#include <string>
#include <algorithm>

enum class GameEngine
{
	Unknown = 0,
	UnrealEngine,
	Unity,
	REDengine,
	Frostbite,
};

enum class GameQuirk : uint32_t
{
	None = 0x00,
	DepthInverted = 0x01,
	MotionVectorScaleOverride = 0x02,
	DisablePredilatedMVs = 0x04,
	ForceTAAJitter = 0x08,
	HUDlessBufferFix = 0x10,
};

inline GameQuirk operator|(GameQuirk a, GameQuirk b)
{
	return static_cast<GameQuirk>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline GameQuirk operator&(GameQuirk a, GameQuirk b)
{
	return static_cast<GameQuirk>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline bool HasQuirk(GameQuirk quirks, GameQuirk check)
{
	return (static_cast<uint32_t>(quirks) & static_cast<uint32_t>(check)) != 0;
}

struct GameQuirkEntry
{
	const wchar_t *exePattern;
	GameEngine engine;
	GameQuirk quirks;
	float mvScaleX = 1.0f;
	float mvScaleY = 1.0f;
};

// Game-specific quirk table based on upstream research and community testing
static const GameQuirkEntry g_GameQuirkTable[] = {
	// Unreal Engine — most UE games use reversed depth and have TAA jitter
	{L"*-win64-shipping.exe", GameEngine::UnrealEngine, GameQuirk::DepthInverted | GameQuirk::ForceTAAJitter},
	{L"*-win64-test.exe", GameEngine::UnrealEngine, GameQuirk::DepthInverted | GameQuirk::ForceTAAJitter},

	// Cyberpunk 2077 (REDengine 4)
	{L"cyberpunk2077.exe", GameEngine::REDengine, GameQuirk::HUDlessBufferFix},

	// The Witcher 3 Next-Gen (REDengine 3)
	{L"witcher3.exe", GameEngine::REDengine, GameQuirk::HUDlessBufferFix},

	// Baldur's Gate 3 — community-reported depth/jitter oddities
	{L"bg3.exe", GameEngine::Unknown, GameQuirk::ForceTAAJitter | GameQuirk::DisablePredilatedMVs},
	{L"bg3_dx11.exe", GameEngine::Unknown, GameQuirk::ForceTAAJitter | GameQuirk::DisablePredilatedMVs},
	{L"baldursgate3.exe", GameEngine::Unknown, GameQuirk::ForceTAAJitter | GameQuirk::DisablePredilatedMVs},
};

inline bool WildcardMatch(const std::wstring& pattern, const std::wstring& text)
{
	std::wstring lowerPattern = pattern;
	std::wstring lowerText = text;
	std::transform(lowerPattern.begin(), lowerPattern.end(), lowerPattern.begin(), ::towlower);
	std::transform(lowerText.begin(), lowerText.end(), lowerText.begin(), ::towlower);

	size_t starPos = lowerPattern.find(L'*');
	if (starPos == std::wstring::npos)
		return lowerPattern == lowerText;

	if (starPos == 0)
	{
		std::wstring suffix = lowerPattern.substr(1);
		return lowerText.length() >= suffix.length() &&
			   lowerText.compare(lowerText.length() - suffix.length(), suffix.length(), suffix) == 0;
	}

	if (starPos == lowerPattern.length() - 1)
	{
		std::wstring prefix = lowerPattern.substr(0, starPos);
		return lowerText.length() >= prefix.length() &&
			   lowerText.compare(0, prefix.length(), prefix) == 0;
	}

	std::wstring prefix = lowerPattern.substr(0, starPos);
	std::wstring suffix = lowerPattern.substr(starPos + 1);
	return lowerText.length() >= prefix.length() + suffix.length() &&
		   lowerText.compare(0, prefix.length(), prefix) == 0 &&
		   lowerText.compare(lowerText.length() - suffix.length(), suffix.length(), suffix) == 0;
}

inline void DetectGameEngineQuirks(const std::wstring& exeName, GameEngine& outEngine, GameQuirk& outQuirks, float& mvScaleX, float& mvScaleY)
{
	outEngine = GameEngine::Unknown;
	outQuirks = GameQuirk::None;
	mvScaleX = 1.0f;
	mvScaleY = 1.0f;

	for (const auto& entry : g_GameQuirkTable)
	{
		if (WildcardMatch(entry.exePattern, exeName))
		{
			outEngine = entry.engine;
			outQuirks = entry.quirks;
			mvScaleX = entry.mvScaleX;
			mvScaleY = entry.mvScaleY;
			return;
		}
	}
}

inline void DetectGameEngineByDLLs(GameEngine& ioEngine, GameQuirk& ioQuirks)
{
	if (ioEngine != GameEngine::Unknown)
		return;

	if (GetModuleHandleW(L"UnityPlayer.dll") != nullptr)
	{
		ioEngine = GameEngine::Unity;
		ioQuirks = ioQuirks | GameQuirk::DisablePredilatedMVs;
	}
}

inline const char *GetEngineName(GameEngine engine)
{
	switch (engine)
	{
	case GameEngine::UnrealEngine: return "Unreal Engine";
	case GameEngine::Unity: return "Unity";
	case GameEngine::REDengine: return "REDengine";
	case GameEngine::Frostbite: return "Frostbite";
	default: return "Unknown";
	}
}
