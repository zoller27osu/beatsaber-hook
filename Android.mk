# Copyright (C) 2009 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


################################################################################################################
# NOTICE:
# IN ORDER TO BUILD THIS PROJECT, YOU MUST CREATE AN ndkpath.txt IN THE SAME DIRECTORY AS THIS FILE
# WITH A PATH TO YOUR NDK DIRECTORY (TYPICALLY c:/android-ndk-r20b OR SIMILAR)
# THEN RUN build.ps1
################################################################################################################

LOCAL_PATH := $(call my-dir)
TARGET_ARCH_ABI := $(APP_ABI)

rwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))

include $(CLEAR_VARS)
# Build the modloader .so object first, ensure we have it and can link with it.
# Keep in mind that this modloader.so should EXACTLY MATCH the header file exported from the modloader
# running in the game you are making a mod for. If it doesn't, symbols will be undefined, and your mod probably won't load.
# When developing this yourself, ensure that the libmodloader.so is specified correctly as below and that your includes
# specify it correctly.
LOCAL_MODULE	        := modloader
LOCAL_SRC_FILES         := ./include/libs/libmodloader.so
LOCAL_EXPORT_C_INCLUDES := ./include/
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
# Create the actual shared object. We want to ensure we load all libraries we wish to use:
LOCAL_SHARED_LIBRARIES := modloader
# Ensure we include the log library so we can log to the android log
LOCAL_LDLIBS     := -llog
# MUST SPECIFY THE libil2cpp FOLDER IN YOUR INCLUDE PATH!
# Ensure you select the correct version of libil2cpp for your game
LOCAL_CFLAGS     := -D'VERSION="0.1.0"' -I'c:/Program Files/Unity/Editor/Data/il2cpp/libil2cpp'
# Suppress macro logs can be specified in order to create a build that has silenced logs.
# TODO: This will eventually be a parameter specifyable.
# LOCAL_CFLAGS	 += -D'SUPPRESS_MACRO_LOGS'
LOCAL_MODULE     := beatsaber-hook
LOCAL_C_INCLUDES := ./include
# Add inline-hook
LOCAL_SRC_FILES  := $(call rwildcard,src/inline-hook,*.c) $(call rwildcard,src/inline-hook,*.cpp)
# Add utils
LOCAL_SRC_FILES  += $(call rwildcard,src/utils,*.cpp)
# Add config
LOCAL_SRC_FILES  += $(call rwildcard,src/config,*.cpp)
include $(BUILD_SHARED_LIBRARY)
