#pragma once

namespace Util
{
	void InitializeLog();
	bool GetSetting(const wchar_t *Key, bool DefaultValue);
	bool GetSetting(const wchar_t *Section, const wchar_t *Key, bool DefaultValue);
	int GetSetting(const wchar_t *Key, int DefaultValue);
	int GetSetting(const wchar_t *Section, const wchar_t *Key, int DefaultValue);
	float GetSetting(const wchar_t *Key, float DefaultValue);
}
