.PHONY: compile clangd clangd_main clangd_front upload_main upload_front monitor clean

compile:
	pio run -e main_esp -e front_esp

clangd: clangd_main

clangd_main:
	pio run -e main_esp -t compiledb

clangd_front:
	pio run -e front_esp -t compiledb

upload_main:
	pio run -e main_esp -t upload

upload_front:
	pio run -e front_esp -t upload

monitor:
	pio device monitor

clean:
	pio run -t clean
