class DisplayController < ApplicationController
  def index
    @latest = Measurement.order("created_at").last

  end
end
