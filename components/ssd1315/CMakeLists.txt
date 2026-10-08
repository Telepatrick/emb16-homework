idf_component_register(
    SRCS "ssd1315.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_driver_i2c driver font8x8
)