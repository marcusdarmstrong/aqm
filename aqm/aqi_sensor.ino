#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <PromLokiTransport.h>
#include <PrometheusArduino.h>
#include "Adafruit_PM25AQI.h"
#include "Adafruit_BME680.h"

#define WIFI_SSID     ""
#define WIFI_PASSWORD ""
#define NODE ""
#define URL "" // "192.168.0.1" or "prometheus.yourdomain.local" No http or https, No port or path
#define PATH "/api/v1/write"
#define PORT 9090

Adafruit_BME680 bme;
Adafruit_PM25AQI aqi = Adafruit_PM25AQI();
PromLokiTransport transport;
PromClient client(transport);
// Number of time series, buffer size in bytes
WriteRequest req(9, 1024);

TimeSeries ts_pm10(1, "aqm_particulate_matter", "{size=\"1.0\",host=\"" NODE "\"}");
TimeSeries ts_pm25(1, "aqm_particulate_matter", "{size=\"2.5\",host=\"" NODE "\"}");
TimeSeries ts_pm100(1, "aqm_particulate_matter", "{size=\"10\",host=\"" NODE "\"}");
TimeSeries ts_temp(1, "aqm_temperature_degf", "{host=\"" NODE "\"}");
TimeSeries ts_pres(1, "aqm_pressure_pa", "{host=\"" NODE "\"}");
TimeSeries ts_hum(1, "aqm_humidity_percentage", "{host=\"" NODE "\"}");
TimeSeries ts_gas(1, "aqm_gas_ohms", "{host=\"" NODE "\"}");
TimeSeries ts_error(1, "aqm_recording_error", "{host=\"" NODE "\"}");
TimeSeries ts_success(1, "aqm_recording_success", "{host=\"" NODE "\"}");

void setup() {
  Serial.begin(9600);

  Wire.begin();

  // Wait one second for sensors to boot up
  delay(1000);
  
  if (!aqi.begin_I2C()) {
    Serial.println("Could not find PM 2.5 sensor!");
    while (1) delay(10);
  } else {
    Serial.println("PM 2.5 sensor found!");
  }
 
  if (!bme.begin()) {
    Serial.println("Could not find BME680 sensor!");
    while (1) delay(10);
  } else {
    Serial.println("BME680 sensor found!");
  }

  // Set up oversampling and filter initialization
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320*C for 150 ms

  // Configure and start the transport layer
  transport.setWifiSsid(WIFI_SSID);
  transport.setWifiPass(WIFI_PASSWORD);
  transport.setDebug(Serial);  // Remove this line to disable debug logging of the client.
  if (!transport.begin()) {
    Serial.println(transport.errmsg);
    while (1) delay(10);
  }

  // Configure the client
  client.setUrl(URL);
  client.setPath((char*)PATH);
  client.setPort(PORT);
  client.setDebug(Serial);  // Remove this line to disable debug logging of the client.
  if (!client.begin()) {
    Serial.println(client.errmsg);
    while (1) delay(10);
  }

  // Add our TimeSeries to the WriteRequest
  req.addTimeSeries(ts_pm10);
  req.addTimeSeries(ts_pm25);
  req.addTimeSeries(ts_pm100);
  req.addTimeSeries(ts_temp);
  req.addTimeSeries(ts_pres);
  req.addTimeSeries(ts_hum);
  req.addTimeSeries(ts_gas);
  req.addTimeSeries(ts_error);
  req.addTimeSeries(ts_success);
  req.setDebug(Serial);

  Serial.print("Free Mem After Setup: ");
  Serial.println(freeMemory());
}

uint32_t successes, errors;

void loop() {
  PM25_AQI_Data data;
  int64_t time = transport.getTimeMillis();

  if (!aqi.read(&data)) {
    Serial.println("Could not read from PM Monitor");
    ts_error.addSample(time, ++errors);
    return;
  }
  if (!bme.performReading()) {
    Serial.println("Could not read from Temp Monitor");
    ts_error.addSample(time, ++errors);
    return;
  }

  Serial.println();
  Serial.print(F("PM 1.0 = "));
  Serial.println(data.pm10_standard);
  ts_pm10.addSample(time, data.pm10_standard);
  
  Serial.print(F("PM 2.5 = "));
  Serial.println(data.pm25_standard);
  ts_pm25.addSample(time, data.pm25_standard);

  Serial.print(F("PM 10 = "));
  Serial.println(data.pm100_standard);
  ts_pm100.addSample(time, data.pm100_standard);

  Serial.print("Temperature = ");
  Serial.print(bme.temperature * 9.0 / 5.0 + 32.0);
  Serial.println(" *F");
  ts_temp.addSample(time, bme.temperature * 9.0 / 5.0 + 32.0);

  Serial.print("Pressure = ");
  Serial.print(bme.pressure);
  Serial.println(" Pa");
  ts_pres.addSample(time, bme.pressure);

  Serial.print("Humidity = ");
  Serial.print(bme.humidity);
  Serial.println(" %");
  ts_hum.addSample(time, bme.humidity);

  Serial.print("Gas = ");
  Serial.print(bme.gas_resistance);
  Serial.println(" Ohms");
  ts_gas.addSample(time, bme.gas_resistance);

  PromClient::SendResult res = client.send(req);

  // We'll clear out the buffers for our internal monitoring timeseries
  // immediately after submission so that we can record the result of the
  // just-dispatched submission.
  ts_error.resetSamples();
  ts_success.resetSamples();
  
  if (!res == PromClient::SendResult::SUCCESS) {
    Serial.println(client.errmsg);
    ts_error.addSample(time, ++errors);
  } else {
    ts_success.addSample(time, ++successes);
  }
  
  ts_pm10.resetSamples();
  ts_pm25.resetSamples();
  ts_pm100.resetSamples();
  ts_temp.resetSamples();
  ts_pres.resetSamples();
  ts_hum.resetSamples();
  ts_gas.resetSamples();
  
  delay(5000);
}
