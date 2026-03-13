git pull

pio run -e esp32-s3-devkit -t upload
pio run -e esp32-s3-devkit -t uploadfs
pio device monitor -b 115200