log-readings.yml

Checks out the data branch into a folder, installs the MQTT client, reads the retained message for each topic, and appends dt,metric,value rows. Skips readings that are out of range or already logged, and commits and pushes only if something changed.