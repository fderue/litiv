#pragma once

#include "BackgroundSubtractorLBSP.h"

//! defines the default value for BackgroundSubtractorLBSP::m_fLBSPThreshold
#define BGSCBLBSP_DEFAULT_LBSP_REL_SIMILARITY_THRESHOLD (0.250f)
//! defines the default offset LBSP threshold value
#define BGSCBLBSP_DEFAULT_LBSP_OFFSET_SIMILARITY_THRESHOLD (3)
//! defines the default value for BackgroundSubtractorLBSP::m_nDescDistThreshold
#define BGSCBLBSP_DEFAULT_DESC_DIST_THRESHOLD (4)
//! defines the default value for BackgroundSubtractorCBLBSP::m_nColorDistThreshold
#define BGSCBLBSP_DEFAULT_COLOR_DIST_THRESHOLD (24)
//! defines the default value for BackgroundSubtractorCBLBSP::m_fLocalWordsPerChannel
#define BGSCBLBSP_DEFAULT_NB_LOCAL_WORDS_PER_CH (8.0f)
//! defines the default value for BackgroundSubtractorCBLBSP::m_fGlobalWordsPerPixelChannel
#define BGSCBLBSP_DEFAULT_NB_GLOBAL_WORDS_PER_CH (10.0f)
//! defines the number of samples to use when computing running averages
#define BGSCBLBSP_N_SAMPLES_FOR_MEAN (25)
//! defines the threshold values used to detect long-term ghosting and trigger a fast edge-based absorption in the model
#define BGSCBLBSP_GHOST_DETECTION_D_MAX (0.010f)
#define BGSCBLBSP_GHOST_DETECTION_S_MIN (0.995f)
#define BGSCBLBSP_GHOST_DETECTION_S_MIN2 (0.999f)
#define BGSCBLBSP_GHOST_DETECTION_D_MAX2 (0.015f)
//! defines the threshold values used to detect high variation regions that are often labelled as foreground and trigger a local, gradual change in distance thresholds
#define BGSCBLBSP_HIGH_VAR_DETECTION_S_MIN (0.850f)
#define BGSCBLBSP_HIGH_VAR_DETECTION_D_MIN (0.175f)
#define BGSCBLBSP_HIGH_VAR_DETECTION_S_MIN2 (0.100f)
#define BGSCBLBSP_HIGH_VAR_DETECTION_D_MIN2 (0.225f)
//! defines the internal threshold adjustment factor to use when treating single channel images
#define BGSCBLBSP_SINGLECHANNEL_THRESHOLD_MODULATION_FACT (0.350f) // or (0.500f) for final version? ... more consistent across categories
//! parameters used for dynamic distance threshold adjustments ('R(x)')
#define BGSCBLBSP_R_SCALE (3.5000f)
#define BGSCBLBSP_R_INCR  (0.0850f)
#define BGSCBLBSP_R_DECR  (0.0300f)
#define BGSCBLBSP_R_LOWER (0.8000f)
#define BGSCBLBSP_R_UPPER (3.5000f)
//! parameters used for adjusting the variation speed of dynamic distance thresholds  ('R2(x)')
#define BGSCBLBSP_R2_OFFST (0.100f)
#define BGSCBLBSP_R2_INCR  (0.800f)
#define BGSCBLBSP_R2_DECR  (0.100f)
//! parameters used for dynamic learning rates adjustments  ('T(x)')
#define BGSCBLBSP_T_DECR  (0.0250f)
#define BGSCBLBSP_T_INCR  (0.2500f)
#define BGSCBLBSP_T_LOWER (2.0000f)
#define BGSCBLBSP_T_UPPER (256.00f)

/*!
	CB-Based Local Binary Similarity Pattern (LBSP) foreground-background segmentation algorithm.

	Note: both grayscale and RGB/BGR images may be used with this extractor (parameters
	are adjusted automatically).

	For more details on the different parameters, go to @@@@@@@@@@@@@@.

	This algorithm is currently NOT thread-safe.
 */
class BackgroundSubtractorCBLBSP : public BackgroundSubtractorLBSP {
public:
	//! full constructor
	BackgroundSubtractorCBLBSP(	float fLBSPThreshold=BGSCBLBSP_DEFAULT_LBSP_REL_SIMILARITY_THRESHOLD,
								size_t nLBSPThresholdOffset=BGSCBLBSP_DEFAULT_LBSP_OFFSET_SIMILARITY_THRESHOLD,
								size_t nInitDescDistThreshold=BGSCBLBSP_DEFAULT_DESC_DIST_THRESHOLD,
								size_t nInitColorDistThreshold=BGSCBLBSP_DEFAULT_COLOR_DIST_THRESHOLD,
								float fLocalWordsPerChannel=BGSCBLBSP_DEFAULT_NB_LOCAL_WORDS_PER_CH,
								float fGlobalWordsPerChannel=BGSCBLBSP_DEFAULT_NB_GLOBAL_WORDS_PER_CH);
	//! default destructor
	virtual ~BackgroundSubtractorCBLBSP();
	//! (re)initiaization method; needs to be called before starting background subtraction (note: also reinitializes the keypoints vector)
	virtual void initialize(const cv::Mat& oInitImg, const std::vector<cv::KeyPoint>& voKeyPoints); // @@@@@@@@@@ currently always reinits internal memory structs without reusing old words, if KPs match
	//! primary model update function; the learning param is used to override the internal learning speed (ignored when <= 0)
	virtual void operator()(cv::InputArray image, cv::OutputArray fgmask, double learningRateOverride=0);
	//! returns a copy of the latest reconstructed background image
	virtual void getBackgroundImage(cv::OutputArray backgroundImage) const;
	//! returns a copy of the latest reconstructed background descriptors image
	virtual void getBackgroundDescriptorsImage(cv::OutputArray backgroundDescImage) const;
	//! sets the keypoints to be used for descriptor extraction, effectively setting the BGModel ROI (note: this function will remove all border keypoints and allocate new words where needed)
	virtual void setBGKeyPoints(std::vector<cv::KeyPoint>& keypoints);

protected:
	struct LocalWord {
		size_t nFirstOcc;
		size_t nLastOcc;
		size_t nOccurrences;
	protected:
		~LocalWord(); // only used to prevent internal polymorphism (keeps algo cost down)
	};
	struct LocalWord_1ch : LocalWord {
		uchar nColor;
		uchar nDescBITS;
		ushort nDesc;
	};
	struct LocalWord_3ch : LocalWord {
		uchar anColor[3];
		uchar anDescBITS[3];
		uchar nDescBITS;
		ushort anDesc[3];
	};
	struct GlobalWord {
		float fLatestWeight;
		cv::Mat oSpatioOccMap;
	protected:
		~GlobalWord(); // only used to prevent internal polymorphism (keeps algo cost down)
	};
	struct GlobalWord_1ch : GlobalWord {
		uchar nColor;
		uchar nDescBITS; // 'smoothness' indicator
	};
	struct GlobalWord_3ch : GlobalWord {
		uchar anColor[3];
		uchar nDescBITS; // 'smoothness' indicator
	};
	//! absolute color distance threshold ('R' or 'radius' in the original ViBe paper, used as the default/initial 'R(x)' value here, paired with BackgroundSubtractorLBSP::m_nDescDistThreshold)
	const size_t m_nColorDistThreshold;
	//! number of local words per channel used to build a background submodel (for a single pixel)
	const float m_fLocalWordsPerChannel;
	//! number of local words used to build a background submodel (for a single pixel, similar to 'N' in ViBe/PBAS -- only usable after initialisation)
	size_t m_nLocalWords;
	//! number of global words per pixel, per channel used to build a complete background model
	const float m_fGlobalWordsPerChannel;
	//! number of global words used to build a complete background model (only usable after initialisation)
	size_t m_nGlobalWords;
	//! total maximum number of local dictionaries (depends on the input frame size)
	size_t m_nMaxLocalDictionaries;
	//! current frame index, used to keep track of word occurrence information
	size_t m_nFrameIndex;

	//! background model local word list & dictionaries
	LocalWord** m_aapLocalDicts;
	LocalWord_1ch* m_apLocalWordList_1ch;
	LocalWord_3ch* m_apLocalWordList_3ch;
	//! background model global word list & dictionary (1x dictionary for the whole model)
	GlobalWord** m_apGlobalDict;
	GlobalWord_1ch* m_apGlobalWordList_1ch;
	GlobalWord_3ch* m_apGlobalWordList_3ch;
	GlobalWord** m_apGlobalWordLookupTable;

	//! per-pixel distance thresholds (equivalent to 'R(x)' in PBAS, but used as a relative value to determine both intensity and descriptor variation thresholds)
	cv::Mat m_oDistThresholdFrame;
	//! per-pixel distance threshold variation modulators ('R2(x)', relative value used to modulate 'R(x)' variations)
	cv::Mat m_oDistThresholdVariationFrame;
	//! per-pixel mean minimal distances from the model ('D_min(x)' in PBAS, used to control variation magnitude and direction of 'T(x)' and 'R(x)')
	cv::Mat m_oMeanMinDistFrame;
	//! per-pixel mean distances between consecutive frames ('D_last(x)', used to detect ghosts and high variation regions in the sequence)
	cv::Mat m_oMeanLastDistFrame;
	//! per-pixel mean segmentation results ('S(x)', used to detect ghosts and high variation regions in the sequence)
	cv::Mat m_oMeanSegmResFrame;
	//! per-pixel blink detection results ('Z(x)', used to determine which frame regions should be assigned stronger 'R(x)' variations)
	cv::Mat m_oBlinksFrame;
	//! per-pixel update rates ('T(x)' in PBAS, which contains pixel-level 'sigmas', as referred to in ViBe)
	cv::Mat m_oUpdateRateFrame;
	//! per-pixel word weight thresholds @@@@@@@@ curr used for debug
	//cv::Mat m_oWeightThresholdFrame;
	//! copy of previously used pixel intensities used to calculate 'D_last(x)'
	cv::Mat m_oLastColorFrame;
	//! copy of previously used descriptors used to calculate 'D_last(x)'
	cv::Mat m_oLastDescFrame;
	//! the foreground mask generated by the method at [t-1] (without post-proc, used for blinking px detection)
	cv::Mat m_oPureFGMask_last;
	//! the foreground mask generated by the method at [t-1] (with post-proc)
	cv::Mat m_oFGMask_last;
	//! the foreground mask generated by the method at [t-1] (with post-proc + dilatation, used for blinking px validation)
	cv::Mat m_oFGMask_last_dilated;

	//! pre-allocated CV_8UC1 matrix used to speed up morph ops
	cv::Mat m_oTempFGMask;
	cv::Mat m_oPureFGBlinkMask_curr;
	cv::Mat m_oPureFGBlinkMask_last;

	//! pre-allocated internal LBSP threshold values for all possible 8-bit intensity values
	size_t m_anLBSPThreshold_8bitLUT[256];

	//! internal cleanup function for the dictionary structures
	void CleanupDictionaries();
	//! internal weight lookup function for local words
	static float GetLocalWordWeight(const LocalWord* w, size_t nCurrFrame);
	//! internal weight lookup function for global words
	static float GetGlobalWordWeight(const GlobalWord* w);
};