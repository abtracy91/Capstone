require 'wiringpi'
require 'json'
require 'uri'
require 'net/http'

server = "http://localhost:3000/measurements"

s = WiringPi::Serial.new('/dev/ttyAMA0', 9600)

sleep(10)
if s.serialDataAvail < 10
  abort("Not enough serial data.")
end

sleep(1) until s.serialDataAvail > 200

data = ""

begin
  c = s.serialGetchar.chr
end until c == "{"

begin
  data += c
  c = s.serialGetchar.chr
end until c == "}"


data += c

mmt = JSON.parse(data.gsub("\"", '"').delete("\r\n"))

params = { 'measurement[weight]' => mmt["weight"].to_i.to_s,
           'measurement[thresholdWeight]' => mmt["thresholdWeight"].to_i.to_s,
           'measurement[speed]' => mmt["speed"].to_i.to_s,
           'measurement[thresholdSpeed]' => mmt["thresholdSpeed"].to_i.to_s,
           'measurement[temperature]' => mmt["temperature"].to_i.to_s,
           'measurement[thresholdTemperature]' => mmt["thresholdTemperature"].to_i.to_s
}

x = Net::HTTP.post_form(URI.parse(server), params)
s.serialClose
