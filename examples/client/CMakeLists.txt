cmake_minimum_required (VERSION 3.0)

project (lwm2mclient)

option(DTLS "Enable DTLS" ON)

set(CMAKE_C_STANDARD 99)

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pg")

include(${CMAKE_CURRENT_LIST_DIR}/../../core/wakaama.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../shared/shared.cmake)

add_definitions(-DLWM2M_CLIENT_MODE -DLWM2M_BOOTSTRAP -DLWM2M_SUPPORT_JSON -DHELPFUL_LOGS)
add_definitions(${SHARED_DEFINITIONS} ${WAKAAMA_DEFINITIONS})

include_directories (${WAKAAMA_SOURCES_DIR} ${SHARED_INCLUDE_DIRS})

SET(sqlite_src src_sql/sqlite3.c)

SET(SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/lwm2mclient.c
    ${CMAKE_CURRENT_LIST_DIR}/lwm2mclient.h
    ${CMAKE_CURRENT_LIST_DIR}/system_api.c
    ${CMAKE_CURRENT_LIST_DIR}/object_security.c
    ${CMAKE_CURRENT_LIST_DIR}/object_server.c
    ${CMAKE_CURRENT_LIST_DIR}/object_device.c
    ${CMAKE_CURRENT_LIST_DIR}/object_firmware.c
    ${CMAKE_CURRENT_LIST_DIR}/object_location.c
    ${CMAKE_CURRENT_LIST_DIR}/object_connectivity_moni.c
    ${CMAKE_CURRENT_LIST_DIR}/object_connectivity_stat.c
    ${CMAKE_CURRENT_LIST_DIR}/object_access_control.c
    ${CMAKE_CURRENT_LIST_DIR}/test_object.c
        object_vehicle.c unixIPC.c jsonParse.c sensorData.h cJSON.c cJSON.h 
        storeSensorData.c object_temperature.c object_pm25.c object_common.c 
        object_common.h object_humidity.c utils.c data_convert.c  sqliteUtils.c)

add_executable(${PROJECT_NAME} ${SOURCES} ${WAKAAMA_SOURCES} ${SHARED_SOURCES} ${sqlite_src} )
target_link_libraries(${PROJECT_NAME} pthread dl) #sqlite depend

# Add WITH_LOGS to debug variant
set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY COMPILE_DEFINITIONS $<$<CONFIG:Debug>:WITH_LOGS>)

SOURCE_GROUP(wakaama FILES ${WAKAAMA_SOURCES})
