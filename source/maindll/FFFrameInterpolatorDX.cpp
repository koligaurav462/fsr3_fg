#include <FidelityFX/host/backends/dx12/ffx_dx12.h>
#include "NGX/NvNGX.h"
#include "FFFrameInterpolatorDX.h"
#include "Util.h"

D3D12_RESOURCE_STATES ffxGetDX12StateFromResourceState(FfxResourceStates state);

FFFrameInterpolatorDX::FFFrameInterpolatorDX(
	ID3D12Device *Device,
	uint32_t OutputWidth,
	uint32_t OutputHeight,
	NGXInstanceParameters *NGXParameters)
	: m_Device(Device),
	  FFFrameInterpolator(OutputWidth, OutputHeight)
{
	FFFrameInterpolator::Create(NGXParameters);
	InitLetterboxDetection();
	m_Device->AddRef();
}

FFFrameInterpolatorDX::~FFFrameInterpolatorDX()
{
	CleanupLetterboxDetection();
	FFFrameInterpolator::Destroy();
	m_Device->Release();
}

FfxErrorCode FFFrameInterpolatorDX::Dispatch(void *CommandList, NGXInstanceParameters *NGXParameters)
{
	const bool isRecordingCommands = NGXParameters->GetUIntOrDefault("DLSSG.IsRecording", 0) != 0;
	const auto cmdList12 = reinterpret_cast<ID3D12GraphicsCommandList *>(CommandList);

	NGXParameters->Set4("DLSSG.FlushRequired", 0);

	// Begin a new command list in the event our caller didn't set one up
	if (!isRecordingCommands)
	{
		ID3D12CommandQueue *recordingQueue = nullptr;
		NGXParameters->GetVoidPointer("DLSSG.CmdQueue", reinterpret_cast<void **>(&recordingQueue));

		ID3D12CommandAllocator *recordingAllocator = nullptr;
		NGXParameters->GetVoidPointer("DLSSG.CmdAlloc", reinterpret_cast<void **>(&recordingAllocator));

		cmdList12->Reset(recordingAllocator, nullptr);
	}

	m_ActiveCommandList = ffxGetCommandListDX12(cmdList12);
	const auto interpolationResult = FFFrameInterpolator::Dispatch(nullptr, NGXParameters);

	// Finish what we started. Restore the command list to its previous state when necessary.
	if (!isRecordingCommands)
		cmdList12->Close();

	return interpolationResult;
}

FfxErrorCode FFFrameInterpolatorDX::InitializeBackendInterface(
	FFInterfaceWrapper *BackendInterface,
	uint32_t MaxContexts,
	NGXInstanceParameters *NGXParameters)
{
	return BackendInterface->Initialize(m_Device, MaxContexts, NGXParameters);
}

FfxCommandList FFFrameInterpolatorDX::GetActiveCommandList() const
{
	return m_ActiveCommandList;
}

std::array<uint8_t, 8> FFFrameInterpolatorDX::GetActiveAdapterLUID() const
{
	const auto luid = m_Device->GetAdapterLuid();

	std::array<uint8_t, sizeof(luid)> result;
	memcpy(result.data(), &luid, result.size());

	return result;
}

void FFFrameInterpolatorDX::CopyTexture(FfxCommandList CommandList, const FfxResource *Destination, const FfxResource *Source)
{
	const auto cmdList12 = reinterpret_cast<ID3D12GraphicsCommandList *>(CommandList);

	D3D12_RESOURCE_BARRIER barriers[2] = {};
	barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barriers[0].Transition.pResource = static_cast<ID3D12Resource *>(Destination->resource); // Destination
	barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barriers[0].Transition.StateBefore = ffxGetDX12StateFromResourceState(Destination->state);
	barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;

	barriers[1] = barriers[0];
	barriers[1].Transition.pResource = static_cast<ID3D12Resource *>(Source->resource); // Source
	barriers[1].Transition.StateBefore = ffxGetDX12StateFromResourceState(Source->state);
	barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;

	cmdList12->ResourceBarrier(2, barriers);
	cmdList12->CopyResource(barriers[0].Transition.pResource, barriers[1].Transition.pResource);
	std::swap(barriers[0].Transition.StateBefore, barriers[0].Transition.StateAfter);
	std::swap(barriers[1].Transition.StateBefore, barriers[1].Transition.StateAfter);
	cmdList12->ResourceBarrier(2, barriers);
}

bool FFFrameInterpolatorDX::LoadTextureFromNGXParameters(
	NGXInstanceParameters *NGXParameters,
	const char *Name,
	FfxResource *OutFfxResource,
	FfxResourceStates State)
{
	ID3D12Resource *resource = nullptr;
	NGXParameters->GetVoidPointer(Name, reinterpret_cast<void **>(&resource));

	if (!resource)
	{
		*OutFfxResource = {};
		return false;
	}

	*OutFfxResource = ffxGetResourceDX12(resource, ffxGetResourceDescriptionDX12(resource), nullptr, State);
	return true;
}

void FFFrameInterpolatorDX::InitLetterboxDetection()
{
	m_Letterbox.enabled = Util::GetSetting(L"GenerationRect", L"EnableAutoLetterboxDetection", false);

	if (!m_Letterbox.enabled)
		return;

	D3D12_HEAP_PROPERTIES readbackHeapProps = {};
	readbackHeapProps.Type = D3D12_HEAP_TYPE_READBACK;

	D3D12_RESOURCE_DESC bufferDesc = {};
	bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferDesc.Height = 1;
	bufferDesc.DepthOrArraySize = 1;
	bufferDesc.MipLevels = 1;
	bufferDesc.SampleDesc.Count = 1;
	bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	// Vertical strip: 8px wide x max height, with D3D12 row pitch alignment (256 bytes)
	const uint32_t verticalBufferSize = 256 * MAX_DIMENSION;
	bufferDesc.Width = verticalBufferSize;

	HRESULT hr = m_Device->CreateCommittedResource(
		&readbackHeapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
		D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_VerticalStripBuffer));

	if (FAILED(hr))
	{
		spdlog::warn("[Letterbox] Failed to create vertical strip buffer, auto-detection disabled");
		m_Letterbox.enabled = false;
		return;
	}

	// Horizontal strip: max width x 8px tall (allocate for max 8 bpp to handle HDR formats)
	const uint32_t horizontalRowPitch = (MAX_DIMENSION * 8 + 255) & ~255;
	bufferDesc.Width = horizontalRowPitch * STRIP_THICKNESS;

	hr = m_Device->CreateCommittedResource(
		&readbackHeapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
		D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_HorizontalStripBuffer));

	if (FAILED(hr))
	{
		spdlog::warn("[Letterbox] Failed to create horizontal strip buffer, auto-detection disabled");
		m_VerticalStripBuffer.Reset();
		m_Letterbox.enabled = false;
		return;
	}

	spdlog::info("[Letterbox] GPU auto-detection enabled (up to {}p)", MAX_DIMENSION);
}

void FFFrameInterpolatorDX::UpdateLetterboxDetection(const FfxResource *Backbuffer)
{
	if (!m_Letterbox.enabled || !m_VerticalStripBuffer || !m_HorizontalStripBuffer ||
		!Backbuffer || !Backbuffer->resource)
		return;

	// Analyze previous frame's readback data (async - no GPU stall)
	if (m_ReadbackPending && m_LastFrameWidth > 0 && m_LastFrameHeight > 0)
	{
		AnalyzeReadbackData(m_LastFrameWidth, m_LastFrameHeight);
		m_ReadbackPending = false;
	}

	// Only copy strips every DETECTION_INTERVAL frames
	if (m_DispatchCount - m_Letterbox.lastDetectionFrame < DETECTION_INTERVAL)
		return;

	const uint32_t width = Backbuffer->description.width;
	const uint32_t height = Backbuffer->description.height;

	if (width > MAX_DIMENSION || height > MAX_DIMENSION)
		return;

	m_Letterbox.lastDetectionFrame = m_DispatchCount;
	m_LastFrameWidth = width;
	m_LastFrameHeight = height;

	auto cmdList = reinterpret_cast<ID3D12GraphicsCommandList *>(m_ActiveCommandList);
	auto backbufferResource = static_cast<ID3D12Resource *>(Backbuffer->resource);

	if (!cmdList || !backbufferResource)
		return;

	D3D12_RESOURCE_DESC texDesc = backbufferResource->GetDesc();

	uint32_t bytesPerPixel = 4;
	if (texDesc.Format == DXGI_FORMAT_R16G16B16A16_FLOAT)
		bytesPerPixel = 8;
	m_LastFrameBytesPerPixel = bytesPerPixel;

	// Transition backbuffer to copy source
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = backbufferResource;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = ffxGetDX12StateFromResourceState(Backbuffer->state);
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;

	if (barrier.Transition.StateBefore != barrier.Transition.StateAfter)
		cmdList->ResourceBarrier(1, &barrier);

	// Vertical strip: 8px wide from center, full height (for top/bottom bars)
	const uint32_t stripCenterX = (width - STRIP_THICKNESS) / 2;
	const uint32_t verticalRowPitch = (STRIP_THICKNESS * bytesPerPixel + 255) & ~255;

	D3D12_TEXTURE_COPY_LOCATION srcLoc = {};
	srcLoc.pResource = backbufferResource;
	srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

	D3D12_TEXTURE_COPY_LOCATION dstLocV = {};
	dstLocV.pResource = m_VerticalStripBuffer.Get();
	dstLocV.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	dstLocV.PlacedFootprint.Footprint.Format = texDesc.Format;
	dstLocV.PlacedFootprint.Footprint.Width = STRIP_THICKNESS;
	dstLocV.PlacedFootprint.Footprint.Height = height;
	dstLocV.PlacedFootprint.Footprint.Depth = 1;
	dstLocV.PlacedFootprint.Footprint.RowPitch = verticalRowPitch;

	D3D12_BOX srcBoxV = { stripCenterX, 0, 0, stripCenterX + STRIP_THICKNESS, height, 1 };
	cmdList->CopyTextureRegion(&dstLocV, 0, 0, 0, &srcLoc, &srcBoxV);

	// Horizontal strip: full width, 8px tall from center (for left/right bars)
	const uint32_t stripCenterY = (height - STRIP_THICKNESS) / 2;
	const uint32_t horizontalRowPitch = (width * bytesPerPixel + 255) & ~255;

	D3D12_TEXTURE_COPY_LOCATION dstLocH = {};
	dstLocH.pResource = m_HorizontalStripBuffer.Get();
	dstLocH.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	dstLocH.PlacedFootprint.Footprint.Format = texDesc.Format;
	dstLocH.PlacedFootprint.Footprint.Width = width;
	dstLocH.PlacedFootprint.Footprint.Height = STRIP_THICKNESS;
	dstLocH.PlacedFootprint.Footprint.Depth = 1;
	dstLocH.PlacedFootprint.Footprint.RowPitch = horizontalRowPitch;

	D3D12_BOX srcBoxH = { 0, stripCenterY, 0, width, stripCenterY + STRIP_THICKNESS, 1 };
	cmdList->CopyTextureRegion(&dstLocH, 0, 0, 0, &srcLoc, &srcBoxH);

	// Transition backbuffer back
	if (barrier.Transition.StateBefore != barrier.Transition.StateAfter)
	{
		std::swap(barrier.Transition.StateBefore, barrier.Transition.StateAfter);
		cmdList->ResourceBarrier(1, &barrier);
	}

	m_ReadbackPending = true;
}

void FFFrameInterpolatorDX::AnalyzeReadbackData(uint32_t frameWidth, uint32_t frameHeight)
{
	if (!m_VerticalStripBuffer || !m_HorizontalStripBuffer)
		return;

	const uint32_t bpp = m_LastFrameBytesPerPixel;

	auto pixelBrightness = [bpp](const uint8_t *pixel) -> float {
		if (bpp == 8)
		{
			float r = pixel[1] / 255.0f;
			float g = pixel[3] / 255.0f;
			float b = pixel[5] / 255.0f;
			return (r + g + b) / 3.0f;
		}
		return (pixel[0] + pixel[1] + pixel[2]) / (255.0f * 3.0f);
	};

	auto rowBrightness = [&](const uint8_t *rowData, uint32_t count) -> float {
		float total = 0.0f;
		for (uint32_t i = 0; i < count; i++)
			total += pixelBrightness(rowData + i * bpp);
		return total / count;
	};

	// Analyze vertical strip (top/bottom bars)
	uint32_t detectedTop = 0, detectedBottom = 0;
	{
		void *mappedData = nullptr;
		const uint32_t rowPitch = (STRIP_THICKNESS * bpp + 255) & ~255;
		D3D12_RANGE readRange = { 0, static_cast<SIZE_T>(rowPitch) * frameHeight };

		if (SUCCEEDED(m_VerticalStripBuffer->Map(0, &readRange, &mappedData)) && mappedData)
		{
			auto data = static_cast<const uint8_t *>(mappedData);

			for (uint32_t y = 0; y < frameHeight; y++)
			{
				if (rowBrightness(data + y * rowPitch, STRIP_THICKNESS) >= BLACK_THRESHOLD)
				{
					detectedTop = y;
					break;
				}
			}

			for (uint32_t y = frameHeight; y > 0; y--)
			{
				if (rowBrightness(data + (y - 1) * rowPitch, STRIP_THICKNESS) >= BLACK_THRESHOLD)
				{
					detectedBottom = frameHeight - y;
					break;
				}
			}

			D3D12_RANGE writeRange = { 0, 0 };
			m_VerticalStripBuffer->Unmap(0, &writeRange);
		}
	}

	// Analyze horizontal strip (left/right bars)
	uint32_t detectedLeft = 0, detectedRight = 0;
	{
		void *mappedData = nullptr;
		const uint32_t rowPitch = (frameWidth * bpp + 255) & ~255;
		D3D12_RANGE readRange = { 0, static_cast<SIZE_T>(rowPitch) * STRIP_THICKNESS };

		if (SUCCEEDED(m_HorizontalStripBuffer->Map(0, &readRange, &mappedData)) && mappedData)
		{
			auto data = static_cast<const uint8_t *>(mappedData);

			for (uint32_t x = 0; x < frameWidth; x++)
			{
				float total = 0.0f;
				for (uint32_t y = 0; y < STRIP_THICKNESS; y++)
					total += pixelBrightness(data + y * rowPitch + x * bpp);

				if (total / STRIP_THICKNESS >= BLACK_THRESHOLD)
				{
					detectedLeft = x;
					break;
				}
			}

			for (uint32_t x = frameWidth; x > 0; x--)
			{
				float total = 0.0f;
				for (uint32_t y = 0; y < STRIP_THICKNESS; y++)
					total += pixelBrightness(data + y * rowPitch + (x - 1) * bpp);

				if (total / STRIP_THICKNESS >= BLACK_THRESHOLD)
				{
					detectedRight = frameWidth - x;
					break;
				}
			}

			D3D12_RANGE writeRange = { 0, 0 };
			m_HorizontalStripBuffer->Unmap(0, &writeRange);
		}
	}

	// Stability check — require consistent results for STABILITY_FRAMES
	if (detectedTop == m_LastDetectedTop && detectedBottom == m_LastDetectedBottom &&
		detectedLeft == m_LastDetectedLeft && detectedRight == m_LastDetectedRight)
	{
		m_DetectionStabilityCount++;
		if (m_DetectionStabilityCount >= STABILITY_FRAMES)
		{
			bool changed = m_Letterbox.confirmedTopBar != detectedTop ||
				m_Letterbox.confirmedBottomBar != detectedBottom ||
				m_Letterbox.confirmedLeftBar != detectedLeft ||
				m_Letterbox.confirmedRightBar != detectedRight;

			m_Letterbox.confirmedTopBar = detectedTop;
			m_Letterbox.confirmedBottomBar = detectedBottom;
			m_Letterbox.confirmedLeftBar = detectedLeft;
			m_Letterbox.confirmedRightBar = detectedRight;

			if (changed)
			{
				if (detectedTop > 0 || detectedBottom > 0 || detectedLeft > 0 || detectedRight > 0)
				{
					spdlog::info("[Letterbox] Detected: top={}px, bottom={}px, left={}px, right={}px (content: {}x{})",
						detectedTop, detectedBottom, detectedLeft, detectedRight,
						frameWidth - detectedLeft - detectedRight,
						frameHeight - detectedTop - detectedBottom);
				}
				else
				{
					spdlog::info("[Letterbox] No black bars detected - using full {}x{}", frameWidth, frameHeight);
				}
				m_Letterbox.loggedDetection = false;
			}
		}
	}
	else
	{
		m_DetectionStabilityCount = 0;
		m_LastDetectedTop = detectedTop;
		m_LastDetectedBottom = detectedBottom;
		m_LastDetectedLeft = detectedLeft;
		m_LastDetectedRight = detectedRight;
	}
}

void FFFrameInterpolatorDX::CleanupLetterboxDetection()
{
	m_VerticalStripBuffer.Reset();
	m_HorizontalStripBuffer.Reset();
}
