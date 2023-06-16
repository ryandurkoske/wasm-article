ifeq ($(OS),Windows_NT)
RMDIR := powershell rm -ErrorAction Ignore -r -force
RM := powershell del -ErrorAction Ignore
MKDIR_P = mkdir "$@" 
else
RMDIR := rm -R
RM := rm
MKDIR_P = mkdir -p "$@"
endif