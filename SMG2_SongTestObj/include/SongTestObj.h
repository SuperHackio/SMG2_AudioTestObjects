#pragma once
#include "syati.h"

enum SongPlayerState {
	SongPlayerState_Initialize = 0,
	SongPlayerState_Appear = 1,
	SongPlayerState_Wait = 2,
	SongPlayerState_Exit = 3,
	SongPlayerState_ChangeScene = 4,
};

class SongPlayerLayout : public LayoutActor {
public:
	SongPlayerLayout();

	virtual void init(const JMapInfoIter& rIter);
	void createBMSPaneName(char* buffer, s32 buffersize, s32 Track);
	void createASTPaneName(char* buffer, s32 buffersize, s32 Track);
};

#define PANE_NAME_BUFFER_SIZE 20
#define MAX_BMS 16
//TODO: Not supposed to be 3 for vanilla
#define MAX_AST 3

class SongTestObj : public NameObj {
public:
	SongTestObj(const char* pName);

	virtual void init(const JMapInfoIter& rIter);
	virtual void movement();

	void performChangeStage(s32 dir);
	void pulsePane(const char*);

	SongPlayerLayout* mLayout;
	JMapInfoIter* mIter; //This is not the Iter passed into Init
	SongPlayerState mState;
	s32 mChangeSceneDir;
	bool mIsNeedAutoPlay;
	u8 mCounterDelayLR;
	u8 mCounterDelayUD;
	s8 mMuteGroupOffset;
};