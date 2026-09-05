#pragma once

#include <revolution/types.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef void (*DWCUserSendCallback)(int size, u8 aid);
    typedef void (*DWCUserRecvCallback)(u8 aid, u8* buffer, int size);
    typedef void (*DWCUserPingCallback)(int latency, u8 aid);

    enum
    {
        DWC_SEND_TYPE_INVALID,
        DWC_SEND_TYPE_USERDATA,
        DWC_SEND_TYPE_MATCH_SYN,
        DWC_SEND_TYPE_MATCH_SYN_ACK,
        DWC_SEND_TYPE_MATCH_ACK,
        DWC_SEND_TYPE_MAX
    };

    BOOL DWC_SendUnreliable(u8 aid, const void* buffer, int size);
    BOOL DWC_SetRecvBuffer(u8 aid, void* buffer, int size);
    BOOL DWC_SetUserRecvCallback(DWCUserRecvCallback callback);
    void DWCi_InitTransport(void* control);

#ifdef __cplusplus
}
#endif
