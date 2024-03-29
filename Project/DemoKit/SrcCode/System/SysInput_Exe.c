/*
    System Input Callback

    System Callback for Input Module.

    @file       SysInput_Exe.c
    @ingroup    mIPRJSYS

    @note

    Copyright   Novatek Microelectronics Corp. 2010.  All rights reserved.
*/

////////////////////////////////////////////////////////////////////////////////
#include "SysCommon.h"
#include "AppCommon.h"
////////////////////////////////////////////////////////////////////////////////
#include "UIFrameworkExt.h"
#include "UICommon.h"
#include "AppLib.h"
#include "GxInput.h"
#include "GxPower.h"
#include "WifiAppCmd.h" 
#include "NvtSystem.h"
//global debug level: PRJ_DBG_LVL
#include "PrjCfg.h"
#if (UI_STYLE==UI_STYLE_DEMO)
#include "Demo_Sound.h"
#else
#include "SoundData.h"
#endif
//local debug level: THIS_DBGLVL
#define THIS_DBGLVL         1 //0=OFF, 1=ERROR, 2=TRACE
///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          SysInputExe
#define __DBGLVL__          ((THIS_DBGLVL>=PRJ_DBG_LVL)?THIS_DBGLVL:PRJ_DBG_LVL)
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "DebugModule.h"
///////////////////////////////////////////////////////////////////////////////
#include "Adc.h"
#include "UIFlowMovieFuncs.h"
#include "UIMenuWndWiFiModuleLink.h"
#include "UIMenuWndWiFiWait.h"

//#NT#Refine code for continue key
#define    BURSTKEY_DEBOUNCE     800//ms
#define    BURSTKEY_INTERVAL     200//ms
#define    TOUCH_TIMER_CNT         2//2X20ms=40ms
#define    DOUBLECLICK_INTERVAL     500//ms

int SX_TIMER_DET_KEY_ID = -1;
int SX_TIMER_DET_TOUCH_ID = -1;
int SX_TIMER_DET_PWR_ID = -1;
int SX_TIMER_DET_MODE_ID = -1;
int SX_TIMER_AUTO_INPUT_ID = -1;

void UI_DetPwrKey(void);
void UI_DetNormalKey(void);
void UI_DetStatusKey(void);

#if (POWERKEY_FUNCTION == ENABLE)
SX_TIMER_ITEM(Input_DetPKey, UI_DetPwrKey, 5, FALSE)
#endif
#if (NORMALKEY_FUNCTION == ENABLE)
SX_TIMER_ITEM(Input_DetNKey, UI_DetNormalKey, 1, FALSE)
#endif
#if (STATUSKEY_FUNCTION == ENABLE)
SX_TIMER_ITEM(Input_DetSKey, UI_DetStatusKey, 7, FALSE)
#endif
#if (TOUCH_FUNCTION == ENABLE)
SX_TIMER_ITEM(Input_DetTP, GxTouch_DetTP, TOUCH_TIMER_CNT, FALSE)
#endif
#if (AUTOINPUT_FUNCTION == ENABLE)
SX_TIMER_ITEM(Input_FeedKey, AutoInput, AUTO_INPUT_TIMER_CNT, FALSE)
#endif

static BOOL         m_uiAnyKeyUnlockEn        = FALSE;

#if (TOUCH_FUNCTION == ENABLE)
static TOUCH_OBJ g_TouchTable[] =
{
    {TP_GESTURE_PRESS,         NVTEVT_PRESS,        PLAYSOUND_SOUND_KEY_TONE},
    {TP_GESTURE_MOVE,          NVTEVT_MOVE,         0},
    {TP_GESTURE_RELEASE,       NVTEVT_RELEASE,      PLAYSOUND_SOUND_KEY_TONE},
    {TP_GESTURE_CLICK,         NVTEVT_CLICK,        0},
    {TP_GESTURE_SLIDE_LEFT,    NVTEVT_SLIDE_LEFT,   0},
    {TP_GESTURE_SLIDE_RIGHT,   NVTEVT_SLIDE_RIGHT,  0},
    {TP_GESTURE_SLIDE_UP,      NVTEVT_SLIDE_UP,     0},
    {TP_GESTURE_SLIDE_DOWN,    NVTEVT_SLIDE_DOWN,   0}
};
#endif


#if (UI_STYLE==UI_STYLE_DEMO)
static KEY_OBJ g_KeyTable[] =
{
    //POWER KEY
    {FLGKEY_KEY_POWER,    KEY_RELEASE,     NVTEVT_KEY_POWER_REL,        0,                            0},
    //NORMAL KEY
    {FLGKEY_ENTER,        KEY_PRESS,       NVTEVT_KEY_ENTER,            0,     PLAYSOUND_SOUND_KEY_TONE},
    {FLGKEY_MENU,         KEY_PRESS,       NVTEVT_KEY_MENU,             0,     PLAYSOUND_SOUND_KEY_TONE},
    {FLGKEY_MODE,         KEY_PRESS,       NVTEVT_KEY_MODE,             0,     PLAYSOUND_SOUND_KEY_TONE},
    {FLGKEY_PLAYBACK,     KEY_PRESS,       NVTEVT_KEY_PLAYBACK,         0,     PLAYSOUND_SOUND_KEY_TONE},
    {FLGKEY_MOVIE,        KEY_PRESS,       NVTEVT_KEY_MOVIE,            0,     PLAYSOUND_SOUND_KEY_TONE},
    {FLGKEY_DEL,          KEY_PRESS,       NVTEVT_KEY_I,                0,     PLAYSOUND_SOUND_KEY_TONE},
    {FLGKEY_UP,           KEY_PRESS,       NVTEVT_KEY_UP,               0,     PLAYSOUND_SOUND_KEY_TONE},
    {FLGKEY_UP,           KEY_CONTINUE,    NVTEVT_KEY_UP,               0,     PLAYSOUND_SOUND_KEY_TONE},
    {FLGKEY_UP,           KEY_RELEASE,     NVTEVT_KEY_UP_REL,           0,     PLAYSOUND_SOUND_KEY_TONE},
    {FLGKEY_DOWN,         KEY_PRESS,       NVTEVT_KEY_DOWN,             0,     PLAYSOUND_SOUND_KEY_TONE},
    {FLGKEY_DOWN,         KEY_CONTINUE,    NVTEVT_KEY_DOWN,             0,     PLAYSOUND_SOUND_KEY_TONE},
    {FLGKEY_DOWN,         KEY_RELEASE,     NVTEVT_KEY_DOWN_REL,         0,     PLAYSOUND_SOUND_KEY_TONE},
    {FLGKEY_LEFT,         KEY_PRESS,       NVTEVT_KEY_LEFT,             0,     PLAYSOUND_SOUND_KEY_TONE},
    {FLGKEY_LEFT,         KEY_CONTINUE,    NVTEVT_KEY_LEFT,             0,     PLAYSOUND_SOUND_KEY_TONE},
    {FLGKEY_LEFT,         KEY_RELEASE,     NVTEVT_KEY_LEFT_REL,         0,     PLAYSOUND_SOUND_KEY_TONE},
    {FLGKEY_RIGHT,        KEY_PRESS,       NVTEVT_KEY_RIGHT,            0,     PLAYSOUND_SOUND_KEY_TONE},
    {FLGKEY_RIGHT,        KEY_CONTINUE,    NVTEVT_KEY_RIGHT,            0,     PLAYSOUND_SOUND_KEY_TONE},
    {FLGKEY_RIGHT,        KEY_RELEASE,     NVTEVT_KEY_RIGHT_REL,        0,     PLAYSOUND_SOUND_KEY_TONE},
    {FLGKEY_DOWN,         KEY_PRESS,       NVTEVT_KEY_DOWN,             0,     PLAYSOUND_SOUND_KEY_TONE},
    {FLGKEY_DOWN,         KEY_CONTINUE,    NVTEVT_KEY_DOWN,             0,     PLAYSOUND_SOUND_KEY_TONE},
    {FLGKEY_DOWN,         KEY_RELEASE,     NVTEVT_KEY_DOWN_REL,         0,     PLAYSOUND_SOUND_KEY_TONE},
    {FLGKEY_ZOOMIN,       KEY_PRESS,       NVTEVT_KEY_ZOOMIN,           0,     PLAYSOUND_SOUND_KEY_TONE},
    {FLGKEY_ZOOMIN,       KEY_CONTINUE,    NVTEVT_KEY_ZOOMIN,           0,     PLAYSOUND_SOUND_KEY_TONE},
    {FLGKEY_ZOOMIN,       KEY_RELEASE,     NVTEVT_KEY_ZOOMRELEASE,      0,     PLAYSOUND_SOUND_KEY_TONE},
    {FLGKEY_ZOOMOUT,      KEY_PRESS,       NVTEVT_KEY_ZOOMOUT,          0,     PLAYSOUND_SOUND_KEY_TONE},
    {FLGKEY_ZOOMOUT,      KEY_CONTINUE,    NVTEVT_KEY_ZOOMOUT,          0,     PLAYSOUND_SOUND_KEY_TONE},
    {FLGKEY_ZOOMOUT,      KEY_RELEASE,     NVTEVT_KEY_ZOOMRELEASE,      0,     PLAYSOUND_SOUND_KEY_TONE},
    {FLGKEY_SHUTTER2,     KEY_PRESS,       NVTEVT_KEY_SHUTTER2,         0,     PLAYSOUND_SOUND_KEY_TONE},
    {FLGKEY_SHUTTER2,     KEY_RELEASE,     NVTEVT_KEY_SHUTTER2_REL,     0,     PLAYSOUND_SOUND_KEY_TONE},
    {FLGKEY_SHUTTER1,     KEY_PRESS,       NVTEVT_KEY_SHUTTER1,         0,     PLAYSOUND_SOUND_KEY_TONE},
    {FLGKEY_SHUTTER1,     KEY_RELEASE,     NVTEVT_KEY_SHUTTER1_REL,     0,     PLAYSOUND_SOUND_KEY_TONE},
#if (STATUSKEY_FUNCTION == ENABLE)
    //STATUS KEY
    {STATUS_KEY_GROUP1,   STATUS_KEY_LVL_1, NVTEVT_KEY_STATUS1,         0,     0},
    {STATUS_KEY_GROUP1,   STATUS_KEY_LVL_2, NVTEVT_KEY_STATUS2,         0,     0},
    {STATUS_KEY_GROUP1,   STATUS_KEY_LVL_3, NVTEVT_KEY_STATUS3,         0,     0},
    {STATUS_KEY_GROUP1,   STATUS_KEY_LVL_4, NVTEVT_KEY_STATUS4,         0,     0},
    {STATUS_KEY_GROUP1,   STATUS_KEY_LVL_5, NVTEVT_KEY_STATUS5,         0,     0},
    {STATUS_KEY_GROUP1,   STATUS_KEY_LVL_6, NVTEVT_KEY_STATUS6,         0,     0},
    {STATUS_KEY_GROUP1,   STATUS_KEY_LVL_7, NVTEVT_KEY_STATUS7,         0,     0},
    {STATUS_KEY_GROUP1,   STATUS_KEY_LVL_8, NVTEVT_KEY_STATUS8,         0,     0},
    {STATUS_KEY_GROUP1,   STATUS_KEY_LVL_9, NVTEVT_KEY_STATUS9,         0,     0},
    {STATUS_KEY_GROUP1,   STATUS_KEY_LVL_10,NVTEVT_KEY_STATUS10,        0,     0}
#endif
};
#else
static KEY_OBJ g_KeyTable[] =
{
    //POWER KEY
    {FLGKEY_KEY_POWER,    KEY_RELEASE,     NVTEVT_KEY_POWER_REL,        0,                0},
    //NORMAL KEY
    {FLGKEY_ENTER,        KEY_PRESS,       NVTEVT_KEY_ENTER,        NVTEVT_KEY_PRESS,     DEMOSOUND_SOUND_KEY_TONE},
    {FLGKEY_MENU,         KEY_PRESS,       NVTEVT_KEY_MENU,         NVTEVT_KEY_PRESS,     DEMOSOUND_SOUND_KEY_TONE},
    {FLGKEY_MODE,         KEY_PRESS,       NVTEVT_KEY_MODE,         NVTEVT_KEY_PRESS,     DEMOSOUND_SOUND_KEY_TONE},
    {FLGKEY_PLAYBACK,     KEY_PRESS,       NVTEVT_KEY_PLAYBACK,     NVTEVT_KEY_PRESS,     DEMOSOUND_SOUND_KEY_TONE},
    {FLGKEY_UP,           KEY_PRESS,       NVTEVT_KEY_UP,           NVTEVT_KEY_PRESS,     DEMOSOUND_SOUND_KEY_TONE},
    {FLGKEY_UP,           KEY_CONTINUE,    NVTEVT_KEY_UP,           NVTEVT_KEY_CONTINUE,     0},
    {FLGKEY_UP,           KEY_RELEASE,     NVTEVT_KEY_UP,           NVTEVT_KEY_RELEASE,     0},
    {FLGKEY_DOWN,         KEY_PRESS,       NVTEVT_KEY_DOWN,         NVTEVT_KEY_PRESS,     DEMOSOUND_SOUND_KEY_TONE},
    {FLGKEY_DOWN,         KEY_CONTINUE,    NVTEVT_KEY_DOWN,         NVTEVT_KEY_CONTINUE,     0},
    {FLGKEY_DOWN,         KEY_RELEASE,     NVTEVT_KEY_DOWN,         NVTEVT_KEY_RELEASE,     0},
    {FLGKEY_LEFT,         KEY_PRESS,       NVTEVT_KEY_LEFT,         NVTEVT_KEY_PRESS,     DEMOSOUND_SOUND_KEY_TONE},
    {FLGKEY_LEFT,         KEY_CONTINUE,    NVTEVT_KEY_LEFT,         NVTEVT_KEY_CONTINUE,     0},
    {FLGKEY_LEFT,         KEY_RELEASE,     NVTEVT_KEY_LEFT,         NVTEVT_KEY_RELEASE,     0},
    {FLGKEY_RIGHT,        KEY_PRESS,       NVTEVT_KEY_RIGHT,        NVTEVT_KEY_PRESS,     DEMOSOUND_SOUND_KEY_TONE},
    {FLGKEY_RIGHT,        KEY_CONTINUE,    NVTEVT_KEY_RIGHT,        NVTEVT_KEY_CONTINUE,     0},
    {FLGKEY_RIGHT,        KEY_RELEASE,     NVTEVT_KEY_RIGHT,        NVTEVT_KEY_RELEASE,     0},
    {FLGKEY_DOWN,         KEY_PRESS,       NVTEVT_KEY_DOWN,         NVTEVT_KEY_PRESS,     DEMOSOUND_SOUND_KEY_TONE},
    {FLGKEY_DOWN,         KEY_CONTINUE,    NVTEVT_KEY_DOWN,         NVTEVT_KEY_CONTINUE,     0},
    {FLGKEY_DOWN,         KEY_RELEASE,     NVTEVT_KEY_DOWN,         NVTEVT_KEY_RELEASE,     0},
    {FLGKEY_ZOOMIN,       KEY_PRESS,       NVTEVT_KEY_ZOOMIN,       NVTEVT_KEY_PRESS,     DEMOSOUND_SOUND_KEY_TONE},
    {FLGKEY_ZOOMIN,       KEY_CONTINUE,    NVTEVT_KEY_ZOOMIN,       NVTEVT_KEY_CONTINUE,     0},
    {FLGKEY_ZOOMIN,       KEY_RELEASE,     NVTEVT_KEY_ZOOMIN,       NVTEVT_KEY_RELEASE,     0},
    {FLGKEY_ZOOMOUT,      KEY_PRESS,       NVTEVT_KEY_ZOOMOUT,      NVTEVT_KEY_PRESS,     DEMOSOUND_SOUND_KEY_TONE},
    {FLGKEY_ZOOMOUT,      KEY_CONTINUE,    NVTEVT_KEY_ZOOMOUT,      NVTEVT_KEY_CONTINUE,     0},
    {FLGKEY_ZOOMOUT,      KEY_RELEASE,     NVTEVT_KEY_ZOOMOUT,      NVTEVT_KEY_RELEASE,     0},
    {FLGKEY_SHUTTER2,     KEY_PRESS,       NVTEVT_KEY_SHUTTER2,     NVTEVT_KEY_PRESS,     DEMOSOUND_SOUND_KEY_TONE},
    {FLGKEY_SHUTTER2,     KEY_RELEASE,     NVTEVT_KEY_SHUTTER2,     NVTEVT_KEY_RELEASE,     0},
    {FLGKEY_SHUTTER1,     KEY_PRESS,       NVTEVT_KEY_SHUTTER1,     NVTEVT_KEY_PRESS,     DEMOSOUND_SOUND_KEY_TONE},
    {FLGKEY_SHUTTER1,     KEY_RELEASE,     NVTEVT_KEY_SHUTTER1,     NVTEVT_KEY_RELEASE,     0},
};
#endif
void KeySoundCB(UINT32 uiSoundID)
{
    if(uiSoundID)
    {
        UISound_Play(uiSoundID);
    }
}

UINT32 Input_GroupStatus2Event(UINT32 status)
{
    UINT32 i=0;
    for(i=0;i<sizeof(g_KeyTable)/sizeof(KEY_OBJ);i++)
    {
    if( (g_KeyTable[i].uiKeyFlag == STATUS_KEY_GROUP1) && (g_KeyTable[i].status == status))
        return g_KeyTable[i].uiKeyEvent;
    }
    return 0;
}

/**
  convert GPIO key to UI key event
  [InputCB internal API]

  @param UINT32 keys: Input key code detected from GPIO mapping
  @return UINT32: NVTEVT
**/
#if 0
static UINT32 Input_Key2Evt(UINT32 keys)
{
    if(keys & FLGKEY_MOVIE)
        return NVTEVT_KEY_MOVIE;
    if(keys & FLGKEY_I)
        return NVTEVT_KEY_I;
    if(keys & FLGKEY_MODE)
        return NVTEVT_KEY_MODE;
    if(keys & FLGKEY_PLAYBACK)
        return NVTEVT_KEY_PLAYBACK;
    if(keys & FLGKEY_MENU)
        return NVTEVT_KEY_MENU;
    if(keys & FLGKEY_FACEDETECT)
        return NVTEVT_KEY_FACEDETECT;
    if(keys & FLGKEY_DEL)
        return NVTEVT_KEY_DEL;
    if(keys & FLGKEY_LEFT)
        return NVTEVT_KEY_LEFT;
    if(keys & FLGKEY_RIGHT)
        return NVTEVT_KEY_RIGHT;
    if(keys & FLGKEY_ENTER)
        return NVTEVT_KEY_ENTER;
    if(keys & FLGKEY_SHUTTER1)
        return NVTEVT_KEY_SHUTTER1;
    if(keys & FLGKEY_SHUTTER2)
        return NVTEVT_KEY_SHUTTER2;
    if(keys & FLGKEY_ZOOMOUT)
        return NVTEVT_KEY_ZOOMOUT;
    if(keys & FLGKEY_ZOOMIN)
        return NVTEVT_KEY_ZOOMIN;
    if(keys & FLGKEY_UP)
        return NVTEVT_KEY_UP;
    if(keys & FLGKEY_DOWN)
        return NVTEVT_KEY_DOWN;
    else
        return NVTEVT_NULL;
}
#endif
//just for backward compatible
void Input_SetKeyMask(KEY_STATUS uiMode, UINT32 uiMask)
{
    SysMan_SetKeyMask(uiMode, uiMask);
}
UINT32 Input_GetKeyMask(KEY_STATUS uiMode)
{
    return SysMan_GetKeyMask(uiMode);
}
void Input_SetKeySoundMask(KEY_STATUS uiMode, UINT32 uiMask)
{
    SysMan_SetKeySoundMask(uiMode, uiMask);
}
UINT32 Input_GetKeySoundMask(KEY_STATUS uiMode)
{
    return SysMan_GetKeySoundMask(uiMode);
}
#if 0
void Input_SetTouchMask(UINT32 uiMask)
{
#if (TOUCH_FUNCTION == ENABLE)
    uiTouchMask = uiMask;
#endif
}
#endif
/**
    reset all mask,usually in new winodw onOpen
*/
void Input_ResetMask(void)
{
    // Set key mask as default
    SysMan_SetKeyMask(KEY_PRESS, FLGKEY_KEY_MASK_DEFAULT);
    SysMan_SetKeyMask(KEY_RELEASE, FLGKEY_KEY_MASK_DEFAULT);
    SysMan_SetKeyMask(KEY_CONTINUE, FLGKEY_KEY_MASK_NULL);
#if (TOUCH_FUNCTION == ENABLE)
    // Set touch mask as default
    SysMan_SetTouchMask(TOUCH_MASK_DEFAULT);
#endif
    SysMan_SetKeySoundMask(KEY_PRESS, FLGKEY_SOUND_MASK_DEFAULT);
    SysMan_SetKeySoundMask(KEY_RELEASE, FLGKEY_KEY_MASK_NULL);
    SysMan_SetKeySoundMask(KEY_CONTINUE, FLGKEY_SOUND_MASK_DEFAULT);
}
/**
    for some case,press any key unlock all,and post NVTEVT_KEY_PRESS
    ex:Timelapse or smile detect
*/
void Input_SetAnyKeyUnlock(BOOL en)
{
    m_uiAnyKeyUnlockEn = en;
}


UINT32 Input_Key2Mode(UINT32 keys)
{
#if (STATUSKEY_FUNCTION == ENABLE)
    switch (keys)
    {
        case NVTEVT_KEY_STATUS1: return DSC_MODE_MOVIE;
        case NVTEVT_KEY_STATUS2: return DSC_MODE_PHOTO_MANUAL;
        case NVTEVT_KEY_STATUS3: return DSC_MODE_PHOTO_MANUAL;
        case NVTEVT_KEY_STATUS4: return DSC_MODE_PHOTO_MANUAL;
        case NVTEVT_KEY_STATUS5: return DSC_MODE_PHOTO_MANUAL;
        case NVTEVT_KEY_STATUS6: return DSC_MODE_PHOTO_SCENE;
        case NVTEVT_KEY_STATUS7: return DSC_MODE_PHOTO_MANUAL;
        case NVTEVT_KEY_STATUS8: return DSC_MODE_PHOTO_MANUAL;
        case NVTEVT_KEY_STATUS9: return DSC_MODE_PHOTO_MANUAL;
        case NVTEVT_KEY_STATUS10: return DSC_MODE_PHOTO_MANUAL;
        default: return DSC_MODE_PHOTO_AUTO;
    }
#else
    return 0;
#endif
}
extern SX_CMD_ENTRY key[];
void System_OnInputInit(void)
{
    //PHASE-1 : Init & Open Drv or DrvExt
    {
        GxKey_RegCB(Key_CB);         //Register CB function of GxInput
        #if (TOUCH_FUNCTION == ENABLE)
        GxTouch_RegCB(Touch_CB);     //Register CB function of GxInput
        #endif
        GxKey_Init();
        SxCmd_AddTable(key);
    }
    //PHASE-2 : Init & Open Lib or LibExt
    {
        //1.設定init值
        //2.設定CB值,
        GxKey_SetContDebounce(BURSTKEY_DEBOUNCE/SxTimer_GetData(SXTIMER_TIMER_BASE));
        GxKey_SetRepeatInterval(BURSTKEY_INTERVAL/SxTimer_GetData(SXTIMER_TIMER_BASE));
        SysMan_RegKeySoundCB(KeySoundCB);
        SysMan_RegKeyTable(g_KeyTable, sizeof(g_KeyTable)/sizeof(KEY_OBJ));
        //3.註冊SxJob服務 ---------> System Job
        //4.註冊SxTimer服務 ---------> Detect Job

        #if (POWERKEY_FUNCTION == ENABLE)
        SX_TIMER_DET_PWR_ID = SxTimer_AddItem(&Timer_Input_DetPKey);
        #endif
        #if (NORMALKEY_FUNCTION == ENABLE)
        SX_TIMER_DET_KEY_ID = SxTimer_AddItem(&Timer_Input_DetNKey);
        #endif
        #if (STATUSKEY_FUNCTION == ENABLE)
        SX_TIMER_DET_MODE_ID = SxTimer_AddItem(&Timer_Input_DetSKey);
        #endif
        #if (TOUCH_FUNCTION == ENABLE)
        SX_TIMER_DET_TOUCH_ID = SxTimer_AddItem(&Timer_Input_DetTP);
        #endif
        #if (AUTOINPUT_FUNCTION == ENABLE)
        SX_TIMER_AUTO_INPUT_ID = SxTimer_AddItem(&Timer_Input_FeedKey);
        #endif

#if _MIPS_TODO
        if(FlowMode_IsPowerOnPlayback())
        {
            GxKey_SetFirstKeyInvalid(KEY_PRESS, FLGKEY_PLAYBACK);
        }
#endif
        #if (STATUSKEY_FUNCTION == ENABLE)
        GxKey_DetStatusKey();
        #endif

        #if (POWERKEY_FUNCTION == ENABLE)
        SxTimer_SetFuncActive(SX_TIMER_DET_PWR_ID, TRUE);
        #endif
        #if (NORMALKEY_FUNCTION == ENABLE)
        SxTimer_SetFuncActive(SX_TIMER_DET_KEY_ID, TRUE);
        #endif
        #if (STATUSKEY_FUNCTION == ENABLE)
        SxTimer_SetFuncActive(SX_TIMER_DET_MODE_ID, TRUE);
        #endif

        #if (TOUCH_FUNCTION == ENABLE)
        //1.設定init值
        GxTouch_Init();
        GxTouch_SetCtrl(GXTCH_DOUBLE_CLICK_INTERVAL,
            DOUBLECLICK_INTERVAL/TOUCH_TIMER_CNT/SxTimer_GetData(SXTIMER_TIMER_BASE));
        SysMan_RegTouchSoundCB(KeySoundCB);
        SysMan_RegTouchTable(g_TouchTable, sizeof(g_TouchTable)/sizeof(TOUCH_OBJ));
        //2.設定CB值,
        //3.註冊SxJob服務 ---------> System Job
        //4.註冊SxTimer服務 ---------> Detect Job
        //SX_TIMER_DET_TOUCH_ID = System_AddSxTimer(GxTouch_DetTP, TOUCH_TIMER_CNT, "GxTouch_DetTP", TRUE);
        //SX_TIMER_DET_TOUCH_ID = SxTimer_AddItem(&Timer_Input_DetTP);
        //5.註冊SxCmd服務 ---------> Cmd Function
        //System_AddSxCmd(Touch_OnCommand); //GxInput

        SxTimer_SetFuncActive(SX_TIMER_DET_TOUCH_ID, TRUE);
        #endif
    }
}

void System_OnInputExit(void)
{
    //PHASE-2 : Close Lib or LibExt
    {
    }
    //PHASE-1 : Close Drv or DrvExt
    {
    }
}

/////////////////////////////////////////////////////////////////////////////

#define KEYSCAN_POWEROFF_COUNT  50 // 1 sec in unit of 100 ms
#define KEYSCAN_COUNT  70 // 1 sec in unit of 20 ms
#define KEYSCAN_PWROFF_INIT_STATE       0
#define KEYSCAN_PWROFF_RELEASE_STATE    1
#define KEYSCAN_PWROFF_PRESS_STATE      2

#define KEYSCAN_PWRKEY_UNKNOWN          0xFFFFFFFF
#define KEYSCAN_PWRKEY_RELEASED         0
#define KEYSCAN_PWRKEY_PRESSED          1


void MY_DetPWRKey(UINT32 uiPostKey)
{
    static UINT32 uiKeyPressCount = 0;
    static UINT32 uiKey    = KEYSCAN_PWRKEY_UNKNOWN;
    static UINT32 uiState    = KEYSCAN_PWROFF_INIT_STATE;
    static BOOL   bPress = FALSE;
    UINT8 RxBuf[1]={0};
                                    UINT8 x;

	if(rtc_getPWRStatus())
	{
	// Reset shutdown timer
        rtc_resetShutdownTimer();
        // Debounce
        if (uiKey == KEYSCAN_PWRKEY_PRESSED)
        {
            if (uiState == KEYSCAN_PWROFF_RELEASE_STATE)
            {
                uiKeyPressCount++; 
                if (uiKeyPressCount > (20))//60
                {
                    uiState = KEYSCAN_PWROFF_PRESS_STATE;
                }
            }

            if (uiState == KEYSCAN_PWROFF_PRESS_STATE)
			{
				uiKeyPressCount = 0;
				bPress = TRUE;
				uiKey    = KEYSCAN_PWRKEY_UNKNOWN;
				uiState    = KEYSCAN_PWROFF_INIT_STATE;
				//System_PowerOff(SYS_POWEROFF_NORMAL);
				UISound_Play(DEMOSOUND_SOUND_KEY_TONE);
                 #if 1
                                if(Getwifi_statue())
                                    {
    				
/*
                                            if(UI_GetData(FL_WIFI_LINK)==WIFI_LINK_OK)
                                            {   //change to normal mode for close app
                                                Ux_PostEvent(NVTEVT_SYSTEM_MODE, 2, System_GetState(SYS_STATE_CURRMODE),SYS_SUBMODE_NORMAL);
                                                //should close network application,then stop wifi
                                                Ux_PostEvent(NVTEVT_EXE_WIFI_STOP, 0);
                                            }
                                            else
                                            {
                                                Ux_CloseWindow(&UIMenuWndWiFiModuleLinkCtrl,0);
												//GxLED_SetCtrl(KEYSCAN_LED_GREEN,SET_TOGGLE_LED,FALSE);  
                                            }
                                            */

                                            WifiApp_SendCmd(WIFIAPP_CMD_NOTIFY_STATUS, WIFIAPP_RET_DISCONNECT);
                                            WifiApp_SendCmd(WIFIAPP_CMD_NOTIFY_STATUS, WIFIAPP_RET_REMOVE_BY_USER);
                                            if (UIFlowWndWiFiMovie_GetStatus() == WIFI_MOV_ST_RECORD)
                    				{
                    				 FlowWiFiMovie_StopRec();
                                            FlowWiFiMovie_StartRec(WIFI_RTSP_LIVEVIEW);
                    				}
                                            if(UI_GetData(FL_WIFI_LINK)==WIFI_LINK_OK||Getwifi_statue())
                                            {   //change to normal mode for close app
                                                Ux_PostEvent(NVTEVT_SYSTEM_MODE, 2, System_GetState(SYS_STATE_CURRMODE),SYS_SUBMODE_NORMAL);
                                                //should close network application,then stop wifi
                                                Ux_PostEvent(NVTEVT_EXE_WIFI_STOP, 0);
                                                //SetWIFI_PWR_Gpio(FALSE);
                                            }
                                            
                                            //if(!GetTV_IN())
                                            // SetTV_IN(TRUE);
                                             
                                    }
                                else
                                    {
                                            if(gMovData.State==MOV_ST_REC)
                                                {
                                                FlowMovie_StopRec();
                                                }
                                             //SetWIFI_PWR_Gpio(TRUE);
                                            // GxSystem_Reboot();
                                             //if(GetTV_IN())
                                            // SetTV_IN(FALSE);
                                             
                                            Ux_OpenWindow(&UIMenuWndWiFiWaitCtrl, 0);
                                                BKG_PostEvent(NVTEVT_BKW_WIFI_ON);
                                                
                                    }
								//GxLED_SetCtrl(KEYSCAN_LED_GREEN,TURNON_LED,TRUE);
				#endif
				 
			}
		}
        else
        {
            uiKey = KEYSCAN_PWRKEY_PRESSED;
        }
	}
    else
    {
        // Debounce
        if (uiKey == KEYSCAN_PWRKEY_RELEASED)
        {
            if (uiState == KEYSCAN_PWROFF_INIT_STATE)
            {
                uiState = KEYSCAN_PWROFF_RELEASE_STATE;
            }

            if (uiKeyPressCount)
            {
				uiKeyPressCount = 0;
				bPress = TRUE;
				//MY_ResetAutoLCDOff();
			      //System_ResetPowerSaveCount();

		            	    //UISound_Play(DEMOSOUND_SOUND_KEY_TONE);
                                //Ux_PostEvent(NVTEVT_KEY_ENTER, 1, NVTEVT_KEY_RELEASE);
                                if(gMovData.State==MOV_ST_REC)
                            {
                            FlowMovie_StopRec();
                            }
                    
				debug_msg("================shutdown!==============\r\n");
                                Ux_PostEvent(NVTEVT_KEY_POWER_REL, 1, NVTEVT_KEY_RELEASE);
                #if 0
                                if(Getwifi_statue())
                                    {
    				

                                            if(UI_GetData(FL_WIFI_LINK)==WIFI_LINK_OK)
                                            {   //change to normal mode for close app
                                                Ux_PostEvent(NVTEVT_SYSTEM_MODE, 2, System_GetState(SYS_STATE_CURRMODE),SYS_SUBMODE_NORMAL);
                                                //should close network application,then stop wifi
                                                Ux_PostEvent(NVTEVT_EXE_WIFI_STOP, 0);
                                            }
                                            else
                                            {
                                                Ux_CloseWindow(&UIMenuWndWiFiModuleLinkCtrl,0);
                                            }
                                    }
                                else
                                    {
                                             if(gMovData.State==MOV_ST_REC)
                                                {
                                                FlowMovie_StopRec();
                                                }
                                            Ux_OpenWindow(&UIMenuWndWiFiWaitCtrl, 0);
                                                BKG_PostEvent(NVTEVT_BKW_WIFI_ON);
                                    }
				#endif
            }
			else if (bPress)
			{
				bPress = FALSE;
				//Ux_PostEvent(uiPostKey, 1, NVTEVT_KEY_RELEASE);
			}
        }
        else
        {
            uiKey = KEYSCAN_PWRKEY_RELEASED;
        }
    }
}


void UI_DetPwrKey(void)
{
      if(!UI_IsForceLock())
    {
#if 0
        GxKey_DetPwrKey();
#else
        MY_DetPWRKey(0);
#endif
    }
}

void UI_DetNormalKey(void)
{
    static UINT32 keyDetectCount = 0;
    if(!UI_IsForceLock())
    {
        GxKey_DetNormalKey();

        if (keyDetectCount < 4)
        {
            keyDetectCount++;
        }

        if (keyDetectCount == 2)
        {
            //recover the key detection after system boot up
            GxKey_SetFirstKeyInvalid(KEY_PRESS, 0);
        }
    }
}

void UI_DetStatusKey(void)
{
//check mode key
    if((!UI_IsForceLock())&&(!UI_IsForceLockStatus()))
        GxKey_DetStatusKey();
}

/////////////////////////////////////////////////////////////////////////////
extern void System_ResetDetCloseLenCount(void);
extern void System_ResetPowerSaveCount(void);
BOOL g_bConsumeStatus = FALSE;

INT32 System_UserKeyFilter(NVTEVT evt, UINT32 paramNum, UINT32 *paramArray)
{
    UINT32 key = evt;
    if(IN_RANGE_EVENT(key, NVTEVT_KEY_STATUS_START, NVTEVT_KEY_STATUS_END)) //Status class
    {
        System_ResetDetCloseLenCount();
        System_ResetPowerSaveCount();
#if (STATUSKEY_FUNCTION == ENABLE)
        DBG_IND("^Bgroup key event=0x%x\r\n", key);

        if(g_bConsumeStatus)
        {
            g_bConsumeStatus = 0;
        }
        else
        {
            UINT32 uiDscMode = Input_Key2Mode(key);
            DBG_IND("^YDscMode=0x%x\r\n", uiDscMode);
            //FlowMode_OnKeyMode(uiDscMode);
            UI_Switch_DscMode(uiDscMode,DSCMODE_SWITCH_FORCE,DSCMODE_SWITCHDIR_DONT_CARE);
        }
#endif
        return NVTEVT_CONSUME;
    }
    else if(IN_RANGE_EVENT(key, NVTEVT_KEY_BUTTON_START, NVTEVT_KEY_BUTTON_END)) //Button class
    {
        if(IN_RANGE_EVENT(key, NVTEVT_KEY_PRESS_START, NVTEVT_KEY_PRESS_END)) //Press key
        {
            if(key == NVTEVT_KEY_POWER)
            {
                return NVTEVT_CONSUME;
            }

            System_ResetDetCloseLenCount();
            if(GxPower_GetControl(GXPWR_CTRL_SLEEP_LEVEL) > 1)  // drop key if sleep level > 1
            {
                //NOTE! do not set GXPWR_CTRL_SLEEP_RESET before get GXPWR_CTRL_SLEEP_LEVEL,
                //  because GXPWR_CTRL_SLEEP_LEVEL status maybe alter by sleep wakeup flow~
                System_ResetPowerSaveCount();
                return NVTEVT_CONSUME;
            }
            else
            {
                System_ResetPowerSaveCount();
                if (m_uiAnyKeyUnlockEn)
                {
                    Ux_PostEvent(NVTEVT_KEY_PRESS_START, 1, key);
                    return NVTEVT_CONSUME;
                }
                return NVTEVT_PASS;
            }
        }
        else if(IN_RANGE_EVENT(key, NVTEVT_KEY_CONTINUE_START, NVTEVT_KEY_CONTINUE_END)) //Contine key
        {
            if(key == NVTEVT_KEY_POWER_CONT)
            {
                return NVTEVT_CONSUME;
            }

            System_ResetDetCloseLenCount();
            System_ResetPowerSaveCount();
            return NVTEVT_PASS;
        }
        else if(IN_RANGE_EVENT(key, NVTEVT_KEY_RELEASE_START, NVTEVT_KEY_RELEASE_END)) //Release key
        {
            if(key == NVTEVT_KEY_POWER_REL)
            {
                System_ResetDetCloseLenCount();
                if(GxPower_GetControl(GXPWR_CTRL_SLEEP_LEVEL) > 1)  // drop key if sleep level > 1
                {
                    //NOTE! do not set GXPWR_CTRL_SLEEP_RESET before get GXPWR_CTRL_SLEEP_LEVEL,
                    //  because GXPWR_CTRL_SLEEP_LEVEL status maybe alter by sleep wakeup flow~
                    System_ResetPowerSaveCount();
                }
                else
                {
                    //g_bIsExitSystemWithPowerKey = TRUE;
                    System_PowerOff(SYS_POWEROFF_NORMAL);
                }
                return NVTEVT_CONSUME;
            }

            System_ResetDetCloseLenCount();
            System_ResetPowerSaveCount();
            return NVTEVT_PASS;
        }
    }
    return NVTEVT_PASS;
}
#if (TOUCH_FUNCTION == ENABLE)
INT32 System_UserTouchFilter(NVTEVT evt, UINT32 paramNum, UINT32 *paramArray)
{
    INT32 Ret = NVTEVT_PASS;
    //DBG_DUMP("^Bevt=%d, point=(%d, %d)\r\n", evt - NVTEVT_PRESS, paramArray[0], paramArray[1]);
    if(evt >= NVTEVT_PRESS && evt <= NVTEVT_SLIDE_DOWN)
    {
        if(GxPower_GetControl(GXPWR_CTRL_SLEEP_LEVEL) > 1)  // drop key if sleep level > 1
        {
            //NOTE! do not set GXPWR_CTRL_SLEEP_RESET before get GXPWR_CTRL_SLEEP_LEVEL,
            //  because GXPWR_CTRL_SLEEP_LEVEL status maybe alter by sleep wakeup flow~
            System_ResetPowerSaveCount();
            return NVTEVT_CONSUME;
        }
        else
        {
            System_ResetPowerSaveCount();
            return NVTEVT_PASS;
        }
    }
    return Ret;
}
#endif
