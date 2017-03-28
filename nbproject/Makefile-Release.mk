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
CND_PLATFORM=None-Windows
CND_DLIB_EXT=dll
CND_CONF=Release
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/src/Forward/DataPacketForward.o \
	${OBJECTDIR}/src/Forward/Generator.o \
	${OBJECTDIR}/src/Forward/MotorInfoForward.o \
	${OBJECTDIR}/src/Forward/Sender.o \
	${OBJECTDIR}/src/Util.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/motorinfoupload.exe

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/motorinfoupload.exe: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/motorinfoupload ${OBJECTFILES} ${LDLIBSOPTIONS}

${OBJECTDIR}/src/Forward/DataPacketForward.o: src/Forward/DataPacketForward.cpp
	${MKDIR} -p ${OBJECTDIR}/src/Forward
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/Forward/DataPacketForward.o src/Forward/DataPacketForward.cpp

${OBJECTDIR}/src/Forward/Generator.o: src/Forward/Generator.cpp
	${MKDIR} -p ${OBJECTDIR}/src/Forward
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/Forward/Generator.o src/Forward/Generator.cpp

${OBJECTDIR}/src/Forward/MotorInfoForward.o: src/Forward/MotorInfoForward.cpp
	${MKDIR} -p ${OBJECTDIR}/src/Forward
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/Forward/MotorInfoForward.o src/Forward/MotorInfoForward.cpp

${OBJECTDIR}/src/Forward/Sender.o: src/Forward/Sender.cpp
	${MKDIR} -p ${OBJECTDIR}/src/Forward
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/Forward/Sender.o src/Forward/Sender.cpp

${OBJECTDIR}/src/Util.o: src/Util.cpp
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/Util.o src/Util.cpp

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