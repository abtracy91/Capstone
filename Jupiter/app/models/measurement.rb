class Measurement < ActiveRecord::Base
  attr_accessible :speed, :temperature, :thresholdSpeed, :thresholdTemperature, :thresholdWeight, :weight
end
