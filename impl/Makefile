# Structure of the project: 
#
# /
#     build/  
#         shared/
#             libproject-name.so
#             src/
#                 object files...
#         static/
#             libproject-name.a
#             src/
#                 object files...
#         executable/
#             project-name
#             src/
#                 object files
#     src/
#         sources files...
#     inc/
#         header files...

# Note: inc and src can be the same folder

#####
##### FOLDER SETTINGS
#####

SRC_FOLDER := src
LIB_FOLDER := lib
INC_FOLDER := include

GLUCOSE_FOLDER := $(LIB_FOLDER)/glucose-syrup-4.1

BUILD_FOLDER := build

BUILD_EXE_FOLDER := $(BUILD_FOLDER)

#####
##### GENERAL SETTINGS
#####

CXX := g++
SXX := ar

#####
##### FILES SETTINGS
#####

# Extension
EXT_SRC_FILE = .cpp
EXT_INC_FILE = .hpp

# Get header from source file
# This function is only used to checks if the header has bee modified and the object file must be rebuild
# So if the header doesn't exist it's ok

# The first argument is the source file relative to $(SRC_FOLDER)
# The header must be relative to $(INC_FOLDER)

# EX: $(1:%$(EXT_SRC_FILE)=%$(EXT_INC_FILE)) 
# will take the file "folder/sub_folder_file.cpp"
# and transform it into "folder/sub_folder_file.hpp"
header-of = $(1:%$(EXT_SRC_FILE)=%$(EXT_INC_FILE))

# Relative to $(SRC_FOLDER)
SRC_EXCLUDE_FILE := 

SRC_MAIN_SOLVER := solver.cpp
SRC_MAIN_GENERATOR := generator.cpp
SRC_MAIN_VERIFIER := verifier.cpp
SRC_MAINS := $(SRC_MAIN_SOLVER) $(SRC_MAIN_GENERATOR) $(SRC_MAIN_VERIFIER)

# Targets
TARGET_SOLVER := $(BUILD_EXE_FOLDER)/solver
TARGET_GENERATOR := $(BUILD_EXE_FOLDER)/generator
TARGET_VERIFIER := $(BUILD_EXE_FOLDER)/verifier

#####
##### FLAGS
#####

OPTI := -O3
FLAGS := -std=c++11  $(OPTI) -pthread
FLAGS += -Wall -Wextra -Wno-pmf-conversions -Wshadow -Wpedantic -Wduplicated-cond -Wduplicated-branches -Wlogical-op 
FLAGS += -Wnull-dereference -Wuseless-cast -Wold-style-cast -Wcast-align -Wcast-qual -Wno-missing-field-initializers 

# Include path
# Must be use with -I
INC_FLAG := -I $(INC_FOLDER) -I $(LIB_FOLDER) -I $(GLUCOSE_FOLDER)

#####
##### LIBRARY
#####

# Path to libaries if not in $PATH, for example (relative to the project folder): lib/
# Must be use with -L
LIBS_PATH := -L $(GLUCOSE_FOLDER)/simp

# For example: -lsfml-graphics
LIBS := -l_release

# Library that require to be build
LIB_TO_BUILD := $(GLUCOSE_FOLDER)/simp/lib_release.a

# Create rules to build the libraries
$(GLUCOSE_FOLDER)/simp/lib_release.a:
	@$(call _special,BUILDING STATIC LIBRARY ($@)...)
	@cd $(GLUCOSE_FOLDER)/simp && make libr

###############################################
#                   PRIVATE                   #
###############################################

#####
##### OTHER
#####

_RESET := \033[0m
_BOLD := \033[1m

_COLOR_RED := \033[31m
_COLOR_GREEN := \033[32m
_COLOR_YELLOW := \033[33m
_COLOR_BLUE := \033[34m
_COLOR_MAGENTA := \033[35m
_COLOR_CYAN := \033[36m
_COLOR_WHITE := \033[37m

SHARED_FLAGS := -fPIC

MAKEFLAGS += --no-print-directory

#####
##### FUNCTIONS
#####

_void =
_space = $(_void) $(_void)
_comma = ,

# join <between> <list>
_join = $(subst $(_space),$(1),$(2))

# _header <message>
_header = echo -e "$(_COLOR_CYAN)$(_BOLD)>>> $(1)$(_RESET)"
# _sub-header <message>
_sub-header = echo -e "$(_COLOR_GREEN)>>> $(1)$(_RESET)"
# _build-msg <target> <from>
_build-msg = echo -e "$(_COLOR_WHITE):: Building $(_BOLD)$(1)$(_RESET)$(_COLOR_WHITE) from $(_BOLD)$(2)$(_RESET)"
# _special <message>
_special = echo -e "$(_COLOR_MAGENTA)$(_BOLD)\# $(1)$(_RESET)"

# not <value>
# return an empty string if value is not
not = $(if $(1),,not-empty-string)

# _remove-folder <folder>
define _remove-folder
	rm -rf $(1)
endef

# _is-empty <value> [<message>]
# example: $(call check-not-empty,FOLDER,The folder must be specified)
_is-empty = $(call not,$(1))
define _is-empty-er
	$(if $(call _is-empty,$(1)),$(error Value is empty $(if $(2),($(2)) )))
endef

# _exists <file> [<message>]
_exists = $(wildcard $(1))
define _exists-er
	$(if $(call _exists,$(1)),,$(error File '$(1)' doesn't exists $(if $(2),($(2)) )))
endef

#####
##### SOURCES
#####

_SRC_MAINS := $(addprefix $(SRC_FOLDER)/,$(SRC_MAINS))
# All sources files not main
_SRC_FILES := $(filter-out $(_SRC_MAINS),$(shell find $(SRC_FOLDER) -name '*$(EXT_SRC_FILE)'))

#####
##### HEADERS
#####

_HEADER_FILES := $(shell find $(INC_FOLDER) -name '*$(EXT_INC_FILE)')

#####
##### DIRECTORIES
#####

# All sources file directories
_SRC_DIR := $(sort $(dir $(_SRC_FILES)))
_SRC_DIR_MAINS := $(sort $(dir $(_SRC_MAINS)))

_EXE_DIR := $(addprefix $(BUILD_EXE_FOLDER)/,$(_SRC_DIR) $(_SRC_DIR_MAINS))

_BUILD_DIR := $(_EXE_DIR)

#####
##### OBJECT FILES
##### 

_OBJ_MAIN_SOLVER := $(SRC_MAIN_SOLVER:%$(EXT_SRC_FILE)=$(BUILD_EXE_FOLDER)/$(SRC_FOLDER)/%.o)
_OBJ_MAIN_GENERATOR := $(SRC_MAIN_GENERATOR:%$(EXT_SRC_FILE)=$(BUILD_EXE_FOLDER)/$(SRC_FOLDER)/%.o)
_OBJ_MAIN_VERIFIER := $(SRC_MAIN_VERIFIER:%$(EXT_SRC_FILE)=$(BUILD_EXE_FOLDER)/$(SRC_FOLDER)/%.o)
_OBJ_SRC_EXE := $(_SRC_FILES:%$(EXT_SRC_FILE)=$(BUILD_EXE_FOLDER)/%.o) 

_LIB_PATH_LD := $(call _join,:,$(strip $(filter-out -L,$(LIBS_PATH))))
export LD_LIBRARY_PATH += $(_LIB_PATH_LD)

#####
##### RULES
#####

.PHONY: all executable 
.PHONY: clean  
.PHONY: re 
.PHONY: format

.DEFAULT_GOAL := all

all:
	@$(call _header,BUILDING EXECUTABLE...)
	@make $(TARGET_SOLVER) $(TARGET_GENERATOR) $(TARGET_VERIFIER)


clean:
	@$(call _header,REMOVING $(BUILD_FOLDER))
	@$(call _remove-folder,$(BUILD_FOLDER))

re:
	@make clean
	@make

format: .clang-format
	@$(call _special,RUNNING CLANG-FORMAT)
	@clang-format --style=file -i $(if $(file),$(file),$(_SRC_FILES) $(_SRC_MAINS) $(_HEADER_FILES))

$(_BUILD_DIR):
	@mkdir -p $(_BUILD_DIR)

###


$(TARGET_SOLVER): $(_BUILD_DIR) $(LIB_TO_BUILD) $(_OBJ_SRC_EXE) $(_OBJ_MAIN_SOLVER)
	@$(call _sub-header,Linking...)
	@$(CXX) $(INC_FLAG) $(FLAGS) $(_OBJ_MAIN_SOLVER) $(_OBJ_SRC_EXE) -o "$@" $(LIBS_PATH) $(LIBS)
	@$(call _header,Executable done ($(TARGET_SOLVER)))

$(TARGET_GENERATOR): $(_BUILD_DIR) $(LIB_TO_BUILD) $(_OBJ_SRC_EXE) $(_OBJ_MAIN_GENERATOR)
	@$(call _sub-header,Linking...)
	@$(CXX) $(INC_FLAG) $(FLAGS) $(_OBJ_MAIN_GENERATOR) $(_OBJ_SRC_EXE) -o "$@" $(LIBS_PATH) $(LIBS)
	@$(call _header,Executable done ($(TARGET_GENERATOR)))

$(TARGET_VERIFIER): $(_BUILD_DIR) $(LIB_TO_BUILD) $(_OBJ_SRC_EXE) $(_OBJ_MAIN_VERIFIER)
	@$(call _sub-header,Linking...)
	@$(CXX) $(INC_FLAG) $(FLAGS) $(_OBJ_MAIN_VERIFIER) $(_OBJ_SRC_EXE) -o "$@" $(LIBS_PATH) $(LIBS)
	@$(call _header,Executable done ($(TARGET_VERIFIER)))


$(BUILD_EXE_FOLDER)/$(SRC_FOLDER)/%.o: $(SRC_FOLDER)/%$(EXT_SRC_FILE) $(INC_FOLDER)/$(call header-of,%$(EXT_SRC_FILE))
	@$(call _build-msg,$(notdir $@),$(call _join,$(_comma)$(_space),$(strip $(notdir $< $(wildcard $(word 2,$^))))))
	@$(CXX) -c $(INC_FLAG) $(FLAGS) -o "$@" "$<"


# Just to avoid file without headers
%$(EXT_INC_FILE):
	