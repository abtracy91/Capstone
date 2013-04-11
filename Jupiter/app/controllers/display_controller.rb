class DisplayController < ApplicationController
  def index
    @latest = Measurement.order("created_at").last

    @last15 = Measurement.all(:conditions => ["created_at >= ?", DateTime.now - 15.minutes], :order => "created_at DESC")

    @last60 = Measurement.all(:conditions => ["created_at >= ?", DateTime.now - 60.minutes], :order => "created_at DESC")

    @last360 = Measurement.all(:conditions => ["created_at >= ?", DateTime.now - 360.minutes], :order => "created_at DESC")

    @thresholdW = Threshold.find_by_id(1).thresholdWeight
    @thresholdS = Threshold.find_by_id(1).thresholdSpeed
    @thresholdT = Threshold.find_by_id(1).thresholdTemperature

  end
end
