
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE

// NKolban's original library has a setServiceSolicitation method. As of writing, this is not in the
// Espressif core libs. If you are using NKolban's branch of the library, or if this moves into
// the core libraries eventually, uncomment this.
// #define BLE_LIB_HAS_SERVICE_SOLICITATION

#include "esp32notifications.h"
#include "ancs_ble_client.h"
#include "ble_security.h"

#include "BLEAddress.h"
#include "BLEDevice.h"
#include "BLEServer.h"
#include "BLEClient.h"
#include "BLEUtils.h"
#include "BLE2902.h"

#include <esp_gatts_api.h>

static char LOG_TAG[] = "BLENotifications";

extern const BLEUUID ancsServiceUUID;

#ifndef BLE_LIB_HAS_SERVICE_SOLICITATION
// Use a static function, instead of doing a whole private implementation just for a this one small patch.
static void setServiceSolicitation(class BLEAdvertisementData &advertisementData, BLEUUID uuid);
#endif

class MyServerCallbacks : public BLEServerCallbacks
{

public:
	BLENotifications *instance;

	MyServerCallbacks(BLENotifications *parent)
			: instance(parent)
	{
	}

	void onConnect(BLEServer *pServer, esp_ble_gatts_cb_param_t *param)
	{
		ESP_LOGI(LOG_TAG, "Device connected");
		instance->client = new ANCSBLEClient(); // @todo memory leaks?
		instance->client->setNotificationArrivedCallback(instance->cbNotification);
		instance->client->setNotificationRemovedCallback(instance->cbRemoved);
		::xTaskCreatePinnedToCore(&ANCSBLEClient::startClientTask, "ClientTask", 10000, new BLEAddress(param->connect.remote_bda), 5, &instance->client->clientTaskHandle, 0);

		delay(1000);

		ESP_LOGI(LOG_TAG, "Set up client");

		if (instance->cbStateChanged)
		{
			instance->cbStateChanged(BLENotifications::StateConnected);
		}
	};

	void onDisconnect(BLEServer *pServer)
	{
		::vTaskDelete(instance->client->clientTaskHandle);
		instance->client->clientTaskHandle = nullptr;
		ESP_LOGI(LOG_TAG, "Device disconnected");
		if (instance->cbStateChanged)
		{
			instance->cbStateChanged(BLENotifications::StateDisconnected);
		}
		delete instance->client;
		instance->client = nullptr;
	}
};

BLENotifications::BLENotifications()
		: cbStateChanged(nullptr), client(nullptr), isAdvertising(false)
{
}

const char *BLENotifications::getNotificationCategoryDescription(NotificationCategory category) const
{
	switch (category)
	{
	case CategoryIDOther:
		return "other";
	case CategoryIDIncomingCall:
		return "incoming call";
	case CategoryIDMissedCall:
		return "missed call";
	case CategoryIDVoicemail:
		return "voicemail";
	case CategoryIDSocial:
		return "social";
	case CategoryIDSchedule:
		return "schedule";
	case CategoryIDEmail:
		return "email";
	case CategoryIDNews:
		return "news";
	case CategoryIDHealthAndFitness:
		return "health and fitness";
	case CategoryIDBusinessAndFinance:
		return "business and finance";
	case CategoryIDLocation:
		return "location";
	case CategoryIDEntertainment:
		return "entertainment";
	default:
		return "unknown";
	}
}

bool BLENotifications::begin(const char *name)
{
	ESP_LOGI(LOG_TAG, "begin()");
	BLEDevice::init(name);
	server = BLEDevice::createServer();
	server->setCallbacks(new MyServerCallbacks(this));
	BLEDevice::setSecurityCallbacks(new NotificationSecurityCallbacks()); // @todo memory leak?

	startAdvertising();
	return true;
}

bool BLENotifications::stop()
{
	if (isAdvertising)
	{
		BLEDevice::stopAdvertising();
		isAdvertising = false;
	}
	if (server != nullptr && server->getConnectedCount() > 0)
	{
		server->disconnect(server->getConnId());
		delay(250);
	}
	if (client != nullptr)
	{
		if (client->clientTaskHandle != nullptr)
		{
			::vTaskDelete(client->clientTaskHandle);
			client->clientTaskHandle = nullptr;
		}
		delete client;
		client = nullptr;
	}
	return true;
}

void BLENotifications::setConnectionStateChangedCallback(ble_notifications_state_changed_t callback)
{
	cbStateChanged = callback;
}

void BLENotifications::setNotificationCallback(ble_notification_arrived_t callback)
{
	cbNotification = callback;
}

void BLENotifications::setRemovedCallback(ble_notification_removed_t callback)
{
	cbRemoved = callback;
}

void BLENotifications::actionPositive(uint32_t uuid)
{
	ESP_LOGI(LOG_TAG, "actionPositive()");
	client->performAction(uuid, uint8_t(ANCS::NotificationActionPositive));
}

void BLENotifications::actionNegative(uint32_t uuid)
{
	ESP_LOGI(LOG_TAG, "actionNegative()");
	client->performAction(uuid, uint8_t(ANCS::NotificationActionNegative));
}

void BLENotifications::startAdvertising()
{
	ESP_LOGI(LOG_TAG, "startAdvertising()");

	// Start soliciting the Apple ANCS service and make the device visible to searches on iOS (from Apple ANCS documentation)
	BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();

	if (isAdvertising == true)
	{
		// Stopping it without checking first seems to cause failures to advertise without debugging on.
		// There is no way to query the BLEAdvertising object to see if it is advertising, so we keep a variable.
		pAdvertising->stop();
	}

	BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();
	oAdvertisementData.setFlags(0x01);

#ifdef BLE_LIB_HAS_SERVICE_SOLICITATION
	oAdvertisementData.setServiceSolicitation(ANCSBLEClient::getAncsServiceUUID());
#else
	setServiceSolicitation(oAdvertisementData, ANCSBLEClient::getAncsServiceUUID());
#endif
#ifdef ENABLE_IOS_SETTINGS_PAIRING_HELPER
	oAdvertisementData.setCompleteServices(BLEUUID((uint16_t)0x1812));
	oAdvertisementData.setAppearance(0x03C1);
#else
	oAdvertisementData.setShortName("ANCS");
#endif

	pAdvertising->setAdvertisementData(oAdvertisementData);

#ifdef ENABLE_IOS_SETTINGS_PAIRING_HELPER
	BLEAdvertisementData scanResponseData = BLEAdvertisementData();
	scanResponseData.setName("Watchy");
	pAdvertising->setScanResponseData(scanResponseData);
#endif

	// Set security
	BLESecurity *pSecurity = new BLESecurity();
	pSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_BOND);
	pSecurity->setCapability(ESP_IO_CAP_OUT);
	pSecurity->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);

	// Start advertising
	pAdvertising->start();
	isAdvertising = true;

	ESP_LOGD(LOG_TAG, "Advertising started!");
}

#ifndef BLE_LIB_HAS_SERVICE_SOLICITATION
void setServiceSolicitation(BLEAdvertisementData &advertisementData, BLEUUID uuid)
{
	char cdata[2];
	switch (uuid.bitSize())
	{
	case 16:
	{
		// [Len] [0x14] [UUID16] data
		cdata[0] = 3;
		cdata[1] = ESP_BLE_AD_TYPE_SOL_SRV_UUID; // 0x14
		std::string payload(cdata, 2);
		payload.append((char *)&uuid.getNative()->uuid.uuid16, 2);
		advertisementData.addData(payload);
		break;
	}

	case 128:
	{
		// [Len] [0x15] [UUID128] data
		cdata[0] = 17;
		cdata[1] = ESP_BLE_AD_TYPE_128SOL_SRV_UUID; // 0x15
		std::string payload(cdata, 2);
		payload.append((char *)uuid.getNative()->uuid.uuid128, 16);
		advertisementData.addData(payload);
		break;
	}

	default:
		return;
	}
}
#endif
