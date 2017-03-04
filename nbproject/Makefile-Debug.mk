#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-Linux
CND_DLIB_EXT=so
CND_CONF=Debug
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/src/CarData.o \
	${OBJECTDIR}/src/DBConnection.o \
	${OBJECTDIR}/src/DataGenerator.o \
	${OBJECTDIR}/src/DataPtrLen.o \
	${OBJECTDIR}/src/DataSender.o \
	${OBJECTDIR}/src/MotorInfoUpload.o \
	${OBJECTDIR}/src/Util.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=-std=c++0x
CXXFLAGS=-std=c++0x

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=-LExternalDependencies/mysqlcpp/lib -LExternalDependencies/SafeStrCpy -L../ByteBuffer -L../paho.mqtt.c/build/output

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk motorinfoupload

motorinfoupload: ${OBJECTFILES}
	g++ -o motorinfoupload ${OBJECTFILES} ${LDLIBSOPTIONS} -lmysqlcppconn-static -lpthread `mysql_config --cflags --libs` -std=c++0x -lsafestrcpy -lbytebuffer -lpaho-mqtt3c -lpaho-mqtt3cs

${OBJECTDIR}/src/CarData.o: src/CarData.cpp
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -g -IExternalDependencies/mysqlcpp/include -IExternalDependencies/mysqlcpp/include/cppconn -IExternalDependencies/SafeStrCpy -I../ByteBuffer -I../paho.mqtt.c/src -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/CarData.o src/CarData.cpp

${OBJECTDIR}/src/DBConnection.o: src/DBConnection.cpp
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -g -IExternalDependencies/mysqlcpp/include -IExternalDependencies/mysqlcpp/include/cppconn -IExternalDependencies/SafeStrCpy -I../ByteBuffer -I../paho.mqtt.c/src -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/DBConnection.o src/DBConnection.cpp

${OBJECTDIR}/src/DataGenerator.o: src/DataGenerator.cpp
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -g -IExternalDependencies/mysqlcpp/include -IExternalDependencies/mysqlcpp/include/cppconn -IExternalDependencies/SafeStrCpy -I../ByteBuffer -I../paho.mqtt.c/src -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/DataGenerator.o src/DataGenerator.cpp

${OBJECTDIR}/src/DataPtrLen.o: src/DataPtrLen.cpp
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -g -IExternalDependencies/mysqlcpp/include -IExternalDependencies/mysqlcpp/include/cppconn -IExternalDependencies/SafeStrCpy -I../ByteBuffer -I../paho.mqtt.c/src -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/DataPtrLen.o src/DataPtrLen.cpp

${OBJECTDIR}/src/DataSender.o: src/DataSender.cpp
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -g -IExternalDependencies/mysqlcpp/include -IExternalDependencies/mysqlcpp/include/cppconn -IExternalDependencies/SafeStrCpy -I../ByteBuffer -I../paho.mqtt.c/src -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/DataSender.o src/DataSender.cpp

${OBJECTDIR}/src/MotorInfoUpload.o: src/MotorInfoUpload.cpp
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -g -IExternalDependencies/mysqlcpp/include -IExternalDependencies/mysqlcpp/include/cppconn -IExternalDependencies/SafeStrCpy -I../ByteBuffer -I../paho.mqtt.c/src -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/MotorInfoUpload.o src/MotorInfoUpload.cpp

${OBJECTDIR}/src/Util.o: src/Util.cpp
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -g -IExternalDependencies/mysqlcpp/include -IExternalDependencies/mysqlcpp/include/cppconn -IExternalDependencies/SafeStrCpy -I../ByteBuffer -I../paho.mqtt.c/src -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/Util.o src/Util.cpp

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
