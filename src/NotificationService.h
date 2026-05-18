#pragma once

#include <Arduino.h>
#include <Watchy.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include "esp32notifications.h"
#include "OpenSansCondBoldCyrillic9pt.h"

class NotificationService
{
public:
    NotificationService();

    bool start(Watchy &watchy);
    void stop();
    void tick(Watchy &watchy);
    bool isActive() const;
    bool isMenuVisible() const;

private:
    static constexpr char DEVICE_NAME[] = "Watchy";
    static constexpr uint8_t MAX_TRACKED_NOTIFICATIONS = 16;
    static constexpr uint16_t BUTTON_DEBOUNCE_MS = 250;
    static constexpr uint16_t EXIT_HOLD_MS = 1200;
    static constexpr uint16_t NOTIFICATION_VIBRATION_MS = 90;
    static constexpr uint16_t NOTIFICATION_BATCH_WINDOW_MS = 3000;
    static constexpr uint16_t ADVERTISING_RESTART_DELAY_MS = 1500;
    static constexpr uint32_t ADVERTISING_KEEPALIVE_MS = 10000;
    static constexpr uint8_t BLUETOOTH_PUSH_MENU_INDEX = 1;

    static constexpr uint8_t MENU_BUTTON = 0x01;
    static constexpr uint8_t BACK_BUTTON = 0x02;
    static constexpr uint8_t UP_BUTTON = 0x04;
    static constexpr uint8_t DOWN_BUTTON = 0x08;

    struct TrackedNotification
    {
        uint32_t uuid = 0;
        uint32_t eventFlags = 0;
        time_t time = 0;
        uint32_t sequence = 0;
        char title[96] = "";
        char subtitle[128] = "";
        char message[224] = "";
        char positiveLabel[40] = "";
        char negativeLabel[40] = "";
    };

    struct PendingAction
    {
        BLENotifications *notifications = nullptr;
        uint32_t uuid = 0;
        bool positiveAction = false;
    };

    struct PendingClearActions
    {
        BLENotifications *notifications = nullptr;
        uint32_t uuids[MAX_TRACKED_NOTIFICATIONS] = {};
        uint8_t count = 0;
    };

    static NotificationService *activeInstance;

    BLENotifications notifications;
    SemaphoreHandle_t stateMutex = nullptr;

    bool notificationsReady = false;
    bool active = false;
    bool startupPending = false;
    bool menuVisible = false;
    bool connected = false;
    bool vibrationActive = false;
    bool backLongPressHandled = false;
    bool idleWatchFaceDrawn = false;
    bool followLatestNotification = true;
    bool notificationBatchPending = false;
    bool notificationStatusBarRefreshRequested = false;
    bool aboutVisible = false;

    volatile bool redrawRequested = true;
    volatile bool restartAdvertisingRequested = false;
    volatile bool vibrationRequested = false;

    uint32_t restartAdvertisingAtMs = 0;
    uint32_t vibrationStopAtMs = 0;
    uint32_t backPressedAtMs = 0;
    uint32_t lastButtonActionAtMs = 0;
    uint32_t lastAdvertisingAttemptAtMs = 0;
    uint32_t notificationBatchDeadlineAtMs = 0;
    uint32_t notificationSequenceCounter = 0;
    uint32_t currentNotificationUUID = 0;

    uint8_t previousButtonMask = 0;
    uint8_t previousAboutButtonMask = 0;
    uint8_t trackedNotificationCount = 0;
    uint8_t currentNotificationIndex = 0;

    char statusText[64] = "Advertising. Pair from iPhone.";
    char titleText[160] = "";
    char subtitleText[256] = "";
    char messageText[512] = "";

    TrackedNotification trackedNotifications[MAX_TRACKED_NOTIFICATIONS] = {};

    static void onStateChanged(BLENotifications::State state);
    static void onNotificationArrived(
        const ArduinoNotification *notification,
        const Notification *rawNotificationData
    );
    static void onNotificationRemoved(
        const ArduinoNotification *notification,
        const Notification *rawNotificationData
    );
    static void runActionTask(void *parameter);
    static void runClearActionsTask(void *parameter);

    void ensureStarted();
    void prepareBluetoothForSleep();
    void setBluetoothDisplayMode(bool enabled);
    void processStateChanged(BLENotifications::State state);
    void processNotificationArrived(const TrackedNotification &notification);
    void processNotificationRemoved(uint32_t uuid);
    void setupButtons();
    void setupVibration();

    void drawScreen(Watchy &watchy);
    void drawStatusBar(Watchy &watchy, const char *notificationCounter = nullptr);
    void drawIdleWatchFace(Watchy &watchy);
    void flushNotificationBatchIfReady();
    void refreshNotificationStatusBarIfNeeded(Watchy &watchy);
    void handleVibration();
    void handleAboutScreen(Watchy &watchy);
    void handleButtons(Watchy &watchy);
    void waitForButtonsReleased(uint32_t timeoutMs);
    uint8_t readButtonMask() const;
    bool isButtonPressed(uint8_t pin) const;

    void lockState();
    void unlockState();

    void copyCString(char *destination, size_t destinationSize, const char *source) const;
    void copyString(char *destination, size_t destinationSize, const String &source) const;
    void setStatus(const char *status);

    int8_t trackedNotificationIndex(uint32_t uuid) const;
    bool isNewerNotification(
        const TrackedNotification &left,
        const TrackedNotification &right
    ) const;
    void sortTrackedNotifications();
    void storeTrackedNotification(
        uint8_t index,
        const TrackedNotification &notification,
        bool isNewNotification
    );
    uint8_t upsertTrackedNotification(const TrackedNotification &notification);
    bool untrackNotification(uint32_t uuid);
    void selectTrackedNotification(uint8_t index);
    void selectRelativeNotification(int8_t delta);
    void performCurrentNotificationAction(bool positiveAction, Watchy &watchy);
    void performAllNotificationDismissActions(Watchy &watchy);
    void sendNotificationActionAsync(uint32_t uuid, bool positiveAction);
    void sendNotificationClearActionsAsync(const uint32_t *uuids, uint8_t count);
    bool isLocalDismissActionLabel(const char *label) const;
};
