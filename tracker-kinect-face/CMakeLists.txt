# Kinect SDK is Windows only
if (WIN32)
	find_package(OpenCV QUIET)
	if(OpenCV_FOUND)
		# Setup cache variable to Kinect SDK path
		if(DEFINED ENV{KINECTSDK20_DIR})
			set(SDK_KINECT20 $ENV{KINECTSDK20_DIR} CACHE PATH $ENV{KINECTSDK20_DIR})
		else()
			set(SDK_KINECT20 $ENV{KINECTSDK20_DIR} CACHE PATH "")
		endif()

		# If we have a valid SDK path, try build that tracker
		if(EXISTS ${SDK_KINECT20})
			# Register our module
			otr_module(tracker-kinect-face)

			# Add include path to Kinect SDK 
			target_include_directories(opentrack-tracker-kinect-face SYSTEM PUBLIC "${SDK_KINECT20}/inc")
	
			# Check processor architecture 
			if(CMAKE_SIZEOF_VOID_P EQUAL 4)
				# 32 bits        
				set (kinect-arch-dir "x86")
			elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
				# 64 bits
				set (kinect-arch-dir "x64")
			else()
				message(FATAL_ERROR "Kinect: architecture not supported!")
			endif()
	
			# Link against Kinect SDK libraries
			target_link_libraries(opentrack-tracker-kinect-face "${SDK_KINECT20}/lib/${kinect-arch-dir}/Kinect20.lib" "${SDK_KINECT20}/lib/${kinect-arch-dir}/Kinect20.Face.lib")
			# Link against OpenCV stuff, needed for video preview
			target_link_libraries(opentrack-tracker-kinect-face opencv_imgproc opentrack-cv opencv_core)

			# Install Kinect Face DLL
			install(FILES "${SDK_KINECT20}/Redist/Face/${kinect-arch-dir}/Kinect20.Face.dll" DESTINATION "./modules/" PERMISSIONS ${opentrack-perms-exec})
			# Install Kinect Face Database
			install(DIRECTORY "${SDK_KINECT20}/Redist/Face/${kinect-arch-dir}/NuiDatabase" DESTINATION "./modules/")
		endif(EXISTS ${SDK_KINECT20})
	endif(OpenCV_FOUND)
endif(WIN32)