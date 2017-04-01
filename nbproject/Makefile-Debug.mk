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
	${OBJECTDIR}/src/Forward/Generator.o \
	${OBJECTDIR}/src/Forward/MotorInfoForward.o \
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
LDLIBSOPTIONS=-L../GavinLib/ByteBuffer/dist -L../GavinLib/GSocket/dist -L../GavinLib/GMqtt/dist

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk motorinfoupload

motorinfoupload: ${OBJECTFILES}
	g++ -o motorinfoupload ${OBJECTFILES} ${LDLIBSOPTIONS} -std=c++0x -lbytebuffer -lgmqtt -lgsocket

${OBJECTDIR}/src/Forward/Generator.o: src/Forward/Generator.cpp
	${MKDIR} -p ${OBJECTDIR}/src/Forward
	${RM} "$@.d"
	$(COMPILE.cc) -g -I../GavinLib/ByteBuffer/src -I../GavinLib/GSocket/src -I../GavinLib/GMqtt/src -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/Forward/Generator.o src/Forward/Generator.cpp

${OBJECTDIR}/src/Forward/MotorInfoForward.o: src/Forward/MotorInfoForward.cpp
	${MKDIR} -p ${OBJECTDIR}/src/Forward
	${RM} "$@.d"
	$(COMPILE.cc) -g -I../GavinLib/ByteBuffer/src -I../GavinLib/GSocket/src -I../GavinLib/GMqtt/src -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/Forward/MotorInfoForward.o src/Forward/MotorInfoForward.cpp

${OBJECTDIR}/src/Util.o: src/Util.cpp
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -g -I../GavinLib/ByteBuffer/src -I../GavinLib/GSocket/src -I../GavinLib/GMqtt/src -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/Util.o src/Util.cpp

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
