#include "ChordTestObj.h"
#include "GalaxyLevelEngine.h"

extern "C"
{
	JMapInfo* __kAutoMap_8045AD60(const char* BcsvName);
	void __kAutoMap_80086520(JAISoundID* result, const char* SoundName);
	AudSoundObject* __kAutoMap_8007C960();
	void __kAutoMap_8007C8D0(s32 bgm, bool, s32);
	// GetMuteState. Tied to a vanilla function, but GLE Overwrites it directly.
	void* __kAutoMap_80083D70(JAISoundID, s32);
	void* __kAutoMap_8007C8B0();

	void* __kAutoMap_8007C9B0();
	u32 __kAutoMap_80042230(const char*);
	void* __kAutoMap_804285C0(void*, u32);
}
extern const void* __vt__11AudMultiBgm;
extern const void* __vt__12AudSingleBgm;

ChordTestObj::ChordTestObj(const char* pName) : NameObj(pName)
{
	mLayout = NULL;
	mIterMusic = NULL;
	mIterChord = NULL;
	mState = ChordPlayerState_Initialize;
	mChangeSceneDir = 0;
	mIsNeedAutoPlay = true;
	mCounterDelayLR = 5;
	mCounterDelayUD = 5;
}

void ChordTestObj::init(const JMapInfoIter& rIter)
{
	s32 CurrentScenario = MR::getCurrentScenarioNo();
	char buffer[32];
	snprintf(buffer, 32, "song_scenario_%d.bcsv", CurrentScenario);
	OSReport("%d %s\n", CurrentScenario, buffer);
	JMapInfo* pSoundResource = __kAutoMap_8045AD60(buffer);
	snprintf(buffer, 32, "chord_scenario_%d.bcsv", CurrentScenario);
	OSReport("%d %s\n", CurrentScenario, buffer);
	JMapInfo* pSoundResource2 = __kAutoMap_8045AD60(buffer);

	if (pSoundResource == NULL || pSoundResource2 == NULL)
	{
		OSReport("Chord Test Init failure\n");
		return;
	}

	mIterMusic = new JMapInfoIter(pSoundResource, 0);
	mIterChord = new JMapInfoIter(pSoundResource2, 0);

	mLayout = new ChordPlayerLayout();
	mLayout->initWithoutIter();
	mLayout->appear();
	MR::connectToSceneMapObjMovement(this);
	MR::registerDemoSimpleCastAll(this);
}

namespace {
	bool testSubPadStickTriggerUp(s32 id)
	{
		register void* pad = MR::getWPad(id);
		register bool result;
		__asm {
			lwz       pad, 0x20(pad)
			lwz       r0, 0x14(pad)
			extrwi    result, r0, 1, 31
		}
		return result;
	}

	bool testSubPadStickTriggerDown(s32 id)
	{
		register void* pad = MR::getWPad(id);
		register bool result;
		__asm {
			lwz       pad, 0x20(pad)
			lwz       r0, 0x14(pad)
			extrwi    result, r0, 1, 30
		}
		return result;
	}
}


void ChordTestObj::movement()
{
	if (mState == ChordPlayerState_Initialize)
	{
		MR::requestStartDemoWithoutCinemaFrame(mLayout, "ChordTest", NULL, NULL);
		MR::startAnim(mLayout, "Appear", 0);
		mState = ChordPlayerState_Appear;
		return;
	}

	if (mState == ChordPlayerState_Appear)
	{
		if (MR::isAnimStopped(mLayout, 0))
		{
			MR::startAnim(mLayout, "Wait", 0);
			mState = ChordPlayerState_Wait;
		}
		return;
	}

	if (mState == ChordPlayerState_Wait)
	{
		JAISoundID JAudioID;
		JAudioID.id = 0xFFFFFFFF;
		if (!mIterMusic->isValid())
		{
			OSReport("Invalid Iter\n");
			goto TryChangeScene;
		}
		const char* pItemName;
		if (!mIterMusic->getValue<const char*>("SoundName", &pItemName))
		{
			OSReport("Failed to read Song %d\n", mIterMusic->mIndex);
			goto TryChangeScene;
		}
		__kAutoMap_80086520(&JAudioID, pItemName);

		if (JAudioID.id == 0xFFFFFFFF)
			goto TryChangeScene;

		MR::setTextBoxFormatRecursive(mLayout, "TxtPlayerMusic", L"%s", pItemName);


		const char* mMelodyName;
		if (!mIterChord->getValue<const char*>("SoundName", &mMelodyName))
		{
			OSReport("Failed to read Chord %d\n", mIterChord->mIndex);
			goto TryChangeScene;
		}

		MR::setTextBoxFormatRecursive(mLayout, "TxtPlayerChord", L"%s", mMelodyName);



		// Scoping to shut the compiler up about goto-ing over the below bools
		{
			if (mCounterDelayLR > 0)
				mCounterDelayLR--;
			if (mCounterDelayUD > 0)
				mCounterDelayUD--;
			bool isTriggerRight = MR::testCorePadTriggerRight(0) || MR::testSubPadStickTriggerRight(0);
			bool isTriggerLeft = MR::testCorePadTriggerLeft(0) || MR::testSubPadStickTriggerLeft(0);
			bool isTriggerUp = MR::testCorePadTriggerUp(0) || testSubPadStickTriggerUp(0);
			bool isTriggerDown = MR::testCorePadTriggerDown(0) || testSubPadStickTriggerDown(0);
			bool isHoldRight = mCounterDelayLR == 0 && MR::testSubPadButtonZ(0) && (MR::testCorePadButtonRight(0) || (MR::getPlayerStickX() > 0.2f && !MR::isNearZero(MR::getPlayerStickX(), 0.2f)));
			bool isHoldLeft = mCounterDelayLR == 0 && MR::testSubPadButtonZ(0) && (MR::testCorePadButtonLeft(0) || (MR::getPlayerStickX() < 0.2f && !MR::isNearZero(MR::getPlayerStickX(), 0.2f)));
			bool isHoldUp = mCounterDelayUD == 0 && MR::testSubPadButtonZ(0) && (MR::testCorePadButtonUp(0) || (MR::getPlayerStickY() > 0.2f && !MR::isNearZero(MR::getPlayerStickY(), 0.2f)));
			bool isHoldDown = mCounterDelayUD == 0 && MR::testSubPadButtonZ(0) && (MR::testCorePadButtonDown(0) || (MR::getPlayerStickY() < 0.2f && !MR::isNearZero(MR::getPlayerStickY(), 0.2f)));
			bool isValidRight = isTriggerRight || isHoldRight;
			bool isValidLeft = isTriggerLeft || isHoldLeft;
			bool isValidUp = isTriggerUp || isHoldUp;
			bool isValidDown = isTriggerDown || isHoldDown;

			if (MR::getPlayerTriggerA() || mIsNeedAutoPlay)
			{
				if (!mIsNeedAutoPlay)
					pulsePane("PlayerPlay");
				mIsNeedAutoPlay = false;
				__kAutoMap_8007C8D0(JAudioID.id, false, -1);
			}
			else if (MR::getPlayerTriggerB() || isValidUp || isValidDown)
			{
				MR::stopStageBGM(30);
				if (MR::getPlayerTriggerB())
					pulsePane("PlayerStop");
			}

			if (MR::testSubPadButtonZ(0) && MR::testSubPadButtonC(0))
			{
				u32 MEID = __kAutoMap_80042230(mMelodyName);
				void* SystemMEObject = __kAutoMap_8007C9B0();
				register void* pMEHandle = __kAutoMap_804285C0(SystemMEObject, MEID);
				if (pMEHandle == NULL)
				{
					MR::startSystemME(mMelodyName);
					pulsePane("PlayerBtnChord");
				}
			}
			else if(MR::testSubPadTriggerC(0))
			{
				MR::startSystemME(mMelodyName);
				pulsePane("PlayerBtnChord");
			}

			if (isValidRight)
			{
				mIterChord->mIndex++;
				if (mIterChord->mIndex >= MR::getCsvDataElementNum(mIterChord->mInfo))
				{
					mIterChord->mIndex = 0;
					mCounterDelayLR = 10;
				}
				else
					mCounterDelayLR = 5;
				pulsePane("PlayerChord");
				pulsePane("PlayerChordNext");
			}
			else if (isValidLeft)
			{
				mIterChord->mIndex--;
				if (mIterChord->mIndex < 0)
				{
					mIterChord->mIndex = MR::getCsvDataElementNum(mIterChord->mInfo) - 1;
					mCounterDelayLR = 10;
				}
				else
					mCounterDelayLR = 5;
				pulsePane("PlayerChord");
				pulsePane("PlayerChordPrev");
			}
			if (isValidUp)
			{
				mIterMusic->mIndex--;
				if (mIterMusic->mIndex < 0)
				{
					mIterMusic->mIndex = MR::getCsvDataElementNum(mIterMusic->mInfo) - 1;
					mCounterDelayUD = 10;
				}
				else
					mCounterDelayUD = 5;
				mIsNeedAutoPlay = true;
				pulsePane("PlayerMusic");
				pulsePane("PlayerMusicPrev");
			}
			else if (isValidDown)
			{
				mIterMusic->mIndex++;
				if (mIterMusic->mIndex >= MR::getCsvDataElementNum(mIterMusic->mInfo))
				{
					mIterMusic->mIndex = 0;
					mCounterDelayUD = 10;
				}
				else
					mCounterDelayUD = 5;
				mIsNeedAutoPlay = true;
				pulsePane("PlayerMusic");
				pulsePane("PlayerMusicNext");
			}
		}

	TryChangeScene:
		if (MR::testCorePadTriggerPlus(0))
		{
			mChangeSceneDir = 1;
			MR::startAnim(mLayout, "End", 0);
			pulsePane("PlayerCategN");
			if (JAudioID.id != 0xFFFFFFFF)
				MR::stopStageBGM(60);
			mState = ChordPlayerState_Exit;
		}
		else if (MR::testCorePadTriggerMinus(0))
		{
			mChangeSceneDir = -1;
			MR::startAnim(mLayout, "End", 0);
			pulsePane("PlayerCategP");
			if (JAudioID.id != 0xFFFFFFFF)
				MR::stopStageBGM(60);
			mState = ChordPlayerState_Exit;
		}
		return;
	}
	if (mState == ChordPlayerState_Exit)
	{
		if (MR::isAnimStopped(mLayout, 0))
		{
			MR::closeSystemWipeMario(60);
			mState = ChordPlayerState_ChangeScene;
		}
	}
	if (mState == ChordPlayerState_ChangeScene)
	{
		if (MR::isSystemWipeActive())
			return;
		performChangeStage(mChangeSceneDir);
	}
}

void ChordTestObj::performChangeStage(s32 dir)
{
	JMapInfo* ChangeSceneListInfo = GLE::getChangeSceneListInfoFromZone(0);
	s32 NextScenario = (MR::getCurrentScenarioNo() - 1) + dir;
	s32 MaxScenario = MR::getCsvDataElementNum(ChangeSceneListInfo);
	if (NextScenario < 0)
		NextScenario = MaxScenario - 1;
	else if (NextScenario >= MaxScenario)
		NextScenario = 0;

	GLE::requestMoveStageFromJMapInfo(ChangeSceneListInfo, NextScenario);
}

void ChordTestObj::pulsePane(const char* paneName)
{
	MR::startPaneAnim(mLayout, paneName, "Push", 0);
}



ChordPlayerLayout::ChordPlayerLayout() : LayoutActor("ChordPlayerLayout", true)
{

}

void ChordPlayerLayout::init(const JMapInfoIter& rIter)
{
	MR::connectToSceneLayout(this);
	initLayoutManager("ChordPlayer", 2);

	MR::setTextBoxMessageRecursive(this, "ShaPlayerTitle", L"SMG2 Chords Player");
	MR::setTextBoxMessageRecursive(this, "TxtPlayerMusic", L"--------");
	MR::setTextBoxMessageRecursive(this, "TxtPlayerChord", L"--------");
	MR::registerDemoSimpleCastAll(this);

	MR::createAndAddPaneCtrl(this, "PlayerChordPrev", 1);
	MR::createAndAddPaneCtrl(this, "PlayerChordNext", 1);
	MR::createAndAddPaneCtrl(this, "PlayerChord", 1);
	MR::createAndAddPaneCtrl(this, "PlayerMusic", 1);
	MR::createAndAddPaneCtrl(this, "PlayerMusicPrev", 1);
	MR::createAndAddPaneCtrl(this, "PlayerMusicNext", 1);
	MR::createAndAddPaneCtrl(this, "PlayerPlay", 1);
	MR::createAndAddPaneCtrl(this, "PlayerStop", 1);
	MR::createAndAddPaneCtrl(this, "PlayerBtnChord", 1);
	MR::createAndAddPaneCtrl(this, "PlayerCategN", 1);
	MR::createAndAddPaneCtrl(this, "PlayerCategP", 1);
}