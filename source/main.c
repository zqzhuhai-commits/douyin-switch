#include <switch.h>
#include <stdio.h>

static const char *urls[] = {
    "https://www.douyin.com/?is_from_mobile_home=1&recommend=1",
    "https://www.douyin.com/",
    "https://www.douyin.com/jingxuan"
};
static void open_page(unsigned index) {
    WebCommonConfig config;
    WebCommonReply reply = {0};
    Result rc = webPageCreate(&config, urls[index]);
    if (R_SUCCEEDED(rc)) rc = webConfigSetWhitelist(&config,
        "^https://([0-9A-Za-z\\-]+\\.)*(douyin\\.com|douyin\\.net|douyinvod\\.com|bytedance\\.com|byteimg\\.com|bytecdn\\.cn|iesdouyin\\.com|snssdk\\.com|amemv\\.com)(/|$)");
    if (R_SUCCEEDED(rc)) {
        webConfigSetPointer(&config, true);
        webConfigSetLeftStickMode(&config, WebLeftStickMode_Pointer);
        webConfigSetTouchEnabledOnContents(&config, true);
        webConfigSetFooter(&config, true);
        webConfigSetBootDisplayKind(&config, WebBootDisplayKind_Black);
        webConfigSetPageCache(&config, true);
        webConfigSetWebAudio(&config, true);
        webConfigSetMediaAutoPlay(&config, true);
        webConfigSetMediaPlayerAutoClose(&config, false);
        consoleClear();
        printf("Douyin Web Test 0.1.0\n\nOpening browser...\n");
        consoleUpdate(NULL);
        rc = webConfigShow(&config, &reply);
    }
    consoleClear();
    printf("Douyin Web Test 0.1.0 - unofficial homebrew\n\n");
    printf("Browser result: 0x%08X\n", rc);
    if (R_SUCCEEDED(rc)) {
        WebExitReason reason;
        if (R_SUCCEEDED(webReplyGetExitReason(&reply, &reason)))
            printf("Browser exit reason: 0x%X\n", reason);
    }
    printf("\nA: Reopen recommended videos\nX: Open main homepage\nY: Open Jingxuan page\n+: Exit\n\n");
    printf("Inside browser: touch the page or move the pointer with left stick.\n");
    printf("Use the browser footer for available controls.\n\n");
    printf("Playback and login compatibility are NOT verified.\n");
    printf("If blank or unable to play, record the system/browser error.\n");
}
int main(void) {
    consoleInit(NULL);
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    PadState pad;
    padInitializeDefault(&pad);
    AppletType type = appletGetAppletType();
    if (type != AppletType_Application && type != AppletType_SystemApplication) {
        printf("Launch the installed NSP from HOME.\nAlbum applet mode is unsupported.\n+: Exit\n");
    } else {
        open_page(0);
    }
    while (appletMainLoop()) {
        padUpdate(&pad);
        u64 down = padGetButtonsDown(&pad);
        if (down & HidNpadButton_Plus) break;
        if (type == AppletType_Application || type == AppletType_SystemApplication) {
            if (down & HidNpadButton_A) open_page(0);
            else if (down & HidNpadButton_X) open_page(1);
            else if (down & HidNpadButton_Y) open_page(2);
        }
        consoleUpdate(NULL);
    }
    consoleExit(NULL);
    return 0;
}
