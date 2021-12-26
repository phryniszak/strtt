set(strtt_source_files 
    strtt.cpp
    strttapp.cpp)

add_executable(strtt ${strtt_source_files})

target_link_libraries(strtt stlink)
