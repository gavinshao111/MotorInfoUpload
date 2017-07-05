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
	${OBJECTDIR}/src/Forward/Constant.o \
	${OBJECTDIR}/src/Forward/Logger.o \
	${OBJECTDIR}/src/Forward/MotorInfoForward.o \
	${OBJECTDIR}/src/Forward/PublicServer.o \
	${OBJECTDIR}/src/Forward/Resource.o \
	${OBJECTDIR}/src/Forward/ResponseReader.o \
	${OBJECTDIR}/src/Forward/TcpServer.o \
	${OBJECTDIR}/src/Forward/TcpSession.o \
	${OBJECTDIR}/src/Forward/Uploader.o \
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

${OBJECTDIR}/src/Forward/Constant.o: src/Forward/Constant.cpp
	${MKDIR} -p ${OBJECTDIR}/src/Forward
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/Forward/Constant.o src/Forward/Constant.cpp

${OBJECTDIR}/src/Forward/Logger.o: src/Forward/Logger.cpp
	${MKDIR} -p ${OBJECTDIR}/src/Forward
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/Forward/Logger.o src/Forward/Logger.cpp

${OBJECTDIR}/src/Forward/MotorInfoForward.o: src/Forward/MotorInfoForward.cpp
	${MKDIR} -p ${OBJECTDIR}/src/Forward
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/Forward/MotorInfoForward.o src/Forward/MotorInfoForward.cpp

${OBJECTDIR}/src/Forward/PublicServer.o: src/Forward/PublicServer.cpp
	${MKDIR} -p ${OBJECTDIR}/src/Forward
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/Forward/PublicServer.o src/Forward/PublicServer.cpp

${OBJECTDIR}/src/Forward/Resource.o: src/Forward/Resource.cpp
	${MKDIR} -p ${OBJECTDIR}/src/Forward
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/Forward/Resource.o src/Forward/Resource.cpp

${OBJECTDIR}/src/Forward/ResponseReader.o: src/Forward/ResponseReader.cpp
	${MKDIR} -p ${OBJECTDIR}/src/Forward
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/Forward/ResponseReader.o src/Forward/ResponseReader.cpp

${OBJECTDIR}/src/Forward/TcpServer.o: src/Forward/TcpServer.cpp
	${MKDIR} -p ${OBJECTDIR}/src/Forward
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/Forward/TcpServer.o src/Forward/TcpServer.cpp

${OBJECTDIR}/src/Forward/TcpSession.o: src/Forward/TcpSession.cpp
	${MKDIR} -p ${OBJECTDIR}/src/Forward
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/Forward/TcpSession.o src/Forward/TcpSession.cpp

${OBJECTDIR}/src/Forward/Uploader.o: src/Forward/Uploader.cpp
	${MKDIR} -p ${OBJECTDIR}/src/Forward
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/Forward/Uploader.o src/Forward/Uploader.cpp

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
