# Add set(CONFIG_USE_device_MCXA156_startup true) in config.cmake to use this component

include_guard(GLOBAL)
message("${CMAKE_CURRENT_LIST_FILE} component is included.")

if(CONFIG_USE_device_MCXA156_system)

if(CONFIG_TOOLCHAIN STREQUAL iar)
  add_config_file(${CMAKE_CURRENT_LIST_DIR}/iar/startup_MCXA156.s "" device_MCXA156_startup.MCXA156)
endif()

if(CONFIG_TOOLCHAIN STREQUAL armgcc)
  add_config_file(${CMAKE_CURRENT_LIST_DIR}/gcc/startup_MCXA156.S "" device_MCXA156_startup.MCXA156)
endif()

if(CONFIG_TOOLCHAIN STREQUAL mdk)
  add_config_file(${CMAKE_CURRENT_LIST_DIR}/arm/startup_MCXA156.S "" device_MCXA156_startup.MCXA156)
endif()

if(CONFIG_TOOLCHAIN STREQUAL mcux)
  add_config_file(${CMAKE_CURRENT_LIST_DIR}/mcuxpresso/startup_mcxa156.c "" device_MCXA156_startup.MCXA156)
  add_config_file(${CMAKE_CURRENT_LIST_DIR}/mcuxpresso/startup_mcxa156.cpp "" device_MCXA156_startup.MCXA156)
endif()

else()

message(SEND_ERROR "device_MCXA156_startup.MCXA156 dependency does not meet, please check ${CMAKE_CURRENT_LIST_FILE}.")

endif()
