/*
   BLE MIDI example

   The example allows a remote BLE device to control the onboard LED and get state updates from the onboard button.

   On startup the sketch will start a BLE advertisement with the configured name, then
   it will accept any incoming connection. The device offers a GATT service and characteristic
   for controlling the onboard LED and reporting the state of an onboard button. The demo will work
   on boards without a built-in button however the button state reporting feature will be disabled.
   With the Simplicity Connect app you can test this functionality by going to the "Demo" tab and selecting "MIDI".

   Find out more on the API usage at: https://docs.silabs.com/bluetooth/latest/bluetooth-stack-api/

   This example only works with the 'BLE (Silabs)' protocol stack variant.

   Get the Simplicity Connect app:
    - https://play.google.com/store/apps/details?id=com.siliconlabs.bledemo
    - https://apps.apple.com/us/app/efr-connect-ble-mobile-app/id1030932759

   Compatible boards:
   - Arduino Nano Matter
   - SparkFun Thing Plus MGM240P
   - xG27 DevKit
   - xG24 Explorer Kit
   - xG24 Dev Kit
   - BGM220 Explorer Kit
   - Ezurio Lyra 24P 20dBm Dev Kit
   - Seeed Studio XIAO MG24 (Sense)

   Author: Giang Nguyen Thu (Silicon Labs)
 */

#include <MIDI.h>
#define MIDI_SERIAL  Serial1

bool btn_notification_enabled = false;
volatile uint8_t btn_state = LOW;
static void btn_state_change_callback();
static void midi_note_on(byte note, byte velocity);
static void midi_note_off(byte note, byte velocity);
static uint8_t conn_handle = 0xFF;

byte note_to_send;
byte note_velocity;

// Event flags for MIDI notes
volatile bool note_on_event = false;
volatile bool note_off_event = false;

PACKSTRUCT(typedef struct
{
  uint8_t  header;
  uint8_t  timestamp;
  uint8_t  status;
  uint8_t  note;
  uint8_t  velocity;
  // uint8_t  pressure;
}) midi_event_packet_t;

typedef union {
  midi_event_packet_t  midi_event;
  uint8_t payload[5];
}midi_data_t;

void handleNoteOn(byte channel, byte pitch, byte velocity)
{
    // Do whatever you want when a note is pressed.
    note_to_send = pitch;
    note_velocity = velocity;
    note_on_event = true;
    Serial.printf("Note on: channel: %d, pitch: %d, velocity: %d\r\n", channel, pitch, velocity);
}

void handleNoteOff(byte channel, byte pitch, byte velocity)
{
    // Do something when the note is released.
    note_to_send = pitch;
    note_velocity = velocity;
    note_off_event = true;
    Serial.printf("Note off: channel: %d, pitch: %d, velocity: %d\r\n", channel, pitch, velocity);
}

// -----------------------------------------------------------------------------
MIDI_CREATE_INSTANCE(HardwareSerial, MIDI_SERIAL, midiA);

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LED_BUILTIN_INACTIVE);
  Serial.begin(115200);
  Serial.println("Silicon Labs MIDI BLE example");

  // If the board has a built-in button configure it for usage
  #ifdef BTN_BUILTIN
  pinMode(BTN_BUILTIN, INPUT_PULLUP);
  attachInterrupt(BTN_BUILTIN, btn_state_change_callback, CHANGE);
  #else // BTN_BUILTIN
  // Avoid warning for unused function on boards without a button
  (void)btn_state_change_callback;
  #endif // BTN_BUILTIN

  Serial.println("MIDI RECV start");
  // Connect the handleNoteOn function to the library,
  midiA.setHandleNoteOn(handleNoteOn);  // Put only the name of the function

  // Do the same for NoteOffs
  midiA.setHandleNoteOff(handleNoteOff);

  // Initiate MIDI communications, listen to all channels
  midiA.begin(MIDI_CHANNEL_OMNI);
}

void loop()
{
  // Call MIDI.read the fastest you can for real-time performance.
  midiA.read();

  // Handle MIDI note-on event
  if (note_on_event) {
    note_on_event = false;
    midi_note_on(note_to_send, note_velocity);
  }
  // Handle MIDI note-off event
  if (note_off_event) {
    note_off_event = false;
    midi_note_off(note_to_send, note_velocity);
  }
}

static void ble_initialize_gatt_db();
static void ble_start_advertising();

static const uint8_t advertised_name[] = "MIDI BLE Example";
static uint16_t gattdb_session_id;
static uint16_t generic_access_service_handle;
static uint16_t name_characteristic_handle;
static uint16_t midi_service_handle;
static uint16_t midi_report_characteristic_handle;

/**************************************************************************//**
 * Bluetooth stack event handler
 * Called when an event happens on BLE the stack
 *
 * @param[in] evt Event coming from the Bluetooth stack
 *****************************************************************************/
void sl_bt_on_event(sl_bt_msg_t *evt)
{
  switch (SL_BT_MSG_ID(evt->header)) {
    case sl_bt_evt_system_boot_id:
    {
      Serial.begin(115200);
      Serial.println("BLE stack booted");

      // Initialize the application specific GATT table
      ble_initialize_gatt_db();

      // Start advertising
      ble_start_advertising();
      Serial.println("BLE advertisement started");
    }
    break;

    case sl_bt_evt_connection_opened_id:
      Serial.println("BLE connection opened");
      conn_handle = evt->data.evt_connection_opened.connection;
      break;

    case sl_bt_evt_connection_closed_id:
      Serial.println("BLE connection closed");
      // Restart the advertisement
      ble_start_advertising();
      Serial.println("BLE advertisement restarted");
      break;

    case sl_bt_evt_gatt_server_attribute_value_id:
      break;

    default:
      break;
  }
}

/**************************************************************************//**
 * Called on button state change - stores the current button state and
 * sets a flag that a button state change occurred.
 * If the board doesn't have a button the function does nothing.
 *****************************************************************************/
static void btn_state_change_callback()
{
  #ifdef BTN_BUILTIN
  btn_state = !digitalRead(BTN_BUILTIN);
  #endif // BTN_BUILTIN
}

/**************************************************************************//**
 * Sends a BLE notification to the client if notifications are enabled and
 * the board has a built-in button.
 * (Currently sends a MIDI note message for demonstration.)
 *****************************************************************************/

static void send_button_state_notification()
{
    Serial.print("Notification sent, button state: ");
    if(btn_state == 1) 
      midi_note_on(note_to_send, note_velocity);
    else 
      midi_note_off(note_to_send, note_velocity);
    Serial.println(btn_state);
}

static void midi_note_on(byte note, byte velocity)
{
  sl_status_t sc;
  static midi_data_t note_on;

  uint32_t tick;
  uint32_t temp;

  tick = sl_sleeptimer_get_tick_count();
  temp = sl_sleeptimer_tick_to_ms(tick);
  // Mask it - only the lower 13 bit needed
  temp = temp & 0x00001fff;
  // Header byte = 0b10xxxxxx where xxxxxxx is top 6 bits of timestamp
  note_on.midi_event.header = 0x80 | (temp >> 7);
  // Timestamp byte = 0b1xxxxxxx where xxxxxxx is lower 7 bits of timestamp
  note_on.midi_event.timestamp = 0x80 | (temp & 0x003f);
  // Status byte = 0b1sssnnnn where sss is message type and nnnn is channel
  note_on.midi_event.status = 0x90;
  // Setting the note parameter
  note_on.midi_event.note = note;
  // Setting the velocity parameter
  note_on.midi_event.velocity = velocity;

  // Sending the assembled midi message
  sc = sl_bt_gatt_server_send_notification(conn_handle, midi_report_characteristic_handle, 5, (uint8_t const *) &note_on);
}

static void midi_note_off(byte note, byte velocity)
{
  sl_status_t sc;
  static midi_data_t note_off;

  uint32_t tick;
  uint32_t temp;

  tick = sl_sleeptimer_get_tick_count();
  temp = sl_sleeptimer_tick_to_ms(tick);
  // Mask it - only the lower 13 bit needed
  temp = temp & 0x00001fff;
  // Header byte = 0b10xxxxxx where xxxxxxx is top 6 bits of timestamp
  note_off.midi_event.header = 0x80 | (temp >> 7);
  // Timestamp byte = 0b1xxxxxxx where xxxxxxx is lower 7 bits of timestamp
  note_off.midi_event.timestamp = 0x80 | (temp & 0x003f);
  // Status byte = 0b1sssnnnn where sss is message type and nnnn is channel
  note_off.midi_event.status = 0x80;
  // Setting the note parameter
  note_off.midi_event.note = note;
  // Setting the velocity parameter
  note_off.midi_event.velocity = velocity;

  // Sending the assembled midi message
  sc = sl_bt_gatt_server_send_notification(conn_handle, midi_report_characteristic_handle, 5, (uint8_t const *) &note_off);
}

/**************************************************************************//**
 * Starts BLE advertisement
 * Initializes advertising if it's called for the first time
 *****************************************************************************/
static void ble_start_advertising()
{
  static uint8_t advertising_set_handle = 0xff;
  static bool init = true;
  sl_status_t sc;

  if (init) {
    // Create an advertising set
    sc = sl_bt_advertiser_create_set(&advertising_set_handle);
    app_assert_status(sc);

    // Set advertising interval to 100ms
    sc = sl_bt_advertiser_set_timing(
      advertising_set_handle,
      160,   // minimum advertisement interval (milliseconds * 1.6)
      160,   // maximum advertisement interval (milliseconds * 1.6)
      0,     // advertisement duration
      0);    // maximum number of advertisement events
    app_assert_status(sc);

    init = false;
  }

  // Generate data for advertising
  sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle, sl_bt_advertiser_general_discoverable);
  app_assert_status(sc);

  // Start advertising and enable connections
  sc = sl_bt_legacy_advertiser_start(advertising_set_handle, sl_bt_advertiser_connectable_scannable);
  app_assert_status(sc);
}

/**************************************************************************//**
 * Initializes the GATT database
 * Creates a new GATT session and adds certain services and characteristics
 *****************************************************************************/
static void ble_initialize_gatt_db()
{
  sl_status_t sc;
  // Create a new GATT database
  sc = sl_bt_gattdb_new_session(&gattdb_session_id);
  app_assert_status(sc);

  // Add the Generic Access service to the GATT DB
  const uint8_t generic_access_service_uuid[] = { 0x00, 0x18 };
  sc = sl_bt_gattdb_add_service(gattdb_session_id,
                                sl_bt_gattdb_primary_service,
                                SL_BT_GATTDB_ADVERTISED_SERVICE,
                                sizeof(generic_access_service_uuid),
                                generic_access_service_uuid,
                                &generic_access_service_handle);
  app_assert_status(sc);

  // Add the Device Name characteristic to the Generic Access service
  // The value of the Device Name characteristic will be advertised
  const sl_bt_uuid_16_t device_name_characteristic_uuid = { .data = { 0x00, 0x2A } };
  sc = sl_bt_gattdb_add_uuid16_characteristic(gattdb_session_id,
                                              generic_access_service_handle,
                                              SL_BT_GATTDB_CHARACTERISTIC_READ,
                                              0x00,
                                              0x00,
                                              device_name_characteristic_uuid,
                                              sl_bt_gattdb_fixed_length_value,
                                              sizeof(advertised_name) - 1,
                                              sizeof(advertised_name) - 1,
                                              advertised_name,
                                              &name_characteristic_handle);
  app_assert_status(sc);

  // Start the Generic Access service
  sc = sl_bt_gattdb_start_service(gattdb_session_id, generic_access_service_handle);
  app_assert_status(sc);

  // Add the MIDI service to the GATT DB
  // UUID: 03B80E5A-EDE8-4B33-A751-6CE34EC4C700
  const uuid_128 midi_service_uuid = {
    .data = { 0x00, 0xC7, 0xC4, 0x4E, 0xE3, 0x6C, 0x51, 0xA7, 0x33, 0x4B, 0xE8, 0xED, 0x5A, 0x0E, 0xB8, 0x03 }
  };
  sc = sl_bt_gattdb_add_service(gattdb_session_id,
                                sl_bt_gattdb_primary_service,
                                SL_BT_GATTDB_ADVERTISED_SERVICE,
                                sizeof(midi_service_uuid),
                                midi_service_uuid.data,
                                &midi_service_handle);
  app_assert_status(sc);

  // Add the ' MIDI Data UUID' characteristic to the MIDI service
  // UUID: 7772E5DB-3868-4112-A1A9-F2669D106BF3
  const uuid_128 midi_report_characteristic_uuid = {
    .data = { 0xF3, 0x6B, 0x10, 0x9D, 0x66, 0xF2, 0xA9, 0xA1, 0x12, 0x41, 0x68, 0x38, 0xDB, 0xE5, 0x72, 0x77 }
  };

  uint8_t midi_char_init_value = 0;
  sc = sl_bt_gattdb_add_uuid128_characteristic(gattdb_session_id,
                                               midi_service_handle,
                                               SL_BT_GATTDB_CHARACTERISTIC_READ | SL_BT_GATTDB_CHARACTERISTIC_NOTIFY
                                               | SL_BT_GATTDB_CHARACTERISTIC_WRITE,
                                               0x00,
                                               0x00,
                                               midi_report_characteristic_uuid,
                                               sl_bt_gattdb_fixed_length_value,
                                               1,                               // max length
                                               sizeof(midi_char_init_value),     // initial value length
                                               &midi_char_init_value,            // initial value
                                               &midi_report_characteristic_handle);

  // Start the MIDI service
  sc = sl_bt_gattdb_start_service(gattdb_session_id, midi_service_handle);
  app_assert_status(sc);

  // Commit the GATT DB changes
  sc = sl_bt_gattdb_commit(gattdb_session_id);
  app_assert_status(sc);
}

#ifndef ARDUINO_SILABS_STACK_BLE_SILABS
  #error "This example is only compatible with the Silicon Labs BLE stack. Please select 'BLE (Silabs)' in 'Tools > Protocol stack'."
#endif
