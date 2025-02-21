# See the build system documentation in IDF programming guide
# for more information about component CMakeLists.txt files.

idf_component_register(
    SRCS wifi_manager.c         # list the source files of this component
    INCLUDE_DIRS include        # optional, add here public include directories
    PRIV_INCLUDE_DIRS   # optional, add here private include directories
    REQUIRES            # optional, list the public requirements (component names)
    PRIV_REQUIRES       esp_wifi esp_netif nvs_flash wifi_provisioning # optional, list the private requirements
)
