#include "SongTestObj.h"
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
}
extern const void* __vt__11AudMultiBgm;
extern const void* __vt__12AudSingleBgm;

SongTestObj::SongTestObj(const char* pName) : NameObj(pName)
{
	mLayout = NULL;
	mIter = NULL;
	mState = SongPlayerState_Initialize;
	mChangeSceneDir = 0;
	mIsNeedAutoPlay = true;
	mCounterDelayLR = 5;
	mCounterDelayUD = 5;
	mMuteGroupOffset = 0;
}

void SongTestObj::init(const JMapInfoIter& rIter)
{
	s32 CurrentScenario = MR::getCurrentScenarioNo();
	char buffer[32];
	snprintf(buffer, 32, "song_scenario_%d.bcsv", CurrentScenario);
	OSReport("%d %s\n", CurrentScenario, buffer);
	JMapInfo* pSoundResource = __kAutoMap_8045AD60(buffer);

	if (pSoundResource == NULL)
	{
		OSReport("Song Test Init failure\n");
		return;
	}

	mIter = new JMapInfoIter(pSoundResource, 0);

	mLayout = new SongPlayerLayout();
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


void SongTestObj::movement()
{
	if (mState == SongPlayerState_Initialize)
	{
		MR::requestStartDemoWithoutCinemaFrame(mLayout, "SongTest", NULL, NULL);
		MR::startAnim(mLayout, "Appear", 0);
		mState = SongPlayerState_Appear;
		return;
	}

	if (mState == SongPlayerState_Appear)
	{
		if (MR::isAnimStopped(mLayout, 0))
		{
			MR::startAnim(mLayout, "Wait", 0);
			mState = SongPlayerState_Wait;
		}
		return;
	}

	if (mState == SongPlayerState_Wait)
	{
		JAISoundID JAudioID;
		JAudioID.id = 0xFFFFFFFF;
		if (!mIter->isValid())
		{
			OSReport("Invalid Iter\n");
			goto TryChangeScene;
		}
		const char* pItemName;
		if (!mIter->getValue<const char*>("SoundName", &pItemName))
		{
			OSReport("Failed to read Song %d\n", mIter->mIndex);
			goto TryChangeScene;
		}
		__kAutoMap_80086520(&JAudioID, pItemName);
		//AudSoundObject* pSystemSound = __kAutoMap_8007C960();
		if (JAudioID.id == 0xFFFFFFFF)
			goto TryChangeScene;

		MR::setTextBoxFormatRecursive(mLayout, "TxtPlayerMusic", L"%s", pItemName);
		MR::setTextBoxFormatRecursive(mLayout, "TxtPlayerID", L"%08X", JAudioID.id);
		MR::setTextBoxFormatRecursive(mLayout, "TxtPlayerMuteID", L"%03d", mMuteGroupOffset);

		{
			char namebuffer[PANE_NAME_BUFFER_SIZE];
			// Get the active BGM... hopefully
			register void* pStageBgmPtr = __kAutoMap_8007C8B0();
			register u32 newID = JAudioID.id;;
			if (pStageBgmPtr != NULL)
			{
				const void** vtblPtr = *(const void***)pStageBgmPtr;
				if (vtblPtr == &__vt__11AudMultiBgm)
				{
					__asm {
						lwz pStageBgmPtr, 0x20(pStageBgmPtr)
						cmpwi pStageBgmPtr, 0
						beq Skip
						lwz newID, 0x18(pStageBgmPtr)
						Skip:
					}
				}
				else if (vtblPtr == &__vt__12AudSingleBgm)
				{
					__asm {
						lwz newID, 0x20(pStageBgmPtr)
					}
				}
			}
			JAISoundID HackMBGM;
			HackMBGM.id = (s32)newID;

			u64* pMuteGroup = (u64*)__kAutoMap_80083D70(HackMBGM, mMuteGroupOffset);

			if (pMuteGroup != NULL)
			{
				u64 BMSData = *pMuteGroup; // ???
				for (s32 i = 0; i < MAX_BMS; i++)
				{
					u32 copy = (u32)((BMSData << i * 4) >> 32);
					mLayout->createBMSPaneName(namebuffer, PANE_NAME_BUFFER_SIZE, i);
					MR::setTextBoxFormatRecursive(mLayout, namebuffer, L"%01X", (copy >> 28) & 0xF);
				}
				pMuteGroup++;
				u64 ASTData = *pMuteGroup; // ???
				for (s32 i = 0; i < MAX_AST; i++)
				{
					u32 copy = (u32)((ASTData << i * 8) >> 32);
					mLayout->createASTPaneName(namebuffer, PANE_NAME_BUFFER_SIZE, i);
					MR::setTextBoxFormatRecursive(mLayout, namebuffer, L"%02X", (copy >> 24) & 0xFF);
				}
				MR::startPaneAnimAndSetFrameAndStop(mLayout, "PlayerMuteN", "Greyed", 0.f, 1);
				MR::startPaneAnimAndSetFrameAndStop(mLayout, "PlayerMuteP", "Greyed", 0.f, 1);
				MR::startPaneAnimAndSetFrameAndStop(mLayout, "PlayerMuteGroup", "Greyed", 0.f, 1);
				MR::startPaneAnimAndSetFrameAndStop(mLayout, "PlayerMuteGrpID", "Greyed", 0.f, 1);
				MR::startPaneAnimAndSetFrameAndStop(mLayout, "PlayerReset", "Greyed", 0.f, 1);
			}
			else
			{
				for (s32 i = 0; i < MAX_BMS; i++)
				{
					mLayout->createBMSPaneName(namebuffer, PANE_NAME_BUFFER_SIZE, i);
					MR::setTextBoxMessageRecursive(mLayout, namebuffer, L"1");
				}
				for (s32 i = 0; i < MAX_AST; i++)
				{
					mLayout->createASTPaneName(namebuffer, PANE_NAME_BUFFER_SIZE, i);
					MR::setTextBoxMessageRecursive(mLayout, namebuffer, L"FF");
				}
				MR::startPaneAnimAndSetFrameAndStop(mLayout, "PlayerMuteN", "Greyed", 1.f, 1);
				MR::startPaneAnimAndSetFrameAndStop(mLayout, "PlayerMuteP", "Greyed", 1.f, 1);
				MR::startPaneAnimAndSetFrameAndStop(mLayout, "PlayerMuteGroup", "Greyed", 1.f, 1);
				MR::startPaneAnimAndSetFrameAndStop(mLayout, "PlayerMuteGrpID", "Greyed", 1.f, 1);
				MR::startPaneAnimAndSetFrameAndStop(mLayout, "PlayerReset", "Greyed", 1.f, 1);
			}
		}

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
				MR::setStageBGMState(mMuteGroupOffset, 0);
			}
			else if (MR::getPlayerTriggerB() || isValidRight || isValidLeft)
			{
				MR::stopStageBGM(30);
				if (MR::getPlayerTriggerB())
					pulsePane("PlayerStop");
			}
			if (isValidRight)
			{
				mIter->mIndex++;
				if (mIter->mIndex >= MR::getCsvDataElementNum(mIter->mInfo))
				{
					mIter->mIndex = 0;
					mCounterDelayLR = 10;
				}
				else
					mCounterDelayLR = 5;
				mIsNeedAutoPlay = true;
				pulsePane("PlayerNext");
				pulsePane("PlayerMusic");
				pulsePane("PlayerID");
			}
			else if (isValidLeft)
			{
				mIter->mIndex--;
				if (mIter->mIndex < 0)
				{
					mIter->mIndex = MR::getCsvDataElementNum(mIter->mInfo) - 1;
					mCounterDelayLR = 10;
				}
				else
					mCounterDelayLR = 5;
				mIsNeedAutoPlay = true;
				pulsePane("PlayerPrev");
				pulsePane("PlayerMusic");
				pulsePane("PlayerID");
			}
			if (isValidUp)
			{
				mMuteGroupOffset--;
				MR::setStageBGMState(mMuteGroupOffset, 60);
				mCounterDelayUD = 5;
				pulsePane("PlayerMuteP");
				pulsePane("PlayerMuteGroup");
				pulsePane("PlayerMuteGrpID");
			}
			else if (isValidDown)
			{
				mMuteGroupOffset++;
				MR::setStageBGMState(mMuteGroupOffset, 60);
				mCounterDelayUD = 5;
				pulsePane("PlayerMuteN");
				pulsePane("PlayerMuteGroup");
				pulsePane("PlayerMuteGrpID");
			}
			else if (MR::testSubPadTriggerC(0))
			{
				if (mMuteGroupOffset < 0)
					pulsePane("PlayerMuteN");
				else if (mMuteGroupOffset > 0)
					pulsePane("PlayerMuteP");
				pulsePane("PlayerReset");
				pulsePane("PlayerMuteGroup");
				pulsePane("PlayerMuteGrpID");
				mMuteGroupOffset = 0;
				MR::setStageBGMState(mMuteGroupOffset, 60);
				mCounterDelayUD = 5;
			}
		}

		TryChangeScene:
		if (MR::testCorePadTriggerPlus(0))
		{
			mChangeSceneDir = 1;
			MR::startAnim(mLayout, "End", 0);
			pulsePane("PlayerScenNext");
			if (JAudioID.id != 0xFFFFFFFF)
				MR::stopStageBGM(60);
			mState = SongPlayerState_Exit;
		}
		else if (MR::testCorePadTriggerMinus(0))
		{
			mChangeSceneDir = -1;
			MR::startAnim(mLayout, "End", 0);
			pulsePane("PlayerScenPrev");
			if (JAudioID.id != 0xFFFFFFFF)
				MR::stopStageBGM(60);
			mState = SongPlayerState_Exit;
		}
		return;
	}
	if (mState == SongPlayerState_Exit)
	{
		if (MR::isAnimStopped(mLayout, 0))
		{
			MR::closeSystemWipeMario(60);
			mState = SongPlayerState_ChangeScene;
		}
	}
	if (mState == SongPlayerState_ChangeScene)
	{
		if (MR::isSystemWipeActive())
			return;
		performChangeStage(mChangeSceneDir);
	}
}

void SongTestObj::performChangeStage(s32 dir)
{
	JMapInfo* ChangeSceneListInfo = GLE::getChangeSceneListInfoFromZone(0);
	s32 NextScenario = (MR::getCurrentScenarioNo()-1) + dir;
	s32 MaxScenario = MR::getCsvDataElementNum(ChangeSceneListInfo);
	if (NextScenario < 0)
		NextScenario = MaxScenario - 1;
	else if (NextScenario >= MaxScenario)
		NextScenario = 0;

	GLE::requestMoveStageFromJMapInfo(ChangeSceneListInfo, NextScenario);
}

void SongTestObj::pulsePane(const char* paneName)
{
	MR::startPaneAnim(mLayout, paneName, "Push", 0);
}



SongPlayerLayout::SongPlayerLayout() : LayoutActor("SongPlayerLayout", true)
{

}

void SongPlayerLayout::init(const JMapInfoIter& rIter)
{
	MR::connectToSceneLayout(this);
	initLayoutManager("MusicPlayer", 2);

	MR::setTextBoxMessageRecursive(this, "ShaPlayerTitle", MR::getGalaxyNameShortOnCurrentLanguage(MR::getCurrentStageName()));
	MR::setTextBoxMessageRecursive(this, "TxtPlayerCateg", MR::getCurrentScenarioNameOnCurrentLanguage());
	MR::setTextBoxMessageRecursive(this, "TxtPlayerMusic", L"--------");
	MR::setTextBoxMessageRecursive(this, "TxtPlayerID", L"--------");
	MR::setTextBoxMessageRecursive(this, "TxtPlayerID", L"--------");
	MR::setTextBoxMessageRecursive(this, "TxtPlayerMuteID", L"---");
	MR::registerDemoSimpleCastAll(this);

	MR::createAndAddPaneCtrl(this, "PlayerMusic", 1);
	MR::createAndAddPaneCtrl(this, "PlayerID", 1);
	MR::createAndAddPaneCtrl(this, "PlayerMuteGroup", 2);
	MR::createAndAddPaneCtrl(this, "PlayerMuteGrpID", 2);
	MR::createAndAddPaneCtrl(this, "PlayerPlay", 1);
	MR::createAndAddPaneCtrl(this, "PlayerStop", 1);
	MR::createAndAddPaneCtrl(this, "PlayerPrev", 1);
	MR::createAndAddPaneCtrl(this, "PlayerNext", 1);
	MR::createAndAddPaneCtrl(this, "PlayerMuteN", 2);
	MR::createAndAddPaneCtrl(this, "PlayerMuteP", 2);
	MR::createAndAddPaneCtrl(this, "PlayerScenNext", 1);
	MR::createAndAddPaneCtrl(this, "PlayerScenPrev", 1);
	MR::createAndAddPaneCtrl(this, "PlayerReset", 2);

	char namebuffer[PANE_NAME_BUFFER_SIZE];
	for (s32 i = 0; i < MAX_BMS; i++)
	{
		createBMSPaneName(namebuffer, PANE_NAME_BUFFER_SIZE, i);
		MR::setTextBoxMessageRecursive(this, namebuffer, L"0");
	}
	for (s32 i = 0; i < MAX_AST; i++)
	{
		createASTPaneName(namebuffer, PANE_NAME_BUFFER_SIZE, i);
		MR::setTextBoxMessageRecursive(this, namebuffer, L"0");
	}
}

void SongPlayerLayout::createBMSPaneName(char* buffer, s32 buffersize, s32 Track)
{
	snprintf(buffer, buffersize, "TxtBMSTrack%d", Track);
}
void SongPlayerLayout::createASTPaneName(char* buffer, s32 buffersize, s32 Track)
{
	snprintf(buffer, buffersize, "TxtASTTrack%d", Track);
}