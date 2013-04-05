class CreateMeasurements < ActiveRecord::Migration
  def change
    create_table :measurements do |t|
      t.integer :weight
      t.integer :thresholdWeight
      t.integer :speed
      t.integer :thresholdSpeed
      t.integer :temperature
      t.integer :thresholdTemperature

      t.timestamps
    end
  end
end
