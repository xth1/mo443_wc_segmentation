
CC=g++
OPENCV_LIBS=`pkg-config --cflags --libs opencv`
EXECUTABLE_NAME=wc_segmentation
SOURCE_FILES=src/main.cpp
#HEADER_FILES=

all: ${SOURCE_FILES} ${HEADER_FILES}
	${CC}  ${OPENCV_LIBS} -o ${EXECUTABLE_NAME} src/main.cpp
