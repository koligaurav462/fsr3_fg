#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include "FFFrameInterpolator.h"

using Microsoft::WRL::ComPtr;

class FFFrameInterpolatorDX final : public FFFrameInterpolator
{
private:
	ID3D12Device *const m_Device;

	// Transient
	FfxCommandList m_ActiveCommandList = {};

	// Letterbox detection readback infrastructure
	ComPtr<ID3D12Resource> m_VerticalStripBuffer;
	ComPtr<ID3D12Resource> m_HorizontalStripBuffer;
	static constexpr uint32_t STRIP_THICKNESS = 8;
	static constexpr uint32_t MAX_DIMENSION = 4320;
	static constexpr float BLACK_THRESHOLD = 0.03f;
	static constexpr uint32_t STABILITY_FRAMES = 3;
	static constexpr uint32_t DETECTION_INTERVAL = 300;
	bool m_ReadbackPending = false;
	uint32_t m_LastFrameWidth = 0;
	uint32_t m_LastFrameHeight = 0;
	uint32_t m_LastFrameBytesPerPixel = 4;
	uint32_t m_LastDetectedTop = 0;
	uint32_t m_LastDetectedBottom = 0;
	uint32_t m_LastDetectedLeft = 0;
	uint32_t m_LastDetectedRight = 0;
	uint32_t m_DetectionStabilityCount = 0;

public:
	FFFrameInterpolatorDX(ID3D12Device *Device, uint32_t OutputWidth, uint32_t OutputHeight, NGXInstanceParameters *NGXParameters);
	FFFrameInterpolatorDX(const FFFrameInterpolatorDX&) = delete;
	FFFrameInterpolatorDX& operator=(const FFFrameInterpolatorDX&) = delete;
	~FFFrameInterpolatorDX();

	FfxErrorCode Dispatch(void *CommandList, NGXInstanceParameters *NGXParameters) override;

private:
	FfxErrorCode InitializeBackendInterface(
		FFInterfaceWrapper *BackendInterface,
		uint32_t MaxContexts,
		NGXInstanceParameters *NGXParameters) override;

	std::array<uint8_t, 8> GetActiveAdapterLUID() const override;
	FfxCommandList GetActiveCommandList() const override;

	void CopyTexture(FfxCommandList CommandList, const FfxResource *Destination, const FfxResource *Source) override;

	void InitLetterboxDetection() override;
	void UpdateLetterboxDetection(const FfxResource *Backbuffer) override;
	void CleanupLetterboxDetection() override;
	void AnalyzeReadbackData(uint32_t frameWidth, uint32_t frameHeight);

	bool LoadTextureFromNGXParameters(
		NGXInstanceParameters *NGXParameters,
		const char *Name,
		FfxResource *OutFfxResource,
		FfxResourceStates State) override;
};
