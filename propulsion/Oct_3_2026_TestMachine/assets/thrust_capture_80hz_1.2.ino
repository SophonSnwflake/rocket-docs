#include <SD.h>
#include <SPI.h>
#include <Hx711.h>

// HX711 RATE must be wired HIGH on the module. The sketch measures the
// conversion period at startup and refuses to arm if it is not near 80 SPS.
const uint8_t HX711_DOUT_PIN = A2;
const uint8_t HX711_CLOCK_PIN = A5;
const uint8_t SD_CHIP_SELECT_PIN = 10;

const unsigned long SERIAL_BAUD = 115200UL;
const unsigned long HX711_STARTUP_TIMEOUT_MS = 500UL;
const unsigned long HX711_SAMPLE_TIMEOUT_MS = 40UL;
const uint8_t RATE_MEASUREMENT_SAMPLES = 8;
const uint8_t TARE_SAMPLES = 30;
const float COUNTS_PER_CALIBRATED_UNIT = 1992.0f;
const uint16_t FLUSH_INTERVAL_SAMPLES = 80;
// Maximum compact stream line: uint32 time + comma + signed long raw + CRLF.
const uint8_t SERIAL_SAMPLE_BYTES_MAX = 24;

// Status flags written in the CSV file.
const uint8_t STATUS_TIME_GAP = 0x01;
const uint8_t STATUS_NEAR_SATURATION = 0x02;
const uint8_t STATUS_HX711_TIMEOUT = 0x04;

Hx711 scale(HX711_DOUT_PIN, HX711_CLOCK_PIN);
File dataFile;
char logFilename[] = "TEST000.CSV";

bool capturing = false;
bool havePreviousSample = false;
uint32_t captureStartedUs = 0;
uint32_t previousSampleClockUs = 0;
uint32_t lastStatusClockUs = 0;
uint32_t sampleIndex = 0;
uint32_t totalMissedSamples = 0;
uint32_t measuredSamplePeriodUs = 0;
long tareOffset = 0;

void fatalError(const __FlashStringHelper *message)
{
  Serial.print(F("FATAL: "));
  Serial.println(message);

  if (dataFile) {
    dataFile.flush();
    dataFile.close();
  }

  // A fatal initialization failure must prevent a test from being armed.
  while (true) {
    delay(1000);
  }
}

char readCommand()
{
  if (Serial.available() <= 0) {
    return '\0';
  }

  char command = Serial.read();
  if (command >= 'a' && command <= 'z') {
    command -= ('a' - 'A');
  }
  return command;
}

void waitForCommand(char expected)
{
  while (true) {
    if (readCommand() == expected) {
      return;
    }
  }
}

bool waitForHx711(unsigned long timeoutMs)
{
  const unsigned long startedMs = millis();
  while (digitalRead(HX711_DOUT_PIN) == HIGH) {
    if (timeoutMs == 0 || millis() - startedMs >= timeoutMs) {
      return false;
    }
  }
  return true;
}

bool readRawValue(long &rawValue, unsigned long timeoutMs)
{
  if (!waitForHx711(timeoutMs)) {
    return false;
  }

  // DOUT remains LOW until clocked, so the original library's internal wait
  // returns immediately. Mask interrupts only during its 25-clock transfer so
  // PD_SCK cannot accidentally stay HIGH for more than 60 us.
  noInterrupts();
  rawValue = scale.getValue();
  interrupts();
  return true;
}

bool readRawAverage(long &average, uint8_t samples,
                    unsigned long perSampleTimeoutMs)
{
  if (samples == 0) {
    return false;
  }

  int64_t sum = 0;
  for (uint8_t i = 0; i < samples; ++i) {
    long rawValue = 0;
    if (!readRawValue(rawValue, perSampleTimeoutMs)) {
      return false;
    }
    sum += rawValue;
  }

  average = static_cast<long>(sum / samples);
  return true;
}

float convertToCalibratedUnits(long rawValue)
{
  return static_cast<float>(rawValue - tareOffset) /
         COUNTS_PER_CALIBRATED_UNIT;
}

bool openUniqueLogFile()
{
  for (uint16_t run = 0; run < 1000; ++run) {
    logFilename[4] = '0' + (run / 100) % 10;
    logFilename[5] = '0' + (run / 10) % 10;
    logFilename[6] = '0' + run % 10;

    if (!SD.exists(logFilename)) {
      dataFile = SD.open(logFilename, FILE_WRITE);
      return static_cast<bool>(dataFile);
    }
  }
  return false;
}

bool measureSampleRate()
{
  long discarded = 0;
  if (!readRawValue(discarded, HX711_STARTUP_TIMEOUT_MS)) {
    return false;
  }

  const uint32_t startedUs = micros();
  for (uint8_t i = 0; i < RATE_MEASUREMENT_SAMPLES; ++i) {
    if (!readRawValue(discarded, 200UL)) {
      return false;
    }
  }
  const uint32_t elapsedUs = micros() - startedUs;
  measuredSamplePeriodUs = elapsedUs / RATE_MEASUREMENT_SAMPLES;

  // 80 SPS is nominally 12.5 ms. This wide range allows oscillator tolerance
  // while still rejecting the roughly 100 ms period of 10 SPS mode.
  return measuredSamplePeriodUs >= 8000UL &&
         measuredSamplePeriodUs <= 25000UL;
}

void writeLogHeader()
{
  dataFile.println(F("# hx711_thrust_capture_v2"));
  dataFile.print(F("# filename,"));
  dataFile.println(logFilename);
  dataFile.print(F("# measured_sample_period_us,"));
  dataFile.println(measuredSamplePeriodUs);
  dataFile.print(F("# measured_sample_rate_sps,"));
  dataFile.println(1000000.0f / static_cast<float>(measuredSamplePeriodUs), 2);
  dataFile.print(F("# offset_raw,"));
  dataFile.println(tareOffset);
  dataFile.print(F("# counts_per_calibrated_unit,"));
  dataFile.println(COUNTS_PER_CALIBRATED_UNIT, 6);
  dataFile.println(F("sample,time_us,raw_offset_binary,calibrated_value,status,missed_before"));
  dataFile.flush();

  if (dataFile.getWriteError()) {
    fatalError(F("Could not write the log header"));
  }
}

void writeSuccessfulSample(uint32_t timeUs, long rawValue, float calibratedValue,
                           uint8_t status, uint16_t missedBefore)
{
  dataFile.print(sampleIndex);
  dataFile.print(',');
  dataFile.print(timeUs);
  dataFile.print(',');
  dataFile.print(rawValue);
  dataFile.print(',');
  dataFile.print(calibratedValue, 3);
  dataFile.print(',');
  dataFile.print(status);
  dataFile.print(',');
  dataFile.println(missedBefore);
}

void streamSampleToSerial(uint32_t timeUs, long rawValue)
{
  // Never wait for UART space. SD logging remains authoritative; when the
  // serial buffer is busy, only this live-display line is dropped.
  if (Serial.availableForWrite() < SERIAL_SAMPLE_BYTES_MAX) {
    return;
  }

  Serial.print(timeUs);
  Serial.write(',');
  Serial.println(rawValue);
}

void writeTimeoutSample(uint32_t timeUs)
{
  dataFile.print(sampleIndex);
  dataFile.print(',');
  dataFile.print(timeUs);
  dataFile.print(F(",,,"));
  dataFile.print(STATUS_HX711_TIMEOUT);
  dataFile.println(F(",0"));
}

void stopCapture()
{
  capturing = false;
  dataFile.print(F("# stopped_at_us,"));
  dataFile.println(micros() - captureStartedUs);
  dataFile.print(F("# detected_missed_samples,"));
  dataFile.println(totalMissedSamples);
  dataFile.flush();

  const bool writeFailed = dataFile.getWriteError();
  dataFile.close();

  Serial.println(F("Capture stopped and file closed."));
  if (writeFailed) {
    Serial.println(F("WARNING: the SD card reported a write error."));
  }
}

void setup()
{
  Serial.begin(SERIAL_BAUD);
  Serial.println(F("HX711 thrust capture startup"));

  // The original library uses INPUT. Enabling the pull-up here makes a
  // disconnected DOUT time out rather than appear as a valid stream of zeros.
  pinMode(HX711_DOUT_PIN, INPUT_PULLUP);
  digitalWrite(HX711_CLOCK_PIN, LOW);

  if (!SD.begin(SD_CHIP_SELECT_PIN)) {
    fatalError(F("SD initialization failed"));
  }
  if (!openUniqueLogFile()) {
    fatalError(F("Could not create TEST000.CSV through TEST999.CSV"));
  }

  delay(500);
  if (!measureSampleRate()) {
    Serial.print(F("Measured period (us): "));
    Serial.println(measuredSamplePeriodUs);
    fatalError(F("HX711 is missing or RATE is not configured for 80 SPS"));
  }

  Serial.print(F("Measured HX711 rate: "));
  Serial.print(1000000.0f / static_cast<float>(measuredSamplePeriodUs), 1);
  Serial.println(F(" SPS"));

  scale.setScale(COUNTS_PER_CALIBRATED_UNIT);

  Serial.println(F("Unload the stand, then send T to tare."));
  waitForCommand('T');

  // Discard the conversion that may have completed before the T command.
  long discarded = 0;
  if (!readRawValue(discarded, HX711_STARTUP_TIMEOUT_MS)) {
    fatalError(F("HX711 timeout before tare"));
  }

  if (!readRawAverage(tareOffset, TARE_SAMPLES,
                      HX711_SAMPLE_TIMEOUT_MS)) {
    fatalError(F("HX711 timeout during tare"));
  }
  scale.setOffset(tareOffset);

  Serial.print(F("Tare offset: "));
  Serial.println(tareOffset);
  Serial.print(F("Log file: "));
  Serial.println(logFilename);

  writeLogHeader();

  Serial.println(F("Send S to start capture. Send X to stop safely."));
  waitForCommand('S');

  // Discard an old conversion so the first recorded point belongs to the new
  // capture interval rather than the time spent waiting for S.
  if (!readRawValue(discarded, HX711_STARTUP_TIMEOUT_MS)) {
    fatalError(F("HX711 timeout while arming"));
  }

  sampleIndex = 0;
  totalMissedSamples = 0;
  havePreviousSample = false;
  captureStartedUs = micros();
  lastStatusClockUs = captureStartedUs;
  capturing = true;
  Serial.println(F("Capture started."));
  Serial.println(F("# stream_format,time_us,raw_offset_binary"));
}

void loop()
{
  if (!capturing) {
    return;
  }

  if (readCommand() == 'X') {
    stopCapture();
    return;
  }

  long rawValue = 0;
  if (!readRawValue(rawValue, HX711_SAMPLE_TIMEOUT_MS)) {
    const uint32_t timeUs = micros() - captureStartedUs;
    writeTimeoutSample(timeUs);
    ++sampleIndex;
  } else {
    const uint32_t sampleClockUs = micros();
    const uint32_t timeUs = sampleClockUs - captureStartedUs;
    uint8_t status = 0;
    uint16_t missedBefore = 0;

    if (havePreviousSample) {
      const uint32_t intervalUs = sampleClockUs - previousSampleClockUs;
      const uint32_t elapsedPeriods =
          (intervalUs + measuredSamplePeriodUs / 2UL) / measuredSamplePeriodUs;
      if (elapsedPeriods > 1UL) {
        const uint32_t missed = elapsedPeriods - 1UL;
        missedBefore = missed > 65535UL ? 65535U : static_cast<uint16_t>(missed);
        totalMissedSamples += missed;
        status |= STATUS_TIME_GAP;
      }
    }

    if (rawValue <= 0x000100L ||
        static_cast<uint32_t>(rawValue) >= 0xFFFF00UL) {
      status |= STATUS_NEAR_SATURATION;
    }

    writeSuccessfulSample(timeUs, rawValue,
                          convertToCalibratedUnits(rawValue), status,
                          missedBefore);
    streamSampleToSerial(timeUs, rawValue);

    previousSampleClockUs = sampleClockUs;
    havePreviousSample = true;
    ++sampleIndex;
  }

  if (dataFile.getWriteError()) {
    Serial.println(F("FATAL: SD write failed; capture stopped."));
    stopCapture();
    return;
  }

  if (sampleIndex != 0 && sampleIndex % FLUSH_INTERVAL_SAMPLES == 0) {
    dataFile.flush();
    if (dataFile.getWriteError()) {
      Serial.println(F("FATAL: SD flush failed; capture stopped."));
      stopCapture();
      return;
    }
  }

  const uint32_t nowUs = micros();
  if (nowUs - lastStatusClockUs >= 1000000UL &&
      Serial.availableForWrite() >= 32) {
    lastStatusClockUs = nowUs;
    Serial.print(F("# samples="));
    Serial.print(sampleIndex);
    Serial.print(F(" missed="));
    Serial.println(totalMissedSamples);
  }
}
