#include <BLEMidi.h>

void BLEMidi::begin(std::string deviceName,
    BLEServerCallbacks *serverCallback,
    BLECharacteristicCallbacks *characteristicCallback) {

    this->deviceName = deviceName;

    BLEDevice::init(deviceName);
    // BLEDevice::setEncryptionLevel(ESP_BLE_SEC_NONE);

    //Start Server
    server = BLEDevice::createServer();
    server->setCallbacks(serverCallback);
    BLEService *service = server->createService(BLEUUID(midiServiceUUID));
    characteristic = service->createCharacteristic(
    BLEUUID(midiCharacteristicUUID),
    BLECharacteristic::PROPERTY_READ   |
    BLECharacteristic::PROPERTY_NOTIFY |
    BLECharacteristic::PROPERTY_WRITE_NR
    );
    characteristic->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED);
    characteristic->setCallbacks(characteristicCallback);
    characteristic->addDescriptor(new BLE2902());
    service->start();

    //Start Advertising
    BLEAdvertisementData advertisementData = BLEAdvertisementData();
    advertisementData.setFlags(ESP_BLE_ADV_FLAG_LIMIT_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT);
    advertisementData.setCompleteServices(BLEUUID(midiServiceUUID));
    advertisementData.setName(deviceName);
    advertising = server->getAdvertising();
    advertising->setAdvertisementData(advertisementData);
    advertising->addServiceUUID(MIDI_SERVICE_UUID);

    BLESecurity *security = new BLESecurity();
    security->setAuthenticationMode(ESP_LE_AUTH_BOND);
    security->setCapability(ESP_IO_CAP_NONE);
    security->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);

    advertising->start();
}

void BLEMidi::sendNote(uint8_t status, uint8_t noteNo, uint8_t vel) {
    uint8_t midiPacket[] = {
        0x80,  // header
        0x80,  // timestamp, not implemented
        0x00,  // status
        0x3c,  // 0x3c == 60 == middle c
        0x00   // velocity
    };
    midiPacket[2] = status; // note on/off, channel 0
    midiPacket[3] = noteNo & 0b01111111;
    midiPacket[4] = vel & 0b01111111;
    characteristic->setValue(midiPacket, 5); // packet, length in bytes)
    characteristic->notify();
}

void BLEMidi::sendCC(uint8_t ccNo, uint8_t value) {
    uint8_t midiPacket[] = {
        0x80,  // header
        0x80,  // timestamp, not implemented
        0xB0,  // status
        0x00,  // CC#
        0x00   // value
    };
    midiPacket[2] = 0xB0; // cc, channel 0
    midiPacket[3] = ccNo & 0b01111111;
    midiPacket[4] = value & 0b01111111;
    characteristic->setValue(midiPacket, 5); // packet, length in bytes)
    characteristic->notify();
}

BLEMidi Midi;