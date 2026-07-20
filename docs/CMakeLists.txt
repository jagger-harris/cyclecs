cmake_minimum_required(VERSION 3.21)

find_package(Doxygen REQUIRED)
find_package(Python3 REQUIRED COMPONENTS Interpreter)

# Paths
set(VENV_DIR "${CMAKE_CURRENT_BINARY_DIR}/.venv")
set(REQUIREMENTS_FILE "${CMAKE_CURRENT_SOURCE_DIR}/requirements.txt")
set(VENV_STAMP "${VENV_DIR}/.installed")

if(WIN32)
    set(VENV_PYTHON "${VENV_DIR}/Scripts/python.exe")
else()
    set(VENV_PYTHON "${VENV_DIR}/bin/python")
endif()

set(DOXYGEN_XML_DIR "${CMAKE_CURRENT_BINARY_DIR}/doxygen/xml")
set(SPHINX_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}")
set(SPHINX_BUILD "${CMAKE_CURRENT_BINARY_DIR}/sphinx")

# Python virtual environment
add_custom_command(
    OUTPUT ${VENV_STAMP}

    COMMAND ${Python3_EXECUTABLE} -m venv ${VENV_DIR}
    COMMAND ${VENV_PYTHON} -m pip install --upgrade pip
    COMMAND ${VENV_PYTHON} -m pip install -r ${REQUIREMENTS_FILE}

    # Create a stamp file so this only runs when needed
    COMMAND ${CMAKE_COMMAND} -E touch ${VENV_STAMP}
    DEPENDS ${REQUIREMENTS_FILE}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Setting up Python documentation environment"
)

add_custom_target(docs_setup
    DEPENDS ${VENV_STAMP}
)

# Doxygen
set(DOXYGEN_GENERATE_XML YES)
set(DOXYGEN_XML_OUTPUT xml)
set(DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/doxygen")

doxygen_add_docs(
    doxygen_xml
    ${CMAKE_SOURCE_DIR}/include
    COMMENT "Generating Doxygen XML"
)

# Sphinx
add_custom_target(
    docs

    COMMAND
        ${VENV_PYTHON}
        -m sphinx
        -b html
        -D breathe_projects.cyclecs=${DOXYGEN_XML_DIR}
        ${SPHINX_SOURCE}
        ${SPHINX_BUILD}/html

    DEPENDS
        docs_setup
        doxygen_xml

    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building Sphinx documentation"
)
