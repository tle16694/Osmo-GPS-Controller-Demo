set(SRCS_LIST
    "app_main.c"
    "../utils/crc/custom_crc16.c"
    "../utils/crc/custom_crc32.c"
    "../protocol/dji_protocol_parser.c"
    "../protocol/dji_protocol_data_processor.c"
    "../protocol/dji_protocol_data_descriptors.c"
    "../protocol/dji_protocol_data_structures.c"
    "../ble/ble.c"
    "../data/data.c"
    "../logic/connect_logic.c"
    "../logic/command_logic.c"
    "../logic/status_logic.c"
    "../logic/enums_logic.c"
    "../logic/key_logic.c"
    "../logic/light_logic.c"
    "../logic/product_nvs.c"
)

if(CONFIG_ENABLE_GNSS)
    list(APPEND SRCS_LIST
        "../logic/gps_logic.c"
        "../test/test_gps.c"
    )
endif()

set(PRIV_REQUIRES_LIST bt nvs_flash esp_driver_gpio)
if(CONFIG_ENABLE_GNSS)
    list(APPEND PRIV_REQUIRES_LIST esp_driver_uart)
endif()

idf_component_register(
    SRCS ${SRCS_LIST}
    PRIV_REQUIRES ${PRIV_REQUIRES_LIST}
    INCLUDE_DIRS "." "../utils/crc" "../protocol" "../ble" "../data" "../logic" "../test"
)
