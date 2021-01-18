# Set usual component variables
set(COMPONENT_SRCS "main.c")
set(COMPONENT_ADD_INCLUDEDIRS "")
set(COMPONENT_REQUIRES soc nvs_flash driver mqtt)

register_component()

#
