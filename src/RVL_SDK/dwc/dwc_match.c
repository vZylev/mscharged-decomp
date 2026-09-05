#include <dwc/dwc_match.h>

#include <dwc/dwc_common.h>
#include <dwc/dwc_error.h>
#include <dwc/dwc_friend.h>
#include <dwc/dwc_main.h>
#include <dwc/dwc_memfunc.h>
#include <dwc/dwc_nonport.h>
#include <dwc/dwc_report.h>
#include <dwc/dwc_transport.h>
#include <gamespy/GP/gp.h>
#include <gamespy/gt2/gt2.h>
#include <gamespy/natneg/natneg.h>
#include <gamespy/qr2/qr2.h>
#include <gamespy/serverbrowsing/sb_serverbrowsing.h>
#include <revolution/os/OSTime.h>
#include <stdio.h>
#include <string.h>

#define DWC_MATCH_COMMAND_ADD_MESSAGE_MAX 512

enum
{
    DWC_MATCH_CANCEL_STATE_INIT = 0,
    DWC_MATCH_CANCEL_STATE_EXEC,
    DWC_MATCH_CANCEL_STATE_NUM
};

enum
{
    DWC_MATCH_CLOSE_STATE_INIT = 0,
    DWC_MATCH_CLOSE_STATE_CHANGE_TO_CLIENT,
    DWC_MATCH_CLOSE_STATE_TIMEOUT,
    DWC_MATCH_CLOSE_STATE_FORCED,
    DWC_MATCH_CLOSE_STATE_NUM
};

typedef enum DWCMatchPpConnectionType
{
    DWC_PP_CONNECTION_SV_CONNECT = 0,
    DWC_PP_CONNECTION_CL_GT2_CONNECT,
    DWC_PP_CONNECTION_CL_GT2_ACCEPT,
    DWC_PP_CONNECTION_CL_FINISH_CONNECT,
    DWC_PP_CONNECTION_SYN_FINISH,
    DWC_PP_CONNECTION_NUM
} DWCMatchPpConnectionType;

typedef enum
{
    DWC_MATCH_RESET_ALL = 0,
    DWC_MATCH_RESET_RESTART,
    DWC_MATCH_RESET_CONTINUE,
    DWC_MATCH_RESET_NUM
} DWCMatchResetLevel;

typedef void (*DWCMatchedSCCallback)(DWCError error, BOOL cancel, BOOL self,
    BOOL isServer, int index, void* param);
typedef void (*DWCNewClientCallback)(int index, void* param);
typedef int (*DWCEvalPlayerCallback)(int index, void* param);

typedef struct DWCstNNInfo
{
    u8 isQR2;
    u8 retryCount;
    u16 port;
    u32 ip;
    int cookie;
} DWCNNInfo;

typedef struct DWCstConnectionInfo
{
    u8 index;
    u8 aid;
    u16 reserve;
    void* param;
} DWCConnectionInfo;

typedef struct DWCstMatchControl
{
    GPConnection* pGpObj;
    GT2Socket* pGt2Socket;
    GT2ConnectionCallbacks* gt2Callbacks;
    u8 gt2ConnectCount;
    u8 gt2NumConnection;
    u8 gt2NumValidConn;
    u8 pad1;
    qr2_t qr2Obj;
    vu8 qr2NNFinishCount;
    vu8 qr2MatchType;
    vu8 qr2NumEntry;
    vu8 qr2IsReserved;
    u8 qr2ShutdownFlag;
    u8 pad2;
    u16 qr2Port;
    u32 qr2IP;
    volatile int qr2Reservation;
    u32 qr2IPList[DWC_MAX_CONNECTIONS];
    u16 qr2PortList[DWC_MAX_CONNECTIONS];
    ServerBrowser sbObj;
    int sbUpdateFlag;
    u8 _EC[4];
    s64 sbUpdateTick;
    int sbPidList[DWC_MAX_CONNECTIONS];
    s64 sbUpdateRequestTick;
    u8 nnRecvCount;
    u8 nnFailureCount;
    u16 nnCookieRand;
    int nnLastCookie;
    s64 nnFailedTime;
    s64 nnFinishTime;
    DWCNNInfo nnInfo;
    DWCMatchState state;
    u8 clLinkProgress;
    u8 friendCount;
    u8 distantFriend;
    u8 resvWaitCount;
    u8 closeState;
    u8 cancelState;
    u8 scResvRetryCount;
    u8 synResendCount;
    u8 cancelSynResendCount;
    u8 clWaitTimeoutCount;
    u8 stopSCFlag;
    u8 pad3;
    u16 baseLatency;
    u16 cancelBaseLatency;
    u16 searchPort;
    u16 pad4;
    u32 searchIP;
    int cmdResendFlag;
    u8 _1C4[4];
    s64 cmdResendTick;
    u32 cmdTimeoutTime;
    u8 _1D4[4];
    s64 cmdTimeoutStartTick;
    u32 synAckBit;
    u32 cancelSynAckBit;
    u32 friendAcceptBit;
    u8 _1EC[4];
    s64 lastSynSent;
    s64 lastCancelSynSent;
    s64 closedTime;
    s64 clWaitTime;
    volatile int profileID;
    int reqProfileID;
    int priorProfileID;
    int cbEventPid;
    u32 ipList[32];
    u16 portList[32];
    u8 aidList[32];
    u32 validAidBitmap;
    const char* gameName;
    const char* secretKey;
    const DWCFriendData* friendList;
    int friendListLen;
    u8 friendIdxList[DWC_MAX_MATCH_IDX_LIST];
    int friendIdxListLen;
    u32 svDataBak[DWC_MAX_CONNECTIONS + 1];
    u32 _3DC;
    u8 _3E0;
    u8 _3E1;
    u16 _3E2;
    u32 _3E4;
    u8 _3E8[0x80];
    int _468;
    int _46C;
    s64 _470;
    DWCMatchedSCCallback matchedCallback;
    void* matchedParam;
    DWCNewClientCallback newClientCallback;
    void* newClientParam;
    DWCEvalPlayerCallback _488;
    void* _48C;
    void (*_490)(void* param);
    void* _494;
} DWCMatchControl;

typedef struct DWCstMatchOptSCBlock
{
    u8 valid;
    u8 lock;
    u16 pad;
} DWCMatchOptSCBlock;

GPResult DWCi_SetGPStatus(int status, const char* statusString,
    const char* locationString);
int DWCi_GetFriendListIndex(int profileID);
void DWCi_StopLogin(DWCError error, int errorCode);
void fn_8049AE0C(int type, int aid, const void* data, int size);
void fn_80499A30(SBServer server);
static int DWCi_EvaluateServers(int sort);
void fn_80499E90(void);

extern const int lbl_804F31F8[];

void SBServerAddIntKeyValue(SBServer server, const char* keyname, int value);

void fn_804970B0(void);
void DWCi_StopMatching(DWCError error, int errorCode);
static SBError DWCi_SBUpdateAsync(int profileID);
static int DWCi_GetDefaultMatchFilter(
    char* filter, int profileID, u8 numEntry, u8 matchType);
static void DWCi_ResetMatchParam(DWCMatchResetLevel level);
static GPResult DWCi_HandleGPError(GPResult result);
static SBError DWCi_HandleSBError(SBError error);
static qr2_error_t DWCi_HandleQR2Error(qr2_error_t error);
static int DWCi_HandleMatchCommandError(int error);
static int DWCi_SendResvCommand(int profileID, BOOL delay);
static BOOL DWCi_RetryReserving(int resendPid);
static int DWCi_CancelReservation(int profileID);
static BOOL DWCi_CancelPreConnectedServerProcess(int clientPid);
static BOOL DWCi_CancelPreConnectedClientProcess(int serverPid);
static int DWCi_PostProcessConnection(DWCMatchPpConnectionType type);
static int DWCi_ResumeMatching(void);
int fn_804979F4(void);
int fn_8049811C(void);
void fn_80498440(void);
static int DWCi_SendMatchCommand(u8 command, int profileID, u32 ip, u16 port,
    const u32 data[], int len);
static SBError DWCi_SendSBMsgCommand(
    u8 command, u32 ip, u16 port, const u32 data[], int len);
static GPResult DWCi_SendGPBuddyMsgCommand(GPConnection* connection,
    u8 command, int profileID, const char* message);
static void DWCi_QR2ServerKeyCallback(
    int keyid, qr2_buffer_t outbuf, void* userdata);
static void DWCi_QR2PlayerKeyCallback(
    int keyid, int index, qr2_buffer_t outbuf, void* userdata);
static void DWCi_QR2TeamKeyCallback(
    int keyid, int index, qr2_buffer_t outbuf, void* userdata);
static void DWCi_QR2KeyListCallback(
    qr2_key_type keytype, qr2_keybuffer_t keybuffer, void* userdata);
static int DWCi_QR2CountCallback(qr2_key_type keytype, void* userdata);
static void DWCi_QR2AddErrorCallback(
    qr2_error_t error, gsi_char* errmsg, void* userdata);
static void DWCi_QR2PublicAddrCallback(
    unsigned int ip, unsigned short port, void* userdata);
static void DWCi_QR2NatnegCallback(int cookie, void* userdata);
static void DWCi_QR2ClientMsgCallback(
    gsi_char* data, int len, void* userdata);

void fn_804993C8(ServerBrowser sb, SBCallbackReason reason,
    SBServer server, void* instance);
static void DWCi_NNProgressCallback(NegotiateState state, void* userdata);
static void DWCi_NNCompletedCallback(NegotiateResult result, SOCKET gamesocket,
    struct sockaddr_in* remoteaddr, void* userdata);

int DWCi_GT2GetConnectionListIdx(void);
GT2Connection* DWCi_GetGT2ConnectionByIdx(int index);
DWCConnectionInfo* DWCi_GetConnectionInfoByIdx(int index);
DWCAccUserData* DWCi_GetUserData(void);
GT2Connection* DWCi_GetGT2ConnectionByProfileID(int profileID, int numHost);
int DWCi_GetProfileIDFromList(int index);
void DWC_CloseConnectionHard(u8 aid);
static u8 DWCi_CheckResvCommand(
    int profileID, u32 qr2IP, u16 qr2Port, u32 matchType, BOOL priorFlag);
static int DWCi_ProcessResvOK(int profileID, u32 ip, u16 port);
static void DWCi_MakeBackupServerData(int profileID, const u32 data[]);
static int DWCi_SendResvCommandToFriend(
    BOOL delay, BOOL init, int resendPid);
static int DWCi_ProcessCancelMatchSynCommand(
    int profileID, u8 command, u32 data);
static void DWCi_SendMatchSynPacket(u8 aid, u16 type);
static NegotiateError DWCi_HandleNNError(NegotiateError error);
int fn_8049925C(int error);
static void DWCi_RestartFromCancel(DWCMatchResetLevel level);

void fn_804929E0(void);

GPResult fn_80492BA0(void);
int fn_80493B94(char* dstMsg, const char* srcMsg, int index);
static BOOL DWCi_ProcessRecvMatchCommand(u8 command, int srcPid, u32 srcIP,
    u16 srcPort, const u32 data[], int len);
static NegotiateError DWCi_NNStartupAsync(
    int isQR2, int cookie, SBServer server);
static NegotiateError DWCi_DoNatNegotiationAsync(DWCNNInfo* nnInfo);
BOOL fn_80492D3C(void);
static DWCMatchControl* DWCi_GetMatchCnt(void);
static void DWCi_SetMatchStatus(DWCMatchState state);
static void DWCi_StopResendingMatchCommand(void);
static void DWCi_FinishCancelMatching(void);
static void DWCi_RestartFromTimeout(void);
static BOOL DWCi_CloseCancelHostAsync(int profileID);
static void DWCi_CloseAllConnectionsByTimeout(void);
static BOOL DWCi_SendCancelMatchSynCommand(int profileID, u8 command);
static u8 DWCi_GetNewMatchKey(void);
static void DWCi_ClearGameMatchKeys(void);
static u8 DWCi_GetAIDFromList(void);
static BOOL DWCi_IsFriendByIdxList(int profileID);
static BOOL DWCi_AreAllBuddies(const u32 pidList[], u32 pidListLen);
static u8 DWCi_GetAIDFromProfileID(int profileID, BOOL idx0);
static u32 DWCi_GetAIDBitmask(BOOL valid);
static int DWCi_InvalidateReservation(void);
static void DWCi_InitClWaitTimeout(void);
static void DWCi_InitOptMinCompParam(BOOL reset);
static int DWCi_ChangeToClient(void);

static char* stpAddFilter;
static DWCMatchOptMinCompleteIn* stpOptMinComp;
static int s_sbCallbackLevel;
static int s_needSbFree;
static DWCMatchControl* stpMatchCnt;
static DWCMatchOptSCBlock stOptSCBlock;

u8 lbl_806C9920[0x100];
static DWCGameMatchKeyData stGameMatchKeys[DWC_QR2_GAME_RESERVED_KEYS];
u8 lbl_806CA158[0x20];

static u32 fn_ByteSwap32(u32 value)
{
    value = ((value >> 8) & 0x00FF00FF) | ((value << 8) & 0xFF00FF00);
    return (value >> 16) | (value << 16);
}

static s64 fn_ElapsedMSec(s64 time)
{
    return (OSGetTime() - time) / (OS_BUS_CLOCK_SPEED / 4 / 1000);
}

BOOL DWC_RegisterMatchingStatus(void)
{
    DWC_Printf(4, "!!DWC_RegisterMatchingStatus() was called!!\n");
    DWC_Printf(4, "But ignored.\n");
    return TRUE;
}

void ClearMOSCConnectBlock(void)
{
    DWC_Printf(4, "[OPT_SC_BLOCK] ClearMOSCConnectBlock\n");
    stOptSCBlock.valid = 0;
    stOptSCBlock.lock = 0;
    stOptSCBlock.pad = 0;
}

BOOL DWC_CancelMatch(void)
{
    if (DWC_IsValidMatchCancel())
    {
        fn_804970B0();
        return TRUE;
    }
    DWC_Printf(4, "Now unable to cancel.\n");
    return FALSE;
}

BOOL DWC_IsValidMatchCancel(void)
{
    if (DWCi_IsError())
    {
        return FALSE;
    }
    if (stpMatchCnt == NULL || DWC_GetState() != DWC_STATE_MATCHING
        || stpMatchCnt->cancelState != 0)
    {
        return FALSE;
    }
    return TRUE;
}

BOOL fn_8048F648(void (*callback)(void* param), void* param)
{
    if (DWCi_IsError() || stpMatchCnt == NULL
        || stpMatchCnt->qr2MatchType != 2
        || (DWC_GetState() != DWC_STATE_MATCHING
            && DWC_GetState() != DWC_STATE_CONNECTED))
    {
        return FALSE;
    }

    if (stpMatchCnt->state == 10)
    {
        stpMatchCnt->qr2NumEntry = stpMatchCnt->gt2NumConnection;
        fn_80492BA0();
        if (callback != NULL)
        {
            callback(param);
        }
    }
    else
    {
        stpMatchCnt->_490 = callback;
        stpMatchCnt->_494 = param;
        stpMatchCnt->stopSCFlag = 1;
    }
    return TRUE;
}

u8 DWC_AddMatchKeyInt(u8 keyID, const char* keyString, const int* valueSrc)
{
    int index;

    if (!keyString || !valueSrc)
    {
        return 0;
    }

    if (keyID >= DWC_QR2_GAME_KEY_START
        && stGameMatchKeys[keyID - DWC_QR2_GAME_KEY_START].keyID)
    {
        if (stGameMatchKeys[keyID - DWC_QR2_GAME_KEY_START].keyStr != NULL
            && strcmp(stGameMatchKeys[keyID - DWC_QR2_GAME_KEY_START].keyStr,
                   keyString)
                != 0)
        {
            return 0;
        }
    }
    else
    {
        keyID = DWCi_GetNewMatchKey();
        if (!keyID)
        {
            return 0;
        }
    }

    index = keyID - DWC_QR2_GAME_KEY_START;
    stGameMatchKeys[index].keyID = keyID;
    stGameMatchKeys[index].isStr = 0;
    stGameMatchKeys[index].pad = 0;

    if (stGameMatchKeys[index].keyStr)
    {
        DWC_Free(DWC_ALLOCTYPE_BASE, stGameMatchKeys[index].keyStr, 0);
    }

    stGameMatchKeys[index].keyStr
        = DWC_Alloc(DWC_ALLOCTYPE_BASE, strlen(keyString) + 1);
    if (!stGameMatchKeys[index].keyStr)
    {
        DWCi_StopMatching(DWC_ERROR_FATAL,
            DWC_ECODE_SEQ_MATCH + DWC_ECODE_TYPE_ALLOC);
        return 0;
    }

    strcpy(stGameMatchKeys[index].keyStr, keyString);

    DWC_Printf(DWC_REPORTFLAG_DEBUG,
        "DWC_AddMatchKeyInt: key='%s', value=%d\n", keyString, *valueSrc);

    stGameMatchKeys[index].value = valueSrc;

    qr2_register_keyA(keyID, stGameMatchKeys[index].keyStr);

    return keyID;
}

u8 DWC_AddMatchKeyString(u8 keyID, const char* keyString,
    const char* valueSrc)
{
    int index;

    if (!keyString || !valueSrc)
    {
        return 0;
    }

    if (keyID >= DWC_QR2_GAME_KEY_START
        && stGameMatchKeys[keyID - DWC_QR2_GAME_KEY_START].keyID)
    {
        if (stGameMatchKeys[keyID - DWC_QR2_GAME_KEY_START].keyStr != NULL
            && strcmp(stGameMatchKeys[keyID - DWC_QR2_GAME_KEY_START].keyStr,
                   keyString)
                != 0)
        {
            return 0;
        }
    }
    else
    {
        keyID = DWCi_GetNewMatchKey();
        if (!keyID)
        {
            return 0;
        }
    }

    index = keyID - DWC_QR2_GAME_KEY_START;
    stGameMatchKeys[index].keyID = keyID;
    stGameMatchKeys[index].isStr = 1;
    stGameMatchKeys[index].pad = 0;

    if (stGameMatchKeys[index].keyStr)
    {
        DWC_Free(DWC_ALLOCTYPE_BASE, stGameMatchKeys[index].keyStr, 0);
    }

    stGameMatchKeys[index].keyStr
        = DWC_Alloc(DWC_ALLOCTYPE_BASE, strlen(keyString) + 1);
    if (!stGameMatchKeys[index].keyStr)
    {
        DWCi_StopMatching(DWC_ERROR_FATAL,
            DWC_ECODE_SEQ_MATCH + DWC_ECODE_TYPE_ALLOC);
        return 0;
    }

    strcpy(stGameMatchKeys[index].keyStr, keyString);

    DWC_Printf(DWC_REPORTFLAG_DEBUG,
        "DWC_AddMatchKeyString: key='%s' value='%s'\n", keyString, valueSrc);

    stGameMatchKeys[index].value = valueSrc;

    qr2_register_keyA(keyID, stGameMatchKeys[index].keyStr);

    return keyID;
}

int fn_8048FACC(int index, const char* key, int defaultValue)
{
    SBServer server;

    if (stpMatchCnt == NULL || DWCi_IsError())
    {
        return defaultValue;
    }
    server = ServerBrowserGetServer(stpMatchCnt->sbObj, index);
    if (server != NULL)
    {
        return SBServerGetIntValueA(server, key, defaultValue);
    }
    return defaultValue;
}

int DWC_SetMatchingOption(int option, const void* optval)
{
    DWC_Printf(4, "!!DWC_SetMatchingOption() was called!! type %d\n", option);

    if (stpMatchCnt == NULL)
    {
        return 1;
    }
    if (optval == NULL)
    {
        return 3;
    }

    switch (option)
    {
    case DWC_MATCH_OPTION_MIN_COMPLETE:
        if (stpMatchCnt->state == 19)
        {
            return 1;
        }
        if (((const DWCMatchOptMinComplete*)optval)->valid != 0)
        {
            if (((const DWCMatchOptMinComplete*)optval)->minEntry <= 1)
            {
                return 3;
            }
        }
        if (stpOptMinComp == NULL)
        {
            stpOptMinComp
                = DWC_Alloc(DWC_ALLOCTYPE_BASE, sizeof(*stpOptMinComp));
            if (stpOptMinComp == NULL)
            {
                return 4;
            }
        }
        stpOptMinComp->valid
            = ((const DWCMatchOptMinComplete*)optval)->valid;
        stpOptMinComp->minEntry
            = ((const DWCMatchOptMinComplete*)optval)->minEntry;
        stpOptMinComp->retry = 0;
        stpOptMinComp->pad = 0;
        stpOptMinComp->timeout
            = ((const DWCMatchOptMinComplete*)optval)->timeout;
        stpOptMinComp->recvBit = 0;
        stpOptMinComp->timeoutBit = 0;
        stpOptMinComp->startTime = OSGetTime();
        stpOptMinComp->lastPollTime = OSGetTime();
        return 0;

    case DWC_MATCH_OPTION_SC_CONNECT_BLOCK:
        stOptSCBlock.valid
            = ((const DWCMatchOptSCConnectBlock*)optval)->valid != 0;
        stOptSCBlock.lock = 0;
        return 0;

    default:
        return 2;
    }
}

int DWC_GetMOMinCompState(u64* time)
{
    u64 passTime;

    if (!stpOptMinComp || !stpOptMinComp->valid)
    {
        return -1;
    }

    passTime = DWCi_Np_TicksToMilliSeconds(
        DWCi_Np_GetTick() - stpOptMinComp->startTime);
    if (time)
    {
        *time = passTime;
    }
    if (passTime >= stpOptMinComp->timeout)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

void fn_8048FD9C(DWCMatchControl* control, void* p0, GT2Socket* socket,
    void* p2, const char* gamename, const char* secretKey,
    const DWCFriendData* p5, int p6)
{
    stpMatchCnt = control;
    stpMatchCnt->pGpObj = p0;
    stpMatchCnt->pGt2Socket = socket;
    stpMatchCnt->gt2Callbacks = p2;
    stpMatchCnt->qr2Obj = NULL;
    stpMatchCnt->qr2IP = 0;
    stpMatchCnt->qr2Port = 0;
    stpMatchCnt->sbObj = NULL;
    stpMatchCnt->state = 0;
    stpMatchCnt->pad1 = 0;
    stpMatchCnt->pad2 = 0;
    stpMatchCnt->pad3 = 0;
    stpMatchCnt->pad4 = 0;
    stpMatchCnt->friendAcceptBit = 0;
    stpMatchCnt->profileID = 0;
    stpMatchCnt->gameName = gamename;
    stpMatchCnt->secretKey = secretKey;
    stpMatchCnt->friendList = p5;
    stpMatchCnt->friendListLen = p6;
    memset(stpMatchCnt->friendIdxList, 0, sizeof(stpMatchCnt->friendIdxList));
    stpMatchCnt->friendIdxListLen = 0;
    stpMatchCnt->matchedCallback = NULL;
    stpMatchCnt->matchedParam = NULL;
    stpMatchCnt->_488 = NULL;
    stpMatchCnt->_48C = NULL;
    stpMatchCnt->_490 = NULL;
    stpMatchCnt->_494 = NULL;
    stpMatchCnt->sbUpdateRequestTick = 0;

    DWCi_ClearGameMatchKeys();

    stOptSCBlock.valid = 0;
    stOptSCBlock.lock = 0;
    stOptSCBlock.pad = 0;

    DWCi_ResetMatchParam(DWC_MATCH_RESET_ALL);
}

qr2_error_t DWCi_QR2Startup(int profileID)
{
    int i;
    qr2_error_t qr2Error = e_qrnoerror;

    if (DWCi_GetMatchCnt()->qr2Obj)
    {
        DWC_Printf(
            DWC_REPORTFLAG_MATCH_NN, "QR2 is already set up.\n");
        return e_qrnoerror;
    }

    DWCi_GetMatchCnt()->profileID = profileID;

    for (i = 0; i < DWC_DNS_ERROR_RETRY_MAX; i++)
    {
        qr2Error = qr2_init_socketA(&DWCi_GetMatchCnt()->qr2Obj,
            gt2GetSocketSOCKET(*DWCi_GetMatchCnt()->pGt2Socket),
            gt2GetLocalPort(*DWCi_GetMatchCnt()->pGt2Socket),
            DWCi_GetMatchCnt()->gameName, DWCi_GetMatchCnt()->secretKey, 1, 1,
            DWCi_QR2ServerKeyCallback, DWCi_QR2PlayerKeyCallback,
            DWCi_QR2TeamKeyCallback, DWCi_QR2KeyListCallback,
            DWCi_QR2CountCallback, DWCi_QR2AddErrorCallback, NULL);
        if (!qr2Error)
        {
            break;
        }

        qr2_shutdown(DWCi_GetMatchCnt()->qr2Obj);
        DWCi_GetMatchCnt()->qr2Obj = NULL;

        if ((qr2Error != e_qrdnserror)
            || (i == DWC_DNS_ERROR_RETRY_MAX - 1))
        {
            DWCi_HandleQR2Error(qr2Error);
            return qr2Error;
        }
    }

    DWCi_GetMatchCnt()->qr2IP = 0;
    DWCi_GetMatchCnt()->qr2Port = 0;

    qr2_register_publicaddress_callback(
        DWCi_GetMatchCnt()->qr2Obj, DWCi_QR2PublicAddrCallback);
    qr2_register_natneg_callback(
        DWCi_GetMatchCnt()->qr2Obj, DWCi_QR2NatnegCallback);
    qr2_register_clientmessage_callback(
        DWCi_GetMatchCnt()->qr2Obj, DWCi_QR2ClientMsgCallback);
    qr2_send_statechanged(DWCi_GetMatchCnt()->qr2Obj);

    return qr2Error;
}

void DWCi_ConnectToAnybodyAsync(u8 numEntry, const char* addFilter,
    DWCMatchedSCCallback matchedCallback, void* matchedParam,
    DWCEvalPlayerCallback evalCallback, void* evalParam)
{
    char filter[0x100];
    unsigned long addFilterLenMax;

    if (stpAddFilter != NULL)
    {
        DWC_Free(DWC_ALLOCTYPE_BASE, stpAddFilter, 0);
        stpAddFilter = NULL;
    }

    if (addFilter != NULL)
    {
        addFilterLenMax = 0x100
            - DWCi_GetDefaultMatchFilter(filter, -1, 32, 3)
            - strlen(" and ()");
        stpAddFilter = DWC_Alloc(DWC_ALLOCTYPE_BASE, addFilterLenMax);
        if (stpAddFilter == NULL)
        {
            DWCi_StopMatching(DWC_ERROR_FATAL,
                DWC_ECODE_SEQ_MATCH + DWC_ECODE_TYPE_ALLOC);
            return;
        }
        memcpy(stpAddFilter, addFilter, addFilterLenMax);
        stpAddFilter[addFilterLenMax - 1] = '\0';
    }

    DWCi_ResetMatchParam(DWC_MATCH_RESET_ALL);
    DWCi_GetMatchCnt()->qr2MatchType = 0;
    DWCi_GetMatchCnt()->qr2NumEntry = numEntry;
    DWCi_GetMatchCnt()->matchedCallback = matchedCallback;
    DWCi_GetMatchCnt()->matchedParam = matchedParam;
    DWCi_GetMatchCnt()->nnFailureCount = 0;
    DWCi_GetMatchCnt()->aidList[0] = 0;
    qr2_register_keyA(0x32, "dwc_pid");
    qr2_register_keyA(0x33, "dwc_mtype");
    qr2_register_keyA(0x34, "dwc_mresv");
    qr2_register_keyA(0x35, "dwc_mver");
    qr2_register_keyA(0x36, "dwc_eval");
    DWCi_GetMatchCnt()->_488 = evalCallback;
    DWCi_GetMatchCnt()->_48C = evalParam;
    DWCi_GetMatchCnt()->state = 2;

    if (DWCi_GetMatchCnt()->sbObj == NULL)
    {
        DWCi_GetMatchCnt()->sbObj = ServerBrowserNewA(
            DWCi_GetMatchCnt()->gameName, DWCi_GetMatchCnt()->gameName,
            DWCi_GetMatchCnt()->secretKey, 0, 20, 1, SBFalse, fn_804993C8,
            NULL);
    }
    if (DWCi_GetMatchCnt()->sbObj == NULL)
    {
        if (DWCi_HandleSBError(5) != 0)
        {
            return;
        }
    }

    DWCi_SetGPStatus(3, "", NULL);
    if (DWCi_HandleSBError(DWCi_SBUpdateAsync(DWCi_GetMatchCnt()->profileID)) != 0)
    {
        return;
    }
    if (DWCi_GetMatchCnt()->qr2Obj == NULL)
    {
        DWCi_QR2Startup(DWCi_GetMatchCnt()->profileID);
    }

    if (stpOptMinComp != NULL && stpOptMinComp->valid != 0)
    {
        stpOptMinComp->recvBit = 0;
        stpOptMinComp->timeoutBit = 0;
        stpOptMinComp->retry = 0;
        stpOptMinComp->lastPollTime = OSGetTime();
        stpOptMinComp->startTime = OSGetTime();
    }
}

void fn_804903EC(u8 a0, DWCMatchedSCCallback callback, void* param,
    void* callback2, void* param2)
{
    DWCi_ResetMatchParam(DWC_MATCH_RESET_ALL);
    stpMatchCnt->qr2MatchType = 2;
    stpMatchCnt->qr2NumEntry = a0;
    stpMatchCnt->matchedCallback = callback;
    stpMatchCnt->matchedParam = param;
    stpMatchCnt->nnFailureCount = 0;
    stpMatchCnt->aidList[0] = 0;
    qr2_register_keyA(0x32, "dwc_pid");
    qr2_register_keyA(0x33, "dwc_mtype");
    qr2_register_keyA(0x34, "dwc_mresv");
    qr2_register_keyA(0x35, "dwc_mver");
    qr2_register_keyA(0x36, "dwc_eval");
    stpMatchCnt->newClientCallback = (DWCNewClientCallback)callback2;
    stpMatchCnt->newClientParam = param2;
    stpMatchCnt->sbPidList[0] = stpMatchCnt->profileID;
    stpMatchCnt->validAidBitmap = 1;
    stpMatchCnt->gt2NumValidConn = 0;
    stOptSCBlock.lock = 0;
    stpMatchCnt->state = 10;

    if (DWCi_HandleGPError(fn_80492BA0()) == 0)
    {
        if (stpMatchCnt->qr2Obj == NULL)
        {
            DWCi_QR2Startup(stpMatchCnt->profileID);
        }
    }
}

void fn_804905D0(int serverPid, DWCMatchedSCCallback matchedCallback,
    void* matchedParam, void* newClientCallback, void* newClientParam)
{
    int result;
    GPResult gpResult;

    DWCi_ResetMatchParam(DWC_MATCH_RESET_ALL);
    DWCi_GetMatchCnt()->qr2MatchType = 3;
    DWCi_GetMatchCnt()->qr2NumEntry = 0;
    DWCi_GetMatchCnt()->matchedCallback = matchedCallback;
    DWCi_GetMatchCnt()->matchedParam = matchedParam;
    DWCi_GetMatchCnt()->nnFailureCount = 0;
    DWCi_GetMatchCnt()->aidList[0] = 0;
    qr2_register_keyA(0x32, "dwc_pid");
    qr2_register_keyA(0x33, "dwc_mtype");
    qr2_register_keyA(0x34, "dwc_mresv");
    qr2_register_keyA(0x35, "dwc_mver");
    qr2_register_keyA(0x36, "dwc_eval");
    DWCi_GetMatchCnt()->newClientCallback = (DWCNewClientCallback)newClientCallback;
    DWCi_GetMatchCnt()->newClientParam = newClientParam;
    DWCi_GetMatchCnt()->qr2IsReserved = 1;
    DWCi_GetMatchCnt()->qr2Reservation = DWCi_GetMatchCnt()->profileID;
    DWCi_GetMatchCnt()->sbPidList[0] = serverPid;
    DWCi_GetMatchCnt()->state = 4;

    if (DWCi_GetMatchCnt()->sbObj == NULL)
    {
        DWCi_GetMatchCnt()->sbObj = ServerBrowserNewA(
            DWCi_GetMatchCnt()->gameName, DWCi_GetMatchCnt()->gameName,
            DWCi_GetMatchCnt()->secretKey, 0, 20, 1, SBFalse, fn_804993C8,
            NULL);
    }
    if (DWCi_GetMatchCnt()->sbObj == NULL)
    {
        if (DWCi_HandleSBError(5) != 0)
        {
            return;
        }
    }

    gpResult = DWCi_SetGPStatus(5, "", NULL);
    if (DWCi_HandleGPError(gpResult) != 0)
    {
        return;
    }
    if (DWCi_GetMatchCnt()->qr2Obj == NULL)
    {
        if (DWCi_QR2Startup(DWCi_GetMatchCnt()->profileID) != 0)
        {
            return;
        }
    }

    result = DWCi_SendResvCommand(DWCi_GetMatchCnt()->sbPidList[0], FALSE);
    if (DWCi_GetMatchCnt()->qr2MatchType == 0)
    {
        result = DWCi_HandleSBError(result);
    }
    else
    {
        result = DWCi_HandleGPError(result);
    }
    if (result != 0)
    {
        return;
    }
}

void fn_8049079C(BOOL enableProcess)
{
    int state;

    if (stpMatchCnt == NULL)
    {
        return;
    }
    if (DWCi_IsError())
    {
        return;
    }

    if (!enableProcess)
    {
        if (stpMatchCnt->qr2Obj != NULL)
        {
            qr2_think(stpMatchCnt->qr2Obj);
        }
        if (stpMatchCnt->pGt2Socket == NULL)
        {
            return;
        }
        gt2Think(*stpMatchCnt->pGt2Socket);
        return;
    }

    state = stpMatchCnt->state;
    if (state == 0)
    {
        return;
    }

    switch (state)
    {
    case 4:
        if (stpMatchCnt->cmdTimeoutTime != 0
            && fn_ElapsedMSec(stpMatchCnt->cmdTimeoutStartTick) >= stpMatchCnt->cmdTimeoutTime)
        {
            stpMatchCnt->cmdTimeoutTime = 0;
            if (stpMatchCnt->qr2MatchType == 3)
            {
                DWC_Printf(0x40, "Timeout: wait server response %d/%d.\n",
                    stpMatchCnt->scResvRetryCount, 5);
                stpMatchCnt->scResvRetryCount++;
                if (stpMatchCnt->scResvRetryCount > 5)
                {
                    DWCi_StopMatching(DWC_ERROR_NETWORK,
                        DWC_ECODE_SEQ_MATCH - 430);
                    return;
                }
                if (stpMatchCnt->qr2MatchType == 0)
                {
                    if (DWCi_HandleSBError(DWCi_SendResvCommand(
                            stpMatchCnt->sbPidList[0], FALSE))
                        != 0)
                    {
                        return;
                    }
                }
                else if (DWCi_HandleGPError(DWCi_SendResvCommand(
                             stpMatchCnt->sbPidList[0], FALSE))
                    != 0)
                {
                    return;
                }
            }
            else
            {
                DWC_Printf(0x40,
                    "NN resv(with %u) timed out. Try next server.\n",
                    stpMatchCnt->sbPidList[0]);
                if (DWCi_RetryReserving(0) == 0)
                {
                    return;
                }
            }
        }
        if (stpMatchCnt->cmdResendFlag != 0
            && fn_ElapsedMSec(stpMatchCnt->cmdResendTick)
                >= stpMatchCnt->gt2NumConnection * 3000 + 3000)
        {
            if (stpMatchCnt->qr2MatchType == 0)
            {
                if (DWCi_HandleSBError(DWCi_SendResvCommand(
                        stpMatchCnt->sbPidList[0], FALSE))
                    != 0)
                {
                    return;
                }
            }
            else if (DWCi_HandleGPError(DWCi_SendResvCommand(
                         stpMatchCnt->sbPidList[0], FALSE))
                != 0)
            {
                return;
            }
        }
        break;

    case 2:
    case 3:
    case 5:
        if (stpMatchCnt->sbUpdateFlag > 0)
        {
            int limit;

            if (state == 3)
            {
                limit = stpMatchCnt->gt2NumConnection * 3000 + 3000;
            }
            else if (stpMatchCnt->sbUpdateFlag == 1)
            {
                limit = 1000;
            }
            else
            {
                limit = 3000;
            }
            if (fn_ElapsedMSec(stpMatchCnt->sbUpdateTick) >= limit)
            {
                if (DWCi_HandleSBError(DWCi_SBUpdateAsync(stpMatchCnt->reqProfileID)) != 0)
                {
                    return;
                }
                stpMatchCnt->sbUpdateFlag = 0;
            }
        }
        break;

    case 7:
        if (stpMatchCnt->nnFinishTime != 0)
        {
            if (fn_ElapsedMSec(stpMatchCnt->nnFinishTime) >= 25000)
            {
                DWC_Printf(0x40, "Timeout: wait gt2Connect().\n");
                stpMatchCnt->nnFinishTime = 0;
                if (DWCi_CancelPreConnectedClientProcess(stpMatchCnt->sbPidList[0]) == 0)
                {
                    return;
                }
            }
        }
        else if (stpMatchCnt->_3E0 == 6
            && fn_ElapsedMSec(stpMatchCnt->_470) >= 6000)
        {
            DWC_Printf(4, "RTT Timeout with DWC_MATCH_STATE_CL_GT2.\n");
            stpMatchCnt->_3E1++;
            if (stpMatchCnt->_3E1 > 5)
            {
                stpMatchCnt->_3E0 = 0xFF;
                stpMatchCnt->_3E1 = 0;
                DWC_Printf(0x40, "Stop resending command %d.\n", 6);
                if (DWCi_CancelPreConnectedClientProcess(stpMatchCnt->sbPidList[0]) == 0)
                {
                    return;
                }
            }
            else
            {
                GPResult result = DWCi_SendMatchCommand(6, stpMatchCnt->_468,
                    stpMatchCnt->_3E4, stpMatchCnt->_3E2,
                    (u32*)stpMatchCnt->_3E8, stpMatchCnt->_46C);
                if (stpMatchCnt->qr2MatchType == 0)
                {
                    if (DWCi_HandleSBError(result) != 0)
                    {
                        return;
                    }
                }
                else if (DWCi_HandleGPError(result) != 0)
                {
                    return;
                }
            }
        }
        break;

    case 11:
        if (stpMatchCnt->_3E0 == 2)
        {
            if (stpMatchCnt->qr2MatchType == 0
                && fn_ElapsedMSec(stpMatchCnt->_470) >= 6000)
            {
            }
            else if (stpMatchCnt->qr2MatchType == 0
                || fn_ElapsedMSec(stpMatchCnt->_470) < 19000)
            {
                break;
            }
            DWC_Printf(0x40, "Reservation timeout. Cancel reservation.\n");
            stpMatchCnt->_3E0 = 0xFF;
            stpMatchCnt->_3E1 = 0;
            if (DWCi_CancelPreConnectedServerProcess(stpMatchCnt->sbPidList[stpMatchCnt->gt2NumConnection + 1])
                == 0)
            {
                return;
            }
        }
        break;

    case 13:
        if (stpMatchCnt->_3E0 == 8
            && fn_ElapsedMSec(stpMatchCnt->_470) >= 30000)
        {
            stpMatchCnt->_3E1++;
            if (stpMatchCnt->_3E1 != 0)
            {
                stpMatchCnt->_3E0 = 0xFF;
                DWC_Printf(0x40, "Wait clients connecting timeout.\n");
                stpMatchCnt->_3E1 = 0;
                if (stpMatchCnt->qr2MatchType == 2)
                {
                    if (DWCi_CancelPreConnectedServerProcess(stpMatchCnt->sbPidList[stpMatchCnt->gt2NumConnection])
                        == 0)
                    {
                        return;
                    }
                }
                else if (stpMatchCnt->qr2MatchType == 2
                    || stpMatchCnt->qr2MatchType == 3)
                {
                    DWC_Printf(8,
                        "DWCi_RestartFromTimeout() shouldn't be called.\n");
                }
                else
                {
                    stpMatchCnt->closeState = 2;
                    gt2CloseAllConnectionsHard(*stpMatchCnt->pGt2Socket);
                    stpMatchCnt->closeState = 0;
                    DWC_Printf(0x40,
                        "Closed all connections and restart matching.\n");
                    DWCi_RestartFromCancel(1);
                }
            }
            else
            {
                GPResult result = DWCi_SendMatchCommand(8, stpMatchCnt->_468,
                    stpMatchCnt->_3E4, stpMatchCnt->_3E2,
                    (u32*)stpMatchCnt->_3E8, stpMatchCnt->_46C);
                if (stpMatchCnt->qr2MatchType == 0)
                {
                    if (DWCi_HandleSBError(result) != 0)
                    {
                        return;
                    }
                }
                else if (DWCi_HandleGPError(result) != 0)
                {
                    return;
                }
            }
        }
        break;

    case 1:
        if (DWC_GetState() != DWC_STATE_MATCHING)
        {
            break;
        }
        if (fn_ElapsedMSec(stpMatchCnt->clWaitTime) >= 30000)
        {
            DWC_Printf(0x40, "No data from server %d/%d.\n",
                stpMatchCnt->clWaitTimeoutCount, 5);
            if (stpMatchCnt->clWaitTimeoutCount >= 5)
            {
                DWC_Printf(0x40,
                    "Timeout: Connection to server was shut down.\n");
                if (DWCi_CancelPreConnectedClientProcess(stpMatchCnt->sbPidList[0]) == 0)
                {
                    return;
                }
            }
            else
            {
                GPResult result = DWCi_SendMatchCommand(0x40,
                    stpMatchCnt->sbPidList[0], stpMatchCnt->qr2IPList[0],
                    stpMatchCnt->qr2PortList[0], NULL, 0);
                if (stpMatchCnt->qr2MatchType == 0)
                {
                    if (DWCi_HandleSBError(result) != 0)
                    {
                        return;
                    }
                }
                else if (DWCi_HandleGPError(result) != 0)
                {
                    return;
                }
                stpMatchCnt->clWaitTimeoutCount++;
                stpMatchCnt->clWaitTime = OSGetTime()
                    - (s64)(OS_BUS_CLOCK_SPEED / 4 / 1000) * 24000;
            }
        }
        break;

    default:
        break;
    }

    if (stpMatchCnt->state == 11 || stpMatchCnt->state == 6)
    {
        if (stpMatchCnt->nnFailedTime != 0
            && fn_ElapsedMSec(stpMatchCnt->nnFailedTime) >= 10000)
        {
            DWC_Printf(0x40, "Timeout : wait NN retry.\n");
            DWCi_NNCompletedCallback(nr_deadbeatpartner, 0, NULL, &stpMatchCnt->nnInfo);
        }
    }

    if (stpMatchCnt->sbObj != NULL)
    {
        s_sbCallbackLevel = 0;
        s_needSbFree = 0;
        ServerBrowserThink(stpMatchCnt->sbObj);
        if (s_needSbFree != 0)
        {
            ServerBrowserFree(stpMatchCnt->sbObj);
            stpMatchCnt->sbObj = NULL;
        }
        if (stpMatchCnt->sbObj != NULL
            && ServerBrowserState(stpMatchCnt->sbObj) != 0)
        {
            if (stpMatchCnt->sbUpdateRequestTick != 0 && OSGetTime() >= stpMatchCnt->sbUpdateRequestTick)
            {
                DWCi_StopMatching(DWC_ERROR_NETWORK,
                    DWC_ECODE_SEQ_MATCH + DWC_ECODE_GS_SB
                        + DWC_ECODE_TYPE_NETWORK);
                DWC_Printf(0x400, "ServerBrowserLimitUpdate timeout.\n");
            }
        }
    }

    if (stpMatchCnt->qr2Obj != NULL)
    {
        qr2_think(stpMatchCnt->qr2Obj);
        if (stpMatchCnt->qr2Obj->userstatechangerequested == 0)
        {
            switch (stpMatchCnt->qr2MatchType)
            {
            case 0:
            case 1:
                switch (stpMatchCnt->state)
                {
                case 1:
                case 2:
                case 3:
                case 4:
                case 6:
                case 11:
                    qr2_send_statechanged(stpMatchCnt->qr2Obj);
                    break;
                default:
                    break;
                }
                break;
            case 2:
                if (stpMatchCnt->state == 11)
                {
                    qr2_send_statechanged(stpMatchCnt->qr2Obj);
                }
                break;
            default:
                break;
            }
        }
    }

    NNThink();

    if (stpMatchCnt->pGt2Socket != NULL)
    {
        gt2Think(*stpMatchCnt->pGt2Socket);
    }

    if (stpMatchCnt->state == 18
        && fn_ElapsedMSec(stpMatchCnt->closedTime) >= 3000)
    {
        DWC_Printf(4, "RTT Timeout with DWCi_MatchProcess.\n");
        DWC_Printf(0x40, "Timeout : Wait prior profileID.\n");
        if (DWCi_ResumeMatching() != 0)
        {
            return;
        }
    }

    if (fn_804979F4() == 0)
    {
        return;
    }
    if (fn_8049811C() == 0)
    {
        return;
    }
    fn_80498440();

    if (stpMatchCnt->stopSCFlag != 0 && stpMatchCnt->state == 10)
    {
        stpMatchCnt->qr2NumEntry = stpMatchCnt->gt2NumConnection;
        fn_80492BA0();
        stpMatchCnt->stopSCFlag = 0;
        if (stpMatchCnt->_490 != NULL)
        {
            stpMatchCnt->_490(stpMatchCnt->_494);
        }
    }
}

GT2Bool DWCi_GT2UnrecognizedMessageCallback(GT2Socket socket, unsigned int ip,
    unsigned short port, GT2Byte* message, int len)
{
    struct sockaddr_in saddr;

    if (!len || !message)
    {
        return GT2False;
    }

    DWCi_Np_CpuClear32(&saddr, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.addr = ip;
    saddr.sin_port = SOHtoNs(port);

    if ((message[0] == QR_MAGIC_1 && message[1] == QR_MAGIC_2)
        || message[0] == '\\')
    {
        DWC_Printf(DWC_REPORTFLAG_MATCH_NN,
            "GT2 Unrecognized : Received QR2 data.\n");
        if (DWCi_GetMatchCnt()->qr2Obj)
        {
            qr2_parse_queryA(DWCi_GetMatchCnt()->qr2Obj, (char*)message, len,
                (struct sockaddr*)&saddr);
        }
        else
        {
            DWC_Printf(
                DWC_REPORTFLAG_MATCH_NN, "  ignore qr2 message.\n");
        }
    }
    else if (!memcmp(message, NNMagicData, NATNEG_MAGIC_LEN))
    {
        DWC_Printf(DWC_REPORTFLAG_MATCH_NN,
            "GT2 Unrecognized : Received NN data.\n");
        NNProcessData((char*)message, len, &saddr);
    }
    else if (message[0] == 0xFE && message[1] == 0xFE)
    {
        DWC_Printf(DWC_REPORTFLAG_MATCH_GT2,
            "GT2 Unrecognized : Not Connected gt2 data.\n");
        return GT2False;
    }
    else
    {
        DWC_Printf(DWC_REPORTFLAG_WARNING,
            "GT2 Unrecognized : Received unrecognized data.\n");
        return GT2False;
    }

    return GT2True;
}

void DWCi_GT2ConnectAttemptCallback(GT2Socket socket,
    GT2Connection connection, unsigned int ip, unsigned short port,
    int latency, GT2Byte* message, int len)
{
#pragma unused(socket)
#pragma unused(len)
    int index;
    GT2Bool result;
    GT2Connection* pGt2Con;
    DWCConnectionInfo* pConInfo;

    if (DWCi_GetMatchCnt()->state == DWC_MATCH_STATE_CL_NN)
    {
        return;
    }

    if (!DWCi_GetMatchCnt()
        || (DWCi_GetMatchCnt()->state != DWC_MATCH_STATE_CL_GT2)
        || (DWCi_GetMatchCnt()->cancelState != DWC_MATCH_CANCEL_STATE_INIT))
    {
        gt2Reject(connection, (const GT2Byte*)"Init state", -1);
        DWC_Printf(DWC_REPORTFLAG_MATCH_GT2,
            "gt2Reject was called : Init state\n");
        return;
    }

    index = DWCi_GT2GetConnectionListIdx();
    if (index == -1)
    {
        gt2Reject(connection, (const GT2Byte*)"Server full", -1);
        DWC_Printf(DWC_REPORTFLAG_MATCH_GT2,
            "gt2Reject was called : Server full\n");
        DWCi_StopMatching(DWC_ERROR_NETWORK,
            DWC_ECODE_SEQ_MATCH + DWC_ECODE_GS_GT2
                + DWC_ECODE_TYPE_CONN_OVER);
        return;
    }

    if ((ip != DWCi_GetMatchCnt()->ipList[DWCi_GetMatchCnt()->gt2NumConnection])
        || (SONtoHs(port)
            != DWCi_GetMatchCnt()->portList[DWCi_GetMatchCnt()->gt2NumConnection]))
    {
        if (message[0]
            && (strtoul((const char*)message, NULL, 10)
                == DWCi_GetMatchCnt()->sbPidList[DWCi_GetMatchCnt()->gt2NumConnection]))
        {
            DWC_Printf(DWC_REPORTFLAG_WARNING,
                "gt2Connect() came before NN complete.\n");
            DWCi_GetMatchCnt()->ipList[DWCi_GetMatchCnt()->gt2NumConnection] = ip;
            DWCi_GetMatchCnt()->portList[DWCi_GetMatchCnt()->gt2NumConnection]
                = SONtoHs(port);
        }
        else
        {
            gt2Reject(
                connection, (const GT2Byte*)"Unknown connect attempt", -1);
            DWC_Printf(DWC_REPORTFLAG_WARNING,
                "gt2Reject was called : Unknown connect attempt from %s\n",
                gt2AddressToString(ip, port, NULL));
            return;
        }
    }

    DWCi_GetMatchCnt()->nnFinishTime = 0;

    result = gt2Accept(connection, DWCi_GetMatchCnt()->gt2Callbacks);
    if (result == GT2False)
    {
        DWC_Printf(
            DWC_REPORTFLAG_WARNING, "Unexpected failure to gt2Accept.\n");
        DWCi_StopMatching(DWC_ERROR_NETWORK,
            DWC_ECODE_SEQ_MATCH + DWC_ECODE_TYPE_OTHER);
        return;
    }

    DWC_Printf(DWC_REPORTFLAG_MATCH_GT2,
        "Accepted connection from %s (latency %d)\n",
        gt2AddressToString(ip, port, NULL), latency);

    DWCi_StopResendingMatchCommand();

    if (DWCi_GetMatchCnt()->gt2NumConnection == 0)
    {
        DWCi_GetMatchCnt()->baseLatency
            = (u16)(((latency >> 1) < 0xffff) ? (latency >> 1) : 0xffff);
    }

    pGt2Con = DWCi_GetGT2ConnectionByIdx(index);
    pConInfo = DWCi_GetConnectionInfoByIdx(index);
    *pGt2Con = connection;
    DWCi_GetMatchCnt()->gt2NumConnection++;

    pConInfo->index = (u8)index;
    pConInfo->aid = DWCi_GetMatchCnt()->aidList[DWCi_GetMatchCnt()->gt2NumConnection - 1];
    pConInfo->reserve = 0;
    pConInfo->param = NULL;

    gt2SetConnectionData(connection, pConInfo);

    DWCi_PostProcessConnection(DWC_PP_CONNECTION_CL_GT2_ACCEPT);
}

void fn_80491AE0(GT2Connection connection, GT2Result result, GT2Byte* message,
    int len)
{
    char pidStr[12];
    int index;
    GT2Connection* pGt2Con;
    GT2Result gt2Result;
    DWCConnectionInfo* pConInfo;

    if (DWCi_GetMatchCnt() == NULL
        || (DWCi_GetMatchCnt()->state != 7
            && DWCi_GetMatchCnt()->state != 12))
    {
        DWC_Printf(0x80, "gt2ConnectedCallback: Already cancel\n");
        return;
    }

    if (result != GT2Success)
    {
        if (message == NULL)
        {
            message = (GT2Byte*)"";
        }
        DWC_Printf(0x80, "GT2 connect failed %d: %s\n", result, message);
        if (result == GT2DuplicateAddress)
        {
            return;
        }
        else if (result == GT2TimedOut)
        {
            DWCi_GetMatchCnt()->gt2ConnectCount++;
            if (DWCi_GetMatchCnt()->gt2ConnectCount > 5)
            {
                DWC_Printf(0x80, "gt2Connect() retry over.\n");
                DWCi_GetMatchCnt()->gt2ConnectCount = 0;
                DWCi_CancelPreConnectedServerProcess(DWCi_GetMatchCnt()->sbPidList[DWCi_GetMatchCnt()->qr2NNFinishCount]);
                return;
            }
            DWC_Printf(0x80, "Retry to gt2Connect.\n");
            (void)snprintf(pidStr, sizeof(pidStr), "%u",
                DWCi_GetMatchCnt()->profileID);
            gt2Result = gt2Connect(*DWCi_GetMatchCnt()->pGt2Socket, NULL,
                gt2AddressToString(
                    DWCi_GetMatchCnt()->ipList[DWCi_GetMatchCnt()->qr2NNFinishCount],
                    DWCi_GetMatchCnt()->portList[DWCi_GetMatchCnt()->qr2NNFinishCount], NULL),
                (GT2Byte*)pidStr, -1, 5000, DWCi_GetMatchCnt()->gt2Callbacks, GT2False);
            if (gt2Result == GT2OutOfMemory)
            {
                fn_8049925C(gt2Result);
                return;
            }
            else if (gt2Result == GT2Success)
            {
            }
            else if (DWCi_CancelPreConnectedServerProcess(
                         DWCi_GetMatchCnt()->sbPidList[DWCi_GetMatchCnt()->qr2NNFinishCount])
                == 0)
            {
                return;
            }
        }
        else if (DWCi_CancelPreConnectedServerProcess(DWCi_GetMatchCnt()->sbPidList[
                     DWCi_GetMatchCnt()->gt2NumConnection + 1])
            == 0)
        {
            return;
        }
    }
    else
    {
        DWC_Printf(0x80, "GT2 connected.\n");
        index = DWCi_GT2GetConnectionListIdx();
        if (index == -1)
        {
            DWC_Printf(8,
                "Don't continue matching without closing connections!!\n");
            DWCi_StopMatching(DWC_ERROR_NETWORK,
                DWC_ECODE_SEQ_MATCH + DWC_ECODE_GS_GT2
                    + DWC_ECODE_TYPE_CONN_OVER);
            return;
        }

        pGt2Con = DWCi_GetGT2ConnectionByIdx(index);
        pConInfo = DWCi_GetConnectionInfoByIdx(index);
        *pGt2Con = connection;
        DWCi_GetMatchCnt()->gt2NumConnection++;
        pConInfo->index = index;
        pConInfo->reserve = 0;
        pConInfo->param = NULL;
        pConInfo->aid = DWCi_GetMatchCnt()->aidList[DWCi_GetMatchCnt()->gt2NumConnection];
        gt2SetConnectionData(connection, pConInfo);

        if (DWCi_GetMatchCnt()->state == 12)
        {
            DWCi_PostProcessConnection(DWC_PP_CONNECTION_SV_CONNECT);
        }
        else
        {
            DWCi_PostProcessConnection(DWC_PP_CONNECTION_CL_GT2_CONNECT);
        }
    }
}

void fn_80491E18(GPConnection* connection, u32 profileId, char* message)
{
    char buf[16];
    u32 list[0x80];
    int i;

    DWC_Printf(0x40, "<GP> RECV-0x%02x <- [--------:-----] [pid=%u]\n",
        (s8)message[0], profileId);

    for (i = 0; i < 0x80; i++)
    {
        if (fn_80493B94(buf, message + 1, i) == -1)
        {
            break;
        }
        list[i] = strtoul(buf, NULL, 10);
    }
    DWCi_ProcessRecvMatchCommand(message[0], profileId, 0, 0, list, i);
}

static void DWCi_StopResendingMatchCommand(void)
{
    DWCi_GetMatchCnt()->_3E0 = 0xff;
    DWCi_GetMatchCnt()->_3E1 = 0;
}

void DWCi_StopMatching(DWCError error, int errorCode)
{
    if (DWCi_GetMatchCnt() == NULL || error == 0)
    {
        return;
    }

    DWCi_CloseAllConnectionsByTimeout();
    DWCi_SetError(error, errorCode);
    DWCi_SetGPStatus(1, "", NULL);
    DWCi_GetMatchCnt()->matchedCallback(error, FALSE,
        DWCi_GetMatchCnt()->cbEventPid ? FALSE : TRUE,
        DWCi_GetMatchCnt()->qr2MatchType == 2 ? TRUE : FALSE,
        DWCi_GetFriendListIndex(DWCi_GetMatchCnt()->cbEventPid),
        DWCi_GetMatchCnt()->matchedParam);
    DWCi_CloseMatching();
}

void fn_80491FC4(void)
{
    if (stpMatchCnt->qr2MatchType == 2)
    {
        return;
    }
    stpMatchCnt->qr2NNFinishCount = 0;
    stpMatchCnt->qr2NumEntry = 0;
    qr2_send_statechanged(stpMatchCnt->qr2Obj);
}

void DWCi_ProcessMatchSynPacket(u8 aid, u16 type, u8* data)
{
    u8 aidIdx;
    u32 bitmask;
    int i;

    DWC_Printf(DWC_REPORTFLAG_MATCH_GT2,
        "Received SYN %d packet from aid %d.\n",
        type - DWC_SEND_TYPE_MATCH_SYN, aid);

    switch (type)
    {
    case DWC_SEND_TYPE_MATCH_SYN:
        if (DWCi_GetMatchCnt()->state == DWC_MATCH_STATE_CL_WAITING)
        {
            if (data[0] == 1)
            {
                DWCi_GetMatchCnt()->cbEventPid = 0;
            }

            aidIdx = data[1];
            DWCi_GetMatchCnt()->aidList[aidIdx] = data[2];
            DWCi_GetMatchCnt()->sbPidList[aidIdx]
                = DWCi_GetMatchCnt()->profileID;

            if ((DWCi_GetMatchCnt()->qr2MatchType
                    == DWC_MATCH_TYPE_ANYBODY)
                || (DWCi_GetMatchCnt()->qr2MatchType
                    == DWC_MATCH_TYPE_FRIEND))
            {
                DWCi_GetMatchCnt()->qr2NumEntry
                    = (u8)DWCi_GetMatchCnt()->gt2NumConnection;
            }

            DWCi_GetMatchCnt()->clWaitTimeoutCount = 0;
            DWCi_SetMatchStatus(DWC_MATCH_STATE_CL_SYN);
        }

        DWCi_SendMatchSynPacket(aid, DWC_SEND_TYPE_MATCH_SYN_ACK);
        break;

    case DWC_SEND_TYPE_MATCH_SYN_ACK:
        if (DWCi_GetMatchCnt()->state == DWC_MATCH_STATE_SV_SYN)
        {
            DWCi_GetMatchCnt()->synAckBit |= 1 << aid;

            if ((data[0] | (data[1] << 8))
                > DWCi_GetMatchCnt()->baseLatency)
            {
                DWCi_GetMatchCnt()->baseLatency
                    = (u16)(data[0] | (data[1] << 8));
            }

            bitmask = DWCi_GetAIDBitmask(FALSE);
            if (DWCi_GetMatchCnt()->synAckBit == bitmask)
            {
                for (i = 1; i <= DWCi_GetMatchCnt()->gt2NumConnection; i++)
                {
                    DWCi_SendMatchSynPacket(DWCi_GetMatchCnt()->aidList[i],
                        DWC_SEND_TYPE_MATCH_ACK);
                }

                DWCi_SetMatchStatus(DWC_MATCH_STATE_SV_SYN_WAIT);

                DWC_Printf(DWC_REPORTFLAG_MATCH_GT2,
                    "Wait max latency %d msec.\n",
                    DWCi_GetMatchCnt()->baseLatency);
            }
        }
        else
        {
            DWCi_SendMatchSynPacket(aid, DWC_SEND_TYPE_MATCH_ACK);
        }
        break;

    case DWC_SEND_TYPE_MATCH_ACK:
        if (DWCi_GetMatchCnt()->state == DWC_MATCH_STATE_CL_SYN)
        {
            DWCi_PostProcessConnection(DWC_PP_CONNECTION_SYN_FINISH);
        }
        break;
    }
}

BOOL DWCi_ProcessMatchClosing(DWCError error, int errorCode)
{
    if (DWC_GetState() != DWC_STATE_MATCHING)
    {
        return FALSE;
    }

    if (DWCi_GetMatchCnt()->qr2MatchType == DWC_MATCH_TYPE_SC_SV)
    {
        DWC_Printf(
            DWC_REPORTFLAG_DEBUG, "DWCi_ProcessMatchClosing: SC_SV.\n");
        return TRUE;
    }

    if (error)
    {
        DWCi_StopMatching(error, errorCode + DWC_ECODE_SEQ_MATCH);
        return TRUE;
    }

    DWCi_GetMatchCnt()->aidList[0] = 0;

    if ((DWCi_GetMatchCnt()->cancelState == DWC_MATCH_CANCEL_STATE_EXEC)
        || (DWCi_GetMatchCnt()->closeState
            == DWC_MATCH_CLOSE_STATE_CHANGE_TO_CLIENT)
        || (DWCi_GetMatchCnt()->closeState == DWC_MATCH_CLOSE_STATE_TIMEOUT))
    {
        return TRUE;
    }

    if (DWCi_GetMatchCnt()->nnInfo.cookie)
    {
        NNCancel(DWCi_GetMatchCnt()->nnInfo.cookie);
        DWCi_GetMatchCnt()->nnInfo.cookie = 0;
    }

    if (DWCi_GetMatchCnt()->gt2NumConnection)
    {
        if (DWCi_GetMatchCnt()->closeState == DWC_MATCH_CLOSE_STATE_INIT)
        {
            DWCi_GetMatchCnt()->closeState = DWC_MATCH_CLOSE_STATE_FORCED;
            gt2CloseAllConnectionsHard(*DWCi_GetMatchCnt()->pGt2Socket);
        }
    }
    else
    {
        if (DWCi_GetMatchCnt()->qr2MatchType == DWC_MATCH_TYPE_SC_CL)
        {
            DWCi_StopMatching(DWC_ERROR_NETWORK,
                DWC_ECODE_SEQ_MATCH + DWC_ECODE_TYPE_SC_CL_FAIL);
        }
        else if (DWCi_GetMatchCnt()->priorProfileID)
        {
            DWC_Printf(DWC_REPORTFLAG_MATCH_NN, "Reserve NN to %u.\n",
                DWCi_GetMatchCnt()->priorProfileID);

            (void)DWCi_ResumeMatching();
        }
        else if (DWCi_GetMatchCnt()->state == DWC_MATCH_STATE_CL_WAITING)
        {
            DWC_Printf(
                DWC_REPORTFLAG_MATCH_NN, "Wait prior profileID.\n");

            DWCi_SetMatchStatus(DWC_MATCH_STATE_WAIT_CLOSE);
            DWCi_GetMatchCnt()->closedTime = DWCi_Np_GetTick();
        }
        else
        {
            DWC_Printf(DWC_REPORTFLAG_MATCH_NN,
                "Restart matching immediately.\n");

            DWCi_RestartFromCancel(DWC_MATCH_RESET_RESTART);
        }
    }

    return TRUE;
}

void DWCi_ProcessMatchSCClosing(int clientPid)
{
    if (DWCi_GetMatchCnt()->closeState != DWC_MATCH_CLOSE_STATE_TIMEOUT)
    {
        DWCi_CancelPreConnectedServerProcess(clientPid);
    }
}

BOOL DWCi_DeleteHostByProfileID(int profileID, int numHost)
{
    int i;

    if (!DWCi_GetMatchCnt())
        return FALSE;

    for (i = 0; i < numHost; i++)
    {
        if (DWCi_GetMatchCnt()->sbPidList[i] == profileID)
        {
            DWCi_DeleteHostByIndex(i, numHost);
            return TRUE;
        }
    }

    return FALSE;
}

int DWCi_DeleteHostByIndex(int index, int numHost)
{
    int profileID;
    int i;

    if (stpMatchCnt == NULL)
    {
        return 0;
    }

    profileID = stpMatchCnt->sbPidList[index];
    stpMatchCnt->validAidBitmap
        &= ~(1 << stpMatchCnt->aidList[index]);
    fn_804929E0();

    for (i = index; i < numHost - 1; i++)
    {
        stpMatchCnt->qr2IPList[i] = stpMatchCnt->qr2IPList[i + 1];
        stpMatchCnt->qr2PortList[i] = stpMatchCnt->qr2PortList[i + 1];
        stpMatchCnt->sbPidList[i] = stpMatchCnt->sbPidList[i + 1];
        stpMatchCnt->ipList[i] = stpMatchCnt->ipList[i + 1];
        stpMatchCnt->portList[i] = stpMatchCnt->portList[i + 1];
        stpMatchCnt->aidList[i] = stpMatchCnt->aidList[i + 1];
    }

    if (numHost > 0)
    {
        stpMatchCnt->qr2IPList[numHost - 1] = 0;
        stpMatchCnt->qr2PortList[numHost - 1] = 0;
        stpMatchCnt->sbPidList[numHost - 1] = 0;
        stpMatchCnt->ipList[numHost - 1] = 0;
        stpMatchCnt->portList[numHost - 1] = 0;
        stpMatchCnt->aidList[numHost - 1] = 0;
    }
    return profileID;
}

int fn_804929A8(void)
{
    if (stpMatchCnt != NULL)
    {
        return stpMatchCnt->gt2NumConnection;
    }
    return 0;
}

int fn_804929C4(void)
{
    if (stpMatchCnt != NULL)
    {
        return stpMatchCnt->gt2NumValidConn;
    }
    return 0;
}

void fn_804929E0(void)
{
    int count = -1;
    int i;

    for (i = 0; i < 32; i++)
    {
        if ((1 << i) & stpMatchCnt->validAidBitmap)
        {
            count++;
        }
    }
    if (count == -1)
    {
        stpMatchCnt->gt2NumValidConn = 0;
    }
    else
    {
        stpMatchCnt->gt2NumValidConn = count;
    }
}

int fn_80492ABC(u8** aidList)
{
    if (stpMatchCnt == NULL)
    {
        return 0;
    }
    *aidList = stpMatchCnt->aidList;
    return stpMatchCnt->gt2NumConnection + 1;
}

int fn_80492AE8(u8** aidList)
{
    int i;

    if (stpMatchCnt == NULL)
    {
        return 0;
    }

    memset(lbl_806CA158, 0, sizeof(lbl_806CA158));
    for (i = 0; i <= stpMatchCnt->gt2NumValidConn; i++)
    {
        if (!(stpMatchCnt->validAidBitmap
                & (1 << stpMatchCnt->aidList[i])))
        {
            break;
        }
        lbl_806CA158[i] = stpMatchCnt->aidList[i];
    }
    *aidList = lbl_806CA158;
    return stpMatchCnt->gt2NumValidConn + 1;
}

GPResult fn_80492BA0(void)
{
    char value[12];
    char status[32];

    if (stpMatchCnt->qr2MatchType != 2)
    {
        return GP_NO_ERROR;
    }
    snprintf(value, sizeof(value), "%u", stpMatchCnt->qr2NumEntry + 1);
    DWC_SetCommonKeyValueString("SCM", value, status, '/');
    snprintf(value, sizeof(value), "%u", stpMatchCnt->gt2NumConnection + 1);
    DWC_AddCommonKeyValueString("SCN", value, status, '/');
    snprintf(value, sizeof(value), "%u", 3);
    DWC_AddCommonKeyValueString("VER", value, status, '/');
    return DWCi_SetGPStatus(6, status, NULL);
}

void fn_80492C74(void)
{
    stpMatchCnt = NULL;
    if (stpAddFilter != NULL)
    {
        DWC_Free(DWC_ALLOCTYPE_BASE, stpAddFilter, 0);
        stpAddFilter = NULL;
    }

    DWCi_ClearGameMatchKeys();

    if (stpOptMinComp != NULL)
    {
        DWC_Free(DWC_ALLOCTYPE_BASE, stpOptMinComp, 0);
        stpOptMinComp = NULL;
    }

    stOptSCBlock.valid = 0;
    stOptSCBlock.lock = 0;
}

BOOL fn_80492D3C(void)
{
    return stpMatchCnt == NULL;
}

static void DWCi_ResetMatchParam(DWCMatchResetLevel level)
{
    DWCi_GetMatchCnt()->gt2ConnectCount = 0;
    DWCi_GetMatchCnt()->nnRecvCount = 0;
    DWCi_GetMatchCnt()->nnCookieRand = (u16)DWCi_GetMathRand32(0x10000);
    DWCi_GetMatchCnt()->nnLastCookie = 0;
    DWCi_GetMatchCnt()->nnFailedTime = 0;
    DWCi_GetMatchCnt()->nnFinishTime = 0;
    DWCi_GetMatchCnt()->clLinkProgress = 0;
    DWCi_GetMatchCnt()->cancelState = 0;
    DWCi_GetMatchCnt()->scResvRetryCount = 0;
    DWCi_GetMatchCnt()->synResendCount = 0;
    DWCi_GetMatchCnt()->cancelSynResendCount = 0;
    DWCi_GetMatchCnt()->resvWaitCount = 0;
    DWCi_GetMatchCnt()->closeState = 0;
    DWCi_GetMatchCnt()->cancelBaseLatency = 0;
    DWCi_GetMatchCnt()->searchPort = 0;
    DWCi_GetMatchCnt()->searchIP = 0;
    DWCi_GetMatchCnt()->lastSynSent = 0;
    DWCi_GetMatchCnt()->closedTime = 0;
    DWCi_Np_CpuClear32(&DWCi_GetMatchCnt()->_3E0, 0x98);

    if (level == DWC_MATCH_RESET_CONTINUE)
    {
        DWCi_GetMatchCnt()->qr2NNFinishCount = (u8)DWCi_GetMatchCnt()->gt2NumConnection;

        if (DWCi_GetMatchCnt()->qr2MatchType == DWC_MATCH_TYPE_SC_CL)
        {
            DWCi_SetMatchStatus(DWC_MATCH_STATE_CL_WAITING);
        }
        else if (DWCi_GetMatchCnt()->qr2MatchType == DWC_MATCH_TYPE_SC_SV)
        {
            DWCi_SetMatchStatus(DWC_MATCH_STATE_SV_WAITING);
        }
    }
    else
    {
        DWCi_GetMatchCnt()->gt2NumConnection = 0;
        DWCi_GetMatchCnt()->gt2NumValidConn = 0;
        DWCi_GetMatchCnt()->qr2NNFinishCount = 0;
        DWCi_GetMatchCnt()->qr2IsReserved = 0;
        DWCi_GetMatchCnt()->qr2Reservation = 0;
        DWCi_GetMatchCnt()->sbUpdateFlag = FALSE;
        DWCi_GetMatchCnt()->friendCount = 0;
        DWCi_GetMatchCnt()->baseLatency = 0;
        DWCi_GetMatchCnt()->cmdResendFlag = FALSE;
        DWCi_GetMatchCnt()->cmdResendTick = 0;
        DWCi_GetMatchCnt()->cmdTimeoutTime = 0;
        DWCi_GetMatchCnt()->cmdTimeoutStartTick = 0;
        DWCi_GetMatchCnt()->synAckBit = 0;
        DWCi_GetMatchCnt()->reqProfileID = 0;
        DWCi_GetMatchCnt()->priorProfileID = 0;
        DWCi_GetMatchCnt()->validAidBitmap = 0;

        DWCi_Np_CpuClear32(DWCi_GetMatchCnt()->qr2IPList,
            sizeof(DWCi_GetMatchCnt()->qr2IPList));
        DWCi_Np_CpuClear16(DWCi_GetMatchCnt()->qr2PortList,
            sizeof(DWCi_GetMatchCnt()->qr2PortList));
        DWCi_Np_CpuClear32(DWCi_GetMatchCnt()->sbPidList,
            sizeof(DWCi_GetMatchCnt()->sbPidList));
        DWCi_Np_CpuClear32(
            &DWCi_GetMatchCnt()->nnInfo, sizeof(DWCNNInfo));
        DWCi_Np_CpuClear32(DWCi_GetMatchCnt()->ipList,
            sizeof(DWCi_GetMatchCnt()->ipList));
        DWCi_Np_CpuClear16(DWCi_GetMatchCnt()->portList,
            sizeof(DWCi_GetMatchCnt()->portList));
        DWCi_Np_CpuClear8(DWCi_GetMatchCnt()->aidList,
            sizeof(DWCi_GetMatchCnt()->aidList));
        DWCi_Np_CpuClear32(DWCi_GetMatchCnt()->svDataBak,
            sizeof(DWCi_GetMatchCnt()->svDataBak));

        if (level == DWC_MATCH_RESET_RESTART)
        {
            if (DWCi_GetMatchCnt()->qr2MatchType == DWC_MATCH_TYPE_ANYBODY)
            {
                DWCi_SetMatchStatus(DWC_MATCH_STATE_CL_SEARCH_HOST);
            }
            else if (DWCi_GetMatchCnt()->qr2MatchType == DWC_MATCH_TYPE_FRIEND)
            {
                DWCi_SetMatchStatus(DWC_MATCH_STATE_CL_WAIT_RESV);
            }
        }
        else
        {
            DWCi_GetMatchCnt()->qr2MatchType = 0;
            DWCi_GetMatchCnt()->qr2NumEntry = 0;
            DWCi_GetMatchCnt()->qr2ShutdownFlag = 0;
            DWCi_GetMatchCnt()->cbEventPid = 0;
            DWCi_GetMatchCnt()->distantFriend = 0;
            DWCi_GetMatchCnt()->stopSCFlag = 0;
            DWCi_GetMatchCnt()->clWaitTimeoutCount = 0;
            DWCi_GetMatchCnt()->clWaitTime = 0;
            DWCi_GetMatchCnt()->newClientCallback = NULL;
            DWCi_GetMatchCnt()->newClientParam = NULL;
        }
    }
}

void DWCi_CloseMatching(void)
{
    DWC_Printf(0x40, " Close Matching....\n");
    if (stpMatchCnt == NULL)
    {
        return;
    }

    if (stpMatchCnt->sbObj != NULL)
    {
        if (s_sbCallbackLevel == 0)
        {
            ServerBrowserFree(stpMatchCnt->sbObj);
            stpMatchCnt->sbObj = NULL;
        }
        else
        {
            s_needSbFree = 1;
        }
    }

    NNFreeNegotiateList();
    stpMatchCnt->state = 0;

    if (stpAddFilter != NULL)
    {
        DWC_Free(DWC_ALLOCTYPE_BASE, stpAddFilter, 0);
        stpAddFilter = NULL;
    }

    DWCi_ClearGameMatchKeys();

    stpMatchCnt->qr2ShutdownFlag = 1;
}

#define DWC_NUM_QR2_MATCH_KEY 7

static SBError DWCi_SBUpdateAsync(int profileID)
{
    char filter[MAX_FILTER_LEN];
    char* pFilter = filter;
    u8 basicFields[DWC_NUM_QR2_MATCH_KEY + DWC_QR2_GAME_RESERVED_KEYS];
    int len;
    int numBasicFields = DWC_NUM_QR2_MATCH_KEY;
    int i;
    SBError sbError = sbe_noerror;

    basicFields[0] = NUMPLAYERS_KEY;
    basicFields[1] = MAXPLAYERS_KEY;
    basicFields[2] = DWC_QR2_PID_KEY;
    basicFields[3] = DWC_QR2_MATCH_TYPE_KEY;
    basicFields[4] = DWC_QR2_MATCH_RESV_KEY;
    basicFields[5] = DWC_QR2_MATCH_VER_KEY;
    basicFields[6] = DWC_QR2_MATCH_EVAL_KEY;

    if ((DWCi_GetMatchCnt()->qr2MatchType == DWC_MATCH_TYPE_ANYBODY)
        || (DWCi_GetMatchCnt()->qr2MatchType == DWC_MATCH_TYPE_FRIEND))
    {
        for (i = 0; i < DWC_QR2_GAME_RESERVED_KEYS; i++)
        {
            if (stGameMatchKeys[i].keyID)
            {
                basicFields[numBasicFields] = stGameMatchKeys[i].keyID;
                numBasicFields++;
            }
        }
    }

    switch (DWCi_GetMatchCnt()->state)
    {
    case DWC_MATCH_STATE_CL_SEARCH_HOST:
        if (!DWCi_GetMatchCnt()->priorProfileID)
        {
            len = DWCi_GetDefaultMatchFilter(filter,
                DWCi_GetMatchCnt()->profileID,
                DWCi_GetMatchCnt()->qr2NumEntry,
                DWCi_GetMatchCnt()->qr2MatchType);

            if (stpAddFilter)
            {
                (void)snprintf(filter, sizeof(filter), "%s and (%s)", filter,
                    stpAddFilter);
            }
            break;
        }
        else
        {
            profileID = DWCi_GetMatchCnt()->priorProfileID;
        }

    case DWC_MATCH_STATE_CL_SEARCH_OWN:
    case DWC_MATCH_STATE_CL_WAIT_RESV:
    case DWC_MATCH_STATE_CL_SEARCH_NN_HOST:
        (void)snprintf(filter, sizeof(filter), "%s = %u",
            DWC_QR2_PID_KEY_STR, profileID);
        DWCi_GetMatchCnt()->reqProfileID = profileID;
        break;

    default:
        DWC_Printf(DWC_REPORTFLAG_ERROR,
            "---DWCi_SBUpdateAsync() illegal state %d.\n",
            DWCi_GetMatchCnt()->state);
        break;
    }

    DWC_Printf(DWC_REPORTFLAG_MATCH_NN, "ServerBrowserFilter : %s\n", filter);

    ServerBrowserClear(DWCi_GetMatchCnt()->sbObj);

    for (i = 0; i < DWC_DNS_ERROR_RETRY_MAX; i++)
    {
        sbError = ServerBrowserLimitUpdate(DWCi_GetMatchCnt()->sbObj, SBTrue,
            SBFalse, basicFields, numBasicFields, filter,
            DWC_SB_UPDATE_MAX_SERVERS);
        if (!sbError)
            break;
        else if (sbError != sbe_dnserror)
            break;
        DWC_Printf(0x400, "Retry\n");
    }

    if (sbError == sbe_noerror)
    {
        DWCi_GetMatchCnt()->sbUpdateRequestTick
            = DWCi_Np_GetTick()
            + (u64)(OS_BUS_CLOCK_SPEED / 4 / 1000) * 30000;
    }

    return sbError;
}

static int DWCi_GetDefaultMatchFilter(
    char* filter, int profileID, u8 numEntry, u8 matchType)
{
    return snprintf(filter, MAX_FILTER_LEN,
        "%s = %d and %s != %u and maxplayers = %d and numplayers < %d and %s "
        "= %d and %s != %s",
        DWC_QR2_MATCH_VER_KEY_STR, DWC_MATCHING_VERSION,
        DWC_QR2_PID_KEY_STR, profileID, numEntry, numEntry,
        DWC_QR2_MATCH_TYPE_KEY_STR, matchType,
        DWC_QR2_MATCH_RESV_KEY_STR, DWC_QR2_PID_KEY_STR);
}

static int IsPrivateAddr(u32 addr_in_netorder)
{
    u32 addr_in_hostorder;
    u8 addr[4];

    addr_in_hostorder = SONtoHl(addr_in_netorder);
    addr[0] = (u8)((addr_in_hostorder & 0xff000000) >> 24);
    addr[1] = (u8)((addr_in_hostorder & 0x00ff0000) >> 16);
    addr[2] = (u8)((addr_in_hostorder & 0x0000ff00) >> 8);
    addr[3] = (u8)(addr_in_hostorder & 0x000000ff);

    if (addr[0] == 10)
        return TRUE;
    if (addr[0] == 172 && addr[1] >= 16 && addr[1] <= 31)
        return TRUE;
    if (addr[0] == 192 && addr[1] == 168)
        return TRUE;
    return FALSE;
}

static NegotiateError DWCi_NNStartupAsync(
    int isQR2, int cookie, SBServer server)
{
    u32 ip;
    u32 senddata[2];
    int index = DWCi_GetMatchCnt()->qr2NNFinishCount;
    int result;
    BOOL nnValid;
    NegotiateError nnError = ne_noerror;

    if (!isQR2)
    {
        cookie = (DWCi_GetMatchCnt()->profileID & 0x0ffff)
            | (DWCi_GetMatchCnt()->nnCookieRand << 16);
        if (SBServerHasPrivateAddress(server))
        {
            if (SBServerGetPublicInetAddress(server)
                == ServerBrowserGetMyPublicIPAddr(DWCi_GetMatchCnt()->sbObj))
            {
                DWC_Printf(0x40, "Server[%d] is behind same NAT as me.\n",
                    index);
                DWCi_GetMatchCnt()->ipList[index]
                    = SBServerGetPrivateInetAddress(server);
                DWCi_GetMatchCnt()->portList[index]
                    = SBServerGetPrivateQueryPort(server);
                nnValid = FALSE;
            }
            else
            {
                DWC_Printf(0x40, "Server[%d] is behind NAT.\n", index);
                nnValid = TRUE;
            }
        }
        else
        {
            ip = SOGetHostID();
            if (IsPrivateAddr(ip))
            {
                DWC_Printf(0x40,
                    "Server[%d] is not behind NAT. But I'm behind NAT.\n",
                    index);
                nnValid = TRUE;
            }
            else
            {
                DWC_Printf(0x40,
                    "Both I and Server[%d] are not behind NAT.\n", index);
                DWCi_GetMatchCnt()->ipList[index]
                    = SBServerGetPublicInetAddress(server);
                DWCi_GetMatchCnt()->portList[index]
                    = SBServerGetPublicQueryPort(server);
                nnValid = FALSE;
            }
        }

        if (nnValid)
        {
            DWCi_GetMatchCnt()->nnCookieRand
                = (u16)DWCi_GetMathRand32(0x10000);
            DWCi_GetMatchCnt()->nnInfo.cookie = cookie;
        }
        else
        {
            senddata[0] = (u32)SOGetHostID();
            senddata[1] = DWCi_HtoLEl(
                gt2GetLocalPort(*DWCi_GetMatchCnt()->pGt2Socket));
            result = DWCi_SendMatchCommand(DWC_MATCH_COMMAND_TELL_ADDR,
                DWCi_GetMatchCnt()->sbPidList[index],
                SBServerGetPublicInetAddress(server),
                SBServerGetPublicQueryPort(server), senddata, 2);
            DWCi_GetMatchCnt()->_3E1 = 0;
            if (result)
                return ne_socketerror;
            DWCi_GetMatchCnt()->nnInfo.cookie = 0;
        }

        DWCi_GetMatchCnt()->nnInfo.isQR2 = 0;
        DWCi_GetMatchCnt()->nnInfo.retryCount = 0;
        DWCi_GetMatchCnt()->nnInfo.port
            = SBServerGetPublicQueryPort(server);
        DWCi_GetMatchCnt()->nnInfo.ip = SBServerGetPublicInetAddress(server);
    }
    else
    {
        DWCi_GetMatchCnt()->nnInfo.isQR2 = 1;
        DWCi_GetMatchCnt()->nnInfo.retryCount = 0;
        DWCi_GetMatchCnt()->nnInfo.port = 0;
        DWCi_GetMatchCnt()->nnInfo.ip = 0;
        DWCi_GetMatchCnt()->nnInfo.cookie = cookie;
        nnValid = TRUE;
    }

    if (nnValid)
    {
        nnError = DWCi_DoNatNegotiationAsync(&DWCi_GetMatchCnt()->nnInfo);
    }
    else
    {
        DWCi_NNCompletedCallback(nr_success,
            gt2GetSocketSOCKET(*DWCi_GetMatchCnt()->pGt2Socket), NULL,
            &DWCi_GetMatchCnt()->nnInfo);
        DWCi_GetMatchCnt()->nnFinishTime = 0;
    }

    return nnError;
}

static NegotiateError DWCi_DoNatNegotiationAsync(DWCNNInfo* nnInfo)
{
    SBError sbError;
    NegotiateError nnError;
    int i;

    if (!nnInfo->isQR2)
    {
        sbError = ServerBrowserSendNatNegotiateCookieToServer(
            DWCi_GetMatchCnt()->sbObj,
            gt2AddressToString(nnInfo->ip, 0, NULL), nnInfo->port,
            nnInfo->cookie);
        if (DWCi_HandleSBError(sbError))
            return ne_socketerror;
        DWC_Printf(0x40, "Send NN cookie = %x.\n", nnInfo->cookie);
    }

    for (i = 0; i < DWC_DNS_ERROR_RETRY_MAX; ++i)
    {
        nnError = NNBeginNegotiationWithSocket(
            gt2GetSocketSOCKET(*DWCi_GetMatchCnt()->pGt2Socket),
            nnInfo->cookie, nnInfo->isQR2, DWCi_NNProgressCallback,
            DWCi_NNCompletedCallback, nnInfo);
        if (nnError == ne_noerror || nnError != ne_dnserror)
            break;
        DWC_Printf(4,
            " dns error occurs when NatNegotiation begin... retry\n");
    }

    return nnError;
}

static int DWCi_SendMatchCommand(u8 command, int profileID, u32 ip, u16 port,
    const u32 data[], int len)
{
    char message[DWC_MATCH_COMMAND_ADD_MESSAGE_MAX];
    char tmpStr[16];
    int msgLen = 0, tmpLen;
    int result;
    int i;

    if ((DWCi_GetMatchCnt()->qr2MatchType == DWC_MATCH_TYPE_ANYBODY)
        || (((DWCi_GetMatchCnt()->qr2MatchType == DWC_MATCH_TYPE_SC_CL)
                || DWCi_GetMatchCnt()->distantFriend)
            && (command == DWC_MATCH_COMMAND_TELL_ADDR)))
    {
        result = (int)DWCi_SendSBMsgCommand(command, ip, port, data, len);
    }
    else
    {
        if (data && len)
        {
            msgLen = snprintf(message, sizeof(message), "%u", data[0]);

            for (i = 1; i < len; i++)
            {
                tmpLen = snprintf(tmpStr, sizeof(tmpStr), "/%u", data[i]);
                DWCi_Np_CpuCopy8(
                    tmpStr, &message[msgLen], (u32)tmpLen);
                msgLen += tmpLen;
            }
        }
        message[msgLen] = '\0';

        result = (int)DWCi_SendGPBuddyMsgCommand(
            DWCi_GetMatchCnt()->pGpObj, command, profileID, message);
    }

    if ((command == DWC_MATCH_COMMAND_RESV_OK)
        || (command == DWC_MATCH_COMMAND_TELL_ADDR)
        || (command == DWC_MATCH_COMMAND_LINK_CLS_REQ)
        || (command == DWC_MATCH_COMMAND_LINK_CLS_SUC))
    {
        DWCi_GetMatchCnt()->_3E0 = command;
        DWCi_GetMatchCnt()->_3E2 = port;
        DWCi_GetMatchCnt()->_3E4 = ip;
        DWCi_GetMatchCnt()->_468 = profileID;
        DWCi_GetMatchCnt()->_46C = len;
        DWCi_GetMatchCnt()->_470 = DWCi_Np_GetTick();
        if (data && len)
        {
            DWCi_Np_CpuCopy32(
                data, DWCi_GetMatchCnt()->_3E8, (u32)len * 4);
        }
    }

    return result;
}

static SBError DWCi_SendSBMsgCommand(
    u8 command, u32 ip, u16 port, const u32 data[], int len)
{
    int i;
    DWCSBMessage senddata;
    SBError sbError;

    if (data && len)
    {
        DWCi_Np_CpuCopy32(data, senddata.data, sizeof(u32) * len);
    }
    else
    {
        len = 0;
    }

    strcpy(senddata.header.identifier, DWC_SB_COMMAND_STRING);
    senddata.header.version = DWCi_HtoLEl(DWC_MATCHING_VERSION);
    senddata.header.command = command;
    senddata.header.size = (u8)(sizeof(u32) * len);
    senddata.header.qr2Port = DWCi_HtoLEs(DWCi_GetMatchCnt()->qr2Port);
    senddata.header.qr2IP = DWCi_GetMatchCnt()->qr2IP;
    senddata.header.profileID = DWCi_HtoLEl(DWCi_GetMatchCnt()->profileID);

    DWC_Printf(DWC_REPORTFLAG_MATCH_NN,
        "<SB> SEND-0x%02x -> [%08x:%d] [pid=--------]\n", command, ip,
        port);

    for (i = 0; i < DWC_DNS_ERROR_RETRY_MAX; i++)
    {
        sbError = ServerBrowserSendMessageToServerA(DWCi_GetMatchCnt()->sbObj,
            gt2AddressToString(ip, 0, NULL), port, (gsi_char*)&senddata,
            (int)(sizeof(DWCSBMessageHeader) + senddata.header.size));

        if (!sbError)
        {
            break;
        }
        else if (sbError != sbe_dnserror)
        {
            break;
        }
    }

    return sbError;
}

static GPResult DWCi_SendGPBuddyMsgCommand(GPConnection* connection,
    u8 command, int profileID, const char* message)
{
    char tmpStr[DWC_MATCH_COMMAND_ADD_MESSAGE_MAX];
    u32 len, len2;
    GPResult result;

    len = (u32)snprintf(tmpStr, sizeof(tmpStr), "%s%dv%s",
        DWC_GP_COMMAND_STRING, DWC_MATCHING_VERSION,
        DWC_GP_COMMAND_MATCH_STRING);
    tmpStr[len] = (char)command;
    tmpStr[len + 1] = '\0';

    if (message)
    {
        len2 = strlen(message);
        DWCi_Np_CpuCopy8(message, &tmpStr[len + 1], len2);
        tmpStr[len + 1 + len2] = '\0';
    }

    result = gpSendBuddyMessage(connection, profileID, tmpStr);

    DWC_Printf(DWC_REPORTFLAG_MATCH_NN,
        "<GP> SEND-0x%02x -> [--------:-----] [pid=%u]\n", command,
        profileID);

    return result;
}

int fn_80493B94(char* dstMsg, const char* srcMsg, int index)
{
    const char* pSrcBegin = srcMsg;
    char* pSrcNext = NULL;
    char* pSrcEnd;
    int len;
    int i;

    pSrcEnd = strchr(pSrcBegin, '\0');

    for (i = 0; i < index; i++)
    {
        pSrcNext = strchr(pSrcBegin, '/');
        if (pSrcNext == NULL)
        {
            return -1;
        }
        pSrcBegin = pSrcNext + 1;
    }

    pSrcNext = strchr(pSrcBegin, '/');
    if (pSrcNext == NULL)
    {
        pSrcNext = pSrcEnd;
    }
    if (pSrcBegin == pSrcNext)
    {
        return -1;
    }
    len = pSrcNext - pSrcBegin;
    memcpy(dstMsg, pSrcBegin, len);
    dstMsg[len] = '\0';
    return len;
}

static BOOL DWCi_ProcessRecvMatchCommand(u8 command, int srcPid, u32 srcIP,
    u16 srcPort, const u32 data[], int len)
{
    u8 sendCommand;
    u8 aid;
    u16 port = 0;
    u32 ip = 0;
    u32 aidIdx;
    u32 senddata[DWC_MAX_MATCH_IDX_LIST + 1];
    int profileID;
    int sendlen = 0;
    int result;
    int i;
    SBError sbError;
    NegotiateError nnError;
    struct sockaddr_in remoteaddr;

    if (stpMatchCnt == NULL || stpMatchCnt->state == 0)
    {
        return TRUE;
    }

    if (DWC_GetState() == DWC_STATE_MATCHING && stpMatchCnt->state == 1
        && (int)srcPid == stpMatchCnt->sbPidList[0])
    {
        DWCi_InitClWaitTimeout();
    }

    switch (command)
    {
    case 1:
    case 11:
    {
        if (DWCi_GetMatchCnt()->qr2MatchType != DWC_MATCH_TYPE_ANYBODY)
        {
            srcIP = data[1];
            srcPort = (u16)DWCi_LEtoHl(data[2]);
            DWC_Printf(DWC_REPORTFLAG_DEBUG,
                "friend IP:%x, port:%d\n", srcIP, srcPort);
        }
        sendCommand = DWCi_CheckResvCommand(srcPid, srcIP, srcPort,
            DWCi_LEtoHl(data[0]),
            command == DWC_MATCH_COMMAND_RESV_PRIOR ? TRUE : FALSE);
        if (sendCommand == DWC_MATCH_COMMAND_RESV_OK)
        {
            DWCi_GetMatchCnt()->friendAcceptBit = 0;
            result = DWCi_ProcessResvOK(srcPid, srcIP, srcPort);
            if (DWCi_HandleMatchCommandError(result))
            {
                return FALSE;
            }

            if (DWCi_GetMatchCnt()->qr2MatchType == DWC_MATCH_TYPE_SC_SV
                && DWCi_GetMatchCnt()->newClientCallback)
            {
                DWCi_GetMatchCnt()->newClientCallback(
                    DWCi_GetFriendListIndex(srcPid),
                    DWCi_GetMatchCnt()->newClientParam);
            }

            senddata[0] = DWCi_HtoLEl(DWCi_GetMatchCnt()->qr2NNFinishCount);
            for (sendlen = 1; sendlen <= DWCi_GetMatchCnt()->qr2NNFinishCount;
                 sendlen++)
            {
                senddata[sendlen]
                    = DWCi_HtoLEl(DWCi_GetMatchCnt()->sbPidList[sendlen]);
            }
            senddata[sendlen++] = DWCi_GetMatchCnt()->qr2IP;
            senddata[sendlen++] = DWCi_HtoLEl(DWCi_GetMatchCnt()->qr2Port);
            DWCi_SetMatchStatus(DWC_MATCH_STATE_SV_OWN_NN);
        }
        else if (sendCommand == DWC_MATCH_COMMAND_RESV_DENY)
        {
            if ((DWCi_GetMatchCnt()->qr2NumEntry > 0)
                && (DWCi_GetMatchCnt()->qr2NNFinishCount
                    == DWCi_GetMatchCnt()->qr2NumEntry))
            {
                if (DWCi_GetMatchCnt()->qr2MatchType
                    == DWC_MATCH_TYPE_SC_SV)
                {
                    senddata[0]
                        = DWCi_HtoLEl(DWC_MATCH_RESV_DENY_REASON_SV_FULL);
                    sendlen = 1;
                }
            }
        }

        if (sendCommand != DWC_MATCH_COMMAND_DUMMY)
        {
            result = DWCi_SendMatchCommand(
                sendCommand, srcPid, srcIP, srcPort, senddata, sendlen);
            if (DWCi_HandleMatchCommandError(result))
            {
                return FALSE;
            }
        }
        break;
    }

    case 2:
    {
        u32 num = DWCi_LEtoHl(data[0]);

        if (stpMatchCnt->state != 4)
        {
            break;
        }
        DWC_Printf(0x40, "Succeeded NN reservation.\n");
        if ((int)srcPid != stpMatchCnt->reqProfileID)
        {
            break;
        }
        stpMatchCnt->priorProfileID = 0;
        stpMatchCnt->resvWaitCount = 0;
        stpMatchCnt->cmdTimeoutTime = 0;
        stpMatchCnt->cmdResendFlag = 0;
        stpMatchCnt->qr2IPList[0] = data[num + 1];
        stpMatchCnt->qr2PortList[0] = DWCi_LEtoHl(data[num + 2]);
        stpMatchCnt->searchIP = data[num + 1];
        stpMatchCnt->searchPort = DWCi_LEtoHl(data[num + 2]);
        DWC_Printf(4, "Server IP:%x, port:%d\n", stpMatchCnt->searchIP,
            stpMatchCnt->searchPort);

        if (DWCi_GetMatchCnt()->qr2MatchType == DWC_MATCH_TYPE_FRIEND)
        {
            if (DWCi_AreAllBuddies(&data[1], num))
            {
                if (DWCi_GetMatchCnt()->gt2NumConnection)
                {
                    DWCi_MakeBackupServerData(srcPid, data);
                }
            }
            else
            {
                DWC_Printf(
                    DWC_REPORTFLAG_MATCH_NN, "But some clients are not friends.\n");

                result = DWCi_CancelReservation(srcPid);
                if (DWCi_HandleMatchCommandError(result))
                {
                    return FALSE;
                }

                result = DWCi_SendResvCommandToFriend(FALSE, FALSE, srcPid);
                if (DWCi_HandleMatchCommandError(result))
                {
                    return FALSE;
                }
                break;
            }
        }

        if (DWCi_GetMatchCnt()->qr2MatchType == DWC_MATCH_TYPE_ANYBODY)
        {
            if (DWCi_GetMatchCnt()->gt2NumConnection)
            {
                DWCi_MakeBackupServerData(srcPid, data);

                result = DWCi_ChangeToClient();
                if (DWCi_HandleMatchCommandError(result))
                {
                    return FALSE;
                }
            }

            DWCi_SetMatchStatus(DWC_MATCH_STATE_CL_NN);
            nnError = DWCi_NNStartupAsync(
                0, 0, ServerBrowserGetServer(DWCi_GetMatchCnt()->sbObj, 0));
            if (DWCi_HandleNNError(nnError) != 0)
            {
                return FALSE;
            }
        }
        else
        {
            DWCi_SetMatchStatus(DWC_MATCH_STATE_CL_SEARCH_NN_HOST);
            sbError = DWCi_SBUpdateAsync(srcPid);
            if (DWCi_HandleSBError(sbError) != 0)
            {
                return FALSE;
            }
        }
        break;
    }

    case 3:
        if (stpMatchCnt->state != 4)
        {
            break;
        }
        if ((int)srcPid != stpMatchCnt->reqProfileID)
        {
            break;
        }
        DWC_Printf(0x40, "Reservation was denied by %u.\n", srcPid);
        if (len > 0 && DWCi_LEtoHl(data[0]) == 0x10)
        {
            DWC_Printf(0x40, "Game server is fully occupied.\n");
            DWCi_StopMatching(DWC_ERROR_SERVER_FULL, 0);
            return FALSE;
        }
        return DWCi_RetryReserving(stpMatchCnt->reqProfileID);

    case 4:
        if (DWCi_GetMatchCnt()->state != DWC_MATCH_STATE_CL_WAIT_RESV)
        {
            break;
        }
        if (srcPid == DWCi_GetMatchCnt()->reqProfileID)
        {
            DWCi_GetMatchCnt()->cmdTimeoutStartTick = DWCi_Np_GetTick();
            if ((DWCi_GetMatchCnt()->priorProfileID
                    && DWCi_GetMatchCnt()->resvWaitCount
                        < DWC_RESV_COMMAND_RETRY_MAX)
                || DWCi_GetMatchCnt()->qr2MatchType == DWC_MATCH_TYPE_SC_CL)
            {
                DWCi_GetMatchCnt()->cmdResendFlag = TRUE;
                DWCi_GetMatchCnt()->cmdResendTick = DWCi_Np_GetTick();
                if (DWCi_GetMatchCnt()->qr2MatchType != DWC_MATCH_TYPE_SC_CL)
                {
                    DWCi_GetMatchCnt()->resvWaitCount++;
                }
                break;
            }

            DWCi_GetMatchCnt()->priorProfileID = 0;
            DWCi_GetMatchCnt()->resvWaitCount = 0;
            if (DWCi_GetMatchCnt()->qr2MatchType == DWC_MATCH_TYPE_ANYBODY)
            {
                DWCi_SetMatchStatus(DWC_MATCH_STATE_CL_SEARCH_HOST);
                DWCi_GetMatchCnt()->sbUpdateFlag
                    = DWC_SB_UPDATE_INTERVAL_SHORT;
                DWCi_GetMatchCnt()->sbUpdateTick = DWCi_Np_GetTick();
            }
            else if (DWCi_GetMatchCnt()->qr2MatchType == DWC_MATCH_TYPE_FRIEND)
            {
                (void)DWCi_SendResvCommandToFriend(TRUE, FALSE, 0);
            }
        }
        break;

    case 5:
        if (stpMatchCnt->qr2IsReserved == 0)
        {
            break;
        }
        if ((int)srcPid != stpMatchCnt->qr2Reservation)
        {
            break;
        }
        if (stpMatchCnt->qr2MatchType == 2 && stpMatchCnt->gt2NumConnection == 1
            && (int)srcPid == stpMatchCnt->sbPidList[1])
        {
            gt2CloseAllConnectionsHard(*stpMatchCnt->pGt2Socket);
        }
        if (DWCi_CancelPreConnectedServerProcess(srcPid) == 0)
        {
            return FALSE;
        }
        break;

    case 6:
        ip = data[0];
        port = DWCi_LEtoHl(data[1]);
        DWC_Printf(0x40,
            "NN parent is behind same NAT as me. Received IP %x & "
            "port %d\n",
            ip, port);
        if (stpMatchCnt->state == 1)
        {
            stpMatchCnt->state = 6;
        }
        else if ((stpMatchCnt->state != 6 && stpMatchCnt->state != 11)
            || (int)srcPid != stpMatchCnt->qr2Reservation)
        {
            DWC_Printf(0x40, "But already canceled reservation.\n");
            break;
        }
        stpMatchCnt->_3E0 = 0xFF;
        if ((int)srcPid
            != stpMatchCnt->sbPidList[stpMatchCnt->gt2NumConnection + 1])
        {
            stpMatchCnt->sbPidList[stpMatchCnt->gt2NumConnection + 1] = srcPid;
        }
        remoteaddr.sin_addr.addr = ip;
        remoteaddr.sin_port = SOHtoNs(port);
        stpMatchCnt->nnInfo.isQR2 = 1;
        DWCi_NNCompletedCallback(nr_success,
            gt2GetSocketSOCKET(*stpMatchCnt->pGt2Socket), &remoteaddr,
            &stpMatchCnt->nnInfo);
        stpMatchCnt->nnFinishTime = 0;
        break;

    case 7:
        if (stpMatchCnt->state != 1
            || (int)srcPid != stpMatchCnt->sbPidList[0])
        {
            DWC_Printf(4, "Ignore delayed NEW_PID_AID command.\n");
            break;
        }
        profileID = (int)DWCi_LEtoHl(data[0]);
        aid = (u8)DWCi_LEtoHl(data[1]);
        if (stpMatchCnt->qr2MatchType == 1 && stpMatchCnt->distantFriend == 0)
        {
            senddata[0]
                = DWCi_HtoLEl((u32)DWCi_IsFriendByIdxList(profileID));
            DWC_Printf(0x40, "profileID %d is acceptable? - %d.\n",
                profileID, senddata[0]);
            result = DWCi_SendMatchCommand(
                0x20, srcPid, srcIP, srcPort, senddata, 1);
            if (DWCi_HandleMatchCommandError(result))
            {
                return FALSE;
            }
        }
        stpMatchCnt->sbPidList[stpMatchCnt->qr2NNFinishCount + 1] = profileID;
        stpMatchCnt->aidList[stpMatchCnt->qr2NNFinishCount + 1] = aid;
        qr2_send_statechanged(stpMatchCnt->qr2Obj);
        if (DWCi_GetMatchCnt()->newClientCallback)
        {
            DWCi_GetMatchCnt()->newClientCallback(
                DWCi_GetFriendListIndex(profileID),
                DWCi_GetMatchCnt()->newClientParam);
        }
        DWC_Printf(0x40,
            "Received new client's profileID = %u & aid = %d.\n", profileID,
            aid);
        break;

    case 8:
        if (DWCi_GetMatchCnt()->state != 1
            || (int)srcPid != DWCi_GetMatchCnt()->sbPidList[0])
        {
            DWC_Printf(4, "Ignore delayed LINK_CLS_REQ command.\n");
            break;
        }
        profileID = (int)DWCi_LEtoHl(data[0]);
        if (profileID == 0)
        {
            aidIdx = DWCi_LEtoHl(data[1]);
            aid = (u8)DWCi_LEtoHl(data[2]);
            DWCi_GetMatchCnt()->aidList[aidIdx] = aid;
            DWCi_GetMatchCnt()->sbPidList[aidIdx] = DWCi_GetMatchCnt()->profileID;
            DWCi_PostProcessConnection(DWC_PP_CONNECTION_CL_FINISH_CONNECT);
        }
        else
        {
            aidIdx = DWCi_LEtoHl(data[1]);
            aid = (u8)DWCi_LEtoHl(data[2]);
            if (DWCi_GetMatchCnt()->sbPidList[aidIdx] == profileID
                && aidIdx == (u32)(DWCi_GetMatchCnt()->gt2NumConnection - 1))
            {
                u32 senddata[1];

                senddata[0] = DWCi_HtoLEl((u32)profileID);
                DWC_Printf(0x40,
                    "Resend command %d for delayed command %d.\n", 9, 8);
                result = DWCi_SendMatchCommand(9, srcPid,
                    DWCi_GetMatchCnt()->qr2IPList[0], DWCi_GetMatchCnt()->qr2PortList[0],
                    senddata, 1);
                if (DWCi_HandleMatchCommandError(result))
                {
                    return FALSE;
                }
                break;
            }
            DWCi_GetMatchCnt()->sbPidList[aidIdx] = profileID;
            DWCi_GetMatchCnt()->aidList[aidIdx] = aid;
            DWCi_GetMatchCnt()->qr2IPList[aidIdx] = data[3];
            DWCi_GetMatchCnt()->qr2PortList[aidIdx] = (u16)DWCi_LEtoHl(data[4]);
            DWCi_GetMatchCnt()->searchIP = data[3];
            DWCi_GetMatchCnt()->searchPort = (u16)DWCi_LEtoHl(data[4]);
            DWC_Printf(4, "Client IP:%x, port:%d\n", DWCi_GetMatchCnt()->searchIP,
                DWCi_GetMatchCnt()->searchPort);
            DWCi_GetMatchCnt()->state = 5;
            DWC_Printf(0x40, "Next, try to connect to %u.\n", profileID);
            sbError = DWCi_SBUpdateAsync(profileID);
            if (DWCi_HandleSBError(sbError) != 0)
            {
                return FALSE;
            }
            DWCi_GetMatchCnt()->cmdTimeoutTime = 0;
            DWCi_GetMatchCnt()->cmdResendFlag = 0;
        }
        break;

    case 9:
        if (DWCi_GetMatchCnt()->state == 13
            && (u32)DWCi_GetMatchCnt()->sbPidList[DWCi_GetMatchCnt()->clLinkProgress + 1]
                == DWCi_LEtoHl(data[0]))
        {
            DWCi_GetMatchCnt()->clLinkProgress++;
            DWCi_PostProcessConnection(DWC_PP_CONNECTION_SV_CONNECT);
        }
        else
        {
            DWC_Printf(0x40, "Ignore delayed command %d.\n", 9);
        }
        break;

    case 10:
        if (stpMatchCnt->state != 1 && stpMatchCnt->state != 18)
        {
            DWC_Printf(4, "Ignore delayed CLOSE_LINK command.\n");
            break;
        }
        if (stpMatchCnt->qr2MatchType == DWC_MATCH_TYPE_ANYBODY
            || DWCi_AreAllBuddies(&data[1], DWCi_LEtoHl(data[0])))
        {
            stpMatchCnt->priorProfileID = (int)DWCi_LEtoHl(data[1]);
            stpMatchCnt->resvWaitCount = 0;
            DWC_Printf(0x40, "Received close command. Next try to %u.\n",
                DWCi_LEtoHl(data[1]));
        }
        else
        {
            stpMatchCnt->priorProfileID = 0;
            DWC_Printf(0x40,
                "Received close command. Server %u or its clients are "
                "not friends.\n",
                DWCi_LEtoHl(data[1]));
        }
        if (stpMatchCnt->gt2NumConnection != 0)
        {
            gt2CloseAllConnectionsHard(*stpMatchCnt->pGt2Socket);
            break;
        }
        if (DWCi_ResumeMatching() != 0)
        {
            return FALSE;
        }
        break;

    case 12:
        if ((int)srcPid != stpMatchCnt->sbPidList[0])
        {
            DWC_Printf(4, "Ignore delayed CANCEL command.\n");
            break;
        }
        DWC_Printf(0x40, "Received cancel command from %u data[0] = %d.\n",
            srcPid, DWCi_LEtoHl(data[0]));
        DWC_Printf(4, "numHost nn=%d gt2=%d, state %d\n",
            stpMatchCnt->qr2NNFinishCount, stpMatchCnt->gt2NumConnection, stpMatchCnt->state);
        if (stpMatchCnt->qr2MatchType == 0 || stpMatchCnt->qr2MatchType == 1)
        {
            if (DWCi_CancelPreConnectedClientProcess(srcPid) == 0)
            {
                return FALSE;
            }
        }
        else if (stpMatchCnt->qr2MatchType == 3)
        {
            stpMatchCnt->cbEventPid = srcPid;
            stpMatchCnt->closeState = 2;
            gt2CloseAllConnectionsHard(*stpMatchCnt->pGt2Socket);
            stpMatchCnt->closeState = 0;
            DWCi_RestartFromCancel(0);
        }
        break;

    case 13:
    case 14:
    case 15:
        if (DWCi_ProcessCancelMatchSynCommand(srcPid, command, DWCi_LEtoHl(data[0])) == 0)
        {
            return FALSE;
        }
        break;

    case 16:
        if ((int)srcPid != stpMatchCnt->sbPidList[0])
        {
            return TRUE;
        }
        DWC_Printf(4, "Close shutdown client.\n");
        for (i = 0; i < len; i++)
        {
            aid = DWCi_GetAIDFromProfileID(
                (int)DWCi_LEtoHl(data[i]), FALSE);
            if (aid != 0xFF)
            {
                DWC_CloseConnectionHard(aid);
            }
        }
        break;

    case 17:
        if (stpOptMinComp != NULL && stpOptMinComp->valid != 0
            && DWCi_Np_TicksToMilliSeconds(
                   DWCi_Np_GetTick() - stpOptMinComp->startTime)
                >= stpOptMinComp->timeout)
        {
            senddata[0] = 0x01000000;
            DWC_Printf(0x80, "[OPT_MIN_COMP] time is %lu.\n",
                DWCi_Np_TicksToMilliSeconds(
                    DWCi_Np_GetTick() - stpOptMinComp->startTime));
        }
        else
        {
            senddata[0] = 0;
        }
        result = DWCi_SendMatchCommand(
            18, srcPid, srcIP, srcPort, senddata, 1);
        if (DWCi_HandleMatchCommandError(result))
        {
            return FALSE;
        }
        break;

    case 18:
        if (stpMatchCnt->state != 19)
        {
            break;
        }
        aid = DWCi_GetAIDFromProfileID(srcPid, FALSE);
        if (aid == 0xFF)
        {
            break;
        }
        stpOptMinComp->recvBit |= 1 << aid;
        if (DWCi_LEtoHl(data[0]) != 0)
        {
            stpOptMinComp->timeoutBit |= 1 << aid;
        }
        break;

    case 19:
        DWCi_StopMatching(DWC_ERROR_MO_SC_CONNECT_BLOCK, 0);
        return FALSE;

    case 0x20:
        if (stpMatchCnt->qr2MatchType != 1 || stpMatchCnt->distantFriend != 0)
        {
            break;
        }
        for (i = 1; i <= stpMatchCnt->gt2NumConnection; i++)
        {
            if ((int)srcPid != stpMatchCnt->sbPidList[i])
            {
                continue;
            }
            DWC_Printf(0x40, "New client was accepted? - %d.\n",
                DWCi_LEtoHl(data[0]));
            if (DWCi_LEtoHl(data[0]) == 0)
            {
                if ((DWCi_GetMatchCnt()->state == DWC_MATCH_STATE_SV_OWN_NN)
                    || (DWCi_GetMatchCnt()->state
                        == DWC_MATCH_STATE_SV_OWN_GT2))
                {
                    result = DWCi_InvalidateReservation();
                    if (DWCi_HandleMatchCommandError(result))
                    {
                        return FALSE;
                    }
                }
                if (stpMatchCnt->nnInfo.cookie != 0)
                {
                    NNCancel(stpMatchCnt->nnInfo.cookie);
                    stpMatchCnt->nnInfo.cookie = 0;
                }
                DWCi_RestartFromTimeout();
            }
            else
            {
                stpMatchCnt->friendAcceptBit
                    |= 1 << DWCi_GetAIDFromProfileID(srcPid, FALSE);
            }
            break;
        }
        break;

    case 0x40:
        for (i = 1; i <= stpMatchCnt->gt2NumConnection; i++)
        {
            if ((int)srcPid != stpMatchCnt->sbPidList[i])
            {
                continue;
            }
            result = DWCi_SendMatchCommand(
                0x41, srcPid, srcIP, srcPort, NULL, 0);
            if (DWCi_HandleMatchCommandError(result))
            {
                return FALSE;
            }
            break;
        }
        break;

    case 0x41:
        break;

    default:
        DWC_Printf(2, "Received unexpected matching command 0x%02x.\n",
            command);
        break;
    }
    return TRUE;
}


static u8 DWCi_CheckResvCommand(
    int profileID, u32 qr2IP, u16 qr2Port, u32 matchType, BOOL priorFlag)
{
    u8 sendCommand;
    int result;

    switch (DWCi_GetMatchCnt()->qr2MatchType)
    {
    case DWC_MATCH_TYPE_FRIEND:
        if (!gpIsBuddy(DWCi_GetMatchCnt()->pGpObj, profileID))
        {
            sendCommand = DWC_MATCH_COMMAND_DUMMY;
            break;
        }
        else if (!DWCi_IsFriendByIdxList(profileID))
        {
            sendCommand = DWC_MATCH_COMMAND_RESV_DENY;
            DWC_Printf(DWC_REPORTFLAG_MATCH_NN,
                "This friend doesn't exist in friendIdxList.\n");
            break;
        }

    case DWC_MATCH_TYPE_ANYBODY:
        if ((matchType != DWCi_GetMatchCnt()->qr2MatchType)
            || (DWCi_GetMatchCnt()->cancelState != DWC_MATCH_CANCEL_STATE_INIT)
            || (DWCi_GetMatchCnt()->qr2NNFinishCount == DWCi_GetMatchCnt()->qr2NumEntry)
            || (DWCi_GetMatchCnt()->qr2IsReserved
                && (DWCi_GetMatchCnt()->qr2Reservation == DWCi_GetMatchCnt()->profileID)))
        {
            sendCommand = DWC_MATCH_COMMAND_RESV_DENY;

            if ((DWCi_GetMatchCnt()->qr2MatchType == DWC_MATCH_TYPE_ANYBODY)
                && !DWCi_GetMatchCnt()->qr2Obj->userstatechangerequested
                && DWCi_GetMatchCnt()->qr2IsReserved
                && (DWCi_GetMatchCnt()->qr2Reservation == DWCi_GetMatchCnt()->profileID))
            {
                qr2_send_statechanged(DWCi_GetMatchCnt()->qr2Obj);
            }
        }
        else if (((DWCi_GetMatchCnt()->state
                       != DWC_MATCH_STATE_CL_SEARCH_HOST)
                     && (DWCi_GetMatchCnt()->state
                         != DWC_MATCH_STATE_CL_WAIT_RESV))
            || ((DWCi_GetMatchCnt()->qr2IP == 0)
                && (DWCi_GetMatchCnt()->qr2Port == 0))
            || ((qr2IP == 0) && (qr2Port == 0)))
        {
            sendCommand = DWC_MATCH_COMMAND_RESV_WAIT;
        }
        else
        {
            if (DWCi_GetMatchCnt()->reqProfileID)
            {
                if (DWCi_GetMatchCnt()->reqProfileID == profileID)
                {
                    if (priorFlag
                        || ((DWCi_GetMatchCnt()->profileID < profileID)
                            && (profileID != DWCi_GetMatchCnt()->priorProfileID)))
                    {
                        sendCommand = DWC_MATCH_COMMAND_RESV_OK;
                    }
                    else
                    {
                        sendCommand = DWC_MATCH_COMMAND_DUMMY;
                    }
                }
                else
                {
                    if (priorFlag
                        || ((DWCi_GetMatchCnt()->profileID < profileID)
                            && !DWCi_GetMatchCnt()->priorProfileID))
                    {
                        result = DWCi_CancelReservation(
                            DWCi_GetMatchCnt()->reqProfileID);
                        if (DWCi_HandleMatchCommandError(result))
                        {
                            return DWC_MATCH_COMMAND_DUMMY;
                        }
                        else
                        {
                            sendCommand = DWC_MATCH_COMMAND_RESV_OK;
                        }
                    }
                    else
                    {
                        sendCommand = DWC_MATCH_COMMAND_RESV_DENY;
                    }
                }
            }
            else
            {
                sendCommand = DWC_MATCH_COMMAND_RESV_OK;
            }
        }
        break;

    case DWC_MATCH_TYPE_SC_SV:
        if (!gpIsBuddy(DWCi_GetMatchCnt()->pGpObj, profileID))
        {
            sendCommand = DWC_MATCH_COMMAND_DUMMY;
        }
        else if ((matchType != DWC_MATCH_TYPE_SC_CL)
            || (DWCi_GetMatchCnt()->qr2NNFinishCount == DWCi_GetMatchCnt()->qr2NumEntry))
        {
            sendCommand = DWC_MATCH_COMMAND_RESV_DENY;
        }
        else if ((stOptSCBlock.valid == 1) && (stOptSCBlock.lock == 1))
        {
            sendCommand = DWC_MATCH_COMMAND_SC_CONN_BLOCK;
        }
        else if ((DWCi_GetMatchCnt()->state != DWC_MATCH_STATE_SV_WAITING)
            || ((DWCi_GetMatchCnt()->qr2IP == 0)
                && (DWCi_GetMatchCnt()->qr2Port == 0))
            || ((qr2IP == 0) && (qr2Port == 0)))
        {
            sendCommand = DWC_MATCH_COMMAND_RESV_WAIT;
        }
        else
        {
            sendCommand = DWC_MATCH_COMMAND_RESV_OK;
        }
        break;
    }

    return sendCommand;
}

static int DWCi_ProcessResvOK(int profileID, u32 ip, u16 port)
{
    u32 senddata[2];
    int result;
    int i;

    if (DWCi_GetMatchCnt()->qr2IsReserved
        && (DWCi_GetMatchCnt()->qr2Reservation == profileID))
    {
        return 0;
    }

    DWCi_GetMatchCnt()->qr2IsReserved = 1;
    DWCi_GetMatchCnt()->qr2Reservation = profileID;
    DWCi_GetMatchCnt()->cmdResendFlag = FALSE;
    DWCi_GetMatchCnt()->cmdTimeoutTime = 0;
    qr2_send_statechanged(DWCi_GetMatchCnt()->qr2Obj);
    DWCi_GetMatchCnt()->reqProfileID = 0;
    DWCi_GetMatchCnt()->sbPidList[DWCi_GetMatchCnt()->qr2NNFinishCount + 1] = profileID;
    DWCi_GetMatchCnt()->qr2IPList[DWCi_GetMatchCnt()->qr2NNFinishCount + 1] = ip;
    DWCi_GetMatchCnt()->qr2PortList[DWCi_GetMatchCnt()->qr2NNFinishCount + 1] = port;
    DWCi_GetMatchCnt()->searchIP = ip;
    DWCi_GetMatchCnt()->searchPort = port;

    DWCi_GetMatchCnt()->aidList[DWCi_GetMatchCnt()->qr2NNFinishCount + 1]
        = DWCi_GetAIDFromList();

    senddata[0] = DWCi_HtoLEl((u32)profileID);
    senddata[1] = DWCi_HtoLEl(
        DWCi_GetMatchCnt()->aidList[DWCi_GetMatchCnt()->qr2NNFinishCount + 1]);

    for (i = 1; i <= DWCi_GetMatchCnt()->qr2NNFinishCount; i++)
    {
        result = DWCi_SendMatchCommand(7, DWCi_GetMatchCnt()->sbPidList[i],
            DWCi_GetMatchCnt()->qr2IPList[i], DWCi_GetMatchCnt()->qr2PortList[i],
            senddata, 2);
        if (result)
        {
            return result;
        }
    }

    DWCi_InitOptMinCompParam(TRUE);

    return 0;
}

static void DWCi_MakeBackupServerData(int profileID, const u32 data[])
{
    u32 len;

    len = data[0] + 2;
    if (len > 2)
    {
        DWCi_Np_CpuCopy32(
            &data[1], &DWCi_GetMatchCnt()->svDataBak[2], sizeof(u32) * (len - 2));
    }

    DWCi_GetMatchCnt()->svDataBak[0] = len - 1;
    DWCi_GetMatchCnt()->svDataBak[1] = (u32)profileID;
}

static int DWCi_SendResvCommand(int profileID, BOOL delay)
{
    u8 command;
    u32 senddata[3];
    int sendlen;
    int result;
    SBServer server;

    if (delay
        || ((DWCi_GetMatchCnt()->qr2IP == 0)
            && (DWCi_GetMatchCnt()->qr2Port == 0)))
    {
        DWCi_GetMatchCnt()->cmdResendFlag = TRUE;
        DWCi_GetMatchCnt()->cmdResendTick = DWCi_Np_GetTick();
        DWCi_GetMatchCnt()->sbPidList[0] = profileID;

        if (!delay)
        {
            DWC_Printf(DWC_REPORTFLAG_DEBUG,
                "Delay ResvCommand - qr2IP & qr2Port = 0.\n");
        }
        return 0;
    }

    if (DWCi_GetMatchCnt()->qr2MatchType == DWC_MATCH_TYPE_ANYBODY)
    {
        server = ServerBrowserGetServer(DWCi_GetMatchCnt()->sbObj, 0);
        DWCi_GetMatchCnt()->sbPidList[0]
            = SBServerGetIntValueA(server, DWC_QR2_PID_KEY_STR, 0);
        DWCi_GetMatchCnt()->qr2IPList[0] = SBServerGetPublicInetAddress(server);
        DWCi_GetMatchCnt()->qr2PortList[0] = SBServerGetPublicQueryPort(server);

        DWCi_GetMatchCnt()->reqProfileID = DWCi_GetMatchCnt()->sbPidList[0];
        sendlen = 1;
    }
    else
    {
        if (DWCi_GetMatchCnt()->qr2MatchType == DWC_MATCH_TYPE_FRIEND)
        {
            DWCi_GetMatchCnt()->sbPidList[0] = profileID;
        }

        DWCi_GetMatchCnt()->reqProfileID = profileID;

        senddata[1] = DWCi_GetMatchCnt()->qr2IP;
        senddata[2] = DWCi_HtoLEl(DWCi_GetMatchCnt()->qr2Port);
        sendlen = 3;
    }

    DWCi_GetMatchCnt()->cmdTimeoutTime = DWC_MATCH_CMD_RESV_TIMEOUT_MSEC;
    DWCi_GetMatchCnt()->cmdTimeoutStartTick = DWCi_Np_GetTick();
    DWCi_GetMatchCnt()->cmdResendFlag = FALSE;

    if (DWCi_GetMatchCnt()->priorProfileID)
    {
        command = DWC_MATCH_COMMAND_RESV_PRIOR;
    }
    else
    {
        command = DWC_MATCH_COMMAND_RESERVATION;
    }

    senddata[0] = DWCi_HtoLEl(DWCi_GetMatchCnt()->qr2MatchType);

    result = DWCi_SendMatchCommand(command, profileID,
        DWCi_GetMatchCnt()->qr2IPList[0], DWCi_GetMatchCnt()->qr2PortList[0], senddata,
        sendlen);

    return result;
}

static int DWCi_SendResvCommandToFriend(BOOL delay, BOOL init, int resendPid)
{
    char version[12];
    char numEntry[4];
    char distantFriend[2];
    int profileID;
    int startIdx = init ? DWCi_GetMatchCnt()->friendCount
                        : (DWCi_GetMatchCnt()->friendCount
                                  < DWCi_GetMatchCnt()->friendIdxListLen - 1)
            ? DWCi_GetMatchCnt()->friendCount + 1
            : 0;
    int buddyIdx;
    int initFinished = 0;
    int versionLen, numEntryLen, distantFriendLen;
    int i;
    GPBuddyStatus status;
    GPResult gpResult;

    while (1)
    {
        if (!init || initFinished)
        {
            DWCi_GetMatchCnt()->friendCount++;
            if (DWCi_GetMatchCnt()->friendCount >= DWCi_GetMatchCnt()->friendIdxListLen)
            {
                DWCi_GetMatchCnt()->friendCount = 0;
            }
        }

        if (initFinished)
        {
            if (DWCi_GetMatchCnt()->friendCount == startIdx)
            {
                DWCi_GetMatchCnt()->cmdTimeoutTime
                    = DWC_MATCH_CMD_RESEND_INTERVAL_MSEC;
                DWCi_GetMatchCnt()->cmdTimeoutStartTick = DWCi_Np_GetTick();
                DWCi_GetMatchCnt()->cmdResendFlag = FALSE;
                return 0;
            }
        }
        initFinished = 1;

        profileID = DWC_GetGsProfileId(DWCi_GetUserData(),
            (DWCAccFriendData*)((u8*)DWCi_GetMatchCnt()->friendList
                + DWCi_GetMatchCnt()
                          ->friendIdxList[DWCi_GetMatchCnt()->friendCount]
                    * sizeof(DWCAccFriendData)));

        if ((profileID == 0) || (profileID == -1))
            continue;

        if (!DWCi_Acc_IsValidFriendData(
                (DWCAccFriendData*)((u8*)DWCi_GetMatchCnt()->friendList
                    + DWCi_GetMatchCnt()
                              ->friendIdxList[DWCi_GetMatchCnt()->friendCount]
                        * sizeof(DWCAccFriendData))))
            continue;

        for (i = 1; i <= DWCi_GetMatchCnt()->gt2NumConnection; i++)
        {
            if (DWCi_GetMatchCnt()->sbPidList[i] == profileID)
                break;
        }

        if (i > DWCi_GetMatchCnt()->gt2NumConnection)
        {
            gpResult = gpGetBuddyIndex(
                DWCi_GetMatchCnt()->pGpObj, profileID, &buddyIdx);
            gpResult |= gpGetBuddyStatus(
                DWCi_GetMatchCnt()->pGpObj, buddyIdx, &status);

            if (!gpResult && (status.status == DWC_STATUS_MATCH_FRIEND))
            {
                versionLen = DWC_GetCommonValueString(
                    DWC_GP_SSTR_KEY_MATCH_VERSION, version,
                    status.statusString, '/');
                numEntryLen = DWC_GetCommonValueString(
                    DWC_GP_SSTR_KEY_MATCH_FRIEND_NUM, numEntry,
                    status.statusString, '/');
                distantFriendLen = DWC_GetCommonValueString(
                    DWC_GP_SSTR_KEY_DISTANT_FRIEND, distantFriend,
                    status.statusString, '/');

                if ((versionLen > 0) && (numEntryLen > 0)
                    && (distantFriendLen > 0)
                    && (strtoul(version, NULL, 10)
                        == DWC_MATCHING_VERSION)
                    && (strtoul(numEntry, NULL, 10)
                        == DWCi_GetMatchCnt()->qr2NumEntry))
                {
                    break;
                }
            }
        }
    }

    if (profileID == resendPid)
        delay = TRUE;

    return DWCi_SendResvCommand(profileID, delay);
}

static int DWCi_HandleMatchCommandError(int error)
{
    if (DWCi_GetMatchCnt()->qr2MatchType == DWC_MATCH_TYPE_ANYBODY)
    {
        return (int)DWCi_HandleSBError((SBError)error);
    }
    else
    {
        return (int)DWCi_HandleGPError((GPResult)error);
    }
}

static BOOL DWCi_RetryReserving(int resendPid)
{
    SBError sbError;
    int result;

    DWCi_GetMatchCnt()->priorProfileID = 0;
    DWCi_GetMatchCnt()->reqProfileID = 0;
    DWCi_GetMatchCnt()->resvWaitCount = 0;
    DWCi_GetMatchCnt()->cmdTimeoutStartTick = DWCi_Np_GetTick();

    if (DWCi_GetMatchCnt()->qr2MatchType == DWC_MATCH_TYPE_ANYBODY)
    {
        DWCi_SetMatchStatus(DWC_MATCH_STATE_CL_SEARCH_HOST);

        sbError = DWCi_SBUpdateAsync(0);
        if (DWCi_HandleSBError(sbError))
        {
            return FALSE;
        }
    }
    else if (DWCi_GetMatchCnt()->qr2MatchType == DWC_MATCH_TYPE_FRIEND)
    {
        result = DWCi_SendResvCommandToFriend(FALSE, FALSE, resendPid);
        if (DWCi_HandleMatchCommandError(result))
        {
            return FALSE;
        }
    }
    else if (DWCi_GetMatchCnt()->qr2MatchType == DWC_MATCH_TYPE_SC_CL)
    {
        DWCi_StopMatching(DWC_ERROR_NETWORK,
            DWC_ECODE_SEQ_MATCH + DWC_ECODE_TYPE_OTHER);
        return FALSE;
    }

    return TRUE;
}

static int DWCi_CancelReservation(int profileID)
{
    int result;

    result = DWCi_SendMatchCommand(DWC_MATCH_COMMAND_RESV_CANCEL, profileID,
        DWCi_GetMatchCnt()->qr2IPList[0], DWCi_GetMatchCnt()->qr2PortList[0], NULL, 0);

    DWCi_GetMatchCnt()->reqProfileID = 0;

    return result;
}

static BOOL DWCi_CancelPreConnectedServerProcess(int clientPid)
{
    int i;
    BOOL isServer;

    if (DWCi_GetMatchCnt()->qr2IsReserved
        && DWCi_GetMatchCnt()->qr2Reservation
            == DWCi_GetMatchCnt()->profileID)
    {
        DWC_Printf(DWC_REPORTFLAG_DEBUG,
            "DWCi_CancelPreConnectedServerProcess : client\n");
        isServer = FALSE;
    }
    else
    {
        DWC_Printf(DWC_REPORTFLAG_DEBUG,
            "DWCi_CancelPreConnectedServerProcess : server\n");
        isServer = TRUE;
    }

    if (isServer)
    {
        DWCi_GetMatchCnt()->qr2IsReserved = 0;
        DWCi_GetMatchCnt()->qr2Reservation = 0;
        qr2_send_statechanged(DWCi_GetMatchCnt()->qr2Obj);
    }

    if (DWCi_GetMatchCnt()->gt2NumConnection < DWC_MAX_CONNECTIONS - 1)
    {
        DWCi_GetMatchCnt()->sbPidList[
            DWCi_GetMatchCnt()->gt2NumConnection + 1] = 0;
    }

    DWCi_GetMatchCnt()->_3E0 = DWC_MATCH_COMMAND_DUMMY;

    if (DWCi_GetMatchCnt()->nnInfo.cookie)
    {
        NNCancel(DWCi_GetMatchCnt()->nnInfo.cookie);
        DWCi_GetMatchCnt()->nnInfo.cookie = 0;
    }

    DWCi_GetMatchCnt()->qr2NNFinishCount
        = (u8)DWCi_GetMatchCnt()->gt2NumConnection;
    DWCi_GetMatchCnt()->reqProfileID = 0;

    if (!isServer)
    {
        if (DWCi_GetMatchCnt()->qr2MatchType == DWC_MATCH_TYPE_SC_CL)
        {
        }
        else
        {
            DWCi_RestartFromTimeout();
        }
    }
    else if (DWCi_GetMatchCnt()->qr2MatchType == DWC_MATCH_TYPE_ANYBODY)
    {
        DWCi_SetMatchStatus(DWC_MATCH_STATE_CL_SEARCH_HOST);
        DWCi_GetMatchCnt()->sbUpdateFlag = DWC_SB_UPDATE_INTERVAL_LONG;
        DWCi_GetMatchCnt()->sbUpdateTick = DWCi_Np_GetTick();
    }
    else if (DWCi_GetMatchCnt()->qr2MatchType == DWC_MATCH_TYPE_FRIEND)
    {
        DWCi_SetMatchStatus(DWC_MATCH_STATE_CL_WAIT_RESV);
        (void)DWCi_SendResvCommandToFriend(TRUE, FALSE, 0);
    }
    else if (DWCi_GetMatchCnt()->qr2MatchType == DWC_MATCH_TYPE_SC_SV)
    {
        DWCi_SetMatchStatus(DWC_MATCH_STATE_SV_CANCEL_SYN);
        DWCi_GetMatchCnt()->cancelSynAckBit = 0;
        DWCi_GetMatchCnt()->cancelBaseLatency = 0;

        DWCi_CloseCancelHostAsync(clientPid);

        for (i = 1; i <= DWCi_GetMatchCnt()->gt2NumConnection; i++)
        {
            if (!DWCi_SendCancelMatchSynCommand(
                    DWCi_GetMatchCnt()->sbPidList[i],
                    DWC_MATCH_COMMAND_CANCEL_SYN))
                return FALSE;
        }

        if (DWCi_GetMatchCnt()->gt2NumConnection == 0)
        {
            DWCi_RestartFromCancel(DWC_MATCH_RESET_CONTINUE);
        }
    }

    return TRUE;
}

static BOOL DWCi_CancelPreConnectedClientProcess(int serverPid)
{
#pragma unused(serverPid)
    BOOL result = TRUE;

    if (DWCi_GetMatchCnt()->qr2MatchType == DWC_MATCH_TYPE_SC_CL)
    {
        if (DWCi_GetMatchCnt()->gt2NumConnection)
        {
            DWCi_CloseAllConnectionsByTimeout();
        }

        DWCi_StopMatching(DWC_ERROR_NETWORK,
            DWC_ECODE_SEQ_MATCH + DWC_ECODE_TYPE_SC_CL_FAIL);
        return FALSE;
    }

    DWCi_GetMatchCnt()->qr2NNFinishCount = (u8)DWCi_GetMatchCnt()->gt2NumConnection;
    DWCi_GetMatchCnt()->priorProfileID = 0;

    if (DWCi_GetMatchCnt()->nnInfo.cookie)
    {
        NNCancel(DWCi_GetMatchCnt()->nnInfo.cookie);
        DWCi_GetMatchCnt()->nnInfo.cookie = 0;
    }

    if (DWCi_GetMatchCnt()->gt2NumConnection)
    {
        DWC_Printf(DWC_REPORTFLAG_MATCH_NN,
            "Close all connection and restart matching.\n");

        DWCi_RestartFromTimeout();
    }
    else
    {
        DWC_Printf(DWC_REPORTFLAG_MATCH_NN,
            "Cancel and restart client process.\n");

        DWCi_SetMatchStatus(DWC_MATCH_STATE_CL_WAIT_RESV);

        DWC_Printf(
            DWC_REPORTFLAG_MATCH_NN, "Cancel and retry to reserve.\n");

        result = DWCi_RetryReserving(0);
    }

    return result;
}

static int DWCi_PostProcessConnection(DWCMatchPpConnectionType mode)
{
    int count = 3;
    BOOL clear = FALSE;
    u32 buf[5];
    int i;

    switch (mode)
    {
    case 0:
        if (stpMatchCnt->clLinkProgress < stpMatchCnt->gt2NumConnection - 1)
        {
            DWC_Printf(0x40, "Send client-client link request.\n");
            stpMatchCnt->state = 13;
            count = 5;
            buf[0] = fn_ByteSwap32(
                stpMatchCnt->sbPidList[stpMatchCnt->clLinkProgress + 1]);
            buf[1] = fn_ByteSwap32(stpMatchCnt->clLinkProgress + 1);
            buf[2] = fn_ByteSwap32(
                stpMatchCnt->aidList[stpMatchCnt->clLinkProgress + 1]);
            buf[3] = stpMatchCnt->qr2IPList[stpMatchCnt->clLinkProgress + 1];
            buf[4] = fn_ByteSwap32(
                stpMatchCnt->qr2PortList[stpMatchCnt->clLinkProgress + 1]);
        }
        else
        {
            DWC_Printf(0x40, "Tell new client completion of matching.\n");
            stpMatchCnt->qr2IsReserved = 0;
            stpMatchCnt->qr2Reservation = 0;
            qr2_send_statechanged(stpMatchCnt->qr2Obj);
            if (stpMatchCnt->qr2MatchType == 0)
            {
                stpMatchCnt->state = 3;
            }
            else if (stpMatchCnt->qr2MatchType == 1)
            {
                stpMatchCnt->state = 4;
            }
            else
            {
                stpMatchCnt->state = 10;
            }
            stpMatchCnt->clLinkProgress = 0;

            if (stpMatchCnt->qr2MatchType == 2
                || stpMatchCnt->gt2NumConnection == stpMatchCnt->qr2NumEntry)
            {
                if (stpMatchCnt->qr2MatchType == 2)
                {
                    stpMatchCnt->cbEventPid
                        = stpMatchCnt->sbPidList[stpMatchCnt->gt2NumConnection];
                }
                else
                {
                    stpMatchCnt->cbEventPid = 0;
                    stpMatchCnt->sbPidList[0] = stpMatchCnt->profileID;
                }
                stpMatchCnt->state = 16;
                stpMatchCnt->synAckBit = 0;
                for (i = 1; i <= stpMatchCnt->gt2NumConnection; i++)
                {
                    DWCi_SendMatchSynPacket(stpMatchCnt->aidList[i],
                        DWC_SEND_TYPE_MATCH_SYN);
                }
            }
            else
            {
                buf[0] = 0;
                buf[1] = fn_ByteSwap32(stpMatchCnt->gt2NumConnection);
                buf[2] = fn_ByteSwap32(
                    stpMatchCnt->aidList[stpMatchCnt->gt2NumConnection]);
                if (stpMatchCnt->qr2MatchType == 0)
                {
                    stpMatchCnt->sbUpdateFlag = 2;
                    stpMatchCnt->sbUpdateTick = OSGetTime();
                }
                else if (stpMatchCnt->qr2MatchType == 1)
                {
                    DWCi_SendResvCommandToFriend(1, 0, 0);
                    if (stpMatchCnt->distantFriend == 0
                        && stpMatchCnt->gt2NumConnection >= 2)
                    {
                        u32 bitmap = 0;

                        for (i = 1; i < stpMatchCnt->gt2NumConnection; i++)
                        {
                            bitmap
                                |= 1 << stpMatchCnt->aidList[i];
                        }
                        bitmap &= ~(
                            1 << stpMatchCnt
                                     ->aidList[stpMatchCnt->gt2NumConnection]);
                        if (stpMatchCnt->friendAcceptBit != bitmap)
                        {
                            DWC_Printf(0x40,
                                "FRIEND_ACCEPT command droped.\n");
                            if (stpMatchCnt->qr2MatchType == 2
                                || stpMatchCnt->qr2MatchType == 3)
                            {
                                DWC_Printf(8,
                                    "DWCi_RestartFromTimeout() "
                                    "shouldn't be called.\n");
                                return 1;
                            }
                            stpMatchCnt->closeState = 2;
                            gt2CloseAllConnectionsHard(
                                *stpMatchCnt->pGt2Socket);
                            stpMatchCnt->closeState = 0;
                            DWC_Printf(0x40,
                                "Closed all connections and restart "
                                "matching.\n");
                            DWCi_RestartFromCancel(1);
                            return 1;
                        }
                    }
                }
                if (stpMatchCnt->qr2MatchType != 2)
                {
                    clear = TRUE;
                }
            }
        }
        if (stpMatchCnt->state != 16)
        {
            if (stpMatchCnt->qr2MatchType == 0)
            {
                if (DWCi_HandleSBError(DWCi_SendMatchCommand(8,
                        stpMatchCnt->sbPidList[stpMatchCnt->gt2NumConnection],
                        stpMatchCnt->qr2IPList[stpMatchCnt->gt2NumConnection],
                        stpMatchCnt->qr2PortList[stpMatchCnt->gt2NumConnection], buf,
                        count))
                    != 0)
                {
                    return 0;
                }
            }
            else if (DWCi_HandleGPError(DWCi_SendMatchCommand(8,
                         stpMatchCnt->sbPidList[stpMatchCnt->gt2NumConnection],
                         stpMatchCnt->qr2IPList[stpMatchCnt->gt2NumConnection],
                         stpMatchCnt->qr2PortList[stpMatchCnt->gt2NumConnection], buf,
                         count))
                != 0)
            {
                return 0;
            }
            stpMatchCnt->_3E1 = 0;
        }
        break;

    case 1:
        stpMatchCnt->state = 1;
        if (stpMatchCnt->qr2MatchType == 3)
        {
            stpMatchCnt->cbEventPid
                = stpMatchCnt->sbPidList[stpMatchCnt->gt2NumConnection];
        }
        clear = TRUE;
        break;

    case 2:
        stpMatchCnt->state = 1;
        if (stpMatchCnt->qr2MatchType == 0
            || stpMatchCnt->qr2MatchType == 1)
        {
            stpMatchCnt->qr2IsReserved = 1;
            stpMatchCnt->qr2Reservation = stpMatchCnt->profileID;
        }
        stpMatchCnt->clWaitTimeoutCount = 0;
        stpMatchCnt->clWaitTime = OSGetTime();
        if (stpMatchCnt->gt2NumConnection > 1)
        {
            buf[0] = fn_ByteSwap32(
                stpMatchCnt->sbPidList[stpMatchCnt->gt2NumConnection - 1]);
            if (stpMatchCnt->qr2MatchType == 0)
            {
                if (DWCi_HandleSBError(DWCi_SendMatchCommand(9, stpMatchCnt->sbPidList[0],
                        stpMatchCnt->qr2IPList[0], stpMatchCnt->qr2PortList[0], buf,
                        1))
                    != 0)
                {
                    return 0;
                }
            }
            else if (DWCi_HandleGPError(DWCi_SendMatchCommand(9,
                         stpMatchCnt->sbPidList[0], stpMatchCnt->qr2IPList[0],
                         stpMatchCnt->qr2PortList[0], buf, 1))
                != 0)
            {
                return 0;
            }
        }
        return 1;

    case 3:
        stpMatchCnt->state = 1;
        stpMatchCnt->clWaitTimeoutCount = 0;
        stpMatchCnt->clWaitTime = OSGetTime();
        stpMatchCnt->cbEventPid = 0;
        clear = TRUE;
        count = 0;
        return 1;

    case 4:
        DWC_Printf(0x40, "Completed matching!\n");
        if (stpMatchCnt->qr2MatchType != 2)
        {
            DWCi_SetGPStatus(2, "", NULL);
        }
        stpMatchCnt->matchedCallback(0, FALSE, stpMatchCnt->cbEventPid == 0,
            FALSE, DWCi_GetFriendListIndex(stpMatchCnt->cbEventPid),
            stpMatchCnt->matchedParam);
        if (stpMatchCnt->qr2MatchType == 0
            || stpMatchCnt->qr2MatchType == 1)
        {
            DWCi_CloseMatching();
        }
        else
        {
            if (stpMatchCnt->sbObj != NULL)
            {
                ServerBrowserFree(stpMatchCnt->sbObj);
                stpMatchCnt->sbObj = NULL;
            }
            NNFreeNegotiateList();
            if (stpMatchCnt->qr2MatchType == 2)
            {
                if (DWCi_HandleGPError(fn_80492BA0()) != 0)
                {
                    return 0;
                }
                if (stOptSCBlock.valid == 1)
                {
                    stOptSCBlock.lock = 1;
                    DWC_Printf(4,
                        "[OPT_SC_BLOCK] Connect block start!\n");
                }
                stpMatchCnt->state = 10;
            }
            else
            {
                stpMatchCnt->state = 1;
            }
            stpMatchCnt->cbEventPid = 0;
        }
        stpMatchCnt->cancelState = 0;
        break;

    default:
        break;
    }

    if (clear && stpMatchCnt->qr2MatchType != 3)
    {
        ServerBrowserClear(stpMatchCnt->sbObj);
    }
    return 1;
}

void fn_804970B0(void)
{
    DWC_Printf(4, "CANCEL! state %d, numHost nn=%d gt2=%d.\n",
        stpMatchCnt->state, stpMatchCnt->qr2NNFinishCount, stpMatchCnt->gt2NumConnection);
    stpMatchCnt->cbEventPid = 0;

    if (stpMatchCnt->state == 2)
    {
        DWCi_FinishCancelMatching();
        return;
    }

    stpMatchCnt->cancelState = 1;
    if (stpMatchCnt->qr2MatchType == 3)
    {
        stpMatchCnt->cancelState = 1;
        if (stpMatchCnt->gt2NumConnection != 0)
        {
            gt2CloseAllConnectionsHard(*stpMatchCnt->pGt2Socket);
        }
        else
        {
            GPResult result = DWCi_SendMatchCommand(5, stpMatchCnt->sbPidList[0],
                stpMatchCnt->qr2IPList[0], stpMatchCnt->qr2PortList[0], NULL, 0);
            stpMatchCnt->reqProfileID = 0;
            if (stpMatchCnt->qr2MatchType == 0)
            {
                result = DWCi_HandleSBError(result);
            }
            else
            {
                result = DWCi_HandleGPError(result);
            }
            if (result != 0)
            {
                return;
            }
        }
        DWCi_FinishCancelMatching();
        return;
    }

    if (stpMatchCnt->state == 4 || stpMatchCnt->state == 5
        || stpMatchCnt->state == 6 || stpMatchCnt->state == 7
        || stpMatchCnt->state == 11 || stpMatchCnt->state == 12)
    {
        GPResult result;

        if (stpMatchCnt->reqProfileID != 0)
        {
            result = DWCi_SendMatchCommand(5, stpMatchCnt->sbPidList[0],
                stpMatchCnt->qr2IPList[0], stpMatchCnt->qr2PortList[0], NULL, 0);
            stpMatchCnt->reqProfileID = 0;
            if (stpMatchCnt->qr2MatchType == 0)
            {
                result = DWCi_HandleSBError(result);
            }
            else
            {
                result = DWCi_HandleGPError(result);
            }
            if (result != 0)
            {
                return;
            }
        }

        {
            u32 buf = 0;

            result = GP_NO_ERROR;
            if (stpMatchCnt->qr2IsReserved != 0 && stpMatchCnt->qr2Reservation != 0
                && stpMatchCnt->qr2Reservation != stpMatchCnt->profileID)
            {
                result = DWCi_SendMatchCommand(12, stpMatchCnt->qr2Reservation,
                    stpMatchCnt->qr2IPList[stpMatchCnt->gt2NumConnection + 1],
                    stpMatchCnt->qr2PortList[stpMatchCnt->gt2NumConnection + 1], &buf, 1);
                stpMatchCnt->qr2IsReserved = 0;
                stpMatchCnt->qr2Reservation = 0;
            }
            if (stpMatchCnt->qr2MatchType == 0)
            {
                result = DWCi_HandleSBError(result);
            }
            else
            {
                result = DWCi_HandleGPError(result);
            }
            if (result != 0)
            {
                return;
            }
        }
    }

    if (stpMatchCnt->nnInfo.cookie != 0)
    {
        NNCancel(stpMatchCnt->nnInfo.cookie);
        stpMatchCnt->nnInfo.cookie = 0;
    }
    if (stpMatchCnt->gt2NumConnection != 0)
    {
        stpMatchCnt->cancelState = 1;
        gt2CloseAllConnectionsHard(*stpMatchCnt->pGt2Socket);
    }
    DWCi_FinishCancelMatching();
}

static void DWCi_FinishCancelMatching(void)
{
    GPResult gpResult;

    gpResult = DWCi_SetGPStatus(1, "", NULL);
    if (DWCi_HandleGPError(gpResult))
    {
        return;
    }

    DWCi_CloseMatching();
    DWCi_GetMatchCnt()->matchedCallback(DWC_ERROR_NONE, TRUE,
        DWCi_GetMatchCnt()->cbEventPid ? FALSE : TRUE,
        DWCi_GetMatchCnt()->cbEventPid
            ? TRUE
            : ((DWCi_GetMatchCnt()->qr2MatchType == 2) ? TRUE : FALSE),
        DWCi_GetFriendListIndex(DWCi_GetMatchCnt()->cbEventPid),
        DWCi_GetMatchCnt()->matchedParam);

    DWCi_GetMatchCnt()->cancelState = 0;
}

static int DWCi_InvalidateReservation(void)
{
    u32 senddata = 0;
    int result = 0;

    if (DWCi_GetMatchCnt()->qr2IsReserved
        && DWCi_GetMatchCnt()->qr2Reservation
        && (DWCi_GetMatchCnt()->qr2Reservation
            != DWCi_GetMatchCnt()->profileID))
    {
        result = DWCi_SendMatchCommand(DWC_MATCH_COMMAND_CANCEL,
            DWCi_GetMatchCnt()->qr2Reservation,
            DWCi_GetMatchCnt()->qr2IPList[
                DWCi_GetMatchCnt()->gt2NumConnection + 1],
            DWCi_GetMatchCnt()->qr2PortList[
                DWCi_GetMatchCnt()->gt2NumConnection + 1],
            &senddata, 1);

        DWCi_GetMatchCnt()->qr2IsReserved = 0;
        DWCi_GetMatchCnt()->qr2Reservation = 0;
    }

    return result;
}

static void DWCi_RestartFromCancel(DWCMatchResetLevel level)
{
    SBError sbError;

    if (level == DWC_MATCH_RESET_ALL)
    {
        DWCi_FinishCancelMatching();
    }
    else
    {
        DWCi_ResetMatchParam(level);

        if (DWCi_GetMatchCnt()->qr2MatchType == 2
            || DWCi_GetMatchCnt()->qr2MatchType == 3)
        {
            DWCi_GetMatchCnt()->matchedCallback(DWC_ERROR_NONE, TRUE,
                DWCi_GetMatchCnt()->cbEventPid ? FALSE : TRUE, FALSE,
                DWCi_GetFriendListIndex(DWCi_GetMatchCnt()->cbEventPid),
                DWCi_GetMatchCnt()->matchedParam);
        }
        else if (DWCi_GetMatchCnt()->qr2MatchType == 0)
        {
            if (level == DWC_MATCH_RESET_RESTART)
            {
                sbError = DWCi_SBUpdateAsync(0);
                if (DWCi_HandleSBError(sbError))
                {
                    return;
                }
            }
        }
        else if (DWCi_GetMatchCnt()->qr2MatchType == 1)
        {
            if (level == DWC_MATCH_RESET_RESTART)
            {
                (void)DWCi_SendResvCommandToFriend(FALSE, FALSE, 0);
            }
        }
        else
        {
            DWC_Printf(DWC_REPORTFLAG_ERROR,
                "ERROR - DWCi_RestartFromCancel : matchType %d, level %d\n",
                DWCi_GetMatchCnt()->qr2MatchType, level);
        }
    }
}

static void DWCi_RestartFromTimeout(void)
{
    if (DWCi_GetMatchCnt()->qr2MatchType == DWC_MATCH_TYPE_SC_SV
        || DWCi_GetMatchCnt()->qr2MatchType == DWC_MATCH_TYPE_SC_CL)
    {
        DWC_Printf(DWC_REPORTFLAG_WARNING,
            "DWCi_RestartFromTimeout() shouldn't be called.\n");
        return;
    }

    DWCi_CloseAllConnectionsByTimeout();

    DWC_Printf(DWC_REPORTFLAG_MATCH_NN,
        "Closed all connections and restart matching.\n");

    DWCi_RestartFromCancel(DWC_MATCH_RESET_RESTART);
}

static int DWCi_ResumeMatching(void)
{
    int result;

    stpMatchCnt->qr2IsReserved = 0;
    stpMatchCnt->qr2Reservation = 0;
    stpMatchCnt->closeState = 0;

    if (stpMatchCnt->priorProfileID != 0)
    {
        if (stpMatchCnt->qr2MatchType == 0)
        {
            stpMatchCnt->state = 3;
            result = DWCi_SBUpdateAsync(0);
            if (DWCi_HandleSBError(result) != 0)
            {
                return result;
            }
        }
        else if (stpMatchCnt->qr2MatchType == 1)
        {
            int error;

            stpMatchCnt->state = 4;
            result = DWCi_SendResvCommand(stpMatchCnt->priorProfileID, FALSE);
            if (stpMatchCnt->qr2MatchType == 0)
            {
                error = DWCi_HandleSBError(result);
            }
            else
            {
                error = DWCi_HandleGPError(result);
            }
            if (error != 0)
            {
                return result;
            }
        }
    }
    else
    {
        DWCi_RestartFromCancel(1);
    }
    return 0;
}

static BOOL DWCi_CloseCancelHostAsync(int profileID)
{
    BOOL ret;
    GT2Connection* connection;

    DWCi_GetMatchCnt()->cbEventPid = profileID;
    connection = DWCi_GetGT2ConnectionByProfileID(
        profileID, DWCi_GetMatchCnt()->gt2NumConnection + 1);

    if (connection)
    {
        DWCi_GetMatchCnt()->closeState = DWC_MATCH_CLOSE_STATE_TIMEOUT;
        gt2CloseConnectionHard(*connection);
        DWCi_GetMatchCnt()->closeState = DWC_MATCH_CLOSE_STATE_INIT;
        ret = TRUE;
    }
    else
    {
        (void)DWCi_DeleteHostByProfileID(
            profileID, DWCi_GetMatchCnt()->gt2NumConnection + 1);
        ret = FALSE;
    }

    return ret;
}

static BOOL DWCi_CloseShutdownClientSC(u32 bitmap)
{
    u8 aid;
    int senddata[DWC_MAX_CONNECTIONS];
    int senddataLen = 0;
    int validPid[DWC_MAX_CONNECTIONS];
    int validPidLen = 0;
    int result;
    int i;

    for (i = 1; i <= DWCi_GetMatchCnt()->gt2NumConnection; i++)
    {
        if (bitmap & (1 << DWCi_GetMatchCnt()->aidList[i]))
        {
            validPid[validPidLen] = DWCi_GetMatchCnt()->sbPidList[i];
            validPidLen++;
        }
        else
        {
            senddata[senddataLen]
                = DWCi_HtoLEl(DWCi_GetMatchCnt()->sbPidList[i]);
            senddataLen++;
        }
    }

    for (i = 0; i < validPidLen; i++)
    {
        result = DWCi_SendMatchCommand(DWC_MATCH_COMMAND_SC_CLOSE_CL,
            validPid[i], 0, 0, (u32*)senddata, senddataLen);
        if (DWCi_HandleMatchCommandError(result))
        {
            return FALSE;
        }
    }

    DWCi_GetMatchCnt()->closeState = DWC_MATCH_CLOSE_STATE_TIMEOUT;

    for (i = 0; i < senddataLen; i++)
    {
        aid = DWCi_GetAIDFromProfileID(senddata[i], FALSE);
        if (aid != 0xff)
        {
            DWC_CloseConnectionHard(aid);
        }
    }

    DWCi_GetMatchCnt()->closeState = DWC_MATCH_CLOSE_STATE_INIT;

    return TRUE;
}

static void DWCi_SendMatchSynPacket(u8 aid, u16 type)
{
    u8 buf[4];

    DWC_Printf(DWC_REPORTFLAG_MATCH_GT2,
        "Sent SYN %d packet to aid %d.\n",
        type - DWC_SEND_TYPE_MATCH_SYN, aid);
    switch (type)
    {
    case DWC_SEND_TYPE_MATCH_SYN:
    {
        u8 i;

        buf[0] = stpMatchCnt->aidList[stpMatchCnt->gt2NumConnection] == aid;
        for (i = 1; i <= stpMatchCnt->gt2NumConnection; i++)
        {
            if (stpMatchCnt->aidList[i] == aid)
            {
                buf[1] = i;
                buf[2] = aid;
                break;
            }
        }
        break;
    }

    case DWC_SEND_TYPE_MATCH_SYN_ACK:
        buf[0] = stpMatchCnt->baseLatency;
        buf[1] = (stpMatchCnt->baseLatency >> 8) & 0xFF;
        break;
    }
    fn_8049AE0C(type, aid, buf, 4);
    stpMatchCnt->lastSynSent = OSGetTime();
}

int fn_804979F4(void)
{
    s64 elapsed;

    if (stpMatchCnt->state == 9 || stpMatchCnt->state == 16
        || stpMatchCnt->state == 17)
    {
        elapsed = fn_ElapsedMSec(stpMatchCnt->lastSynSent);
    }
    else
    {
        return 1;
    }

    switch (stpMatchCnt->state)
    {
    case 9:
        if (elapsed > 6000)
        {
            DWC_Printf(0x80, "[SYN] No ACK from server %d/%d.\n",
                stpMatchCnt->clWaitTimeoutCount, 5);
            if (DWC_GetState() == DWC_STATE_MATCHING
                && stpMatchCnt->clWaitTimeoutCount >= 5)
            {
                DWC_Printf(0x80,
                    "Timeout: [SYN] Connection to server was shut down.\n");
                if (DWCi_CancelPreConnectedClientProcess(stpMatchCnt->sbPidList[0]) == 0)
                {
                    return 0;
                }
            }
            else
            {
                stpMatchCnt->clWaitTimeoutCount++;
                DWCi_SendMatchSynPacket(stpMatchCnt->aidList[0],
                    DWC_SEND_TYPE_MATCH_SYN_ACK);
            }
        }
        break;

    case 16:
        if (elapsed > 6000)
        {
            stpMatchCnt->synResendCount++;
            if (stpMatchCnt->synResendCount > 5)
            {
                DWC_Printf(0x40,
                    "Timeout: wait SYN-ACK (aidbitmap 0x%x). "
                    "Restart matching.\n",
                    stpMatchCnt->synAckBit);
                if (stpMatchCnt->qr2MatchType == 0
                    || stpMatchCnt->qr2MatchType == 1)
                {
                    stpMatchCnt->closeState = 2;
                    gt2CloseAllConnectionsHard(*stpMatchCnt->pGt2Socket);
                    stpMatchCnt->closeState = 0;
                    DWCi_RestartFromCancel(1);
                }
                else
                {
                    if (DWCi_CloseShutdownClientSC(stpMatchCnt->synAckBit) == 0)
                    {
                        return 0;
                    }
                    if (stpMatchCnt->gt2NumConnection != 0)
                    {
                        stpMatchCnt->synResendCount = 0;
                        stpMatchCnt->lastSynSent = OSGetTime();
                    }
                    else if (DWCi_CancelPreConnectedServerProcess(stpMatchCnt->cbEventPid) == 0)
                    {
                        return 0;
                    }
                }
            }
            else
            {
                int i;

                for (i = 1; i <= stpMatchCnt->gt2NumConnection; i++)
                {
                    if ((stpMatchCnt->synAckBit
                            & (1 << stpMatchCnt->aidList[i]))
                        == 0)
                    {
                        DWCi_SendMatchSynPacket(stpMatchCnt->aidList[i],
                            DWC_SEND_TYPE_MATCH_SYN);
                    }
                }
            }
        }
        break;

    case 17:
        if (elapsed > stpMatchCnt->baseLatency)
        {
            DWCi_PostProcessConnection(DWC_PP_CONNECTION_SYN_FINISH);
        }
        break;
    }
    return 1;
}

static BOOL DWCi_SendCancelMatchSynCommand(int profileID, u8 command)
{
    u32 senddata;
    int len;
    int result;

    DWC_Printf(DWC_REPORTFLAG_MATCH_GT2,
        "Sent CANCEL SYN %d command to %u.\n",
        command - DWC_MATCH_COMMAND_CANCEL_SYN, profileID);

    if (command == DWC_MATCH_COMMAND_CANCEL_SYN)
    {
        senddata = DWCi_HtoLEl((u32)DWCi_GetMatchCnt()->cbEventPid);
        len = 1;
    }
    else
    {
        len = 0;
    }

    result = DWCi_SendMatchCommand(
        command, profileID, 0, 0, &senddata, len);
    if (DWCi_HandleMatchCommandError(result))
        return FALSE;

    DWCi_GetMatchCnt()->lastCancelSynSent = DWCi_Np_GetTick();

    return TRUE;
}

static int DWCi_ProcessCancelMatchSynCommand(
    int profileID, u8 command, u32 data)
{
    DWC_Printf(0x80, "Received CANCEL SYN %d command from %u.\n",
        command - 13, profileID);
    if (DWC_GetState() != DWC_STATE_CONNECTED)
    {
        DWC_Printf(0x80, "Ignore delayed CANCEL SYN.\n");
        return 1;
    }

    switch (command)
    {
    case 13:
    {
        if (stpMatchCnt->state != 8)
        {
            stpMatchCnt->state = 8;
            DWCi_CloseCancelHostAsync((int)data);
        }
        if (!DWCi_SendCancelMatchSynCommand(
                profileID, DWC_MATCH_COMMAND_CANCEL_SYN_ACK))
        {
            return 0;
        }
        break;
    }

    case 14:
        if (stpMatchCnt->state == 14)
        {
            u64 elapsed;
            u8 aid = 0xFF;
            int i;

            elapsed = (u64)(OSGetTime() - stpMatchCnt->lastCancelSynSent)
                / (OS_BUS_CLOCK_SPEED / 4 / 1000) / 2;
            if (elapsed > 300 && elapsed - 300 > stpMatchCnt->cancelBaseLatency)
            {
                stpMatchCnt->cancelBaseLatency = elapsed - 300;
            }

            for (i = 1; i <= stpMatchCnt->gt2NumConnection; i++)
            {
                if (stpMatchCnt->sbPidList[i] == profileID)
                {
                    aid = stpMatchCnt->aidList[i];
                    break;
                }
            }
            if (aid != 0xFF)
            {
                stpMatchCnt->cancelSynAckBit |= 1 << aid;
            }

            if ((stpMatchCnt->validAidBitmap & ~1) == stpMatchCnt->cancelSynAckBit)
            {
                for (i = 1; i <= stpMatchCnt->gt2NumConnection; i++)
                {
                    if (!DWCi_SendCancelMatchSynCommand(
                            stpMatchCnt->sbPidList[i],
                            DWC_MATCH_COMMAND_CANCEL_ACK))
                    {
                        return 0;
                    }
                }
                stpMatchCnt->state = 15;
                DWC_Printf(0x80, "Wait max latency %d msec.\n",
                    stpMatchCnt->cancelBaseLatency);
            }
        }
        else
        {
            if (!DWCi_SendCancelMatchSynCommand(
                    profileID, DWC_MATCH_COMMAND_CANCEL_ACK))
            {
                return 0;
            }
        }
        break;

    case 15:
        if (stpMatchCnt->state == 8)
        {
            DWCi_RestartFromCancel(2);
        }
        break;
    }
    return 1;
}

int fn_8049811C(void)
{
    s64 elapsed;

    if (stpMatchCnt->state == 8 || stpMatchCnt->state == 14
        || stpMatchCnt->state == 15)
    {
        elapsed = fn_ElapsedMSec(stpMatchCnt->lastCancelSynSent);
    }
    else
    {
        return 1;
    }

    switch (stpMatchCnt->state)
    {
    case 8:
        if (elapsed > 6000)
        {
            if (!DWCi_SendCancelMatchSynCommand(stpMatchCnt->sbPidList[0],
                    DWC_MATCH_COMMAND_CANCEL_SYN_ACK))
            {
                return 0;
            }
        }
        break;

    case 14:
        if (elapsed > 6000)
        {
            stpMatchCnt->cancelSynResendCount++;
            if (stpMatchCnt->cancelSynResendCount > 5)
            {
                DWC_Printf(0x40,
                    "Timeout: wait cancel SYN-ACK (aidbitmap 0x%x).\n",
                    stpMatchCnt->cancelSynAckBit);
                if (DWCi_CloseShutdownClientSC(stpMatchCnt->cancelSynAckBit) == 0)
                {
                    return 0;
                }
                if (stpMatchCnt->gt2NumConnection != 0)
                {
                    stpMatchCnt->cancelSynResendCount = 0;
                    stpMatchCnt->lastCancelSynSent = OSGetTime();
                }
                else
                {
                    DWCi_RestartFromCancel(2);
                }
            }
            else
            {
                int i;

                for (i = 1; i <= stpMatchCnt->gt2NumConnection; i++)
                {
                    if ((stpMatchCnt->cancelSynAckBit
                            & (1 << stpMatchCnt->aidList[i]))
                        == 0)
                    {
                        if (!DWCi_SendCancelMatchSynCommand(
                                stpMatchCnt->sbPidList[i],
                                DWC_MATCH_COMMAND_CANCEL_SYN))
                        {
                            return 0;
                        }
                    }
                }
            }
        }
        break;

    case 15:
        if (elapsed > stpMatchCnt->cancelBaseLatency)
        {
            DWCi_RestartFromCancel(2);
        }
        break;
    }
    return 1;
}

static void DWCi_CloseAllConnectionsByTimeout(void)
{
    DWCi_GetMatchCnt()->closeState = DWC_MATCH_CLOSE_STATE_TIMEOUT;
    gt2CloseAllConnectionsHard(*DWCi_GetMatchCnt()->pGt2Socket);
    DWCi_GetMatchCnt()->closeState = DWC_MATCH_CLOSE_STATE_INIT;
}

static u8 DWCi_GetNewMatchKey(void)
{
    u8 i;

    for (i = 0; i < DWC_QR2_GAME_RESERVED_KEYS; i++)
    {
        if (!stGameMatchKeys[i].keyID)
        {
            return (u8)(DWC_QR2_GAME_KEY_START + i);
        }
    }

    return 0;
}

static void DWCi_ClearGameMatchKeys(void)
{
    int i;

    for (i = 0; i < 154; i++)
    {
        if (stGameMatchKeys[i].keyStr != NULL)
        {
            DWC_Free(DWC_ALLOCTYPE_BASE, stGameMatchKeys[i].keyStr, 0);
        }
    }
    memset(stGameMatchKeys, 0, sizeof(stGameMatchKeys));
}

static u8 DWCi_GetAIDFromList(void)
{
    u8 i;
    int j;

    for (i = 0; i < DWC_MAX_CONNECTIONS; i++)
    {
        for (j = 0; j <= DWCi_GetMatchCnt()->qr2NNFinishCount; j++)
        {
            if (DWCi_GetMatchCnt()->aidList[j] == i)
            {
                break;
            }
        }
        if (j > DWCi_GetMatchCnt()->qr2NNFinishCount)
        {
            break;
        }
    }

    return i;
}

static BOOL DWCi_IsFriendByIdxList(int profileID)
{
    int listProfileID;
    int i;

    if (DWCi_GetMatchCnt()->friendList == NULL)
        return FALSE;

    for (i = 0; i < DWCi_GetMatchCnt()->friendIdxListLen; i++)
    {
        if (((listProfileID = DWCi_GetProfileIDFromList(
                  DWCi_GetMatchCnt()->friendIdxList[i]))
                > 0)
            && (listProfileID == profileID))
        {
            return TRUE;
        }
    }

    return FALSE;
}

static BOOL DWCi_AreAllBuddies(const u32 pidList[], u32 pidListLen)
{
    int i;

    if (DWCi_GetMatchCnt()->distantFriend
        && (DWCi_GetMatchCnt()->state == DWC_MATCH_STATE_CL_WAIT_RESV))
    {
        return TRUE;
    }

    for (i = 0; i < pidListLen; i++)
    {
        if (!DWCi_IsFriendByIdxList((int)DWCi_LEtoHl(pidList[i])))
        {
            return FALSE;
        }

        if (DWCi_GetMatchCnt()->distantFriend
            && (DWCi_GetMatchCnt()->state == DWC_MATCH_STATE_CL_WAITING))
        {
            return TRUE;
        }
    }

    return TRUE;
}

static u8 DWCi_GetAIDFromProfileID(int profileID, BOOL idx0)
{
    int i;

    for (i = (idx0 ? 0 : 1); i <= DWCi_GetMatchCnt()->gt2NumConnection; i++)
    {
        if (DWCi_GetMatchCnt()->sbPidList[i] == profileID)
        {
            return DWCi_GetMatchCnt()->aidList[i];
        }
    }

    return 0xff;
}

static u32 DWCi_GetAIDBitmask(BOOL valid)
{
    u32 bitmask = 0;
    int i;

    if (valid)
    {
        return DWCi_GetMatchCnt()->validAidBitmap & ~1;
    }
    else
    {
        for (i = 1; i <= DWCi_GetMatchCnt()->gt2NumConnection; i++)
        {
            bitmask |= 1 << DWCi_GetMatchCnt()->aidList[i];
        }
        return bitmask;
    }
}

static void DWCi_InitClWaitTimeout(void)
{
    DWCi_GetMatchCnt()->clWaitTimeoutCount = 0;
    DWCi_GetMatchCnt()->clWaitTime = DWCi_Np_GetTick();
}

static void DWCi_InitOptMinCompParam(BOOL reset)
{
    if (stpOptMinComp && stpOptMinComp->valid)
    {
        stpOptMinComp->recvBit = 0;
        stpOptMinComp->timeoutBit = 0;
        stpOptMinComp->retry = 0;
        stpOptMinComp->lastPollTime = DWCi_Np_GetTick();
        if (!reset)
        {
            stpOptMinComp->startTime = DWCi_Np_GetTick();
        }
    }
}

void fn_80498440(void)
{
    if (stpOptMinComp == NULL)
    {
        return;
    }
    if (stpOptMinComp->valid == 0)
    {
        return;
    }
    if (stpMatchCnt->qr2MatchType == 2)
    {
        return;
    }
    if (stpMatchCnt->qr2MatchType == 3)
    {
        return;
    }

    if (stpMatchCnt->state == 19)
    {
        u32 bitmap = 0;
        int i;
        GPResult result;

        for (i = 1; i <= stpMatchCnt->gt2NumConnection; i++)
        {
            bitmap |= 1 << stpMatchCnt->aidList[i];
        }

        if (stpOptMinComp->recvBit == bitmap)
        {
            if (stpOptMinComp->timeoutBit == bitmap)
            {
                DWC_Printf(0x80,
                    "[OPT_MIN_COMP] Timeout occured in all hosts.\n");
                stpMatchCnt->qr2NumEntry = stpMatchCnt->gt2NumConnection;
                stpMatchCnt->clLinkProgress = stpMatchCnt->gt2NumConnection - 1;
                DWCi_PostProcessConnection(DWC_PP_CONNECTION_SV_CONNECT);
            }
            else
            {
                DWC_Printf(0x80,
                    "[OPT_MIN_COMP] Some clients is in time.\n");
                stpOptMinComp->lastPollTime = OSGetTime();
                stpOptMinComp->recvBit = 0;
                if (stpMatchCnt->qr2MatchType == 0)
                {
                    stpMatchCnt->state = 3;
                    stpMatchCnt->sbUpdateFlag = 2;
                    stpMatchCnt->sbUpdateTick = OSGetTime();
                }
                else
                {
                    stpMatchCnt->state = 4;
                    DWCi_SendResvCommandToFriend(1, 0, 0);
                }
            }
        }
        else if (fn_ElapsedMSec(stpOptMinComp->lastPollTime)
            >= stpOptMinComp->retry * 6000)
        {
            DWC_Printf(4, "[OPT_MIN_COMP] Timeout: wait poll-ACK %d/%d.\n",
                stpOptMinComp->retry - 1, 5);
            if (stpOptMinComp->retry > 5)
            {
                DWC_Printf(4,
                    "[OPT_MIN_COMP] Timeout: aidbitmap 0x%x. "
                    "Restart matching.\n",
                    stpOptMinComp->recvBit);
                if (stpOptMinComp != NULL && stpOptMinComp->valid != 0)
                {
                    stpOptMinComp->recvBit = 0;
                    stpOptMinComp->timeoutBit = 0;
                    stpOptMinComp->retry = 0;
                    stpOptMinComp->lastPollTime = OSGetTime();
                }
                stpMatchCnt->closeState = 2;
                gt2CloseAllConnectionsHard(*stpMatchCnt->pGt2Socket);
                stpMatchCnt->closeState = 0;
                DWCi_RestartFromCancel(1);
            }
            else
            {
                for (i = 1; i <= stpMatchCnt->gt2NumConnection; i++)
                {
                    if ((stpOptMinComp->recvBit
                            & (1 << stpMatchCnt->aidList[i]))
                        == 0)
                    {
                        result = DWCi_SendMatchCommand(17, stpMatchCnt->sbPidList[i],
                            stpMatchCnt->qr2IPList[i], stpMatchCnt->qr2PortList[i],
                            NULL, 0);
                        if (stpMatchCnt->qr2MatchType == 0)
                        {
                            result = DWCi_HandleSBError(result);
                        }
                        else
                        {
                            result = DWCi_HandleGPError(result);
                        }
                        if (result != 0)
                        {
                            return;
                        }
                    }
                }
                stpOptMinComp->retry++;
            }
        }
    }
    else if (stpMatchCnt->state == 3 || stpMatchCnt->state == 4)
    {
        int i;
        GPResult result;

        if (stpMatchCnt->gt2NumConnection < stpOptMinComp->minEntry - 1)
        {
            return;
        }
        if (stpOptMinComp->retry != 0
            || fn_ElapsedMSec(stpOptMinComp->startTime)
                < stpOptMinComp->timeout)
        {
            if (stpOptMinComp->retry == 0)
            {
                return;
            }
            if (fn_ElapsedMSec(stpOptMinComp->lastPollTime)
                < stpOptMinComp->timeout / 4)
            {
                return;
            }
        }

        if (stpMatchCnt->reqProfileID != 0)
        {
            result = DWCi_SendMatchCommand(5, stpMatchCnt->reqProfileID,
                stpMatchCnt->qr2IPList[0], stpMatchCnt->qr2PortList[0], NULL, 0);
            stpMatchCnt->reqProfileID = 0;
            if (stpMatchCnt->qr2MatchType == 0)
            {
                result = DWCi_HandleSBError(result);
            }
            else
            {
                result = DWCi_HandleGPError(result);
            }
            if (result != 0)
            {
                return;
            }
        }

        stpMatchCnt->state = 19;
        DWC_Printf(0x80, "[OPT_MIN_COMP] Poll timeout (my time is %lu).\n",
            fn_ElapsedMSec(stpOptMinComp->startTime));
        for (i = 1; i <= stpMatchCnt->gt2NumConnection; i++)
        {
            result = DWCi_SendMatchCommand(17, stpMatchCnt->sbPidList[i],
                stpMatchCnt->qr2IPList[i], stpMatchCnt->qr2PortList[i], NULL, 0);
            if (stpMatchCnt->qr2MatchType == 0)
            {
                result = DWCi_HandleSBError(result);
            }
            else
            {
                result = DWCi_HandleGPError(result);
            }
            if (result != 0)
            {
                return;
            }
        }
        stpOptMinComp->lastPollTime = OSGetTime();
        stpOptMinComp->retry = 1;
    }
}

static GPResult DWCi_HandleGPError(GPResult result)
{
    int errorCode;
    DWCError dwcError;

    if (result == GP_NO_ERROR)
    {
        return GP_NO_ERROR;
    }

    DWC_Printf(DWC_REPORTFLAG_ERROR, "Match, GP error %d\n", result);

    switch (result)
    {
    case GP_MEMORY_ERROR:
        dwcError = DWC_ERROR_FATAL;
        errorCode = DWC_ECODE_TYPE_ALLOC;
        break;
    case GP_PARAMETER_ERROR:
        dwcError = DWC_ERROR_FATAL;
        errorCode = DWC_ECODE_TYPE_PARAM;
        break;
    case GP_NETWORK_ERROR:
        dwcError = DWC_ERROR_NETWORK;
        errorCode = DWC_ECODE_TYPE_NETWORK;
        break;
    case GP_SERVER_ERROR:
        dwcError = DWC_ERROR_NETWORK;
        errorCode = DWC_ECODE_TYPE_SERVER;
        break;
    }

    errorCode += DWC_ECODE_SEQ_MATCH + DWC_ECODE_GS_GP;

    DWCi_StopMatching(dwcError, errorCode);

    return result;
}

static SBError DWCi_HandleSBError(SBError error)
{
    int errorCode;
    DWCError dwcError;

    if (error == sbe_noerror)
    {
        return sbe_noerror;
    }

    DWC_Printf(DWC_REPORTFLAG_ERROR, "Match, SB error %d\n", error);

    switch (error)
    {
    case sbe_socketerror:
        dwcError = DWC_ERROR_NETWORK;
        errorCode = DWC_ECODE_TYPE_SOCKET;
        break;
    case sbe_dnserror:
        dwcError = DWC_ERROR_NETWORK;
        errorCode = DWC_ECODE_TYPE_DNS;
        break;
    case sbe_connecterror:
        dwcError = DWC_ERROR_NETWORK;
        errorCode = DWC_ECODE_TYPE_SERVER;
        break;
    case sbe_dataerror:
        dwcError = DWC_ERROR_NETWORK;
        errorCode = DWC_ECODE_TYPE_DATA;
        break;
    case sbe_allocerror:
        dwcError = DWC_ERROR_FATAL;
        errorCode = DWC_ECODE_TYPE_ALLOC;
        break;
    case sbe_paramerror:
        dwcError = DWC_ERROR_FATAL;
        errorCode = DWC_ECODE_TYPE_PARAM;
        break;
    }

    errorCode += DWC_ECODE_SEQ_MATCH + DWC_ECODE_GS_SB;

    DWCi_StopMatching(dwcError, errorCode);

    return error;
}

static qr2_error_t DWCi_HandleQR2Error(qr2_error_t error)
{
    int errorCode;
    DWCError dwcError;

    if (error == e_qrnoerror)
    {
        return e_qrnoerror;
    }

    DWC_Printf(DWC_REPORTFLAG_ERROR, "Match, QR2 error %d\n", error);

    switch (error)
    {
    case e_qrwsockerror:
        dwcError = DWC_ERROR_NETWORK;
        errorCode = DWC_ECODE_TYPE_SOCKET;
        break;
    case e_qrbinderror:
        dwcError = DWC_ERROR_NETWORK;
        errorCode = DWC_ECODE_TYPE_BIND;
        break;
    case e_qrdnserror:
        dwcError = DWC_ERROR_NETWORK;
        errorCode = DWC_ECODE_TYPE_DNS;
        break;
    case e_qrconnerror:
        dwcError = DWC_ERROR_NETWORK;
        errorCode = DWC_ECODE_TYPE_PEER;
        break;
    case e_qrnochallengeerror:
        dwcError = DWC_ERROR_NETWORK;
        errorCode = DWC_ECODE_TYPE_SERVER;
        break;
    }

    switch (DWC_GetState())
    {
    case DWC_STATE_LOGIN:
        errorCode += DWC_ECODE_SEQ_LOGIN + DWC_ECODE_GS_QR2;
        DWCi_StopLogin(dwcError, errorCode);
        break;
    case DWC_STATE_UPDATE_SERVERS:
        errorCode += DWC_ECODE_SEQ_FRIEND + DWC_ECODE_GS_QR2;
        DWCi_StopFriendProcess(dwcError, errorCode);
        break;
    case DWC_STATE_MATCHING:
        errorCode += DWC_ECODE_SEQ_MATCH + DWC_ECODE_GS_QR2;
        DWCi_StopMatching(dwcError, errorCode);
        break;
    default:
        errorCode += DWC_ECODE_SEQ_ETC + DWC_ECODE_GS_QR2;
        DWCi_SetError(dwcError, errorCode);
        break;
    }

    return error;
}

static NegotiateError DWCi_HandleNNError(NegotiateError error)
{
    int errorCode;
    DWCError dwcError;

    if (error == ne_noerror)
    {
        return ne_noerror;
    }

    DWC_Printf(DWC_REPORTFLAG_ERROR, "Match, NN error %d\n", error);

    switch (error)
    {
    case ne_allocerror:
        dwcError = DWC_ERROR_FATAL;
        errorCode = DWC_ECODE_TYPE_ALLOC;
        break;
    case ne_socketerror:
        dwcError = DWC_ERROR_NETWORK;
        errorCode = DWC_ECODE_TYPE_SOCKET;
        break;
    case ne_dnserror:
        dwcError = DWC_ERROR_NETWORK;
        errorCode = DWC_ECODE_TYPE_DNS;
        break;
    }

    errorCode += DWC_ECODE_SEQ_MATCH + DWC_ECODE_GS_NN;

    DWCi_StopMatching(dwcError, errorCode);

    return error;
}

int fn_804990FC(int result)
{
    int type;
    int code;

    if (result == 0)
    {
        return 0;
    }
    DWC_Printf(8, "Match, NN result %d\n", result);
    switch (result)
    {
    case nr_deadbeatpartner:
        return 1;
    case nr_inittimeout:
        return 2;
    case nr_pingtimeout:
        DWC_Printf(8, "NN Ping Timeout\n");
        type = DWC_ERROR_NETWORK;
        code = -70;
        break;
    default:
        type = DWC_ERROR_NETWORK;
        code = -9;
        break;
    }
    if (type != 0)
    {
        DWCi_StopMatching(type,
            code + DWC_ECODE_SEQ_MATCH + DWC_ECODE_GS_NN);
    }
    return result;
}

int fn_8049925C(int error)
{
    int type;
    int code;

    if (error == 0)
    {
        return 0;
    }
    DWC_Printf(2, "Match, GT2 error %d\n", error);
    switch (error)
    {
    case GT2OutOfMemory:
        type = DWC_ERROR_FATAL;
        code = DWC_ECODE_TYPE_ALLOC;
        break;
    case GT2Rejected:
    case GT2DuplicateAddress:
        type = 0;
        code = 0;
        error = 0;
        break;
    case GT2NetworkError:
        type = DWC_ERROR_NETWORK;
        code = DWC_ECODE_TYPE_NETWORK;
        break;
    case GT2AddressError:
        type = DWC_ERROR_NETWORK;
        code = -30;
        break;
    case GT2TimedOut:
        type = DWC_ERROR_NETWORK;
        code = -70;
        break;
    case GT2NegotiationError:
        type = DWC_ERROR_NETWORK;
        code = -80;
        break;
    }
    if (type != 0)
    {
        DWCi_StopMatching(type,
            code + DWC_ECODE_SEQ_MATCH + DWC_ECODE_GS_GT2);
    }
    return error;
}

static int DWCi_ChangeToClient(void)
{
    int result;
    int i;

    for (i = 1; i <= stpMatchCnt->gt2NumConnection; i++)
    {
        result = DWCi_SendMatchCommand(10, stpMatchCnt->sbPidList[i],
            stpMatchCnt->qr2IPList[i], stpMatchCnt->qr2PortList[i],
            (const u32*)stpMatchCnt->svDataBak,
            *(const int*)stpMatchCnt->svDataBak + 1);
        if (result != 0)
        {
            return result;
        }
    }

    stpMatchCnt->qr2IsReserved = 0;
    stpMatchCnt->qr2Reservation = 0;
    stpMatchCnt->closeState = 1;
    gt2CloseAllConnectionsHard(*stpMatchCnt->pGt2Socket);
    stpMatchCnt->closeState = 0;
    DWC_Printf(0x40, "Closed all connections. Begin NN to %u\n",
        stpMatchCnt->reqProfileID);

    return 0;
}

static int DWCi_CheckDWCServer(SBServer server)
{
    if (SBServerGetIntValueA(server, "numplayers", -1) == -1)
    {
        return 0;
    }
    if (SBServerGetIntValueA(server, "maxplayers", -1) == -1)
    {
        return 0;
    }
    if (SBServerGetIntValueA(server, "dwc_mtype", -1) == -1)
    {
        return 0;
    }
    if (SBServerGetIntValueA(server, "dwc_mresv", -1) == -1)
    {
        if (SBServerGetIntValueA(server, "dwc_mresv", 0) == 0)
        {
            return 0;
        }
    }
    if (SBServerGetIntValueA(server, "dwc_mver", -1) == -1)
    {
        return 0;
    }
    return SBServerGetIntValueA(server, "dwc_pid", 0);
}

void fn_804993C8(ServerBrowser sb, SBCallbackReason reason, SBServer server,
    void* instance)
{
    int profileID;
    int result;
    int i;
    NegotiateError nnError;

    DWC_Printf(0x40, "SBCallback : reason %d (state = %d)\n", reason,
        stpMatchCnt->state);
    s_sbCallbackLevel++;

    switch (reason)
    {
    case sbc_serveradded:
        fn_80499A30(server);
        stpMatchCnt->sbUpdateRequestTick = OSGetTime()
            + (u64)(OS_BUS_CLOCK_SPEED / 4 / 1000) * 30000;
        break;

    case sbc_updatecomplete:
        stpMatchCnt->sbUpdateRequestTick = 0;
        for (i = 0; i < ServerBrowserCount(sb); i++)
        {
            server = ServerBrowserGetServer(sb, i);
            if (!DWCi_CheckDWCServer(server))
            {
                ServerBrowserRemoveServer(sb, server);
                DWC_Printf(0x400, "Deleted server [%d].\n", i);
                i--;
            }
        }

        switch (stpMatchCnt->state)
        {
        case 2:
            for (i = 0; i < ServerBrowserCount(sb); i++)
            {
                server = ServerBrowserGetServer(sb, i);
                if (DWCi_GetMatchCnt()->qr2IP
                    && (DWCi_GetMatchCnt()->qr2IP
                        == SBServerGetPublicInetAddress(server))
                    && DWCi_GetMatchCnt()->qr2Port
                    && (DWCi_GetMatchCnt()->qr2Port
                        == SBServerGetPublicQueryPort(server)))
                {
                    break;
                }
            }
            if (i < ServerBrowserCount(sb))
            {
                stpMatchCnt->state = 3;
                stpMatchCnt->reqProfileID = 0;
                DWCi_HandleSBError(DWCi_SBUpdateAsync(stpMatchCnt->reqProfileID));
            }
            else
            {
                stpMatchCnt->sbUpdateFlag = 2;
                stpMatchCnt->sbUpdateTick = OSGetTime();
            }
            break;

        case 3:
            DWCi_EvaluateServers(1);
            fn_80499E90();
            if (ServerBrowserCount(sb) != 0)
            {
                result = DWCi_SendResvCommand(0, FALSE);

                if (stpMatchCnt->qr2MatchType == 0)
                {
                    result = DWCi_HandleSBError(result);
                }
                else
                {
                    result = DWCi_HandleGPError(result);
                }
                if (result != 0)
                {
                    break;
                }
                stpMatchCnt->state = 4;
                stpMatchCnt->sbUpdateFlag = 0;
            }
            else
            {
                stpMatchCnt->sbUpdateFlag = 2;
                stpMatchCnt->sbUpdateTick = OSGetTime();
            }
            break;

        case 5:
        {
            DWC_Printf(4, "searchIP: %x, searchPort: %d\n",
                stpMatchCnt->searchIP, stpMatchCnt->searchPort);
            while (ServerBrowserCount(sb) != 0)
            {
                server = ServerBrowserGetServer(sb, 0);
                if ((SBServerGetPublicInetAddress(server)
                        == DWCi_GetMatchCnt()->searchIP)
                    && (SBServerGetPublicQueryPort(server)
                        == DWCi_GetMatchCnt()->searchPort))
                {
                    break;
                }
                else
                {
                    ServerBrowserRemoveServer(sb, server);
                }
            }
            if (ServerBrowserCount(sb) != 0)
            {
                profileID = SBServerGetIntValueA(
                    ServerBrowserGetServer(sb, 0), "dwc_pid", 0);
                if (stpMatchCnt->qr2MatchType == 1
                    && profileID == stpMatchCnt->sbPidList[0])
                {
                    if (DWCi_EvaluateServers(0) != 0)
                    {
                        if (stpMatchCnt->gt2NumConnection != 0)
                        {
                            result = DWCi_ChangeToClient();
                            if (stpMatchCnt->qr2MatchType == 0)
                            {
                                result = DWCi_HandleSBError(result);
                            }
                            else
                            {
                                result = DWCi_HandleGPError(result);
                            }
                            if (result != 0)
                            {
                                break;
                            }
                        }
                    }
                    else
                    {
                        result = DWCi_SendMatchCommand(5, stpMatchCnt->sbPidList[0],
                            stpMatchCnt->qr2IPList[0], stpMatchCnt->qr2PortList[0],
                            NULL, 0);
                        stpMatchCnt->reqProfileID = 0;
                        if (stpMatchCnt->qr2MatchType == 0)
                        {
                            result = DWCi_HandleSBError(result);
                        }
                        else
                        {
                            result = DWCi_HandleGPError(result);
                        }
                        if (result != 0)
                        {
                            break;
                        }
                        stpMatchCnt->state = 4;
                        result = DWCi_SendResvCommandToFriend(
                            0, 0, stpMatchCnt->sbPidList[0]);
                        if (stpMatchCnt->qr2MatchType == 0)
                        {
                            DWCi_HandleSBError(result);
                        }
                        else
                        {
                            DWCi_HandleGPError(result);
                        }
                        break;
                    }
                }
                stpMatchCnt->state = 6;
                nnError = DWCi_NNStartupAsync(0, 0, ServerBrowserGetServer(sb, 0));
                DWCi_HandleNNError(nnError);
            }
            else
            {
                stpMatchCnt->sbUpdateFlag = 2;
                stpMatchCnt->sbUpdateTick = OSGetTime();
            }
            break;
        }

        default:
            stpMatchCnt->sbUpdateRequestTick = OSGetTime()
                + (u64)(OS_BUS_CLOCK_SPEED / 4 / 1000) * 30000;
            break;
        }
        break;

    case sbc_queryerror:
        break;

    default:
        break;
    }

    s_sbCallbackLevel--;
}

void fn_80499A30(SBServer server)
{
    int i;

    DWC_Printf(0x400, "SBServerGetPrivateAddress     = %s\n",
        SBServerGetPrivateAddress(server));
    DWC_Printf(0x400, "SBServerGetPrivateInetAddress = %x\n",
        SBServerGetPrivateInetAddress(server));
    DWC_Printf(0x400, "SBServerGetPrivateQueryPort   = %d\n",
        SBServerGetPrivateQueryPort(server));
    DWC_Printf(0x400, "SBServerGetPublicAddress      = %s\n",
        SBServerGetPublicAddress(server));
    DWC_Printf(0x400, "SBServerGetPublicInetAddres   = %x\n",
        SBServerGetPublicInetAddress(server));
    DWC_Printf(0x400, "SBServerGetPublicQueryPort    = %d\n",
        SBServerGetPublicQueryPort(server));
    DWC_Printf(0x400, "SBServerHasPrivateAddress     = %d\n",
        SBServerHasPrivateAddress(server));
    DWC_Printf(0x400, "numplayers  = %d\n",
        SBServerGetIntValueA(server, "numplayers", -1));
    DWC_Printf(0x400, "maxplayers  = %d\n",
        SBServerGetIntValueA(server, "maxplayers", -1));
    DWC_Printf(0x400, "%s     = %u\n", "dwc_pid",
        SBServerGetIntValueA(server, "dwc_pid", 0));
    DWC_Printf(0x400, "%s   = %u\n", "dwc_mresv",
        SBServerGetIntValueA(server, "dwc_mresv", -1));
    DWC_Printf(0x400, "%s   = %d\n", "dwc_mtype",
        SBServerGetIntValueA(server, "dwc_mtype", -1));
    DWC_Printf(0x400, "%s   = %d\n", "dwc_mver",
        SBServerGetIntValueA(server, "dwc_mver", -1));

    for (i = 0; i < 154; i++)
    {
        if (stGameMatchKeys[i].keyID != 0)
        {
            if (stGameMatchKeys[i].isStr != 0)
            {
                DWC_Printf(0x400, "%s  = %s\n", stGameMatchKeys[i].keyStr,
                    SBServerGetStringValueA(server,
                        stGameMatchKeys[i].keyStr, "NONE"));
            }
            else
            {
                DWC_Printf(0x400, "%s  = %d\n", stGameMatchKeys[i].keyStr,
                    SBServerGetIntValueA(server, stGameMatchKeys[i].keyStr,
                        -1));
            }
        }
    }
}

static int DWCi_EvaluateServers(int sort)
{
    int eval;
    int deleteFlag = FALSE;
    int localDelete;
    int profileID;
    int i;
    int j;
    SBServer server;

    for (i = 0; i < ServerBrowserCount(DWCi_GetMatchCnt()->sbObj); i++)
    {
        server = ServerBrowserGetServer(DWCi_GetMatchCnt()->sbObj, i);

        if (DWCi_GetMatchCnt()->qr2MatchType == 0)
        {
            profileID = SBServerGetIntValueA(server, "dwc_pid", 0);
            localDelete = FALSE;

            for (j = 1; j <= DWCi_GetMatchCnt()->gt2NumConnection; j++)
            {
                if (DWCi_GetMatchCnt()->sbPidList[j] == profileID)
                {
                    ServerBrowserRemoveServer(DWCi_GetMatchCnt()->sbObj, server);
                    localDelete = TRUE;
                    i--;
                    break;
                }
            }
            if (localDelete)
            {
                continue;
            }
        }

        if (DWCi_GetMatchCnt()->_488 != NULL)
        {
            eval = DWCi_GetMatchCnt()->_488(i, DWCi_GetMatchCnt()->_48C);

            if (eval > 0)
            {
                if (eval > 0x800000 - 1)
                {
                    eval = 0x800000 - 1;
                }
                SBServerAddIntKeyValue(server, "dwc_eval",
                    (eval << 8) | DWCi_GetMathRand32(0x100));
            }
            else
            {
                ServerBrowserRemoveServer(DWCi_GetMatchCnt()->sbObj, server);
                DWC_Printf(0x400,
                    "Deleted server [%d] (eval point is %d).\n", i, eval);
                deleteFlag = TRUE;
                i--;
            }
        }
        else
        {
            SBServerAddIntKeyValue(server, "dwc_eval", DWCi_GetMathRand32(0x80));
        }
    }

    if (sort != 0 && ServerBrowserCount(DWCi_GetMatchCnt()->sbObj) != 0)
    {
        ServerBrowserSortA(DWCi_GetMatchCnt()->sbObj, SBFalse, "dwc_eval",
            sbcm_int);
    }

    if (deleteFlag && ServerBrowserCount(DWCi_GetMatchCnt()->sbObj) == 0)
    {
        return 0;
    }
    return 1;
}

void fn_80499E90(void)
{
    u32 rand;
    int maxEval = 0;
    int total = 0;
    int i;
    int cumulative[6];

    if (ServerBrowserCount(stpMatchCnt->sbObj) <= 1)
    {
        return;
    }

    for (i = 0; i < ServerBrowserCount(stpMatchCnt->sbObj); i++)
    {
        int value = SBServerGetIntValueA(
            ServerBrowserGetServer(stpMatchCnt->sbObj, i), "dwc_eval", -1);

        if (value > maxEval)
        {
            maxEval = value;
        }
        total += lbl_804F31F8[i];
    }

    rand = DWCi_GetMathRand32(0x64);
    for (i = 0; i < ServerBrowserCount(stpMatchCnt->sbObj); i++)
    {
        if (i == ServerBrowserCount(stpMatchCnt->sbObj) - 1)
        {
            cumulative[i] = 100;
            break;
        }
        cumulative[i] = lbl_804F31F8[i] * 100 / total
            + (i > 0 ? cumulative[i - 1] : 0);
        if (rand < cumulative[i])
        {
            break;
        }
    }

    DWC_Printf(0x40, "Server[%d] is selected (%d/100: rand %d)\n", i,
        cumulative[i], rand);
    if (maxEval < 0x7FFFFFFF)
    {
        maxEval++;
    }
    SBServerAddIntKeyValue(ServerBrowserGetServer(stpMatchCnt->sbObj, i),
        "dwc_eval", maxEval);
    ServerBrowserSortA(stpMatchCnt->sbObj, SBFalse, "dwc_eval", sbcm_int);
}

static void DWCi_QR2ServerKeyCallback(
    int keyid, qr2_buffer_t outbuf, void* userdata)
{
#pragma unused(userdata)
    int index;

    switch (keyid)
    {
    case NUMPLAYERS_KEY:
        qr2_buffer_add_int(outbuf, DWCi_GetMatchCnt()->qr2NNFinishCount);
        DWC_Printf(DWC_REPORTFLAG_QR2_REQ,
            "QR2, Received ServerKeyReq : keyID %d - %d\n", keyid,
            DWCi_GetMatchCnt()->qr2NNFinishCount);
        break;
    case MAXPLAYERS_KEY:
        qr2_buffer_add_int(outbuf, DWCi_GetMatchCnt()->qr2NumEntry);
        DWC_Printf(DWC_REPORTFLAG_QR2_REQ,
            "QR2, Received ServerKeyReq : keyID %d - %d\n", keyid,
            DWCi_GetMatchCnt()->qr2NumEntry);
        break;
    case DWC_QR2_PID_KEY:
        qr2_buffer_add_int(outbuf, DWCi_GetMatchCnt()->profileID);
        DWC_Printf(DWC_REPORTFLAG_QR2_REQ,
            "QR2, Received ServerKeyReq : keyID %d - %d\n", keyid,
            DWCi_GetMatchCnt()->profileID);
        break;
    case DWC_QR2_MATCH_TYPE_KEY:
        qr2_buffer_add_int(outbuf, DWCi_GetMatchCnt()->qr2MatchType);
        DWC_Printf(DWC_REPORTFLAG_QR2_REQ,
            "QR2, Received ServerKeyReq : keyID %d - %d\n", keyid,
            DWCi_GetMatchCnt()->qr2MatchType);
        break;
    case DWC_QR2_MATCH_RESV_KEY:
        qr2_buffer_add_int(outbuf, DWCi_GetMatchCnt()->qr2Reservation);
        DWC_Printf(DWC_REPORTFLAG_QR2_REQ,
            "QR2, Received ServerKeyReq : keyID %d - %d\n", keyid,
            DWCi_GetMatchCnt()->qr2Reservation);
        break;
    case DWC_QR2_MATCH_VER_KEY:
        qr2_buffer_add_int(outbuf, DWC_MATCHING_VERSION);
        DWC_Printf(DWC_REPORTFLAG_QR2_REQ,
            "QR2, Received ServerKeyReq : keyID %d - %d\n", keyid,
            DWC_MATCHING_VERSION);
        break;
    case DWC_QR2_MATCH_EVAL_KEY:
        qr2_buffer_add_int(outbuf, 1);
        DWC_Printf(DWC_REPORTFLAG_QR2_REQ,
            "QR2, Received ServerKeyReq : keyID %d - %d\n", keyid, 1);
        break;
    default:
        index = keyid - DWC_QR2_GAME_KEY_START;
        if ((index >= 0 && index < DWC_QR2_GAME_RESERVED_KEYS)
            && stGameMatchKeys[index].keyID)
        {
            if (stGameMatchKeys[index].isStr)
            {
                qr2_buffer_addA(
                    outbuf, (char*)stGameMatchKeys[index].value);
            }
            else
            {
                qr2_buffer_add_int(
                    outbuf, *(const int*)stGameMatchKeys[index].value);
            }
        }
        break;
    }
}

static void DWCi_QR2PlayerKeyCallback(
    int keyid, int index, qr2_buffer_t outbuf, void* userdata)
{
}

static void DWCi_QR2TeamKeyCallback(
    int keyid, int index, qr2_buffer_t outbuf, void* userdata)
{
}

static void DWCi_QR2KeyListCallback(
    qr2_key_type keytype, qr2_keybuffer_t keybuffer, void* userdata)
{
#pragma unused(userdata)
    int i;

    switch (keytype)
    {
    case key_server:
        qr2_keybuffer_add(keybuffer, NUMPLAYERS_KEY);
        qr2_keybuffer_add(keybuffer, MAXPLAYERS_KEY);
        qr2_keybuffer_add(keybuffer, DWC_QR2_PID_KEY);
        qr2_keybuffer_add(keybuffer, DWC_QR2_MATCH_TYPE_KEY);
        qr2_keybuffer_add(keybuffer, DWC_QR2_MATCH_RESV_KEY);
        qr2_keybuffer_add(keybuffer, DWC_QR2_MATCH_VER_KEY);
        qr2_keybuffer_add(keybuffer, DWC_QR2_MATCH_EVAL_KEY);

        for (i = 0; i < DWC_QR2_GAME_RESERVED_KEYS; i++)
        {
            if (stGameMatchKeys[i].keyID)
            {
                qr2_keybuffer_add(keybuffer, stGameMatchKeys[i].keyID);
            }
        }
        break;
    case key_player:
        break;
    case key_team:
        break;
    }

    DWC_Printf(DWC_REPORTFLAG_QR2_REQ,
        "QR2, Received KeyListReq : keytype %d\n", keytype);
}

static int DWCi_QR2CountCallback(qr2_key_type keytype, void* userdata)
{
    return 0;
}

static void DWCi_QR2AddErrorCallback(
    qr2_error_t error, gsi_char* errmsg, void* userdata)
{
    DWC_Printf(2, "QR2 Failed query addition to master server %d\n", error);
    DWC_Printf(2, "%s\n", errmsg);
    DWCi_HandleQR2Error(error);
}

static void DWCi_QR2PublicAddrCallback(
    unsigned int ip, unsigned short port, void* userdata)
{
    DWC_Printf(0x40, "Got my query IP %08x & port %d.\n", ip, port);
    stpMatchCnt->qr2IP = ip;
    stpMatchCnt->qr2Port = port;
}

static void DWCi_QR2NatnegCallback(int cookie, void* userdata)
{
    DWC_Printf(0x40, "Got NN request, cookie = %x.\n", cookie);
    if (stpMatchCnt->state == 1)
    {
        stpMatchCnt->state = 6;
    }
    else if (stpMatchCnt->state != 6 && stpMatchCnt->state != 11)
    {
        DWC_Printf(0x40, "But already canceled reservation.\n");
        return;
    }

    if (cookie == stpMatchCnt->nnLastCookie)
    {
        stpMatchCnt->nnRecvCount++;
    }
    else
    {
        stpMatchCnt->nnRecvCount = 0;
        stpMatchCnt->nnLastCookie = cookie;
    }
    stpMatchCnt->nnFailedTime = 0;
    if (DWCi_HandleNNError(DWCi_NNStartupAsync(1, cookie, NULL)) == 0)
    {
        stpMatchCnt->_3E0 = 0xFF;
    }
}

static void DWCi_QR2ClientMsgCallback(
    gsi_char* data, int len, void* userdata)
{
#pragma unused(userdata)
    int offset = 0;
    BOOL boolResult;
    DWCSBMessage sbMsg;

    if (DWC_GetState() != DWC_STATE_MATCHING)
    {
        if (DWC_GetState() != DWC_STATE_CONNECTED
            || (DWCi_GetMatchCnt()->qr2MatchType != 2
                && DWCi_GetMatchCnt()->qr2MatchType != 3))
        {
            DWC_Printf(4, "Ignore delayed SB matching command.\n");
            return;
        }
    }

    while (offset + (int)sizeof(DWCSBMessageHeader) <= len)
    {
        DWCi_Np_CpuCopy8(data, &sbMsg, sizeof(DWCSBMessageHeader));
        sbMsg.header.version = DWCi_LEtoHl(sbMsg.header.version);
        sbMsg.header.qr2Port = DWCi_LEtoHs(sbMsg.header.qr2Port);
        sbMsg.header.profileID = DWCi_LEtoHl(sbMsg.header.profileID);
        if (strncmp(sbMsg.header.identifier, DWC_SB_COMMAND_STRING, 4) != 0)
        {
            DWC_Printf(8, "Got undefined SBcommand.\n");
            return;
        }
        if (sbMsg.header.version != DWC_MATCHING_VERSION)
        {
            DWC_Printf(8, "Got different version SBcommand.\n");
            return;
        }
        DWCi_Np_CpuCopy8(data + sizeof(DWCSBMessageHeader), sbMsg.data,
            sbMsg.header.size);
        DWC_Printf(0x40, "<SB> RECV-0x%02x <- [%08x:%d] [pid=%u]\n",
            sbMsg.header.command, sbMsg.header.qr2IP, sbMsg.header.qr2Port,
            sbMsg.header.profileID);
        boolResult = DWCi_ProcessRecvMatchCommand(sbMsg.header.command,
            sbMsg.header.profileID, sbMsg.header.qr2IP, sbMsg.header.qr2Port,
            sbMsg.data, sbMsg.header.size >> 2);
        if (!boolResult)
        {
            return;
        }
        offset += sizeof(DWCSBMessageHeader) + sbMsg.header.size;
    }
}

static void DWCi_NNProgressCallback(NegotiateState state, void* userdata)
{
    DWC_Printf(0x40, "NN, Got state update: %d\n", state);
}

static void DWCi_NNCompletedCallback(NegotiateResult result, SOCKET gamesocket,
    struct sockaddr_in* remoteaddr, void* userdata)
{
    DWCNNInfo* info = (DWCNNInfo*)userdata;

    DWC_Printf(0x40, "NN, Complete NAT Negotiation. result : %d\n", result);
    if (info != NULL)
    {
        DWC_Printf(0x40, "NN cookie = %x.\n", info->cookie);
    }
    if ((stpMatchCnt->state != 6 && stpMatchCnt->state != 11)
        || info == NULL)
    {
        DWC_Printf(4, "Ignore delayed NN after cancel.\n");
        return;
    }

    if (result == nr_success)
    {
        int count;

        if (remoteaddr != NULL)
        {
            DWC_Printf(0x40, "NN, remote address : %s\n",
                gt2AddressToString(remoteaddr->sin_addr.addr,
                    SONtoHs(remoteaddr->sin_port), NULL));
        }
        info->cookie = 0;
        stpMatchCnt->qr2NNFinishCount++;
        count = stpMatchCnt->qr2NNFinishCount;
        if (info->isQR2 != 0)
        {
            GT2Result res;
            char buf[0xC];

            stpMatchCnt->ipList[count] = remoteaddr->sin_addr.addr;
            stpMatchCnt->portList[count] = SONtoHs(remoteaddr->sin_port);
            DWC_Printf(0x40, "NN child finished Nat Negotiation.\n");
            stpMatchCnt->nnRecvCount = 0;
            stpMatchCnt->nnLastCookie = 0;
            stpMatchCnt->nnFailedTime = 0;
            if (stpMatchCnt->state == 11)
            {
                stpMatchCnt->state = 12;
            }
            else
            {
                stpMatchCnt->state = 7;
            }
            stpMatchCnt->gt2ConnectCount = 0;
            DWC_Printf(0x80, "gt2Connect() to pidList[%d] (%s)\n", count,
                gt2AddressToString(stpMatchCnt->ipList[count],
                    stpMatchCnt->portList[count], NULL));
            snprintf(buf, sizeof(buf), "%u", stpMatchCnt->profileID);
            res = gt2Connect(*stpMatchCnt->pGt2Socket, NULL,
                gt2AddressToString(stpMatchCnt->ipList[count],
                    stpMatchCnt->portList[count], NULL),
                (const GT2Byte*)buf, -1, 5000, stpMatchCnt->gt2Callbacks,
                GT2False);
            if (res == GT2OutOfMemory)
            {
                fn_8049925C(res);
                return;
            }
            if (res != GT2Success)
            {
                if (DWCi_CancelPreConnectedServerProcess(stpMatchCnt->sbPidList[count]) == 0)
                {
                    return;
                }
            }
        }
        else
        {
            DWC_Printf(0x40, "NN parent finished Nat Negotiation.\n");
            if (remoteaddr != NULL)
            {
                stpMatchCnt->ipList[count - 1] = remoteaddr->sin_addr.addr;
                stpMatchCnt->portList[count - 1]
                    = SONtoHs(remoteaddr->sin_port);
            }
            stpMatchCnt->nnFinishTime = OSGetTime();
            stpMatchCnt->state = 7;
        }
    }
    else
    {
        int level;

        if (info->cookie == 0)
        {
            DWC_Printf(4, "Ignore delayed NN error after cancel.\n");
            return;
        }
        level = fn_804990FC(result);
        if (level != 2 && level != 1)
        {
            return;
        }

        if (info->isQR2 == 0)
        {
            DWC_Printf(0x40, "Failed %d/%d NN send.\n", info->retryCount,
                1);
            if (level == 1 || (level == 2 && info->retryCount >= 1))
            {
                DWC_Printf(0x40, "Abort NN.\n");
                info->cookie = 0;
                if (stpMatchCnt->qr2MatchType != 3)
                {
                    stpMatchCnt->nnFailureCount++;
                    DWC_Printf(0x40, "NN failure %d/%d.\n",
                        stpMatchCnt->nnFailureCount, 5);
                }
                if (stpMatchCnt->qr2MatchType == 3
                    || stpMatchCnt->nnFailureCount >= 5)
                {
                    if (stpMatchCnt != NULL)
                    {
                        BOOL isServer;
                        BOOL self;

                        stpMatchCnt->closeState = 2;
                        gt2CloseAllConnectionsHard(*stpMatchCnt->pGt2Socket);
                        stpMatchCnt->closeState = 0;
                        DWCi_SetError(DWC_ERROR_NETWORK, DWC_ECODE_SEQ_MATCH + DWC_ECODE_GS_NN - 420);
                        DWCi_SetGPStatus(1, "", NULL);
                        {
                            DWCMatchControl* control = stpMatchCnt;

                            isServer = control->qr2MatchType == 2;
                            self = control->cbEventPid == 0;
                            control->matchedCallback(DWC_ERROR_NETWORK, FALSE, self, isServer,
                                DWCi_GetFriendListIndex(control->cbEventPid), control->matchedParam);
                        }
                        DWCi_CloseMatching();
                    }
                    return;
                }
                if (DWCi_CancelPreConnectedClientProcess(
                        stpMatchCnt->sbPidList[stpMatchCnt->gt2NumConnection])
                    == 0)
                {
                    return;
                }
            }
            else
            {
                int res = 0;
                int retry;

                info->retryCount++;
                if (info->isQR2 == 0
                    && DWCi_HandleSBError(
                           ServerBrowserSendNatNegotiateCookieToServerA(
                               stpMatchCnt->sbObj,
                               gt2AddressToString(info->ip, 0, NULL),
                               info->port, info->cookie))
                        != 0)
                {
                    res = 2;
                }
                else
                {
                    if (info->isQR2 == 0)
                    {
                        DWC_Printf(0x40, "Send NN cookie = %x.\n",
                            info->cookie);
                    }
                    for (retry = 0; retry < 5; retry++)
                    {
                        res = NNBeginNegotiationWithSocket(
                            gt2GetSocketSOCKET(*stpMatchCnt->pGt2Socket),
                            info->cookie, info->isQR2, DWCi_NNProgressCallback,
                            DWCi_NNCompletedCallback, info);
                        if (res == 0 || res != 3)
                        {
                            break;
                        }
                        DWC_Printf(4,
                            " dns error occurs when NatNegotiation "
                            "begin... retry\n");
                    }
                }
                if (DWCi_HandleNNError(res) == 0)
                {
                    return;
                }
            }
        }
        else
        {
            DWC_Printf(0x40, "Failed %d/%d NN recv.\n", stpMatchCnt->nnRecvCount,
                1);
            stpMatchCnt->nnFailedTime = OSGetTime();
            if (level == 1 || (level == 2 && stpMatchCnt->nnRecvCount >= 1))
            {
                DWC_Printf(0x40, "Abort NN.\n");
                info->cookie = 0;
                if (stpMatchCnt->qr2MatchType != 3
                    && stpMatchCnt->qr2MatchType != 2)
                {
                    if (stpMatchCnt->qr2MatchType != 3)
                    {
                        stpMatchCnt->nnFailureCount++;
                        DWC_Printf(0x40, "NN failure %d/%d.\n",
                            stpMatchCnt->nnFailureCount, 5);
                    }
                    if (stpMatchCnt->qr2MatchType == 3
                        || stpMatchCnt->nnFailureCount >= 5)
                    {
                        if (stpMatchCnt != NULL)
                        {
                            BOOL isServer;
                            BOOL self;

                            stpMatchCnt->closeState = 2;
                            gt2CloseAllConnectionsHard(*stpMatchCnt->pGt2Socket);
                            stpMatchCnt->closeState = 0;
                            DWCi_SetError(DWC_ERROR_NETWORK, DWC_ECODE_SEQ_MATCH + DWC_ECODE_GS_NN - 420);
                            DWCi_SetGPStatus(1, "", NULL);
                            {
                                DWCMatchControl* control = stpMatchCnt;

                                isServer = control->qr2MatchType == 2;
                                self = control->cbEventPid == 0;
                                control->matchedCallback(DWC_ERROR_NETWORK, FALSE, self, isServer,
                                    DWCi_GetFriendListIndex(control->cbEventPid), control->matchedParam);
                            }
                            DWCi_CloseMatching();
                        }
                        return;
                    }
                }
                stpMatchCnt->nnRecvCount = 0;
                stpMatchCnt->nnLastCookie = 0;
                stpMatchCnt->nnFailedTime = 0;
                if (DWCi_CancelPreConnectedServerProcess(
                        stpMatchCnt->sbPidList[stpMatchCnt->gt2NumConnection + 1])
                    == 0)
                {
                    return;
                }
            }
        }
    }
}

static DWCMatchControl* DWCi_GetMatchCnt(void)
{
    return stpMatchCnt;
}

static void DWCi_SetMatchStatus(DWCMatchState state)
{
    stpMatchCnt->state = state;
}
