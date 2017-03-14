function(generate_file file outputdir arguments)
  set(classpath ${GROOVY_DIR}/groovy-all-${GROOVY_VER}.jar
                ${GROOVY_DIR}/commons-lang-${COMMONS_VER}.jar
                ${CMAKE_SOURCE_DIR}/tools/codegenerator
                ${CMAKE_CURRENT_SOURCE_DIR}/../python)
  if(NOT CORE_SYSTEM_NAME STREQUAL windows)
    set(devnull "/dev/null")
    string(REPLACE ";" ":" classpath "${classpath}")
  else()
    set(devnull "nul")
  endif()

  set(CPP_FILE ${outputdir}/${file}.cpp)
  add_custom_command(OUTPUT ${CPP_FILE}
                     COMMAND mkdir ${outputdir} > ${devnull} 2> ${devnull} || true
                     COMMAND ${SWIG_EXECUTABLE}
                     ARGS -w401 -c++ ${arguments} -o ${outputdir}/${file}.xml -xml -I${CMAKE_SOURCE_DIR}/xbmc -xmllang python ${CMAKE_CURRENT_SOURCE_DIR}/../swig/${file}
                     COMMAND ${Java_JAVA_EXECUTABLE}
                     ARGS -cp "${classpath}" groovy.ui.GroovyMain ${CMAKE_SOURCE_DIR}/tools/codegenerator/Generator.groovy ${outputdir}/${file}.xml ${CMAKE_CURRENT_SOURCE_DIR}/../python/PythonSwig.cpp.template ${outputdir}/${file}.cpp > ${devnull}
                     DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../swig/${file} ${CMAKE_CURRENT_SOURCE_DIR}/../python/PythonSwig.cpp.template)
  set(SOURCES ${SOURCES} "${CPP_FILE}" PARENT_SCOPE)
endfunction()

find_package(Java COMPONENTS Runtime REQUIRED)
find_package(SWIG REQUIRED)

# The generated bindings
set(INPUTS AddonModuleXbmcaddon.i
           AddonModuleXbmcgui.i
           AddonModuleXbmc.i
           AddonModuleXbmcplugin.i
           AddonModuleXbmcvfs.i
           AddonModuleXbmcwsgi.i)

set(GROOVY_DIR ${CMAKE_SOURCE_DIR}/tools/codegenerator/groovy)
set(GROOVY_VER 2.4.4)
set(COMMONS_VER 2.6)

foreach(INPUT IN LISTS INPUTS)
  generate_file(${INPUT} py2 "-python")
  list(APPEND GEN_SRCS ${CMAKE_CURRENT_BINARY_DIR}/py2/${INPUT}.cpp)
  generate_file(${INPUT} py3 "-python -py3")
  list(APPEND GEN_SRCS ${CMAKE_CURRENT_BINARY_DIR}/py3/${INPUT}.cpp)
endforeach()

add_library(python_binding STATIC ${SOURCES})
set_target_properties(python_binding PROPERTIES POSITION_INDEPENDENT_CODE TRUE
                                                FOLDER "Build Utilities")
set(core_DEPENDS python_binding ${core_DEPENDS} CACHE STRING "" FORCE)
if(WIN32)
  add_precompiled_header(python_binding pch.h ${CMAKE_SOURCE_DIR}/xbmc/platform/win32/pch.cpp PCH_TARGET kodi)
endif()
