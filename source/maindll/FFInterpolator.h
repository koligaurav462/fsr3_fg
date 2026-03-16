#pragma once

#include <FidelityFX/host/ffx_frameinterpolation.h>

struct FFInterpolatorDispatchParameters
{
	FfxCommandList CommandList;

    FfxDimensions2D RenderSize;
	FfxDimensions2D OutputSize;

    FfxResource InputColorBuffer;
    FfxResource InputHUDLessColorBuffer;
    FfxResource InputDepth;
    FfxResource InputMotionVectors;
	FfxResource InputDistortionField;

    FfxResource InputOpticalFlowVector;
    FfxResource InputOpticalFlowSceneChangeDetection;
    FfxFloatCoords2D OpticalFlowScale;
    int OpticalFlowBlockSize;

    FfxResource OutputInterpolatedColorBuffer;

	bool MotionVectorsFullResolution;
	bool MotionVectorJitterCancellation;
	bool MotionVectorsDilated;

	FfxFloatCoords2D MotionVectorScale;
	FfxFloatCoords2D MotionVectorJitterOffsets;

	bool HDR;
	bool DepthInverted;
	bool DepthPlaneInfinite;
	bool Reset;
	bool DebugTearLines;
	bool DebugView;

	// Generation Rect: Custom interpolation region (0,0,0,0 = full screen)
	int InterpolationRectLeft;
	int InterpolationRectTop;
	int InterpolationRectWidth;   // 0 = use display width
	int InterpolationRectHeight;  // 0 = use display height

    float CameraNear;
	float CameraFar;
	float CameraFovAngleVertical;
	FfxFloatCoords2D MinMaxLuminance;
};

class FFInterpolator
{
private:
	const uint32_t m_MaxRenderWidth;
	const uint32_t m_MaxRenderHeight;

	const FfxInterface m_BackendInterface;
	FfxInterface m_SharedBackendInterface;
	FfxUInt32 m_SharedEffectContextId = {};

	FfxFrameInterpolationContextDescription m_ContextDescription = {};
	std::optional<FfxFrameInterpolationContext> m_FSRContext;
	bool m_ContextFlushPending = false;

	std::optional<FfxResourceInternal> m_DilatedDepth;
	std::optional<FfxResourceInternal> m_DilatedMotionVectors;
	std::optional<FfxResourceInternal> m_ReconstructedPrevDepth;

	uint64_t m_FrameID = 0;

public:
	FFInterpolator(
		const FfxInterface& BackendInterface,
		const FfxInterface& SharedBackendInterface,
		FfxUInt32 SharedEffectContextId,
		uint32_t MaxRenderWidth,
		uint32_t MaxRenderHeight);
	FFInterpolator(const FFInterpolator&) = delete;
	FFInterpolator& operator=(const FFInterpolator&) = delete;
	~FFInterpolator();

	FfxErrorCode Dispatch(const FFInterpolatorDispatchParameters& Parameters);

private:
	FfxErrorCode CreateContextDeferred(const FFInterpolatorDispatchParameters& Parameters);
	void DestroyContext();
};
