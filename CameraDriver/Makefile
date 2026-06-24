upload_fs:
	pio run -t uploadfs

compile:
	pio run

upload:
	pio run -t upload

monitor:
	pio device monitor

code_completion:
	pio run -t compiledb

release: upload_fs upload_firmware
