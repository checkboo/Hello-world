// BTKeyboard — ESP32 BLE HID Host wrapper.
//
// This is the highest-risk module in the firmware. The arduino-esp32 v2/v3
// distribution ships the BT controller and Bluedroid stack and the IDF
// headers are reachable. What is NOT guaranteed is that
// CONFIG_BT_HID_HOST_ENABLED is on in the prebuilt libs. If linking fails on
// esp_hidh symbols, switch the platformio.ini to `framework = arduino, espidf`
// and add an sdkconfig.defaults that turns the option on.
//
// Reference: ESP-IDF examples/bluetooth/esp_hid_host (dual-mode HID host).

#include "BTKeyboard.h"

#include <Preferences.h>
#include <string.h>

#include <vector>

#include "Config.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_hidh.h"

namespace journal {

namespace {

KeyCallback                 g_cb           = nullptr;
volatile BtState            g_state        = BtState::Off;
esp_hidh_dev_t*             g_dev          = nullptr;
uint8_t                     g_lastReport[8]= {0};
bool                        g_haveLast     = false;
bool                        g_stackUp      = false;

std::vector<DiscoveredPeer> g_found;
volatile bool               g_scanDone     = false;

void emitDiff(const uint8_t* prev, const uint8_t* cur) {
  if (!g_cb) return;
  for (int i = 2; i < 8; ++i) {
    uint8_t pk = prev[i];
    if (pk == 0 || pk == 1) continue;
    bool stillDown = false;
    for (int j = 2; j < 8; ++j) if (cur[j] == pk) { stillDown = true; break; }
    if (!stillDown) { KeyEvent e{prev[0], pk, false}; g_cb(e); }
  }
  for (int i = 2; i < 8; ++i) {
    uint8_t ck = cur[i];
    if (ck == 0 || ck == 1) continue;
    bool wasDown = false;
    for (int j = 2; j < 8; ++j) if (prev[j] == ck) { wasDown = true; break; }
    if (!wasDown) { KeyEvent e{cur[0], ck, true}; g_cb(e); }
  }
}

void onHidh(void* /*arg*/, esp_event_base_t /*base*/, int32_t id, void* event_data) {
  esp_hidh_event_t       evt = (esp_hidh_event_t)id;
  esp_hidh_event_data_t* d   = (esp_hidh_event_data_t*)event_data;
  switch (evt) {
    case ESP_HIDH_OPEN_EVENT:
      if (d->open.status == ESP_OK) {
        g_dev      = d->open.dev;
        g_state    = BtState::Connected;
        g_haveLast = false;
        memset(g_lastReport, 0, sizeof(g_lastReport));
        esp_hidh_dev_set_protocol(g_dev, 0 /*BOOT*/);
      } else {
        g_state = BtState::Lost;
      }
      break;
    case ESP_HIDH_INPUT_EVENT:
      if (d->input.length >= 8) {
        uint8_t cur[8];
        memcpy(cur, d->input.data, 8);
        if (g_haveLast) emitDiff(g_lastReport, cur);
        else            { uint8_t z[8] = {0}; emitDiff(z, cur); }
        memcpy(g_lastReport, cur, 8);
        g_haveLast = true;
      }
      break;
    case ESP_HIDH_CLOSE_EVENT:
      g_dev      = nullptr;
      g_haveLast = false;
      g_state    = BtState::Lost;
      break;
    default:
      break;
  }
}

void onGap(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t* param) {
  switch (event) {
    case ESP_GAP_BLE_SCAN_RESULT_EVT: {
      const auto& r = param->scan_rst;
      if (r.search_evt == ESP_GAP_SEARCH_INQ_RES_EVT) {
        // Decode complete or short name out of the advertising payload.
        uint8_t adv_name_len = 0;
        uint8_t* adv_name = esp_ble_resolve_adv_data(
            const_cast<uint8_t*>(r.ble_adv), ESP_BLE_AD_TYPE_NAME_CMPL, &adv_name_len);
        if (!adv_name) {
          adv_name = esp_ble_resolve_adv_data(
              const_cast<uint8_t*>(r.ble_adv), ESP_BLE_AD_TYPE_NAME_SHORT, &adv_name_len);
        }
        // Filter to advertisers that mention the HID service UUID 0x1812.
        uint8_t uuid_len = 0;
        uint8_t* uuid_data = esp_ble_resolve_adv_data(
            const_cast<uint8_t*>(r.ble_adv), ESP_BLE_AD_TYPE_16SRV_CMPL, &uuid_len);
        bool isHid = false;
        if (uuid_data && uuid_len >= 2) {
          for (uint8_t i = 0; i + 1 < uuid_len; i += 2) {
            uint16_t u = uuid_data[i] | (uuid_data[i + 1] << 8);
            if (u == 0x1812) { isHid = true; break; }
          }
        }
        if (!isHid) {
          uuid_data = esp_ble_resolve_adv_data(
              const_cast<uint8_t*>(r.ble_adv), ESP_BLE_AD_TYPE_16SRV_PART, &uuid_len);
          if (uuid_data && uuid_len >= 2) {
            for (uint8_t i = 0; i + 1 < uuid_len; i += 2) {
              uint16_t u = uuid_data[i] | (uuid_data[i + 1] << 8);
              if (u == 0x1812) { isHid = true; break; }
            }
          }
        }
        if (!isHid) break;

        // Skip duplicates.
        for (const auto& p : g_found) {
          if (memcmp(p.bda, r.bda, 6) == 0) return;
        }
        DiscoveredPeer peer{};
        memcpy(peer.bda, r.bda, 6);
        size_t n = adv_name_len < sizeof(peer.name) - 1 ? adv_name_len : sizeof(peer.name) - 1;
        if (adv_name && n) memcpy(peer.name, adv_name, n);
        peer.name[n] = '\0';
        g_found.push_back(peer);
      } else if (r.search_evt == ESP_GAP_SEARCH_INQ_CMPL_EVT) {
        g_scanDone = true;
      }
      break;
    }
    case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
      g_scanDone = true;
      break;
    default:
      break;
  }
}

bool initStackOnce() {
  if (g_stackUp) return true;

  // Release Classic-BT memory (we only use BLE).
  esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);

  esp_bt_controller_config_t cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
  if (esp_bt_controller_init(&cfg) != ESP_OK)            return false;
  if (esp_bt_controller_enable(ESP_BT_MODE_BLE) != ESP_OK) return false;
  if (esp_bluedroid_init() != ESP_OK)                    return false;
  if (esp_bluedroid_enable() != ESP_OK)                  return false;
  if (esp_ble_gap_register_callback(onGap) != ESP_OK)    return false;

  // Security: bonded, secure-connections, MITM not required.
  esp_ble_auth_req_t  auth = ESP_LE_AUTH_REQ_SC_BOND;
  esp_ble_io_cap_t    io   = ESP_IO_CAP_NONE;
  uint8_t key_size = 16;
  uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
  uint8_t resp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
  esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE,    &auth, sizeof(auth));
  esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE,         &io,   sizeof(io));
  esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE,       &key_size, sizeof(key_size));
  esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY,       &init_key, sizeof(init_key));
  esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY,        &resp_key, sizeof(resp_key));

  g_stackUp = true;
  return true;
}

}  // namespace

bool BTKeyboard::begin(KeyCallback cb) {
  g_cb = cb;
  if (!initStackOnce()) { g_state = BtState::Off; return false; }

  esp_hidh_config_t cfg = {};
  cfg.callback         = onHidh;
  cfg.event_stack_size = 4096;
  cfg.callback_arg     = nullptr;
  if (esp_hidh_init(&cfg) != ESP_OK) {
    g_state = BtState::Off;
    return false;
  }
  g_state = BtState::Off;
  return true;
}

bool BTKeyboard::hasBond() {
  Preferences p;
  if (!p.begin(NVS_NS, true)) return false;
  bool b = p.isKey(NVS_KEY_BONDED) && p.getBool(NVS_KEY_BONDED, false);
  p.end();
  return b;
}

void BTKeyboard::forgetBond() {
  Preferences p;
  if (p.begin(NVS_NS, false)) {
    p.remove(NVS_KEY_BONDED);
    p.remove("bda");
    p.end();
  }
  int n = esp_ble_get_bond_device_num();
  if (n > 0) {
    std::vector<esp_ble_bond_dev_t> list(n);
    if (esp_ble_get_bond_device_list(&n, list.data()) == ESP_OK) {
      for (int i = 0; i < n; ++i) esp_ble_remove_bond_device(list[i].bd_addr);
    }
  }
}

bool BTKeyboard::connectBonded() {
  Preferences p;
  if (!p.begin(NVS_NS, true)) return false;
  uint8_t bda[6] = {0};
  size_t got = p.getBytes("bda", bda, 6);
  p.end();
  if (got != 6) return false;
  g_state = BtState::Connecting;
  esp_err_t err = esp_hidh_dev_open(bda, ESP_HID_TRANSPORT_BLE, BLE_ADDR_TYPE_PUBLIC);
  if (err != ESP_OK) { g_state = BtState::Lost; return false; }
  return true;
}

bool BTKeyboard::startScan() {
  g_found.clear();
  g_scanDone = false;
  g_state    = BtState::Scanning;

  esp_ble_scan_params_t sp = {};
  sp.scan_type          = BLE_SCAN_TYPE_ACTIVE;
  sp.own_addr_type      = BLE_ADDR_TYPE_PUBLIC;
  sp.scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL;
  sp.scan_interval      = 0x50;
  sp.scan_window        = 0x30;
  sp.scan_duplicate     = BLE_SCAN_DUPLICATE_DISABLE;
  esp_ble_gap_set_scan_params(&sp);
  esp_ble_gap_start_scanning(PAIRING_SCAN_S);

  uint32_t deadline = millis() + (uint32_t)PAIRING_SCAN_S * 1000 + 500;
  while (!g_scanDone && millis() < deadline) delay(50);
  esp_ble_gap_stop_scanning();
  g_state = BtState::Off;
  return !g_found.empty();
}

void   BTKeyboard::stopScan()              { esp_ble_gap_stop_scanning(); }
size_t BTKeyboard::discoveredCount()       { return g_found.size(); }
const DiscoveredPeer& BTKeyboard::discovered(size_t i) { return g_found[i]; }

bool BTKeyboard::pairAndBond(const DiscoveredPeer& peer) {
  g_state = BtState::Connecting;
  esp_err_t err = esp_hidh_dev_open(
      const_cast<uint8_t*>(peer.bda), ESP_HID_TRANSPORT_BLE, BLE_ADDR_TYPE_PUBLIC);
  if (err != ESP_OK) { g_state = BtState::Lost; return false; }
  Preferences p;
  if (p.begin(NVS_NS, false)) {
    p.putBool(NVS_KEY_BONDED, true);
    p.putBytes("bda", peer.bda, 6);
    p.end();
  }
  return true;
}

BtState BTKeyboard::state() { return g_state; }

}  // namespace journal
