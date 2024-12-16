set(SOURCES "camera_lib.c")

idf_component_register(SRCS ${SOURCES}
                    INCLUDE_DIRS "."
                    REQUIRES wifi_pro
                    REQUIRES server_cfg
                    REQUIRES dht11
                    REQUIRES mqtt_cfg
                    REQUIRES json
)