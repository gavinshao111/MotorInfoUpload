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
	${OBJECTDIR}/src/BlockQueue.o \
	${OBJECTDIR}/src/MotorInfoUpload.o \
	${OBJECTDIR}/src/main.o


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
LDLIBSOPTIONS=-LExternalDependencies/mysqlcpp/lib -LExternalDependencies/SafeStrCpy -L../ByteBuffer

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk motorinfoupload

motorinfoupload: ${OBJECTFILES}
	g++ -o motorinfoupload ${OBJECTFILES} ${LDLIBSOPTIONS} -lmysqlcppconn-static -lpthread `mysql_config --cflags --libs` -std=c++0x -lsafestrcpy -lbytebuffer

${OBJECTDIR}/src/BlockQueue.o: src/BlockQueue.cpp
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -g -IExternalDependencies/mysqlcpp/include -IExternalDependencies/mysqlcpp/include/cppconn -IExternalDependencies/SafeStrCpy -I../ByteBuffer -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/BlockQueue.o src/BlockQueue.cpp

${OBJECTDIR}/src/MotorInfoUpload.o: src/MotorInfoUpload.cpp
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -g -IExternalDependencies/mysqlcpp/include -IExternalDependencies/mysqlcpp/include/cppconn -IExternalDependencies/SafeStrCpy -I../ByteBuffer -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/MotorInfoUpload.o src/MotorInfoUpload.cpp

${OBJECTDIR}/src/main.o: src/main.cpp
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -g -IExternalDependencies/mysqlcpp/include -IExternalDependencies/mysqlcpp/include/cppconn -IExternalDependencies/SafeStrCpy -I../ByteBuffer -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/main.o src/main.cpp

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
