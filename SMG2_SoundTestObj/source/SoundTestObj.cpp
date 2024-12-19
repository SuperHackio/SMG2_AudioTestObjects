#include "SoundTestObj.h"
#include "GalaxyLevelEngine.h"

extern "C"
{
	JMapInfo* __kAutoMap_8045AD60(const char* BcsvName);
	void __kAutoMap_80086520(JAISoundID* result, const char* SoundName);
	AudSoundObject* __kAutoMap_8007C960();
}

SoundTestObj::SoundTestObj(const char* pName) : NameObj(pName)
{
	mLayout = NULL;
	mIter = NULL;
	mState = SoundPlayerState_Initialize;
	mChangeSceneDir = 0;
	mIsNeedAutoPlay = true;
	mCounterDelay = 5;
}

void SoundTestObj::init(const JMapInfoIter& rIter)
{
	s32 CurrentScenario = MR::getCurrentScenarioNo();
	char buffer[32];
	snprintf(buffer, 32, "sound_scenario_%d.bcsv", CurrentScenario);
	OSReport("%d %s\n", CurrentScenario, buffer);
	JMapInfo* pSoundResource = __kAutoMap_8045AD60(buffer);

	if (pSoundResource == NULL)
	{
		OSReport("Sound Test Init failure\n");
		return;
	}

	mIter = new JMapInfoIter(pSoundResource, 0);

	mLayout = new SoundPlayerLayout();
	mLayout->initWithoutIter();
	mLayout->appear();
	MR::connectToSceneMapObjMovement(this);
	MR::registerDemoSimpleCastAll(this);
}

void SoundTestObj::movement()
{
	if (mState == SoundPlayerState_Initialize)
	{
		MR::requestStartDemoWithoutCinemaFrame(mLayout, "SoundTest", NULL, NULL);
		MR::startAnim(mLayout, "Appear", 0);
		mState = SoundPlayerState_Appear;
		return;
	}

	if (mState == SoundPlayerState_Appear)
	{
		if (MR::isAnimStopped(mLayout, 0))
		{
			MR::startAnim(mLayout, "Wait", 0);
			mState = SoundPlayerState_Wait;
		}
		return;
	}

	if (mState == SoundPlayerState_Wait)
	{
		JAISoundID JAudioID;
		JAudioID.id = 0xFFFFFFFF;
		if (!mIter->isValid())
		{
			OSReport("Invalid Iter\n");
			goto TryChangeScene;
		}
		const char* pSoundName;
		if (!mIter->getValue<const char*>("SoundName", &pSoundName))
		{
			OSReport("Failed to read Sound %d\n", mIter->mIndex);
			goto TryChangeScene;
		}
		__kAutoMap_80086520(&JAudioID, pSoundName);
		//AudSoundObject* pSystemSound = __kAutoMap_8007C960();
		if (JAudioID.id == 0xFFFFFFFF)
			goto TryChangeScene;

		MR::setTextBoxFormatRecursive(mLayout, "TxtPlayerSound", L"%s", pSoundName);
		MR::setTextBoxFormatRecursive(mLayout, "TxtPlayerID", L"%08X", JAudioID.id);

		// Scoping to shut the compiler up about goto-ing over the below bools
		{
			if (mCounterDelay > 0)
				mCounterDelay--;
			bool isTriggerRight = MR::testCorePadTriggerRight(0) || MR::testSubPadStickTriggerRight(0);
			bool isTriggerLeft = MR::testCorePadTriggerLeft(0) || MR::testSubPadStickTriggerLeft(0);
			bool isHoldRight = mCounterDelay == 0 && MR::testSubPadButtonZ(0) && (MR::testCorePadButtonRight(0) || (MR::getPlayerStickX() > 0.2f && !MR::isNearZero(MR::getPlayerStickX(), 0.2f)));
			bool isHoldLeft = mCounterDelay == 0 && MR::testSubPadButtonZ(0) && (MR::testCorePadButtonLeft(0) || (MR::getPlayerStickX() < 0.2f && !MR::isNearZero(MR::getPlayerStickX(), 0.2f)));
			bool isValidRight = isTriggerRight || isHoldRight;
			bool isValidLeft = isTriggerLeft || isHoldLeft;

			if (MR::getPlayerTriggerA() || mIsNeedAutoPlay)
			{
				if (!mIsNeedAutoPlay)
					pulsePane("PlayerPlay");
				mIsNeedAutoPlay = false;
				MR::startSystemSE(JAudioID, 0x64, -1);
			}
			else if (MR::getPlayerTriggerB() || isValidRight || isValidLeft)
			{
				MR::stopSystemSE(JAudioID, 0);
				if (MR::getPlayerTriggerB())
					pulsePane("PlayerStop");
			}
			if (isValidRight)
			{
				mIter->mIndex++;
				if (mIter->mIndex >= MR::getCsvDataElementNum(mIter->mInfo))
				{
					mIter->mIndex = 0;
					mCounterDelay = 10;
				}
				else
					mCounterDelay = 5;
				mIsNeedAutoPlay = true;
				pulsePane("PlayerNext");
				pulsePane("PlayerSound");
				pulsePane("PlayerID");
			}
			else if (isValidLeft)
			{
				mIter->mIndex--;
				if (mIter->mIndex < 0)
				{
					mIter->mIndex = MR::getCsvDataElementNum(mIter->mInfo) - 1;
					mCounterDelay = 10;
				}
				else
					mCounterDelay = 5;
				mIsNeedAutoPlay = true;
				pulsePane("PlayerPrev");
				pulsePane("PlayerSound");
				pulsePane("PlayerID");
			}
		}

		TryChangeScene:
		if (MR::testCorePadTriggerPlus(0))
		{
			mChangeSceneDir = 1;
			MR::startAnim(mLayout, "End", 0);
			pulsePane("PlayerCategN");
			if (JAudioID.id != 0xFFFFFFFF)
				MR::stopSystemSE(JAudioID, 60);
			mState = SoundPlayerState_Exit;
		}
		else if (MR::testCorePadTriggerMinus(0))
		{
			mChangeSceneDir = -1;
			MR::startAnim(mLayout, "End", 0);
			pulsePane("PlayerCategP");
			if (JAudioID.id != 0xFFFFFFFF)
				MR::stopSystemSE(JAudioID, 60);
			mState = SoundPlayerState_Exit;
		}
		return;
	}
	if (mState == SoundPlayerState_Exit)
	{
		if (MR::isAnimStopped(mLayout, 0))
		{
			MR::closeSystemWipeMario(60);
			mState = SoundPlayerState_ChangeScene;
		}
	}
	if (mState == SoundPlayerState_ChangeScene)
	{
		if (MR::isSystemWipeActive())
			return;
		performChangeStage(mChangeSceneDir);
	}
}

void SoundTestObj::performChangeStage(s32 dir)
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

void SoundTestObj::pulsePane(const char* paneName)
{
	MR::startPaneAnim(mLayout, paneName, "Push", 0);
}



SoundPlayerLayout::SoundPlayerLayout() : LayoutActor("SoundPlayerLayout", true)
{

}

void SoundPlayerLayout::init(const JMapInfoIter& rIter)
{
	MR::connectToSceneLayout(this);
	initLayoutManager("SoundPlayer", 2);

	MR::setTextBoxMessageRecursive(this, "ShaPlayerTitle", MR::getCurrentGalaxyNameOnCurrentLanguage());
	MR::setTextBoxMessageRecursive(this, "TxtPlayerCateg", MR::getCurrentScenarioNameOnCurrentLanguage());
	MR::setTextBoxMessageRecursive(this, "TxtPlayerSound", L"--------");
	MR::setTextBoxMessageRecursive(this, "TxtPlayerID", L"--------");
	MR::registerDemoSimpleCastAll(this);

	MR::createAndAddPaneCtrl(this, "PlayerSound", 1);
	MR::createAndAddPaneCtrl(this, "PlayerID", 1);
	MR::createAndAddPaneCtrl(this, "PlayerPlay", 1);
	MR::createAndAddPaneCtrl(this, "PlayerStop", 1);
	MR::createAndAddPaneCtrl(this, "PlayerPrev", 1);
	MR::createAndAddPaneCtrl(this, "PlayerNext", 1);
	MR::createAndAddPaneCtrl(this, "PlayerCategN", 1);
	MR::createAndAddPaneCtrl(this, "PlayerCategP", 1);
}