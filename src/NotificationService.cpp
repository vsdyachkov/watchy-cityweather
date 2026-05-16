#include "NotificationService.h"

#include <Fonts/FreeSansBold12pt7b.h>
#include <cstring>

#include "Images.h"

NotificationService *NotificationService::activeInstance = nullptr;
constexpr char NotificationService::DEVICE_NAME[];

namespace
{
void printRightAligned(Adafruit_GFX &display, int16_t xRight, int16_t y, const String &text)
{
    int16_t x1;
    int16_t y1;
    uint16_t width;
    uint16_t height;
    display.getTextBounds(text, 0, 0, &x1, &y1, &width, &height);
    display.setCursor(xRight - width, y);
    display.print(text);
}
}

NotificationService::NotificationService() {}

bool NotificationService::start(Watchy &watchy)
{
    (void)watchy;
    activeInstance = this;
    setupButtons();
    setupVibration();

    active = true;
    startupPending = true;

    lockState();
    const bool hasNotificationScreen = trackedNotificationCount > 0 && currentNotificationIndex > 0;
    redrawRequested = false;
    idleWatchFaceDrawn = false;
    unlockState();

    previousButtonMask = readButtonMask();
    backPressedAtMs = 0;
    backLongPressHandled = false;
    menuVisible = true;

    return hasNotificationScreen;
}

void NotificationService::stop()
{
    active = false;
    startupPending = false;
    menuVisible = false;
    digitalWrite(VIB_MOTOR_PIN, LOW);
    vibrationActive = false;
    vibrationRequested = false;
    restartAdvertisingRequested = false;
    prepareBluetoothForSleep();
}

void NotificationService::tick(Watchy &watchy)
{
    if (!active)
    {
        return;
    }

    if (startupPending)
    {
        uint8_t currentButtonMask = readButtonMask();
        previousButtonMask = currentButtonMask;
        if (currentButtonMask != 0)
        {
            return;
        }

        delay(300);
        ensureStarted();
        startupPending = false;
        previousButtonMask = readButtonMask();
    }

    if (restartAdvertisingRequested && static_cast<int32_t>(millis() - restartAdvertisingAtMs) >= 0)
    {
        bool shouldRestartAdvertising = false;
        lockState();
        if (restartAdvertisingRequested && static_cast<int32_t>(millis() - restartAdvertisingAtMs) >= 0)
        {
            restartAdvertisingRequested = false;
            shouldRestartAdvertising = !connected;
            if (shouldRestartAdvertising)
            {
                lastAdvertisingAttemptAtMs = millis();
            }
        }
        unlockState();

        if (shouldRestartAdvertising)
        {
            notifications.startAdvertising();
        }
    }
    else if (notificationsReady && !startupPending)
    {
        bool shouldRefreshAdvertising = false;
        uint32_t now = millis();
        lockState();
        if (
            !connected &&
            !restartAdvertisingRequested &&
            (lastAdvertisingAttemptAtMs == 0 || now - lastAdvertisingAttemptAtMs >= ADVERTISING_KEEPALIVE_MS)
        )
        {
            lastAdvertisingAttemptAtMs = now;
            shouldRefreshAdvertising = true;
        }
        unlockState();

        if (shouldRefreshAdvertising)
        {
            notifications.startAdvertising();
        }
    }

    handleVibration();

    if (!menuVisible && redrawRequested && !vibrationActive && !vibrationRequested)
    {
        drawScreen(watchy);
    }

    handleButtons(watchy);
}

bool NotificationService::isActive() const
{
    return active;
}

void NotificationService::ensureStarted()
{
    if (stateMutex == nullptr)
    {
        stateMutex = xSemaphoreCreateMutex();
    }

    if (notificationsReady)
    {
        if (!connected)
        {
            restartAdvertisingRequested = true;
            restartAdvertisingAtMs = millis() + ADVERTISING_RESTART_DELAY_MS;
        }
        return;
    }

    notifications.setConnectionStateChangedCallback(onStateChanged);
    notifications.setNotificationCallback(onNotificationArrived);
    notifications.setRemovedCallback(onNotificationRemoved);
    notifications.begin(DEVICE_NAME);
    notificationsReady = true;
}

void NotificationService::prepareBluetoothForSleep()
{
    activeInstance = nullptr;
    startupPending = false;
    connected = false;
    restartAdvertisingRequested = false;

    // Do not call BLENotifications::stop() here: the library deinitializes
    // ESP32 BLE internals, which can lock up when an iPhone is connected.
    // Deep sleep/reset will tear the radio down reliably after we leave this mode.
}

void NotificationService::setupButtons()
{
#ifdef ARDUINO_ESP32S3_DEV
    pinMode(MENU_BTN_PIN, INPUT_PULLUP);
    pinMode(BACK_BTN_PIN, INPUT_PULLUP);
    pinMode(UP_BTN_PIN, INPUT_PULLUP);
    pinMode(DOWN_BTN_PIN, INPUT_PULLUP);
#else
    pinMode(MENU_BTN_PIN, INPUT);
    pinMode(BACK_BTN_PIN, INPUT);
    pinMode(UP_BTN_PIN, INPUT);
    pinMode(DOWN_BTN_PIN, INPUT);
#endif
}

void NotificationService::setupVibration()
{
    pinMode(VIB_MOTOR_PIN, OUTPUT);
    digitalWrite(VIB_MOTOR_PIN, LOW);
}

void NotificationService::onStateChanged(BLENotifications::State state)
{
    if (activeInstance != nullptr)
    {
        activeInstance->processStateChanged(state);
    }
}

void NotificationService::onNotificationArrived(
    const ArduinoNotification *notification,
    const Notification *rawNotificationData
)
{
    (void)rawNotificationData;
    if (activeInstance != nullptr)
    {
        if (notification == nullptr)
        {
            return;
        }

        TrackedNotification trackedNotification;
        trackedNotification.uuid = notification->uuid;
        trackedNotification.eventFlags = notification->eventFlags;
        trackedNotification.time = notification->time;
        activeInstance->copyString(
            trackedNotification.title,
            sizeof(trackedNotification.title),
            notification->title
        );
        activeInstance->copyString(
            trackedNotification.message,
            sizeof(trackedNotification.message),
            notification->message
        );
        activeInstance->copyString(
            trackedNotification.positiveLabel,
            sizeof(trackedNotification.positiveLabel),
            notification->positiveActionLabel
        );
        activeInstance->copyString(
            trackedNotification.negativeLabel,
            sizeof(trackedNotification.negativeLabel),
            notification->negativeActionLabel
        );
        activeInstance->processNotificationArrived(trackedNotification);
    }
}

void NotificationService::onNotificationRemoved(
    const ArduinoNotification *notification,
    const Notification *rawNotificationData
)
{
    (void)rawNotificationData;
    if (activeInstance != nullptr)
    {
        if (notification == nullptr)
        {
            return;
        }
        activeInstance->processNotificationRemoved(notification->uuid);
    }
}

void NotificationService::runActionTask(void *parameter)
{
    PendingAction *action = static_cast<PendingAction *>(parameter);
    if (action != nullptr && action->notifications != nullptr && action->uuid != 0)
    {
        if (action->positiveAction)
        {
            action->notifications->actionPositive(action->uuid);
        }
        else
        {
            action->notifications->actionNegative(action->uuid);
        }
    }

    delete action;
    vTaskDelete(nullptr);
}

void NotificationService::processStateChanged(BLENotifications::State state)
{
    lockState();
    switch (state)
    {
        case BLENotifications::StateConnected:
            connected = true;
            restartAdvertisingRequested = false;
            copyCString(statusText, sizeof(statusText), "iOS");
            break;
        case BLENotifications::StateDisconnected:
            connected = false;
            copyCString(statusText, sizeof(statusText), "iOS");
            restartAdvertisingRequested = true;
            restartAdvertisingAtMs = millis() + ADVERTISING_RESTART_DELAY_MS;
            break;
    }
    if (trackedNotificationCount == 0)
    {
        redrawRequested = !idleWatchFaceDrawn;
    }
    unlockState();
}

void NotificationService::processNotificationArrived(const TrackedNotification &notification)
{
    if (notification.uuid == 0)
    {
        return;
    }

    lockState();
    int8_t existingIndex = trackedNotificationIndex(notification.uuid);
    uint8_t storedIndex = upsertTrackedNotification(notification);
    bool isNewNotification = existingIndex < 0;

    if (isNewNotification)
    {
        idleWatchFaceDrawn = false;
        selectTrackedNotification(storedIndex);
        vibrationRequested = true;
    }
    else if (currentNotificationUUID != 0)
    {
        int8_t currentIndex = trackedNotificationIndex(currentNotificationUUID);
        currentNotificationIndex = currentIndex >= 0 ? static_cast<uint8_t>(currentIndex + 1) : 0;
    }

    copyCString(statusText, sizeof(statusText), "iOS");
    redrawRequested = true;
    unlockState();
}

void NotificationService::processNotificationRemoved(uint32_t uuid)
{
    if (uuid == 0)
    {
        return;
    }

    lockState();
    untrackNotification(uuid);
    if (trackedNotificationCount == 0)
    {
        idleWatchFaceDrawn = false;
        if (!connected)
        {
            restartAdvertisingRequested = true;
            restartAdvertisingAtMs = millis() + ADVERTISING_RESTART_DELAY_MS;
        }
    }
    copyCString(statusText, sizeof(statusText), "iOS");
    redrawRequested = true;
    unlockState();
}

void NotificationService::drawScreen(Watchy &watchy)
{
    char localTitle[sizeof(titleText)];
    char localMessage[sizeof(messageText)];
    char localPositiveLabel[sizeof(trackedNotifications[0].positiveLabel)] = "";
    char localNegativeLabel[sizeof(trackedNotifications[0].negativeLabel)] = "";
    uint32_t localEventFlags = 0;
    uint8_t localNotificationCount = 0;
    uint8_t localNotificationIndex = 0;

    lockState();
    copyCString(localTitle, sizeof(localTitle), titleText);
    copyCString(localMessage, sizeof(localMessage), messageText);
    localNotificationCount = trackedNotificationCount;
    localNotificationIndex = currentNotificationIndex;

    if (localNotificationIndex > 0 && localNotificationIndex <= localNotificationCount)
    {
        const TrackedNotification &currentNotification =
            trackedNotifications[localNotificationIndex - 1];
        localEventFlags = currentNotification.eventFlags;
        copyCString(
            localPositiveLabel,
            sizeof(localPositiveLabel),
            currentNotification.positiveLabel
        );
        copyCString(
            localNegativeLabel,
            sizeof(localNegativeLabel),
            currentNotification.negativeLabel
        );
    }
    redrawRequested = false;
    unlockState();

    if (localNotificationCount == 0 || localNotificationIndex == 0)
    {
        if (!idleWatchFaceDrawn)
        {
            drawIdleWatchFace(watchy);
            idleWatchFaceDrawn = true;
        }
        return;
    }

    idleWatchFaceDrawn = false;

    Watchy::display.setFullWindow();
    Watchy::display.epd2.asyncPowerOn();
    Watchy::display.fillScreen(GxEPD_WHITE);
    Watchy::display.setTextColor(GxEPD_BLACK);
    drawStatusBar(watchy);

    constexpr int16_t contentTop = 24;
    constexpr int16_t contentLeft = 4;
    constexpr int16_t contentWidth = 192;
    constexpr int16_t footerTop = 176;
    constexpr int16_t footerBaseline = 195;
    constexpr int16_t headerBaseline = contentTop + 14;

    String counterText = String(localNotificationIndex) + "/" + String(localNotificationCount);
    OpenSansCondensed::printLine(
        Watchy::display,
        OpenSansCondBoldCyrillic9pt,
        counterText,
        160,
        headerBaseline,
        36,
        true,
        GxEPD_BLACK
    );

    constexpr int16_t titleBaseline = headerBaseline;
    constexpr int16_t titleMessageGap = 10;
    uint8_t titleLines = OpenSansCondensed::printBlock(
        Watchy::display,
        OpenSansCondBoldCyrillic9pt,
        String(localTitle),
        contentLeft,
        titleBaseline,
        150,
        2,
        GxEPD_BLACK,
        true
    );

    int16_t messageY =
        titleBaseline + titleLines * OpenSansCondBoldCyrillic9pt.lineHeight + titleMessageGap;
    uint8_t messageLines =
        footerTop > messageY
            ? static_cast<uint8_t>((footerTop - messageY) / OpenSansCondBoldCyrillic9pt.lineHeight + 1)
            : 1;
    if (messageLines == 0)
    {
        messageLines = 1;
    }
    OpenSansCondensed::printBlock(
        Watchy::display,
        OpenSansCondBoldCyrillic9pt,
        String(localMessage),
        contentLeft,
        messageY,
        contentWidth,
        messageLines,
        GxEPD_BLACK
    );

    constexpr int16_t footerWidth = 92;
    bool footerHasAction = false;
    if ((localEventFlags & ANCS::EventFlagPositiveAction) != 0)
    {
        OpenSansCondensed::printLine(
            Watchy::display,
            OpenSansCondBoldCyrillic9pt,
            String(localPositiveLabel),
            4,
            footerBaseline,
            footerWidth,
            false,
            GxEPD_BLACK
        );
        footerHasAction = true;
    }
    if ((localEventFlags & ANCS::EventFlagNegativeAction) != 0)
    {
        OpenSansCondensed::printLine(
            Watchy::display,
            OpenSansCondBoldCyrillic9pt,
            String(localNegativeLabel),
            104,
            footerBaseline,
            footerWidth,
            true,
            GxEPD_BLACK
        );
        footerHasAction = true;
    }
    if (!footerHasAction)
    {
        OpenSansCondensed::printLine(
            Watchy::display,
            OpenSansCondBoldCyrillic9pt,
            "Hold BACK to exit",
            4,
            footerBaseline,
            192,
            false,
            GxEPD_BLACK
        );
    }

    Watchy::display.display(true);
}

void NotificationService::drawStatusBar(Watchy &watchy)
{
    Watchy::RTC.read(watchy.currentTime);

    String timeText =
        (watchy.currentTime.Hour < 10 ? "0" : "") + String(watchy.currentTime.Hour) + ":" +
        (watchy.currentTime.Minute < 10 ? "0" : "") + String(watchy.currentTime.Minute);

    Watchy::display.setTextColor(GxEPD_BLACK);
    Watchy::display.setFont(&FreeSansBold12pt7b);
    Watchy::display.setCursor(-1, 17);
    Watchy::display.print(timeText);

    Watchy::display.drawBitmap(136, 3, wifi, 19, 16, GxEPD_BLACK);
    if (!WIFI_CONFIGURED)
    {
        Watchy::display.drawLine(139, 3, 151, 17, GxEPD_BLACK);
        Watchy::display.drawLine(140, 3, 152, 17, GxEPD_BLACK);
    }

    Watchy::display.setTextColor(GxEPD_BLACK);
    Watchy::display.setFont(&FreeMonoBold9pt7b);
    float voltage = watchy.getBatteryVoltage();
    int batteryPercent = constrain((voltage - 3.3) * 111.11, 0, 100);
    printRightAligned(Watchy::display, 196, 16, String(batteryPercent) + "%");

    Watchy::display.drawFastHLine(0, 21, 200, GxEPD_BLACK);
}

void NotificationService::drawIdleWatchFace(Watchy &watchy)
{
    watchy.showWatchFace(true);
}

void NotificationService::handleVibration()
{
    uint32_t now = millis();
    if (vibrationActive && static_cast<int32_t>(now - vibrationStopAtMs) >= 0)
    {
        digitalWrite(VIB_MOTOR_PIN, LOW);
        vibrationActive = false;
    }

    if (!vibrationActive && vibrationRequested)
    {
        vibrationRequested = false;
        vibrationActive = true;
        vibrationStopAtMs = now + NOTIFICATION_VIBRATION_MS;
        digitalWrite(VIB_MOTOR_PIN, HIGH);
    }
}

void NotificationService::handleButtons(Watchy &watchy)
{
    uint8_t currentButtonMask = readButtonMask();
    uint8_t pressedButtons = currentButtonMask & ~previousButtonMask;
    uint8_t releasedButtons = previousButtonMask & ~currentButtonMask;
    uint32_t now = millis();

    lockState();
    bool hasNotifications = trackedNotificationCount > 0;
    unlockState();

    if (menuVisible)
    {
        if (pressedButtons != 0 && now - lastButtonActionAtMs >= BUTTON_DEBOUNCE_MS)
        {
            if ((pressedButtons & MENU_BUTTON) != 0)
            {
                if (menuIndex == BLUETOOTH_PUSH_MENU_INDEX)
                {
                    stop();
                    watchy.showMenu(menuIndex, true);
                    waitForButtonsReleased(700);
                    watchy.deepSleep();
                    return;
                }
                lastButtonActionAtMs = now;
            }
            else if ((pressedButtons & BACK_BUTTON) != 0)
            {
                menuVisible = false;
                redrawRequested = true;
                if (!vibrationActive && !vibrationRequested)
                {
                    drawScreen(watchy);
                }
                lastButtonActionAtMs = now;
            }
            else if ((pressedButtons & UP_BUTTON) != 0)
            {
                menuIndex--;
                if (menuIndex < 0)
                {
                    menuIndex = MENU_LENGTH - 1;
                }
                watchy.showMenu(menuIndex, true);
                lastButtonActionAtMs = now;
            }
            else if ((pressedButtons & DOWN_BUTTON) != 0)
            {
                menuIndex++;
                if (menuIndex > MENU_LENGTH - 1)
                {
                    menuIndex = 0;
                }
                watchy.showMenu(menuIndex, true);
                lastButtonActionAtMs = now;
            }
        }

        previousButtonMask = currentButtonMask;
        return;
    }

    if (!hasNotifications && pressedButtons != 0)
    {
        if ((pressedButtons & MENU_BUTTON) != 0)
        {
            menuVisible = true;
            watchy.showMenu(menuIndex, true);
            lastButtonActionAtMs = now;
            waitForButtonsReleased(700);
            previousButtonMask = readButtonMask();
            return;
        }
        else
        {
            Watchy::RTC.read(watchy.currentTime);
            watchy.showWatchFace(true);
            idleWatchFaceDrawn = true;
        }
        previousButtonMask = currentButtonMask;
        return;
    }

    if ((pressedButtons & BACK_BUTTON) != 0)
    {
        backPressedAtMs = now;
        backLongPressHandled = false;
    }

    if ((currentButtonMask & BACK_BUTTON) != 0 && !backLongPressHandled)
    {
        if (backPressedAtMs != 0 && now - backPressedAtMs >= EXIT_HOLD_MS)
        {
            backLongPressHandled = true;
            menuVisible = false;
            lockState();
            trackedNotificationCount = 0;
            selectTrackedNotification(0);
            idleWatchFaceDrawn = false;
            redrawRequested = true;
            unlockState();
            previousButtonMask = currentButtonMask;
            return;
        }
    }

    if ((releasedButtons & BACK_BUTTON) != 0)
    {
        if (!backLongPressHandled && now - lastButtonActionAtMs >= BUTTON_DEBOUNCE_MS)
        {
            selectRelativeNotification(-1);
            lastButtonActionAtMs = now;
        }
        backPressedAtMs = 0;
        backLongPressHandled = false;
    }

    if (pressedButtons != 0 && now - lastButtonActionAtMs >= BUTTON_DEBOUNCE_MS)
    {
        if ((pressedButtons & MENU_BUTTON) != 0)
        {
            performCurrentNotificationAction(true, watchy);
            lastButtonActionAtMs = now;
        }
        else if ((pressedButtons & UP_BUTTON) != 0)
        {
            selectRelativeNotification(1);
            lastButtonActionAtMs = now;
        }
        else if ((pressedButtons & DOWN_BUTTON) != 0)
        {
            performCurrentNotificationAction(false, watchy);
            lastButtonActionAtMs = now;
        }
    }

    previousButtonMask = currentButtonMask;
}

void NotificationService::waitForButtonsReleased(uint32_t timeoutMs)
{
    uint32_t startMs = millis();
    while (readButtonMask() != 0 && millis() - startMs < timeoutMs)
    {
        delay(20);
    }
    previousButtonMask = readButtonMask();
}

uint8_t NotificationService::readButtonMask() const
{
    uint8_t buttonMask = 0;
    if (isButtonPressed(MENU_BTN_PIN))
    {
        buttonMask |= MENU_BUTTON;
    }
    if (isButtonPressed(BACK_BTN_PIN))
    {
        buttonMask |= BACK_BUTTON;
    }
    if (isButtonPressed(UP_BTN_PIN))
    {
        buttonMask |= UP_BUTTON;
    }
    if (isButtonPressed(DOWN_BTN_PIN))
    {
        buttonMask |= DOWN_BUTTON;
    }
    return buttonMask;
}

bool NotificationService::isButtonPressed(uint8_t pin) const
{
#ifdef ARDUINO_ESP32S3_DEV
    return digitalRead(pin) == LOW;
#else
    return digitalRead(pin) == HIGH;
#endif
}

void NotificationService::lockState()
{
    if (stateMutex != nullptr)
    {
        xSemaphoreTake(stateMutex, portMAX_DELAY);
    }
}

void NotificationService::unlockState()
{
    if (stateMutex != nullptr)
    {
        xSemaphoreGive(stateMutex);
    }
}

void NotificationService::copyCString(
    char *destination,
    size_t destinationSize,
    const char *source
) const
{
    if (destinationSize == 0)
    {
        return;
    }

    if (source == nullptr)
    {
        destination[0] = '\0';
        return;
    }

    strncpy(destination, source, destinationSize - 1);
    destination[destinationSize - 1] = '\0';
}

void NotificationService::copyString(
    char *destination,
    size_t destinationSize,
    const String &source
) const
{
    if (destinationSize == 0)
    {
        return;
    }

    source.toCharArray(destination, destinationSize);
    destination[destinationSize - 1] = '\0';

    size_t used = strlen(destination);
    if (used == 0)
    {
        return;
    }

    size_t firstByte = used - 1;
    while (firstByte > 0 && (static_cast<uint8_t>(destination[firstByte]) & 0xC0) == 0x80)
    {
        firstByte--;
    }

    uint8_t leadByte = static_cast<uint8_t>(destination[firstByte]);
    uint8_t expectedLength = 1;
    if ((leadByte & 0xE0) == 0xC0)
    {
        expectedLength = 2;
    }
    else if ((leadByte & 0xF0) == 0xE0)
    {
        expectedLength = 3;
    }
    else if ((leadByte & 0xF8) == 0xF0)
    {
        expectedLength = 4;
    }
    else if (leadByte >= 0x80)
    {
        destination[firstByte] = '\0';
        return;
    }

    if (firstByte + expectedLength > used)
    {
        destination[firstByte] = '\0';
    }
}

void NotificationService::setStatus(const char *status)
{
    lockState();
    copyCString(statusText, sizeof(statusText), status);
    redrawRequested = true;
    unlockState();
}

int8_t NotificationService::trackedNotificationIndex(uint32_t uuid) const
{
    for (uint8_t index = 0; index < trackedNotificationCount; index++)
    {
        if (trackedNotifications[index].uuid == uuid)
        {
            return static_cast<int8_t>(index);
        }
    }
    return -1;
}

bool NotificationService::isNewerNotification(
    const TrackedNotification &left,
    const TrackedNotification &right
) const
{
    if (left.time > 0 && right.time > 0 && left.time != right.time)
    {
        return left.time > right.time;
    }
    return left.sequence > right.sequence;
}

void NotificationService::sortTrackedNotifications()
{
    for (uint8_t index = 1; index < trackedNotificationCount; index++)
    {
        TrackedNotification current = trackedNotifications[index];
        int8_t sortedIndex = index - 1;
        while (sortedIndex >= 0 && isNewerNotification(current, trackedNotifications[sortedIndex]))
        {
            trackedNotifications[sortedIndex + 1] = trackedNotifications[sortedIndex];
            sortedIndex--;
        }
        trackedNotifications[sortedIndex + 1] = current;
    }
}

void NotificationService::storeTrackedNotification(
    uint8_t index,
    const TrackedNotification &notification,
    bool isNewNotification
)
{
    trackedNotifications[index].uuid = notification.uuid;
    trackedNotifications[index].eventFlags = notification.eventFlags;
    trackedNotifications[index].time = notification.time;
    if (isNewNotification || trackedNotifications[index].sequence == 0)
    {
        notificationSequenceCounter++;
        trackedNotifications[index].sequence = notificationSequenceCounter;
    }
    copyCString(
        trackedNotifications[index].title,
        sizeof(trackedNotifications[index].title),
        notification.title
    );
    copyCString(
        trackedNotifications[index].message,
        sizeof(trackedNotifications[index].message),
        notification.message
    );
    copyCString(
        trackedNotifications[index].positiveLabel,
        sizeof(trackedNotifications[index].positiveLabel),
        notification.positiveLabel
    );
    copyCString(
        trackedNotifications[index].negativeLabel,
        sizeof(trackedNotifications[index].negativeLabel),
        notification.negativeLabel
    );
}

uint8_t NotificationService::upsertTrackedNotification(const TrackedNotification &notification)
{
    int8_t existingIndex = trackedNotificationIndex(notification.uuid);
    if (existingIndex >= 0)
    {
        uint8_t index = static_cast<uint8_t>(existingIndex);
        storeTrackedNotification(index, notification, false);
        sortTrackedNotifications();
        int8_t sortedIndex = trackedNotificationIndex(notification.uuid);
        return sortedIndex >= 0 ? static_cast<uint8_t>(sortedIndex) : 0;
    }

    if (trackedNotificationCount >= MAX_TRACKED_NOTIFICATIONS)
    {
        for (uint8_t index = 1; index < MAX_TRACKED_NOTIFICATIONS; index++)
        {
            trackedNotifications[index - 1] = trackedNotifications[index];
        }
        uint8_t index = MAX_TRACKED_NOTIFICATIONS - 1;
        storeTrackedNotification(index, notification, true);
        sortTrackedNotifications();
        int8_t sortedIndex = trackedNotificationIndex(notification.uuid);
        return sortedIndex >= 0 ? static_cast<uint8_t>(sortedIndex) : 0;
    }

    uint8_t index = trackedNotificationCount;
    storeTrackedNotification(index, notification, true);
    trackedNotificationCount++;
    sortTrackedNotifications();
    int8_t sortedIndex = trackedNotificationIndex(notification.uuid);
    return sortedIndex >= 0 ? static_cast<uint8_t>(sortedIndex) : 0;
}

bool NotificationService::untrackNotification(uint32_t uuid)
{
    int8_t existingIndex = trackedNotificationIndex(uuid);
    if (existingIndex < 0)
    {
        return false;
    }

    for (uint8_t index = static_cast<uint8_t>(existingIndex) + 1; index < trackedNotificationCount; index++)
    {
        trackedNotifications[index - 1] = trackedNotifications[index];
    }

    trackedNotificationCount--;
    if (trackedNotificationCount < MAX_TRACKED_NOTIFICATIONS)
    {
        trackedNotifications[trackedNotificationCount] = TrackedNotification();
    }

    if (trackedNotificationCount == 0)
    {
        selectTrackedNotification(0);
        return true;
    }

    if (currentNotificationUUID == uuid)
    {
        uint8_t nextIndex = static_cast<uint8_t>(existingIndex);
        if (nextIndex >= trackedNotificationCount)
        {
            nextIndex = trackedNotificationCount - 1;
        }
        selectTrackedNotification(nextIndex);
    }
    else
    {
        int8_t currentIndex = trackedNotificationIndex(currentNotificationUUID);
        currentNotificationIndex = currentIndex >= 0 ? static_cast<uint8_t>(currentIndex + 1) : 0;
    }

    return true;
}

void NotificationService::selectTrackedNotification(uint8_t index)
{
    if (trackedNotificationCount == 0 || index >= trackedNotificationCount)
    {
        currentNotificationUUID = 0;
        currentNotificationIndex = 0;
        titleText[0] = '\0';
        messageText[0] = '\0';
        return;
    }

    currentNotificationIndex = index + 1;
    currentNotificationUUID = trackedNotifications[index].uuid;
    copyCString(titleText, sizeof(titleText), trackedNotifications[index].title);
    copyCString(messageText, sizeof(messageText), trackedNotifications[index].message);
}

void NotificationService::selectRelativeNotification(int8_t delta)
{
    lockState();
    if (trackedNotificationCount > 1)
    {
        uint8_t currentIndex = currentNotificationIndex > 0 ? currentNotificationIndex - 1 : 0;
        if (delta < 0)
        {
            currentIndex = currentIndex == 0 ? trackedNotificationCount - 1 : currentIndex - 1;
        }
        else
        {
            currentIndex = currentIndex + 1 >= trackedNotificationCount ? 0 : currentIndex + 1;
        }
        selectTrackedNotification(currentIndex);
        redrawRequested = true;
    }
    unlockState();
}

void NotificationService::performCurrentNotificationAction(bool positiveAction, Watchy &watchy)
{
    uint32_t uuid = 0;
    bool actionAvailable = false;
    bool drawImmediately = false;
    bool sendToPhone = true;

    lockState();
    if (currentNotificationIndex > 0 && currentNotificationIndex <= trackedNotificationCount)
    {
        const TrackedNotification &currentNotification =
            trackedNotifications[currentNotificationIndex - 1];
        uint32_t actionFlag =
            positiveAction ? ANCS::EventFlagPositiveAction : ANCS::EventFlagNegativeAction;
        bool localDismiss =
            !positiveAction && isLocalDismissActionLabel(currentNotification.negativeLabel);
        actionAvailable = (currentNotification.eventFlags & actionFlag) != 0 || localDismiss;
        uuid = currentNotification.uuid;

        if (actionAvailable)
        {
            copyCString(statusText, sizeof(statusText), "iOS");
            if (localDismiss)
            {
                sendToPhone = false;
                active = true;
                activeInstance = this;
                startupPending = false;
                menuVisible = false;
                untrackNotification(uuid);
                if (trackedNotificationCount == 0)
                {
                    idleWatchFaceDrawn = false;
                    if (notificationsReady && !connected)
                    {
                        restartAdvertisingRequested = true;
                        restartAdvertisingAtMs = millis() + ADVERTISING_RESTART_DELAY_MS;
                    }
                }
                redrawRequested = true;
                drawImmediately = !menuVisible;
            }
            else if (!connected)
            {
                actionAvailable = false;
            }
        }
    }
    unlockState();

    if (uuid == 0 || !actionAvailable)
    {
        return;
    }

    if (drawImmediately && !vibrationActive && !vibrationRequested)
    {
        drawScreen(watchy);
    }

    if (sendToPhone)
    {
        sendNotificationActionAsync(uuid, positiveAction);
    }
    if (positiveAction)
    {
        setStatus("iOS");
    }
}

void NotificationService::sendNotificationActionAsync(uint32_t uuid, bool positiveAction)
{
    PendingAction *action = new PendingAction();
    action->notifications = &notifications;
    action->uuid = uuid;
    action->positiveAction = positiveAction;

    BaseType_t created = xTaskCreatePinnedToCore(
        runActionTask,
        "ANCSAction",
        4096,
        action,
        1,
        nullptr,
        1
    );
    if (created != pdPASS)
    {
        delete action;
        if (positiveAction)
        {
            notifications.actionPositive(uuid);
        }
        else
        {
            notifications.actionNegative(uuid);
        }
    }
}

bool NotificationService::isLocalDismissActionLabel(const char *label) const
{
    if (label == nullptr || label[0] == '\0')
    {
        return false;
    }

    return strstr(label, "Clear") != nullptr ||
        strstr(label, "Dismiss") != nullptr ||
        strstr(label, "Close") != nullptr ||
        strstr(label, "Remove") != nullptr ||
        strstr(label, "Очист") != nullptr ||
        strstr(label, "Удал") != nullptr ||
        strstr(label, "Закры") != nullptr;
}
