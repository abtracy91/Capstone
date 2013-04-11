class CreateThresholds < ActiveRecord::Migration
  def change
    create_table :thresholds do |t|
      t.integer :thresholdWeight
      t.integer :thresholdSpeed
      t.integer :thresholdTemperature

      t.timestamps
    end
  end
end
