#pragma once
#include "syati.h"

enum SoundPlayerState {
	SoundPlayerState_Initialize = 0,
	SoundPlayerState_Appear = 1,
	SoundPlayerState_Wait = 2,
	SoundPlayerState_Exit = 3,
	SoundPlayerState_ChangeScene = 4,
};

class SoundPlayerLayout : public LayoutActor {
public:
	SoundPlayerLayout();

	virtual void init(const JMapInfoIter& rIter);
};

class SoundTestObj : public NameObj {
public:
	SoundTestObj(const char* pName);

	virtual void init(const JMapInfoIter& rIter);
	virtual void movement();

	void performChangeStage(s32 dir);
	void pulsePane(const char*);

	SoundPlayerLayout* mLayout;
	JMapInfoIter* mIter; //This is not the Iter passed into Init
	SoundPlayerState mState;
	s32 mChangeSceneDir;
	bool mIsNeedAutoPlay;
	u8 mCounterDelay;
};