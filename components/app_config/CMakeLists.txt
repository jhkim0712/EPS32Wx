# Header-only shared component: common constants/config/types used by nearly
# every other component. No REQUIRES needed for the headers themselves;
# app_config_t only uses <stdint.h>/<stdbool.h>.
idf_component_register(
    INCLUDE_DIRS "include"
)
