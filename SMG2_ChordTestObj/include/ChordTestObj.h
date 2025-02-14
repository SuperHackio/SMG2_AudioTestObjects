#pragma once
#include "syati.h"

enum ChordPlayerState {
	ChordPlayerState_Initialize = 0,
	ChordPlayerState_Appear = 1,
	ChordPlayerState_Wait = 2,
	ChordPlayerState_Exit = 3,
	ChordPlayerState_ChangeScene = 4,
};

class ChordPlayerLayout : public LayoutActor {
public:
	ChordPlayerLayout();

	virtual void init(const JMapInfoIter& rIter);
};

class ChordTestObj : public NameObj {
public:
	ChordTestObj(const char* pName);

	virtual void init(const JMapInfoIter& rIter);
	virtual void movement();

	void performChangeStage(s32 dir);
	void pulsePane(const char*);

	ChordPlayerLayout* mLayout;
	JMapInfoIter* mIterMusic; //This is not the Iter passed into Init
	JMapInfoIter* mIterChord; //This is not the Iter passed into Init
	ChordPlayerState mState;
	s32 mChangeSceneDir;
	bool mIsNeedAutoPlay;
	u8 mCounterDelayLR;
	u8 mCounterDelayUD;
};


namespace MR
{
	//TODO: Find a place in Syati for this...
	void startSystemME(const char*);
	bool hasME();
	//u32 getMeID(char const*);
}