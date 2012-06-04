
CC=g++
OPENCV_LIBS=`pkg-config --cflags --libs opencv`
EXECUTABLE_NAME=op_morfologico
SOURCE_FILES=morfologia.cpp
#HEADER_FILES=

all: ${SOURCE_FILES} ${HEADER_FILES}
	${CC}  ${OPENCV_LIBS} -o ${EXECUTABLE_NAME} ${SOURCE_FILES}
